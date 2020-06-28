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

#include <iostream>

using namespace std;

const int INV_1 = 1;
const int INV_2 = 2;
const int INV_3 = 3;
const int TERROR = 4;
const int PERIL = 5;
const int SCROLL = 6;
const int INV_4 = 7;
const int INV_5 = 8;
const int INV_6 = 9;
const int INV_7 = 10;
const int INV_8 = 11;
const int INV_9 = 12;
const int INV_10 = 13;
const int INV_11 = 14;
const int INV_12 = 15;
const int TERROR_PERIL = 16;
const int TERROR_SCROLL = 17;
const int PERIL_SCROLL = 18;
const int Y_INV_1 = 101;
const int Y_INV_2 = 102;
const int Y_INV_3 = 103;
const int Y_INV_4 = 104;
const int Y_PERIL = 105;
const int Y_SCROLL = 106;
const int R_INV_2 = 201;
const int R_INV_3 = 202;
const int R_INV_4 = 203;
const int R_WILD = 204;
const int R_PERIL = 205;
const int R_SCROLL = 206;

const int GREEN = 0;
const int YELLOW = 1;
const int RED = 2;

int goalConv(string goal) {
	if (goal == "INV_1") {
		return INV_1;
	}
	else if (goal == "INV_2") {
		return INV_2;
	}
	else if (goal == "INV_3") {
		return INV_3;
	}
	else if (goal == "INV_4") {
		return INV_4;
	}
	else if (goal == "INV_5") {
		return INV_5;
	}
	else if (goal == "INV_6") {
		return INV_6;
	}
	else if (goal == "INV_7") {
		return INV_7;
	}
	else if (goal == "INV_8") {
		return INV_8;
	}
	else if (goal == "INV_9") {
		return INV_9;
	}
	else if (goal == "INV_10") {
		return INV_10;
	}
	else if (goal == "INV_11") {
		return INV_11;
	}
	else if (goal == "INV_12") {
		return INV_12;
	}
	else if (goal == "TERROR") {
		return TERROR;
	}
	else if (goal == "PERIL") {
		return PERIL;
	}
	else if (goal == "SCROLL") {
		return SCROLL;
	}
	else if (goal == "TERROR+PERIL" || goal == "PERIL+TERROR") {
		return TERROR_PERIL;
	}
	else if (goal == "TERROR+SCROLL" || goal == "SCROLL+TERROR") {
		return TERROR_SCROLL;
	}
	else if (goal == "PERIL+SCROLL" || goal == "SCROLL+PERIL") {
		return PERIL_SCROLL;
	}
	else {
		cout << "UNKNOWN GOAL: " << goal << endl;
		return -1;
	}
}

int getInvValue(int inv) {
	switch (inv) {
	case INV_1:
	case Y_INV_1:
		return 1;
	case INV_2:
	case Y_INV_2:
	case R_INV_2:
		return 2;
	case INV_3:
	case Y_INV_3:
	case R_INV_3:
		return 3;
	case INV_4:
	case Y_INV_4:
	case R_INV_4:
		return 4;
	case INV_5:
		return 5;
	case INV_6:
		return 6;
	case INV_7:
		return 7;
	case INV_8:
		return 8;
	case INV_9:
		return 9;
	case INV_10:
		return 10;
	case INV_11:
		return 11;
	case INV_12:
		return 12;
	}
	return -1;
}