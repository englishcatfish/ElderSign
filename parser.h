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
#include <string>
#include <vector>
#include <algorithm>

#include "const.h"

using namespace std;

string serializeTasks(vector<vector<int>> tasks, bool inOrder) {
	string str = "";
	for (int i = 0; i < tasks.size(); i++) {
		if (i > 0) {
			str += (inOrder ? ">" : "|");
		}
		for (int j = 0; j < tasks[i].size(); j++) {
			if (j > 0) {
				str += ",";
			}
			str += to_string(tasks[i][j]);
		}
	}
	return str;
}

string serialize(vector<vector<int>> tasks, bool inOrder, int numGreen, bool yellowDie, bool redDie, int numFocus, int numSpell, int numClue, vector<int> heldDice) {
	sort(heldDice.begin(), heldDice.end());
	string heldStr = to_string(numFocus) + ":" + to_string(numSpell) + ":" + to_string(numClue) + ":";
	for (int i = 0; i < heldDice.size(); i++) {
		if (i > 0) {
			heldStr += ",";
		}
		heldStr += to_string(heldDice[i]);
	}
	string tasksStr = serializeTasks(tasks, inOrder);
	string s = tasksStr + ":" + to_string(numGreen) + ":" + to_string(yellowDie) + ":" + to_string(redDie) + ":" + heldStr;
	return s;
}

vector<string> split(std::string& str, char delim = ' ') {
	vector<string> cont = vector<string>();
	std::size_t current, previous = 0;
	current = str.find(delim);
	while (current != std::string::npos) {
		cont.push_back(str.substr(previous, current - previous));
		previous = current + 1;
		current = str.find(delim, previous);
	}
	cont.push_back(str.substr(previous, current - previous));
	return cont;
}

void parseTasks(string task, vector<vector<int>>& tasks, bool& inOrder) {
	tasks = vector<vector<int>>();
	inOrder = task.find('>') != -1;

	if (task.compare("") == 0) {
		return;
	}

	vector<string> s1 = inOrder ? split(task, '>') : split(task, '|');
	for (int i = 0; i < s1.size(); i++) {
		vector<string> ints = split(s1[i], ',');
		vector<int> task = vector<int>();
		for (int j = 0; j < ints.size(); j++) {
			task.push_back(goalConv(ints[j]));
		}
		sort(task.begin(), task.end());
		tasks.push_back(task);
	}
}

string tabs(int n) {
	string s = "";
	for (int i = 0; i < n; i++) {
		s += (i == n - 1) ? "|_" : "  ";
	}
	return s;
}