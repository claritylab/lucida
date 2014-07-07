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
#include <Modules.hh>
#include "WindowFunction.hh"

using namespace Core;
using namespace Signal;

// WindowFunction
/////////////////

Choice WindowFunction::typeChoice("rectangular", Rectangular,
				  "hamming", Hamming,
				  "hanning",  Hanning,
				  "bartlett", Bartlett,
				  "blackman", Blackman,
				  "kaiser", Kaiser,
				  Choice::endMark());
ParameterChoice  WindowFunction::paramType("type", &typeChoice, "type of window", Hamming);

WindowFunction* WindowFunction::create(Type type)
{
    switch(type)
	{
	case WindowFunction::Rectangular:
	    return new RectangularWindowFunction;
	case WindowFunction::Hamming:
	    return new HammingWindowFunction;
	case WindowFunction::Hanning:
	    return new HanningWindowFunction;
	case WindowFunction::Bartlett:
	    return new BartlettWindowFunction;
	case WindowFunction::Blackman:
	    return new BlackmanWindowFunction;
	}
    return 0;
}


void WindowFunction::setLength(u32 l) {

    if (length() != l) {

	window_.resize(l);
	needInit_ = true;
    }
}


// RectangularWindowFunction
///////////////////////////

bool RectangularWindowFunction::init() {

    for (u32 n = 0; n < window_.size(); n ++)
	window_[n] = 1.0;

    return WindowFunction::init();
}


// BartlettWindowFunction
////////////////////////

bool BartlettWindowFunction::init() {

    if (window_.size() <= 1)
	return false;

    u32 M = window_.size() - 1;
    for (u32 n = 0; n <= M / 2; n ++)
	window_[n] = window_[M - n] = 2.0 * (Float)n / (Float)M;

    return WindowFunction::init();
}


// HammingWindowFunction
////////////////////////

bool HammingWindowFunction::init() {

    if (window_.size() <= 1)
	return false;

    u32 M = window_.size() - 1;
    for (u32 n = 0; n <= M / 2; n ++)
	window_[n] = window_[M - n] = 0.54 - 0.46 * cos(2.0 * M_PI * n / M);

    return WindowFunction::init();
}


// HanningWindowFunction
///////////////////////

bool HanningWindowFunction::init() {

    if (window_.size() <= 1)
	return false;

    u32 M = window_.size() - 1;
    for (u32 n = 0; n <= M / 2; n ++)
	window_[n] = window_[M - n] = 0.5 - 0.5 * cos(2.0 * M_PI * n / M);

    return WindowFunction::init();
}


// BlackmanWindowFunction
////////////////////////

bool BlackmanWindowFunction::init() {

    if (window_.size() <= 1)
	return false;

    u32 M = window_.size() - 1;

    for (u32 n = 0; n <= M / 2; n ++)
	window_[n] = window_[M - n] =
	    0.42 - 0.5 * cos( 2.0 * M_PI * n / M) + 0.08 * cos( 4.0 * M_PI * n / M);

    return WindowFunction::init();
}
