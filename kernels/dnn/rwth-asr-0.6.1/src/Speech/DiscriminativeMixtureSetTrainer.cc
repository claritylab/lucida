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
#include "DiscriminativeMixtureSetTrainer.hh"
#include <Mm/Module.hh>

using namespace Speech;

/**
 *  Discriminative mixture set trainer
 */
DiscriminativeMixtureSetTrainer::DiscriminativeMixtureSetTrainer(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

DiscriminativeMixtureSetTrainer::~DiscriminativeMixtureSetTrainer()
{}

void DiscriminativeMixtureSetTrainer::accumulateDenominator(
    Core::Ref<const Feature::Vector> f, Mm::MixtureIndex m, Mm::Weight w)
{
    required_cast(Mm::DiscriminativeMixtureSetEstimator*, estimator_)->accumulateDenominator(m, f, w);
}
void DiscriminativeMixtureSetTrainer::accumulateObjectiveFunction(Mm::Score f)
{
    required_cast(Mm::DiscriminativeMixtureSetEstimator*, estimator_)->accumulateObjectiveFunction(f);
}


/**
 *  Convert mixture set trainer
 */
ConvertMixtureSetTrainer::ConvertMixtureSetTrainer(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{
    estimator_ = 0;
}

ConvertMixtureSetTrainer::~ConvertMixtureSetTrainer()
{}

Mm::ConvertMixtureSetEstimator* ConvertMixtureSetTrainer::createMixtureSetEstimator() const
{
    return new Mm::ConvertMixtureSetEstimator(config);
}

void ConvertMixtureSetTrainer::read()
{
    verify(!estimator_);
    Core::Ref<Mm::MixtureSet> mixtureSet =
	Mm::Module::instance().readMixtureSet(config);
    if (mixtureSet) {
	Mm::ConvertMixtureSetEstimator *estimator = createMixtureSetEstimator();
	estimator->setMixtureSet(mixtureSet);
	estimator_ = estimator;
    } else {
	criticalError("Could not read mixture set.");
    }
}
