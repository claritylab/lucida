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
#include <Core/Assertions.hh>
#include <Core/StringUtilities.hh>
#include "FastFourierTransform.hh"

using namespace Signal;

FastFourierTransform::FastFourierTransform(u32 length, Data sampleRate) :
    length_(0),
    sampleRate_(0)
{
    setLength(length);
    setInputSampleRate(sampleRate);
}

u32 FastFourierTransform::setLength(u32 length)
{
    if (length == 0)
	return length_ = 0;

    double power = std::log((double)length) / std::log((double)2);
    if (Core::isAlmostEqual(power, rint(power))) power = rint(power);
    else power = ceil(power);
    ensure(power < 32);
    return (length_ = (1 << (u32)power));
}

bool FastFourierTransform::zeroPadding(std::vector<Data> &data)
{
    require_(data.size() <= maximalInputSize());
    data.resize(maximalInputSize(), 0);
    return true;
}

bool FastFourierTransform::estimateContinuous(std::vector<Data> &data)
{
    verify_(sampleRate_ > 0);
    if (sampleRate_ != 1) {
	std::transform(data.begin(), data.end(), data.begin(),
		       std::bind2nd(std::multiplies<Data>(), 1 / (Data)sampleRate_));
    }
    return true;
}

bool FastFourierTransform::transform(std::vector<Data> &data)
{
    if (data.size() > maximalInputSize()) {
	lastError_ = Core::form("Input data size (%zd) is larger then maximal input size (%d).",
				data.size(), maximalInputSize());
	return false;
    }

    return zeroPadding(data) &&
	applyAlgorithm(data) &&
	estimateContinuous(data);
}

// RealFastFourierTransform
///////////////////////////

void RealFastFourierTransform::unpack(std::vector<Data> &data)
{
    require_(data.size() == maximalInputSize());

    data.push_back(data[1]);
    data.push_back(0);
    data[1] = 0;
}

bool RealFastFourierTransform::applyAlgorithm(std::vector<Data> &data)
{
    fft_.transformReal(data);
    unpack(data);
    return true;
}

// RealInverseFastFourierTransform
//////////////////////////////////

bool RealInverseFastFourierTransform::pack(std::vector<Data> &data)
{
    require_(data.size() == maximalInputSize());

    if (data[1] != (Data)0 || data[data.size() - 1] != (Data)0) {
	lastError_ = Core::form("For real inverse FFT, imag(input[0]) and imag(input[FFT-length - 1]) need to be zero.");
	return false;
    }

    data[1] = data[data.size() - 2];
    data.resize(data.size() - 2);
    return true;
}

bool RealInverseFastFourierTransform::applyAlgorithm(std::vector<Data> &data)
{
    if (!pack(data))
	return false;
    fft_.transformReal(data, true);
    return true;
}

bool RealInverseFastFourierTransform::estimateContinuous(std::vector<Data> &data)
{
    verify_(sampleRate_ > 0);
    if (sampleRate_ != 2) {
	std::transform(data.begin(), data.end(), data.begin(),
		       std::bind2nd(std::multiplies<Data>(), 2 / (Data)sampleRate_));
    }
    return true;
}

bool ComplexFastFourierTransform::applyAlgorithm(std::vector<Data> &data)
{
    fft_.transform(data);
    return true;
}

bool ComplexInverseFastFourierTransform::applyAlgorithm(std::vector<Data> &data)
{
    fft_.transform(data, true);
    return true;
}



// FastFourierTransformNode
///////////////////////////

const Core::ParameterInt Signal::paramFftLength(
    "length", "number of FFT points", 0, 0);

const Core::ParameterFloat Signal::paramFftMaximumInputSize(
    "maximum-input-size", "number of FFT points = max-input-size * sampe-rate", 0, 0);
