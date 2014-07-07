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
#ifndef _MM_COMBINED_FEATURE_SCORER_HH
#define _MM_COMBINED_FEATURE_SCORER_HH

#include "FeatureScorer.hh"
#include "ScaledFeatureScorer.hh"
#include <iostream>

namespace Mm {

    class CombinedFeatureScorer : public ScaledFeatureScorer {
	typedef ScaledFeatureScorer Precursor;
    public:
	typedef std::vector<MixtureIndex> MixtureIndexTableRow;
	typedef std::vector<MixtureIndexTableRow> MixtureIndexTable;
    protected:
	class CombinedContextScorer : public ContextScorer {
	private:
	    friend class CombinedFeatureScorer;
	    const CombinedFeatureScorer *combinedFeatureScorer_;
	    std::vector<Scorer> contextScorers_;
	    const MixtureIndexTable &mixtureIndexTable_;
	public:
	    CombinedContextScorer(Core::Ref<const Feature>,
				  const CombinedFeatureScorer *);
	    CombinedContextScorer(const FeatureVector &,
				  const CombinedFeatureScorer *);
	    virtual Score score(EmissionIndex e) const;
	    virtual EmissionIndex nEmissions() const { return combinedFeatureScorer_->nEmissions(); }
	};
	friend class CombinedContextScorer;
    private:
	std::vector<Core::Ref<ScaledFeatureScorer> > featureScorers_;
	const MixtureIndexTable &mixtureIndexTable_;
	size_t nModels_;
    private:
	bool verifyMixtureIndexTable();
	EmissionIndex nEmissions() const { return mixtureIndexTable_.size(); }
	size_t nModels() const { return nModels_; }
    public:
	CombinedFeatureScorer(const Core::Configuration &configuration,
			      const std::vector<Core::Ref<ScaledFeatureScorer > > &featureScorers,
			      const MixtureIndexTable &mixtureTable);
	virtual ~CombinedFeatureScorer();

	virtual void distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate);

	virtual EmissionIndex nMixtures() const { return nEmissions(); }
	virtual void getFeatureDescription(FeatureDescription &description) const;
	virtual void getDependencies(Core::DependencySet &dependencies) const;

	virtual Scorer getScorer(Core::Ref<const Feature> f) const {
	    return Scorer(new CombinedContextScorer(f, this));
	}
	virtual Scorer getScorer(const FeatureVector &featureVector) const {
	    return Scorer(new CombinedContextScorer(featureVector, this));
	}

    }; // class CombinedFeatureScorer
} // namespace Mm


#endif // _MM_COMBINED_FEATURE_SCORER_HH
