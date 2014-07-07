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
#ifndef _SIGNAL_TEMPORALINTEGRATION_HH
#define _SIGNAL_TEMPORALINTEGRATION_HH

#include <Core/Parameter.hh>
#include <Flow/Data.hh>
#include <Flow/Vector.hh>

#include "TimeWindowBuffer.hh"
#include "WindowFunction.hh"
#include "SlidingAlgorithmNode.hh"

namespace Signal {

/**
* Performs a reduction of the temporal dimension of a (Gammatone) filterbank
* output. A window of the length lengthInS is applied every shiftInS, and
* the samples are summed up, weighted by the window function.
*/
class TemporalIntegration : public TimeWindowBuffer<Flow::Vector<f32> > {
public:
    typedef TimeWindowBuffer<Flow::Vector<f32> > Precursor;
    typedef TimeWindowBuffer<Flow::Vector<f32> >::Time Time;
    typedef Flow::Vector<f32> Sample;
private:
    Time lengthInS_;
    Time shiftInS_;
    WindowFunction* windowFunction_;
protected:
    virtual void init();
    virtual void transform(Flow::Vector<Sample> &out);
public:
    TemporalIntegration();
    virtual ~TemporalIntegration();

    void setWindowFunction(WindowFunction* windowFunction);

    void setSampleRate(f64 sampleRate);

    void setLengthInS(Time length);
    Time lengthInS() const { return lengthInS_; }

    void setShiftInS(Time shift);
    Time shiftInS() const { return shiftInS_; }
};


/**
 * Temporal Integration Node
 *
 * Parameters:
 * shift: shift length in s
 * length: window length in s
*/
class TemporalIntegrationNode : public SlidingAlgorithmNode<TemporalIntegration> {
public:
    typedef SlidingAlgorithmNode<TemporalIntegration> Predecessor;
private:
    static const Core::ParameterFloat paramShift;
    static const Core::ParameterFloat paramLength;
    static const Core::ParameterBool paramFlushAll;
    static const Core::ParameterBool paramFlushBeforeGap;
public:
    static std::string filterName() { return "signal-temporalintegration"; }

    TemporalIntegrationNode(const Core::Configuration &c);
    virtual ~TemporalIntegrationNode() {}

    virtual bool setParameter(const std::string &name, const std::string &value);
    virtual bool configure();
};
} // namespace Signal

#endif // _SIGNAL_TEMPORALINTEGRATION_HH
