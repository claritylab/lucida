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
#include "ScatterMatricesEstimator.hh"

using namespace Speech;


// ----------------------------------------------------------------------------------------

TextDependentScatterMatricesEstimator::TextDependentScatterMatricesEstimator(
    const Core::Configuration &c) :
    Component(c),
    AcousticModelTrainer(c, Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition),
    estimator_(new Signal::ScatterMatricesEstimator(c))
{
    estimator_->setNumberOfClasses(acousticModel()->nEmissions());
}


TextDependentScatterMatricesEstimator::TextDependentScatterMatricesEstimator(
    const Core::Configuration &c, Core::Ref<Signal::ScatterMatricesEstimator> estimator) :
    Component(c),
    AcousticModelTrainer(c, Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition),
    estimator_(estimator)
{
    estimator_->setNumberOfClasses(acousticModel()->nEmissions());
}

void TextDependentScatterMatricesEstimator::setFeatureDescription(
    const Mm::FeatureDescription &description)
{
    description.verifyNumberOfStreams(1);
    size_t d; description.mainStream().getValue(Mm::FeatureDescription::nameDimension, d);
    estimator_->setDimension(d);
}

// ----------------------------------------------------------------------------------------
TextIndependentScatterMatricesEstimator::TextIndependentScatterMatricesEstimator(
    const Core::Configuration &c) :
    Component(c), LabeledFeatureProcessor(c),
    estimator_(new Signal::ScatterMatricesEstimator(c))
{}

TextIndependentScatterMatricesEstimator::TextIndependentScatterMatricesEstimator(
    const Core::Configuration &c, Core::Ref<Signal::ScatterMatricesEstimator> estimator) :
    Component(c), LabeledFeatureProcessor(c), estimator_(estimator)
{}

void TextIndependentScatterMatricesEstimator::setFeatureDescription(
    const Mm::FeatureDescription &description)
{
    description.verifyNumberOfStreams(1);
    size_t d; description.mainStream().getValue(Mm::FeatureDescription::nameDimension, d);
    estimator_->setDimension(d);
}
