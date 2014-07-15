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
#ifndef _MM_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH
#define _MM_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH

#include "AbstractMixtureSetEstimator.hh"
#include "MixtureSetEstimator.hh"
#include "DiscriminativeMixtureEstimator.hh"

namespace Mm {

    /**
     * DiscriminativeMixtureSetEstimator
     */
    class DiscriminativeMixtureSetEstimator : public AbstractMixtureSetEstimator
    {
	typedef AbstractMixtureSetEstimator Precursor;
    protected:
	Sum objectiveFunction_;
	Core::Configuration mixtureEstimatorConfig_;
    protected:
	virtual DiscriminativeMixtureEstimator& mixtureEstimator(MixtureIndex mixture) {
	    return *required_cast(DiscriminativeMixtureEstimator*, mixtureEstimators_[mixture].get());
	}
	void loadPreviousMixtureSet();
	virtual bool accumulateMixture(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os) { return false; }
	virtual const std::string magic() const { return "DTACC"; }
	virtual void load();
	virtual void initialize(const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &) {}
	virtual void finalize(MixtureSet &, const MixtureSetEstimatorIndexMap &, const CovarianceToMeanSetMap &);
    public:
	DiscriminativeMixtureSetEstimator(const Core::Configuration &);
	virtual ~DiscriminativeMixtureSetEstimator();

	virtual void reset();
	virtual void read(Core::BinaryInputStream &);
	virtual void write(Core::BinaryOutputStream &);
	virtual void write(Core::XmlWriter &);
	virtual bool operator==(const AbstractMixtureSetEstimator&) const;
	virtual void estimate(MixtureSet &);

	virtual bool accumulate(Core::BinaryInputStreams &is, Core::BinaryOutputStream &os);
	virtual bool accumulate(const AbstractMixtureSetEstimator&);
	void accumulateDenominator(
	    MixtureIndex mixtureIndex, Core::Ref<const Feature::Vector>, Weight);
	void accumulateObjectiveFunction(Score);
	Sum objectiveFunction() const { return objectiveFunction_; }
	bool distributePreviousMixtureSet(const MixtureSet &);
    };

} //namespace Mm

#endif //_MM_DISCRIMINATIVE_MIXTURE_SET_ESTIMATOR_HH
