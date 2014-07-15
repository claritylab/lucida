// Copyright 2011 RWTH Aachen University. All rights reserved.
//
// Licensed under the RWTH ASR License (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.hltpr.rwth-aachen.de/rwth-asr/rwth-asr-license.html
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <Core/Utility.hh>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <string>
#include <algorithm>

using namespace Core;

int Core::getline(std::istream& is, std::string& str, std::string delim) {
    int token;
    std::string::size_type pos = std::string::npos;

    // check if end of stream is reached
    if (is.get() == EOF) return EOF;
    is.unget();

    // tokenize stream contents
    str = "";
    while (((token = is.get()) != EOF) &&
	   ((pos = delim.find(token)) == std::string::npos)) {
	str += char(token);
    }

    if (pos == std::string::npos) return 0;

    return pos + 1;
}

std::string& Core::itoa(std::string &s, unsigned int val) {
    s = "";
    if (val < 10) { // small integers are very frequent
	s += ('0' + val);
    } else {
	do {
	    s += ('0' + (val % 10));
	    val /= 10;
	    } while (val);
	std::reverse(s.begin(), s.end());
    }
    return s;
}

s32 Core::differenceUlp(f32 af, f32 bf) {
    union {
	s32 i;
	f32 f;
    } a, b;
    a.f = af;
    b.f = bf;
    if (a.i < 0) a.i = 0x80000000 - a.i;
    if (b.i < 0) b.i = 0x80000000 - b.i;
    return Core::abs(a.i - b.i);
}

s64 Core::differenceUlp(f64 af, f64 bf) {
    union {
	s64 i;
	f64 f;
    } a, b;
    a.f = af;
    b.f = bf;
    if (a.i < 0) a.i = (s64(1) << 63) - a.i;
    if (b.i < 0) b.i = (s64(1) << 63) - b.i;
    return Core::abs(a.i - b.i);
}
