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
#include "CombinedFeatureScorer.hh"

using namespace Mm;


//==================================================================================================
CombinedFeatureScorer::
CombinedContextScorer::CombinedContextScorer(Core::Ref<const Feature> f,
					     const CombinedFeatureScorer *fs) :
    ContextScorer(),
    combinedFeatureScorer_(fs),
    mixtureIndexTable_(fs->mixtureIndexTable_)
{
    require(f->nStreams() == fs->nModels());
    for(u32 i = 0; i < fs->nModels(); ++ i) {
	Core::Ref<const Feature> modelFeature(new Feature((*f)[i]));
	contextScorers_.push_back(fs->featureScorers_[i]->getScorer(modelFeature));
    }
}

CombinedFeatureScorer::
CombinedContextScorer::CombinedContextScorer(const FeatureVector &featureVector,
					     const CombinedFeatureScorer *fs) :
    ContextScorer(),
    combinedFeatureScorer_(fs),
    mixtureIndexTable_(fs->mixtureIndexTable_)
{
    require(1 == fs->nModels());
    Core::Ref<const Feature> modelFeature(new Feature(featureVector));
    contextScorers_.push_back(fs->featureScorers_.front()->getScorer(modelFeature));
}

Score CombinedFeatureScorer::CombinedContextScorer::score(EmissionIndex e) const
{
    Score result = 0;
    std::vector<Scorer>::const_iterator scorer = contextScorers_.begin();
    std::vector<Scorer>::const_iterator lastScorer = contextScorers_.end();
    MixtureIndexTableRow::const_iterator mixtureIndex = mixtureIndexTable_[e].begin();

	// negativ logarithms of probabilities are added
	// -> we calculate the joined probability of the models
	// assuming statistical independence
    if (scorer != lastScorer) {
	do {
	    result += (*scorer)->score(*mixtureIndex);
	    ++scorer; ++mixtureIndex;
	} while (scorer != lastScorer);
    }
    return result;
}

//==================================================================================================

CombinedFeatureScorer::CombinedFeatureScorer(
    const Core::Configuration &configuration,
    const std::vector<Core::Ref<ScaledFeatureScorer> > &featureScorers,
    const MixtureIndexTable &mixtureIndexTable) :
    Core::Component(configuration),
    Precursor(configuration),
    featureScorers_(featureScorers),
    mixtureIndexTable_(mixtureIndexTable),
    nModels_(featureScorers.size())
{
    require(!featureScorers_.empty());
    require(verifyMixtureIndexTable());

    for(u32 i = 0; i < nModels(); ++ i)
	featureScorers_[i]->setParentScale(scale());
}

CombinedFeatureScorer::~CombinedFeatureScorer()
{}

void CombinedFeatureScorer::distributeScaleUpdate(const Mc::ScaleUpdate &scaleUpdate)
{
    for(u32 i = 0; i < nModels(); ++ i)
	featureScorers_[i]->updateScales(scaleUpdate);
}

bool CombinedFeatureScorer::verifyMixtureIndexTable()
{
    for(EmissionIndex  e = 0; e < nEmissions(); ++ e) {
	if (mixtureIndexTable_[e].size() != nModels()) return false;
    }

    for(size_t i = 0; i < nModels(); ++ i) {
	MixtureIndex nMixtures = featureScorers_[i]->nMixtures();
	for(EmissionIndex e = 0; e < nEmissions(); ++e) {
	    const MixtureIndexTableRow &l(mixtureIndexTable_[e]);
	    if ((l[i] < 0) || (l[i] >= nMixtures)) return false;
	}
    }
    return true;
}

void CombinedFeatureScorer::getFeatureDescription(FeatureDescription &description) const
{
    for(size_t m = 0; m < nModels(); ++ m) {
	FeatureDescription::Stream &stream(description[m]);
	FeatureDescription d(FeatureDescription::prepareName(
				 fullName(), "sub-feature-description"));
	featureScorers_[m]->getFeatureDescription(d);
	ensure(d.nStreams() == 1);
	stream = d.mainStream();
    }
}

void CombinedFeatureScorer::getDependencies(Core::DependencySet &dependencies) const
{
    Core::DependencySet d;
    for(u32 i = 0; i < nModels(); ++ i)
	featureScorers_[i]->Mc::Component::getDependencies(d);

    dependencies.add(name(), d);
    Precursor::getDependencies(dependencies);
}
