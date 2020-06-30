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

#pragma once

#include <vector>

using namespace std;

vector<int> initRoll(int numGreen, bool yellowDie, bool redDie) {
	vector<int> roll = vector<int>();
	for (int i = 0; i < numGreen; i++) {
		roll.push_back(1);
	}
	if (yellowDie) {
		roll.push_back(101);
	}
	if (redDie) {
		roll.push_back(201);
	}
	return roll;
}

bool nonDescending(vector<int>& roll) {
	for (int i = 1; i < roll.size(); i++) {
		if (roll[i - 1] > roll[i]) {
			return false;
		}
	}
	return true;
}

void incrRollHelper(vector<int>& roll) {
	bool carry = true;
	for (int i = 0; carry && i < roll.size(); i++) {
		roll[i] = ((roll[i] % 100) % 6 == 0) ? roll[i] - 5 : roll[i] + 1;
		carry = roll[i] % 6 == 1;
	}
}

void incrRoll(vector<int>& roll) {
	incrRollHelper(roll);
	while (!nonDescending(roll)) {
		incrRollHelper(roll);
	}
}

int rollCombinations(vector<int> roll) {
	int numGreen = 0;
	int counts[6] = { 0,0,0,0,0,0 };
	for (int i = 0; i < roll.size(); i++) {
		if (roll[i] <= 6) {
			numGreen++;
			counts[roll[i] - 1]++;
		}
	}
	int total = fac(numGreen);
	for (int i = 0; i < 6; i++) {
		total /= fac(counts[i]);
	}
	return total;
}

int rollCombinations(vector<int> roll, int mask) {
	int numGreen = 0;
	int counts[6] = { 0,0,0,0,0,0 };
	for (int i = 0; i < roll.size(); i++) {
		int p = 1 << i;
		if ((mask & p) != p) {
			continue;
		}
		if (roll[i] <= 6) {
			numGreen++;
			counts[roll[i] - 1]++;
		}
	}
	int total = fac(numGreen);
	for (int i = 0; i < 6; i++) {
		total /= fac(counts[i]);
	}
	return total;
}

vector<int> maskRoll(vector<int> roll, int mask) {
	vector<int> maskedRoll = vector<int>();
	for (int i = 0; i < roll.size(); i++) {
		int n = intPow(2, i);
		if ((mask & n) != n) {
			continue;
		}
		maskedRoll.push_back(roll[i]);
	}
	return maskedRoll;
}

void removeFromRoll(vector<int>& roll, vector<int>& dice) {
	for (int die : dice) {
		for (int i = 0; i < roll.size(); i++) {
			if (die == roll[i]) {
				roll.erase(roll.begin() + i, roll.begin() + i + 1);
			}
		}
	}
	return;
}

void resetRoll(vector<int>& roll) {
	for (int i = 0; i < roll.size(); i++) {
		int dieColor = roll[i] / 100;
		roll[i] = dieColor * 100 + 1;
	}
	sort(roll.begin(), roll.end());
	return;
}

string rollToString(vector<int> roll, bool sortRoll) {
	if (sortRoll) {
		sort(roll.begin(), roll.end());
	}
	string s = "";
	for (int die : roll) {
		s += to_string(die);
	}
	return s;
}

int distanceDiff(vector<int> r1, vector<int> r2) {
	int diff = 0;
	for (int i = 0; i < r1.size(); i++) {
		bool found = false;
		for (int j = 0; !found && j < r2.size(); j++) {
			if (r1[i] == r2[j]) {
				r2[j] = -1;
				found = true;
			}
		}
		if (!found) {
			diff++;
		}
	}
	return diff;
}

int maskDiff(vector<int> r1, vector<int> r2) {
	int mask = 0;
	for (int i = 0; i < r1.size(); i++) {
		mask <<= 1;
		bool found = false;
		for (int j = 0; !found && j < r2.size(); j++) {
			if (r1[i] == r2[j]) {
				r2[j] = -1;
				found = true;
			}
		}
		if (!found) {
			mask++;
		}
	}
	return mask;
}

int rerollCombinations(vector<int> roll, vector<int> reroll, int mask) {	
	int numGreen = 0;
	int counts[6] = { 0,0,0,0,0,0 };
	for (int i = 0; i < roll.size(); i++) {
		int p = 1 << i;
		if ((mask & p) == p) {
			continue;
		}
		bool found = false;
		for (int j = 0; !found && j < reroll.size(); j++) {
			if (roll[i] == reroll[j]) {
				reroll[j] = -1;
				found = true;
			}
		}
	}
	for (int i = 0; i < reroll.size(); i++) {
		if (reroll[i] == -1) {
			continue;
		}
		if (reroll[i] <= 6) {
			numGreen++;
			counts[reroll[i] - 1]++;
		}
	}
	
	int total = fac(numGreen);
	for (int i = 0; i < 6; i++) {
		total /= fac(counts[i]);
	}
	return total;
}

bool maskedReroll(vector<int> roll, vector<int> reroll, int mask) {
	for (int i = 0; i < roll.size(); i++) {
		int p = 1 << i;
		if ((p & mask) == p) {
			continue;
		}
		bool found = false;
		for (int j = 0; !found && j < reroll.size(); j++) {
			if (roll[i] == reroll[j]) {
				reroll[j] = -1;
				found = true;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}