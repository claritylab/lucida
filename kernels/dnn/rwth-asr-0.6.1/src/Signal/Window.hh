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
#ifndef _SIGNAL_WINDOW_HH
#define _SIGNAL_WINDOW_HH


#include <Core/Parameter.hh>

#include <Flow/Data.hh>
#include <Flow/Vector.hh>

#include "WindowBuffer.hh"
#include "WindowFunction.hh"
#include "SlidingAlgorithmNode.hh"


namespace Signal {

    /** Window */

    class Window : public WindowBuffer {
    public:
	typedef WindowBuffer Predecessor;
	typedef WindowBuffer::Time Time;
	typedef WindowBuffer::Sample Sample;
    private:
	Time lengthInS_;
	Time shiftInS_;
	WindowFunction* windowFunction_;
    protected:
	virtual void init();
	virtual void transform(Flow::Vector<Sample> &out);
    public:
	Window();
	virtual ~Window();

	void setWindowFunction(WindowFunction* windowFunction);

	void setSampleRate(f64 sampleRate);

	void setLengthInS(Time length);
	Time lengthInS() const { return lengthInS_; }

	void setShiftInS(Time shift);
	Time shiftInS() const { return shiftInS_; }
    };


    /** WindowNode */
    class WindowNode : public SlidingAlgorithmNode<Window> {
    public:
	typedef SlidingAlgorithmNode<Window> Predecessor;
    private:
	static const Core::ParameterFloat paramShift;
	static const Core::ParameterFloat paramLength;
	static const Core::ParameterBool paramFlushAll;
	static const Core::ParameterBool paramFlushBeforeGap;
    public:
	static std::string filterName() { return "signal-window"; }

	WindowNode(const Core::Configuration &c);
	virtual ~WindowNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
    };

}


#endif // _SIGNAL_WINDOW_HH
