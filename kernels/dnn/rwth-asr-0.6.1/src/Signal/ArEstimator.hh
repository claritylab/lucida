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
#ifndef _SIGNAL_ARESTIMATOR_HH
#define _SIGNAL_ARESTIMATOR_HH

#include "CrossCorrelation.hh"
#include "SegmentEstimator.hh"
#include <Math/LevinsonLse.hh>

namespace Signal
{

    /** Autoregression estimator for segments of amplitude spectrum */
    class SegmentwiseArEstimator : public SegmentwiseEstimator {
    public:
	typedef BandpassAutocorrelation::Data Data;

	BandpassAutocorrelation autocorrelation_;
	Math::LevinsonLeastSquares leastSquares_;

	std::vector<Data> autocorrelationFunction_;
	size_t amplitudeLength_;
    public:
	SegmentwiseArEstimator();
	~SegmentwiseArEstimator();

	virtual bool setSignal(const std::vector<Data> &);
	virtual void setOrder(u8 order) { autocorrelationFunction_.resize(order + 1); }

	virtual bool work(Data &estimationError) { return work(&estimationError, 0, 0); }
	bool work(Data* estimation_error, std::vector<Data>* A_tilde, Data* energy);

	s32 getMaxSegmentValue() const { return amplitudeLength_ - 1; };
    };

    /** Flow network parameter for autoregressive coefficients */
    class AutoregressiveCoefficients : public Flow::Timestamp {
	typedef Flow::Timestamp Precursor;
    public:
	typedef f32 Coefficient;
    private:
	Coefficient gain_;
	std::vector<Coefficient> a_;
    public:
	static const Flow::Datatype *type() {
	    static Flow::DatatypeTemplate<AutoregressiveCoefficients> dt("autoregressive-coefficients");
	    return &dt;
	}
	AutoregressiveCoefficients() : Precursor(type()), gain_(0) {}
	virtual ~AutoregressiveCoefficients() {}
	virtual Data* clone() const { return new AutoregressiveCoefficients(*this); };

	Coefficient& gain() { return gain_; };
	std::vector<Coefficient>& a() { return a_; };

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const;
	virtual bool read(Core::BinaryInputStream &i);
	virtual bool write(Core::BinaryOutputStream &o) const;
    };

    /** Estimates autoregression parameters from autocorrelation.
     *  Input: autocorrelation.
     *  Output is an autoregressive-parameter object.
     */
    class AutocorrelationToAutoregressionNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	Math::LevinsonLeastSquares levinsonLeastSquares_;
    public:
	static std::string filterName() {
	    return std::string("signal-autocorrelation-to-autoregression");
	}
	AutocorrelationToAutoregressionNode(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c) {}
	virtual ~AutocorrelationToAutoregressionNode() {}

	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };
}

#endif // _SIGNAL_ARESTIMATOR_HH
