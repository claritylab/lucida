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
#include "FastFourierTransform.hh"
#include <Core/Assertions.hh>
#include <ext/numeric>
#include <cmath>

using namespace Math;

const f64 FastFourierTransform::Pi  = 3.141592653589793238;
const f64 FastFourierTransform::DPi = 6.28318530717959;

FastFourierTransform::FastFourierTransform()
    : size_(0)
{}

void FastFourierTransform::createBitReversalReordering(u32 size)
{
    size_ = size;
    u32 length = size / 2;
    reording_.resize(size_);
    __gnu_cxx::iota(reording_.begin(), reording_.end(), 0);

    u32 j = 1;
    for (u32 i = 1; i < size; i += 2) {
	if (j > i) {
	    reording_[i-1] = j - 1;
	    reording_[i] = j;
	}
	u32 m = length;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }
}



void FastFourierTransform::bitReversalReordering(std::vector<f32> &v)
{
    if (v.size() != size_)
	createBitReversalReordering(v.size());
    verify(!reording_.empty());
    for (size_t i = 0; i < size_; ++i) {
	// exchange two complex values
	std::swap(v[i], v[ reording_[i] ]);
    }
}


void FastFourierTransform::transform(std::vector<f32> &v, bool inverse)
{
    const u32 size = v.size();
    const f64 thetaBase = DPi * (inverse ? -1.0 : 1.0);

    bitReversalReordering(v);
    u32 curLength = 2;
    // estimate DFFT using Danielson and Lanczos formula
    while (curLength < size) {
	u32 step = curLength << 1;
	// initialization of trigonometric recurrence
	f64 theta = thetaBase / curLength;
	f64 sinHTheta = std::sin(0.5 * theta);
	f64 wpR = -2.0 * sinHTheta * sinHTheta;
	f64 wpI = std::sin(theta);
	f64 wR = 1.0;
	f64 wI = 0.0;
	for (u32 m = 1; m < curLength; m += 2) {
	    for (u32 i = m; i <= size; i += step) {
		// Danielson & Lanczos formula
		u32 j = i + curLength;
		f32 tmpR = wR * v[j-1] - wI * v[j];
		f32 tmpI = wR * v[j]   + wI * v[j-1];
		v[j-1] = v[i-1] - tmpR;
		v[j]   = v[i]   - tmpI;
		v[i-1] += tmpR;
		v[i]   += tmpI;
	    }
	    // trigonometric recurrence
	    f64 oldWR = wR;
	    wR = wR * wpR - wI    * wpI + wR;
	    wI = wI * wpR + oldWR * wpI + wI;
	}
	curLength = step;
    }
}

void FastFourierTransform::transformReal(std::vector<float> &v, bool inverse )
{
    // the real data is treated as complex array, transformed by transform(..)
    // and then separated to get the transform of the real data
    const u32 size = v.size();
    const u32 sizeD4 = size >> 2;
    const f64 theta = (Pi * (inverse ? -1 : 1)) / (size>>1);
    const f32 c = (inverse ? 0.5 : -0.5);
    if (!inverse) {
	// forward transform data
	transform(v, false);
    }
    // initialization trigonometric recurrence
    f64 sinHTheta = std::sin(0.5 * theta);
    f64 wpR = -2.0 * sinHTheta * sinHTheta;
    f64 wpI = std::sin(theta);
    f64 wR = wpR + 1;
    f64 wI = wpI;

    for (u32 i = 1; i < sizeD4; ++i) {
	u32 i1 = i + i;
	u32 i2 = i1 + 1;
	u32 i3 = size - i1;
	u32 i4 = i3 + 1;
	// separate the two transforms ...
	f64 h1R = 0.5 * (v[i1] + v[i3]);
	f64 h1I = 0.5 * (v[i2] - v[i4]);
	f64 h2R = -c * (v[i2] + v[i4]);
	f64 h2I =  c * (v[i1] - v[i3]);
	// ... and combine them to the true
	// transform of the original real data
	v[i1] =  h1R + wR * h2R - wI * h2I;
	v[i2] =  h1I + wR * h2I + wI * h2R;
	v[i3] =  h1R - wR * h2R + wI * h2I;
	v[i4] = -h1I + wR * h2I + wI * h2R;
	// trigonometric recurrence
	f64 oldWR = wR;
	wR = wR * wpR - wI * wpI + wR;
	wI = wI * wpR + oldWR * wpI + wI;
    }
    if (inverse) {
	f32 h = v[0];
	v[0] = 0.5 * (h + v[1]);
	v[1] = 0.5 * (h - v[1]);
	// inverse transform
	transform(v, true);
    } else {
	f32 h = v[0];
	v[0] = h + v[1];
	v[1] = h - v[1];
    }
}
