// Copyright: 2020 englishcatfish
// License: BSD-3
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <set>
#include <mutex>
#include <thread>
#include <future>

#include "csv.h"
#include "math.h"
#include "roll.h"
#include "parser.h"
#include "const.h"

using namespace std;

int numThreads = 1;

mutex savedMutex;
fstream savedFile;
unordered_map<string, double> saved;
bool savedEnabled = true;

mutex bestMatchMutex;
unordered_map<string, double> bestMatchProbHistory;
mutex bestSuccessMutex;
unordered_map<string, double> bestSuccessForRollHistory;
bool largeMemEnabled = false;

mutex rootMutex;
int globalTotalRolls;
vector<int> globalRoll;
vector<pair<double, int>> globalProbs;

string currentLineText;

double calc(vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice);
vector<vector<int>> getHoldable(vector<vector<int>> tasks, vector<int> roll, int numHold, vector<int> heldDice, bool discardOne);

// returns TRUE when all available dice selected by the mask can be applied to the task
bool subsetRollMatchesTask(vector<int> roll, int mask, vector<int> task, vector<int> focused) {
	roll.insert(roll.end(), focused.begin(), focused.end());
	for (int i = 0; i < task.size(); i++) {
		int goal = task[i];
		bool metGoal = false;
		if (goal == TERROR || goal == TERROR_PERIL || goal == TERROR_SCROLL) {
			for (int j = 0; !metGoal && j < roll.size(); j++) {
				int n = intPow(2, j);
				if ((mask & n) != n) {
					continue;
				}
				int die = roll[j];
				if (die == TERROR || die == R_WILD) {
					mask -= n;
					metGoal = true;
				}
			}
		}
		if (goal == PERIL || goal == TERROR_PERIL || goal == PERIL_SCROLL) {
			for (int j = 0; !metGoal && j < roll.size(); j++) {
				int n = intPow(2, j);
				if ((mask & n) != n) {
					continue;
				}
				int die = roll[j];
				if (die == PERIL || die == Y_PERIL || die == R_PERIL || die == R_WILD) {
					mask -= n;
					metGoal = true;
				}
			}
		}
		if (goal == SCROLL || goal == TERROR_SCROLL || goal == PERIL_SCROLL) {
			for (int j = 0; !metGoal && j < roll.size(); j++) {
				int p = intPow(2, j);
				if ((mask & p) != p) {
					continue;
				}
				int die = roll[j];
				if (die == SCROLL || die == Y_SCROLL || die == R_SCROLL || die == R_WILD) {
					mask -= p;
					metGoal = true;
				}
			}
		}
		int numInvs = getInvValue(goal);
		if (numInvs > 0) {
			for (int j = 0; !metGoal && j < roll.size(); j++) {
				int n = intPow(2, j);
				if ((mask & n) != n) {
					continue;
				}
				int die = roll[j];
				if (die == INV_1 || die == Y_INV_1) {
					numInvs -= 1;
					mask -= n;
				} else if (die == INV_2 || die == Y_INV_2 || die == R_INV_2) {
					numInvs -= 2;
					mask -= n;
				} else if (die == INV_3 || die == Y_INV_3 || die == R_INV_3) {
					numInvs -= 3;
					mask -= n;
				}
				else if (die == Y_INV_4 || die == R_INV_4 || die == R_WILD) {
					numInvs -= 4;
					mask -= n;
				}
				metGoal = numInvs <= 0;
			}
		}
		if (!metGoal) {
			return false;
		}
	}
	return mask == 0;
}

double bestMatchProb(vector<int> roll, vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice) {
	string key = "";
	if (largeMemEnabled) {
		key = rollToString(roll) + serialize(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);
		bestMatchMutex.lock();
		if (bestMatchProbHistory.find(key) != bestMatchProbHistory.end()) {
			double val = bestMatchProbHistory[key];
			bestMatchMutex.unlock();
			return bestMatchProbHistory[key];
		}
		bestMatchMutex.unlock();
	}
	
	// shadow vars
	vector<vector<int>> _tasks;
	int _numGreen; 
	bool _yellowDie, _redDie;
	vector<int> _heldDice;
	
	double bestProb = 0.0;
	for (int mask = 0; mask < intPow(2, roll.size() + heldDice.size()); mask++) {
		for (int j = 0; j < tasks.size(); j++) {
			if (subsetRollMatchesTask(roll, mask, tasks[j], heldDice)) {
				_tasks = tasks;
				_tasks.erase(_tasks.begin() + j, _tasks.begin() + j + 1);
				_numGreen = numGreen;
				_yellowDie = yellowDie;
				_redDie = redDie;
				_heldDice = maskRoll(heldDice, ~(mask / intPow(2, roll.size())));

				for (int i = 0; i < roll.size(); i++) {
					int n = intPow(2, i);
					if ((mask & n) != n) {
						continue;
					}
					int dieColor = roll[i] / 100;
					if (dieColor == GREEN) {
						_numGreen--;
					}
					else if (dieColor == YELLOW) {
						_yellowDie = false;
					}
					else if (dieColor == RED) {
						_redDie = false;
					}
				}
				double p = calc(_tasks, inOrder, _numGreen, _yellowDie, _redDie, numFocus, numSpell, numClue, _heldDice);
				bestProb = max(p, bestProb);

				// successful and spell some dice
				if (numSpell > 0) {
					vector<int> _roll = maskRoll(roll, ~mask);
					vector<vector<int>> spellable = getHoldable(_tasks, _roll, numSpell, _heldDice, false);
					for (vector<int> spelledDice : spellable) {
						int __numGreen = _numGreen;
						bool __yellowDie = _yellowDie;
						bool __redDie = _redDie;
						vector<int> __heldDice = _heldDice;
						for (int n : spelledDice) {
							int dieColor = n / 100;
							if (dieColor == GREEN) {
								__numGreen--;
							}
							else if (dieColor == YELLOW) {
								__yellowDie = false;
							}
							else if (dieColor == RED) {
								__redDie = false;
							}
						}
						__heldDice.insert(__heldDice.end(), spelledDice.begin(), spelledDice.end());
						p = calc(_tasks, inOrder, __numGreen, __yellowDie, __redDie, numFocus, numSpell - spelledDice.size(), numClue, __heldDice);
						bestProb = max(p, bestProb);
					}
				}
			}
			if (inOrder) {
				break;
			}
		}
	}
	if (largeMemEnabled) {
		bestMatchMutex.lock();
		bestMatchProbHistory[key] = bestProb;
		bestMatchMutex.unlock();
	}
	return bestProb;
}

bool hasMinDice(vector<vector<int>> tasks, int numGreen, bool yellowDie, bool redDie, vector<int> heldDice) {
	int totalDiceHave = numGreen + (yellowDie ? 1 : 0) + (redDie ? 1 : 0) + heldDice.size();
	if (totalDiceHave == heldDice.size()) {
		return false;
	}
	int totalDiceNeeded = 0;
	for (int i = 0; i < tasks.size(); i++) {
		for (int j = 0; j < tasks[i].size(); j++) {
			if (getInvValue(tasks[i][j]) != -1) {
				totalDiceNeeded += ((getInvValue(tasks[i][j]) + 2) / 3);
			}
			else {
				totalDiceNeeded++;
			}
		}
	}
	return totalDiceNeeded <= totalDiceHave;
}

double throwAwayProb(vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice, vector<int> focusDiceNew, vector<int> spellDiceNew) {
	int _numGreen = numGreen;
	bool _yellowDie = yellowDie;
	bool _redDie = redDie;

	for (int n : focusDiceNew) {
		int dieColor = n / 100;
		if (dieColor == GREEN) {
			_numGreen--;
		}
		else if (dieColor == YELLOW) {
			_yellowDie = false;
		}
		else if (dieColor == RED) {
			_redDie = false;
		}
	}

	for (int n : spellDiceNew) {
		int dieColor = n / 100;
		if (dieColor == GREEN) {
			_numGreen--;
		}
		else if (dieColor == YELLOW) {
			_yellowDie = false;
		}
		else if (dieColor == RED) {
			_redDie = false;
		}
	}

	heldDice.insert(heldDice.end(), focusDiceNew.begin(), focusDiceNew.end());
	numFocus -= focusDiceNew.size();
	heldDice.insert(heldDice.end(), spellDiceNew.begin(), spellDiceNew.end());
	numSpell -= spellDiceNew.size();

	// sometimes it is better to throw a yellow die away (e.g., when needing to roll terrors)
	if (_numGreen > 0 && _yellowDie) {
		double throwGreen = calc(tasks, inOrder, _numGreen - 1, _yellowDie, _redDie, numFocus, numSpell, numClue, heldDice);
		double throwYellow = calc(tasks, inOrder, _numGreen, false, _redDie, numFocus, numSpell, numClue, heldDice);
		return (throwYellow > throwGreen) ? throwYellow : throwGreen;
	}
	else {
		// order of importance: GREEN < YELLOW < RED
		if (_numGreen > 0) {
			_numGreen--;
		}
		else if (_yellowDie) {
			_yellowDie = false;
		}
		else if (_redDie) {
			_redDie = false;
		}
		return calc(tasks, inOrder, _numGreen, _yellowDie, _redDie, numFocus, numSpell, numClue, heldDice);
	}
}

bool dieMatchesGoal(int die, int goal) {
	if (getInvValue(goal) != -1) {
		if (die == INV_1 || die == INV_2 || die == INV_3
			|| die == Y_INV_1 || die == Y_INV_2 || die == Y_INV_3 || die == Y_INV_4
			|| die == R_INV_2 || die == R_INV_3 || die == R_INV_4) {
			return true;
		}
	}
	else if ((goal == TERROR || goal == TERROR_PERIL || goal == TERROR_SCROLL) && die == TERROR) {
		return true;
	}
	else if ((goal == PERIL || goal == TERROR_PERIL || goal == PERIL_SCROLL) && (die == PERIL || die == Y_PERIL || die == R_PERIL)) {
		return true;
	}
	else if ((goal == SCROLL || goal == TERROR_SCROLL || goal == PERIL_SCROLL) && (die == SCROLL || die == Y_SCROLL || die == R_SCROLL)) {
		return true;
	}
	return false;
}

bool isPartialMatch(vector<vector<int>> tasks, vector<int> roll, int mask) {
	for (int j = 0; j < roll.size(); j++) {
		int p = intPow(2, j);
		if ((mask & p) != p) {
			continue;
		}
		int die = roll[j];
		if (die == R_WILD) {
			continue;
		}
		bool foundMatch = false;
		for (vector<int> task : tasks) {
			for (int i = 0; i < task.size(); i++) {
				if (dieMatchesGoal(die, task[i])) {
					// if INV, then reduce
					if (getInvValue(task[i]) != -1) {
						int newInvValue = getInvValue(task[i]) - getInvValue(die);
						if (newInvValue > 3) {
							task[i] = newInvValue + 3;
							foundMatch = true;
							break;
						}
						else if (newInvValue > 0) {
							task[i] = newInvValue;
							foundMatch = true;
							break;
						}
					}
					task.erase(task.begin() + i, task.begin() + i + 1);
					foundMatch = true;
				}
			}
			if (foundMatch) {
				break;
			}
		}
		if (!foundMatch) {
			return false;
		}
	}
	return true;
}

void filterHeldDice(vector<vector<int>>& tasks, vector<int>& heldDice) {
	// remove (filter out) held dice from tasks (not necessary to be 100%, won't know what the wild die should apply to)
	for (int heldDie : heldDice) {
		bool goToNext = false;
		for (vector<int> task : tasks) {
			for (int i = 0; i < task.size(); i++) {
				if (dieMatchesGoal(heldDie, task[i])) {
					// if INV, then reduce
					if (getInvValue(task[i]) != -1) {
						int newInvValue = getInvValue(task[i]) - getInvValue(heldDie);
						if (newInvValue > 3) {
							task[i] = newInvValue + 3;
							goToNext = true;
							break;
						}
						else if (newInvValue > 0) {
							task[i] = newInvValue;
							goToNext = true;
							break;
						}
					}
					task.erase(task.begin() + i, task.begin() + i + 1);
				}
			}
		}
	}
}

vector<vector<int>> getHoldable(vector<vector<int>> tasks, vector<int> roll, int numHold, vector<int> heldDice, bool discardOne) {
	vector<vector<int>> holdable = vector<vector<int>>();
	int maxFocus = roll.size() - (1 + discardOne);
	numHold = min(numHold, maxFocus);
	if (tasks.size() == 0 || numHold < 1) {
		return holdable;
	}
	
	filterHeldDice(tasks, heldDice);

	for (int i = 1; i <= numHold; i++) {
		for (int mask = 1; mask < intPow(2, roll.size()); mask++) {
			if (numBits(mask) != i) {
				continue;
			}
			if (isPartialMatch(tasks, roll, mask)) {
				holdable.push_back(maskRoll(roll, mask));
			}
		}
	}
	
	return holdable;
}

double bestStrategyForRoll(vector<int> roll, vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice) {
	string key2 = "";
	if (largeMemEnabled) {
		key2 = rollToString(roll) + serialize(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);
		bestSuccessMutex.lock();
		if (bestSuccessForRollHistory.find(key2) != bestSuccessForRollHistory.end()) {
			double val = bestSuccessForRollHistory[key2];
			bestSuccessMutex.unlock();
			return val;
		}
		bestSuccessMutex.unlock();
	}
	
	// find best fit sub-set of roll to match a task
	double bestProb = bestMatchProb(roll, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);

	// fail, no focus, use spelledDice
	vector<vector<int>> spellable = getHoldable(tasks, roll, numSpell, heldDice, true);
	for (vector<int> spelledDice : spellable) {
		double p = throwAwayProb(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice, vector<int>(), spelledDice);
		bestProb = max(p, bestProb);
	}

	// say that we want to or do fail the task, can any be focused?
	vector<vector<int>> focusable = getHoldable(tasks, roll, numFocus, heldDice, true);
	for (vector<int> focusedDice : focusable) {
		double p = throwAwayProb(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice, focusedDice, vector<int>());
		bestProb = max(p, bestProb);

		// get spelledDice
		vector<int> _roll = roll;
		removeFromRoll(_roll, focusedDice);
		vector<int> _heldDice = heldDice;
		_heldDice.insert(_heldDice.end(), focusedDice.begin(), focusedDice.end());
		vector<vector<int>> spellable = getHoldable(tasks, _roll, numSpell, _heldDice, false);
		for (vector<int> spelledDice : spellable) {
			p = throwAwayProb(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice, focusedDice, spelledDice);
			bestProb = max(p, bestProb);
		}
	}

	// use a clue
	// this will recursively call current method in case there are multiple clues
	if (numClue > 0) {
		for (int m = 1; m <= roll.size(); m++) {
			// re-roll n-dice
			for (int mask = 1; mask < intPow(2, roll.size()); mask++) {
				if (numBits(mask) != m) {
					continue;
				}
				
				double success = 0.0;
				vector<int> rollKeep = maskRoll(roll, ~mask);

				if (largeMemEnabled) {
					string key3 = rollToString(rollKeep) + key2;
					bestSuccessMutex.lock();
					if (bestSuccessForRollHistory.find(key3) != bestSuccessForRollHistory.end()) {
						success = bestSuccessForRollHistory[key3];
						bestSuccessMutex.unlock();
					}
					else {
						bestSuccessMutex.unlock();
						vector<int> rollNew = maskRoll(roll, mask);
						resetRoll(rollNew);
						int n = 0;
						int totalNewRolls = intPow(6, rollNew.size());
						for (int j = 0; j < totalNewRolls; j += n) {
							n = rollCombinations(rollNew);
							vector<int> _roll = rollKeep;
							_roll.insert(_roll.end(), rollNew.begin(), rollNew.end());
							sort(_roll.begin(), _roll.end());

							double p = bestStrategyForRoll(_roll, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue - 1, heldDice);
							success += n * p / totalNewRolls;
							incrRoll(rollNew);
						}
						bestSuccessMutex.lock();
						bestSuccessForRollHistory[key3] = success;
						bestSuccessMutex.unlock();
					}
				}
				else {
					vector<int> rollNew = maskRoll(roll, mask);
					resetRoll(rollNew);
					int n = 0;
					int totalNewRolls = intPow(6, rollNew.size());
					for (int j = 0; j < totalNewRolls; j += n) {
						n = rollCombinations(rollNew);
						vector<int> _roll = rollKeep;
						_roll.insert(_roll.end(), rollNew.begin(), rollNew.end());
						sort(_roll.begin(), _roll.end());

						double p = bestStrategyForRoll(_roll, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue - 1, heldDice);
						success += n * p / totalNewRolls;
						incrRoll(rollNew);
					}
				}
				bestProb = max(success, bestProb);
			}
		}
	}
	if (largeMemEnabled) {
		bestSuccessMutex.lock();
		bestSuccessForRollHistory[key2] = bestProb;
		bestSuccessMutex.unlock();
	}
	return bestProb;
}

double calc(vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice) {
	string key = serialize(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);
	savedMutex.lock();
	if (saved.find(key) != saved.end()) {
		double val = saved[key];
		savedMutex.unlock();
		return val;
	}
	savedMutex.unlock();
	if (tasks.size() == 0) {
		savedMutex.lock();
		saved[key] = 1.0;
		if (savedEnabled) {
			savedFile << key << "#" << 1.0 << endl;
		}
		savedMutex.unlock();
		return 1.0;
	}
	if (!hasMinDice(tasks, numGreen, yellowDie, redDie, heldDice)) {
		savedMutex.lock();
		saved[key] = 0.0;
		if (savedEnabled) {
			savedFile << key << "#" << 0.0 << endl;
		}
		savedMutex.unlock();
		return 0.0;
	}

	vector<int> roll = initRoll(numGreen, yellowDie, redDie);
	int totalRolls = intPow(6, roll.size());
	int n = 0;
	double success = 0.0;

	double discardOneProb = throwAwayProb(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice, vector<int>(), vector<int>());

	for (int i = 0; i < totalRolls; i += n) {
		n = rollCombinations(roll);
		double bestProb = discardOneProb;
		double p = bestStrategyForRoll(roll, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);
		bestProb = max(p, bestProb);
		success += n * bestProb / totalRolls;
		incrRoll(roll);
	}
	savedMutex.lock();
	saved[key] = success;
	if (savedEnabled) {
		savedFile << key << "#" << success << endl;
	}
	savedMutex.unlock();
	return success;
}

// continue running bestStrategyForRoll until no jobs/rolls left
void launchThread(vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice) {
	int totalRolls = intPow(6, globalRoll.size());
	while (true) {
		rootMutex.lock();
		cout << currentLineText << "..." << (100 * globalTotalRolls / totalRolls) << "%\r" << flush;
		// check if done
		if (globalTotalRolls >= totalRolls) {
			rootMutex.unlock();
			break;
		}
		vector<int> roll = globalRoll;
		int n = rollCombinations(globalRoll);;
		globalTotalRolls += n;
		incrRoll(globalRoll);
		rootMutex.unlock();

		double p = bestStrategyForRoll(roll, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);
		
		rootMutex.lock();
		globalProbs.push_back(pair<double, int>(p, n));
		rootMutex.unlock();
	}
	return;
}

double calcInit(vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue) {
	vector<int> heldDice = vector<int>();
	string key = serialize(tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice);
	if (saved.find(key) != saved.end()) {
		return saved[key];
	}
	if (tasks.size() == 0) {
		saved[key] = 1.0;
		if (savedEnabled) {
			savedFile << key << "#" << 1.0 << endl;
		}
		return 1.0;
	}
	if (!hasMinDice(tasks, numGreen, yellowDie, redDie, heldDice)) {
		saved[key] = 0.0;
		if (savedEnabled) {
			savedFile << key << "#" << 0.0 << endl;
		}
		return 0.0;
	}

	globalTotalRolls = 0;
	globalRoll = initRoll(numGreen, yellowDie, redDie);
	globalProbs = vector<pair<double,int>>();

	auto throwAwayThread = async(launch::async, throwAwayProb, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice, vector<int>(), vector<int>());
	vector<thread> threads = vector<thread>();

	// launch n-1 threads because throwAwayProb has its own thread
	for (int i = 1; i < numThreads; i++) {
		threads.push_back(thread(launchThread, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice));
	}

	double discardOneProb = throwAwayThread.get();
	// throwAwayProb thread finished, can launch nth thread
	threads.push_back(thread(launchThread, tasks, inOrder, numGreen, yellowDie, redDie, numFocus, numSpell, numClue, heldDice));
	for (thread & t : threads) {
		if (t.joinable()) {
			t.join();
		}
	}

	double success = 0.0;
	for (pair<double, int> g : globalProbs) {
		success += (g.second * max(discardOneProb, g.first));
	}
	success /= intPow(6, globalRoll.size());
	saved[key] = success;
	if (savedEnabled) {
		savedFile << key << "#" << success << endl;
	}
	return success;
}

void readInSaved(bool clearSaved) {
	if (clearSaved) {
		savedFile.open("saved_values.txt", ios::out | ios::trunc);
		return;
	}
	
	savedFile.open("saved_values.txt", ios::out | ios::app);
	savedFile.close();
	savedFile.open("saved_values.txt", ios::out | ios::in);
	string str;
	while (getline(savedFile, str)) {
		vector<string> values = split(str, '#');
		saved[values[0]] = stod(values[1]);
	}
	savedFile.clear();
	return;
}

void printHelp() {
	cout << "./eldersign" << endl;
	cout << endl << "Flags: " << endl;
	cout << "\t--focus <n>\t\tNumber of focus events available (default = 0)" << endl;
	cout << "\t--spell <n>\t\tNumber of spells available (default = 0)" << endl;
	cout << "\t--clue <n>\t\tNumber of clues available (default = 0)" << endl;
	cout << "\t--in-file <cards_file>\tDefault is cards.csv" << endl;
	cout << "\t--num-threads <n>\tSpecify number of cores you want to use, will greatly improve performance (default = 1)." << endl;
	cout << "\t--task <task>\t\tOverrides input file, can specify one-off inputs." << endl;
	cout << "\t--disabled-saved\tUse if you don't wish to use/update the on disk cache" << endl;
	cout << "\t--clear-saved\t\tUse to reset the on disk cache" << endl;
	cout << "\t--large-mem\t\tUse when clues > 0 to help cache additional calculations. Uses a lot of memory." << endl;
	cout << "\t--no-range\t\tProgram iterates all 0..focus and 0..spell combinations. This turns that off." << endl;
	cout << "\t--scenarios <n>\t\tPerform calculations for the first n scenarios." << endl;
	cout << "\t--help\t\t\tThis print out." << endl;
	cout << endl << "Following example is equivalent to default ./eldersign" << endl;
	cout << "./eldersign --focus 0 --spell 0 --clue 0 --in-file cards.csv --num-threads 1" << endl;
}

void runTask(string name, string taskStr, int maxFocus, int maxSpell, int maxClue, bool noRange, int numCols) {
	vector<vector<int>> tasks = vector<vector<int>>();
	bool inOrder = false;

	parseTasks(taskStr, tasks, inOrder);
	if (!inOrder) {
		sort(tasks.begin(), tasks.end());
	}

	for (int numFocus = (noRange ? maxFocus : 0); numFocus <= maxFocus; numFocus++) {
		for (int numSpell = (noRange ? maxSpell : 0); numSpell <= maxSpell; numSpell++) {
			for (int numClue = (noRange ? maxClue : 0); numClue <= maxClue; numClue++) {
				currentLineText = name + "#" + taskStr + "#" + to_string(numFocus) + " Focus#" + to_string(numSpell) + " Spell#" + to_string(numClue) + " Clue";
				cout << endl << currentLineText << "\r" << flush;
				if (numCols <= 0) continue;
				currentLineText += "#";
				currentLineText += to_string(calcInit(tasks, inOrder, 5, false, false, numFocus, numSpell, numClue) * 100.0);
				cout << currentLineText << "\r" << flush;
				if (numCols <= 1) continue;
				currentLineText += "#";
				currentLineText += to_string(calcInit(tasks, inOrder, 6, false, false, numFocus, numSpell, numClue) * 100.0);
				cout << currentLineText << "\r" << flush;
				if (numCols <= 2) continue;
				currentLineText += "#";
				currentLineText += to_string(calcInit(tasks, inOrder, 6, true, false, numFocus, numSpell, numClue) * 100.0);
				cout << currentLineText << "\r" << flush;
				if (numCols <= 3) continue;
				currentLineText += "#";
				currentLineText += to_string(calcInit(tasks, inOrder, 6, false, true, numFocus, numSpell, numClue) * 100.0);
				cout << currentLineText << "\r" << flush;
				if (numCols <= 4) continue;
				currentLineText += "#";
				currentLineText += to_string(calcInit(tasks, inOrder, 6, true, true, numFocus, numSpell, numClue) * 100.0);
				cout << currentLineText << "\r" << flush;
				if (numCols <= 5) continue;
				currentLineText += "#";
				currentLineText += to_string(calcInit(tasks, inOrder, 7, true, true, numFocus, numSpell, numClue) * 100.0);
				cout << currentLineText << "\r" << flush;
			}
		}
	}
}

int main(int argc, char** argv) {
	string filename = "cards.csv";
	bool noRange = false;
	bool clearSaved = false;
	int maxFocus = 0;
	int maxSpell = 0;
	int maxClue = 0;
	int numCols = 6;
	string taskStr = "";

	int argIdx = 1;
	while (argIdx < argc) {
		string flag = argv[argIdx++];
		if (flag == "--no-range") {
			noRange = true;
		}
		else if (flag == "--focus" && argIdx < argc) {
			maxFocus = stoi(argv[argIdx++]);
		}
		else if (flag == "--spell" && argIdx < argc) {
			maxSpell = stoi(argv[argIdx++]);
		}
		else if (flag == "--clue" && argIdx < argc) {
			maxClue = stoi(argv[argIdx++]);
		}
		else if (flag == "--scenarios" && argIdx < argc) {
			numCols = stoi(argv[argIdx++]);
		}
		else if (flag == "--large-mem") {
			largeMemEnabled = true;
		}
		else if (flag == "--in-file" && argIdx < argc) {
			filename = argv[argIdx++];
		}
		else if (flag == "--clear-saved") {
			clearSaved = true;
		}
		else if (flag == "--task") {
			taskStr = argv[argIdx++];
		}
		else if (flag == "--disable-saved") {
			savedEnabled = false;
		}
		else if (flag == "--num-threads" && argIdx < argc) {
			numThreads = stoi(argv[argIdx++]);
		}
		else if (flag == "--help") {
			printHelp();
			return 0;
		}
		else {
			cout << "Bad argument list" << endl;
			printHelp();
			return 0;
		}
	}
	
	saved = unordered_map<string, double>();
	if (savedEnabled) {
		readInSaved(clearSaved);
	}
	cout << "Card Title#Tasks#Focus#Spell#Clue#5G#6G#6G+Y#6G+R#6G+YR#6G+YRW";
	if (taskStr != "") {
		runTask("OneOff", taskStr, maxFocus, maxSpell, maxClue, noRange, numCols);
	}
	else {
		io::CSVReader<3, io::trim_chars<' '>, io::double_quote_escape<',', '\"'>> in(filename);
		in.read_header(io::ignore_extra_column, "Expansion", "Name", "Tasks");
		string expansion;
		string name;

		while (in.read_row(expansion, name, taskStr)) {
			runTask(name, taskStr, maxFocus, maxSpell, maxClue, noRange, numCols);
		}
	}

	cout << endl;

	if (savedEnabled) {
		savedFile.close();
	}

	return 0;
}
