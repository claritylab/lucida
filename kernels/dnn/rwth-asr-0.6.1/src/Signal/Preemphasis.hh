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
#ifndef _SIGNAL_PREEMPHASIS_HH
#define _SIGNAL_PREEMPHASIS_HH

#include <Core/Parameter.hh>

#include <Flow/Data.hh>
#include <Flow/Vector.hh>

#include "Node.hh"

namespace Signal {

    /** Preemphasis */

    class Preemphasis {
    private:

	f32 alpha_;
	f32 previous_;

	Flow::Time previousEndTime_;
	f64 sampleRate_;

	bool needInit_;

	void init(f32 initialValue);

    public:

	Preemphasis();


	void setAlpha(f32 alpha);

	void setSampleRate(f64 sampleRate);


	void reset(void) { needInit_ = true; }


	void apply(Flow::Vector<f32> &v);
    };


    /** PreemphasisNode */

    class PreemphasisNode : public SleeveNode, Preemphasis {
    private:
	static Core::ParameterFloat paramAlpha;

    public:

	static std::string filterName() { return "signal-preemphasis"; }

	PreemphasisNode(const Core::Configuration &c);

	virtual ~PreemphasisNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);


	virtual bool configure();


	virtual void reset() { Preemphasis::reset(); }


	virtual bool work(Flow::PortId p);
    };

} // namespace Signal

#endif // _SIGNAL_PREEMPHASIS_HH
