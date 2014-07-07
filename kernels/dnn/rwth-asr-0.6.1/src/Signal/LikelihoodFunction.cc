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
#include <Core/Directory.hh>
#include <Mm/Module.hh>
#include "LikelihoodFunction.hh"

using namespace Signal;


// IndependentSequenceLikelihood
////////////////////////////////


IndependentSequenceLikelihood::IndependentSequenceLikelihood(const Core::Configuration &c) :
    Component(c),
    Precursor(c)
{
    logLikelihoodFunctions_ = Mm::Module::instance().createFeatureScorer(c);
    if (logLikelihoodFunctions_ == 0)
	error("Could not initialize the log-likelihood function.");
}

IndependentSequenceLikelihood::~IndependentSequenceLikelihood()
{}

bool IndependentSequenceLikelihood::setClasses(const std::vector<std::string> &classLabels)
{
    if (logLikelihoodFunctions_ == 0)
	return false;
    if (logLikelihoodFunctions_->nMixtures() != classLabels.size()) {
	error("Number of mixtures (%d) does not match to the number of classes (%zd).",
	      logLikelihoodFunctions_->nMixtures(), classLabels.size());
	return false;
    }
    scores_.resize(classLabels.size());
    reset();
    return true;
}

bool IndependentSequenceLikelihood::setDimension(size_t dimension)
{
    if (logLikelihoodFunctions_ == 0)
	return false;
    Mm::FeatureDescription description(Core::Configuration::prepareResourceName(
					   fullName(), "feature-description"));
    logLikelihoodFunctions_->getFeatureDescription(description);
    if (!description.verifyNumberOfStreams(1) ||
	!description.mainStream().verifyValue("dimension", dimension)) {
	error("Failed to set dimension");
	return false;
    }
    return true;
}

void IndependentSequenceLikelihood::reset()
{
    std::fill(scores_.begin(), scores_.end(), 0);
    Precursor::reset();
}

void IndependentSequenceLikelihood::feed(
    const std::vector<Data> &featureVector, Weight featureScoreWeight,
    ScoreVector *currentScores)
{
    verify_(logLikelihoodFunctions_ != 0);

    Core::Ref<const Mm::Feature> feature(new Mm::Feature(featureVector));
    Mm::FeatureScorer::Scorer scorer = logLikelihoodFunctions_->getScorer(feature);
    size_t nClasses = logLikelihoodFunctions_->nMixtures();
    if (currentScores) currentScores->resize(nClasses);
    for(u32 c = 0; c < nClasses; ++ c) {
	Score score = featureScoreWeight * scorer->score(c);
	if (currentScores) (*currentScores)[c] = score;
	scores_[c] += score;
    }
    Precursor::feed(featureVector, featureScoreWeight);
}
