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
#ifndef SIGNAL_DELIMITER_HH
#define SIGNAL_DELIMITER_HH

#include <Core/Component.hh>

namespace Signal {

    /** Delimiter class for start stop detection.
     */
    class Delimiter : public Core::Component
    {
    public:
	typedef u32 TimeframeIndex;
	typedef std::pair<TimeframeIndex, TimeframeIndex> Delimitation;
	static const Core::ParameterInt paramNumberOfIterations;
	static const Core::ParameterFloat paramPenalty;
	static const Core::ParameterFloat paramMinimumSpeechProportion;
    protected:
	u32 numberOfIterations_;
	f64 f_; // penalty
	f32 minimumSpeechProportion_;
	std::vector<f64> Q_;
	std::vector<f64> M_;

	mutable Core::XmlChannel statisticsChannel_;
    protected:
	f64 logLikelihood(u32 ib, u32 ie) const;
    public:
	Delimiter(const Core::Configuration&);
	virtual ~Delimiter() {}

	void reset() {
	    Q_.clear(); Q_.push_back(0);
	    M_.clear(); M_.push_back(0);
	}
	void feed(f64 xx) {
	    M_.push_back(M_.back() + xx);
	    if (f_ > 0) {
		Q_.push_back(Q_.back() + (xx * xx + f_ / 2.0) / f_);
	    } else {
		Q_.push_back(Q_.back() + xx * xx);
	    }
	    if (Q_.back() > Core::Type<f64>::max)
		error("delimit overflow => segment to long?");
	}
	virtual Delimitation getDelimitation() const;
	const u32 nFeatures() const {
	    verify(Q_.size() == M_.size());
	    return Q_.size() - 1;
	}
    }; // end class


    /** Sietill Delimiter (silence  proportion may be large)
     */
    class SietillDelimiter : public Delimiter
    {
	typedef Delimiter Precursor;
    public:
	SietillDelimiter(const Core::Configuration &c) : Precursor(c) {}
	virtual ~SietillDelimiter() {}

	virtual Delimitation getDelimitation() const;
    };


} // end namespace

#endif  // end ifndef
