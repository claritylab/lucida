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
#ifndef _SPEECH_MIXTURE_SET_TRAINER_HH
#define _SPEECH_MIXTURE_SET_TRAINER_HH

#include <Mm/MixtureSet.hh>
#include <Mm/AbstractMixtureSetEstimator.hh>
#include <Mm/AssigningFeatureScorer.hh>
#include <Mm/MixtureSetEstimator.hh>
#include "DataExtractor.hh"

namespace Speech {

     /**
     * MixtureSetTrainer
     */
    class MixtureSetTrainer : virtual public Core::Component
    {
	typedef Component Precursor;
    public:
	static const Core::ParameterString paramOldMixtureSetFilename;
	static const Core::ParameterString paramNewMixtureSetFilename;
	static const Core::ParameterBool paramSplitFirst;
	static const Core::ParameterBool paramForceCovarianceTying;
    protected:
	Mm::AbstractMixtureSetEstimator *estimator_;
	Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer_;
    protected:
	virtual Mm::AbstractMixtureSetEstimator* createMixtureSetEstimator() const = 0;
	const Core::Ref<Mm::MixtureSet> estimate() const;
	void setAssigningFeatureScorer(
	    const Core::Ref<Mm::MixtureSet>,
	    Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer = Core::Ref<Mm::AssigningFeatureScorer>());
	bool firstRun() { return paramOldMixtureSetFilename(config).empty(); }
// 	void checkNumberOfMixture(size_t nMixtures);
// 	void checkFeatureDimension(size_t dimension);
	void checkCovarianceTying();
	void clear();
	bool read(const std::string &filename, Mm::AbstractMixtureSetEstimator &);
	void read(const std::string &filename);
	void write(const std::string &filename) const;
	virtual const Core::Ref<Mm::MixtureSet> getMixtureSet(size_t nMixtures, size_t dimension);
    public:
	MixtureSetTrainer(const Core::Configuration &);
	virtual ~MixtureSetTrainer();

	void initializeAccumulation(
	    size_t nMixtures, size_t dimension,
	    Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer = Core::Ref<Mm::AssigningFeatureScorer>(),
	    Core::Ref<Mm::MixtureSet> mixtureSet = Core::Ref<Mm::MixtureSet>());
	void accumulate(Core::Ref<const Feature::Vector> f, Mm::MixtureIndex m) {
	    verify_(estimator_ != 0);
	    estimator_->accumulate(m, f);
	}
	void accumulate(Core::Ref<const Feature::Vector> f, Mm::MixtureIndex m, Mm::Weight w) {
	    verify_(estimator_ != 0); estimator_->accumulate(m, f, w);
	}

	virtual void read();
	bool combine(const std::vector<std::string> &toCombine);
	bool combinePartitions(const std::vector<std::string> &toCombine);

	void write() const { write(paramNewMixtureSetFilename(config)); }
	void write(Core::XmlWriter &o) const { verify(estimator_ != 0); estimator_->write(o); }

	size_t dimension() const { return estimator_ != 0 ? estimator_->dimension() : 0; }
	size_t nMixtures() const { return estimator_ != 0 ? estimator_->nMixtures() : 0; }

	bool map(const std::string &mappingFilename);
	void setWeightThreshold(Mm::Weight weightThreshold) {
	    if (estimator_) estimator_->setWeightThreshold(weightThreshold);
	}
	void setAssigningFeatureScorer(
	    Core::Ref<const Mm::AssigningFeatureScorer> assigningFeatureScorer);
    };

    /**
     * maximum likelihood mixture set trainer
     */
    class MlMixtureSetTrainer : public MixtureSetTrainer
    {
	typedef MixtureSetTrainer Precursor;
    protected:
	virtual Mm::MixtureSetEstimator* createMixtureSetEstimator() const;
    public:
	MlMixtureSetTrainer(const Core::Configuration &);
	virtual ~MlMixtureSetTrainer();
    };

} // namespace Speech

#endif // _SPEECH_MIXTURE_SET_TRAINER_HH
