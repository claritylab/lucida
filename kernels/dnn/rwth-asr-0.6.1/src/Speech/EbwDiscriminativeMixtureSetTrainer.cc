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
#include "EbwDiscriminativeMixtureSetTrainer.hh"
#include <Mm/EbwDiscriminativeMixtureSetEstimator.hh>

using namespace Speech;

/**
 *  EBW without i-smoothing
 */
EbwDiscriminativeMixtureSetTrainer::EbwDiscriminativeMixtureSetTrainer(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

EbwDiscriminativeMixtureSetTrainer::~EbwDiscriminativeMixtureSetTrainer()
{}

Mm::DiscriminativeMixtureSetEstimator*
EbwDiscriminativeMixtureSetTrainer::createMixtureSetEstimator() const
{
    return new Mm::EbwDiscriminativeMixtureSetEstimator(config);
}


/**
 *  EBW with i-smoothing
 */
EbwDiscriminativeMixtureSetTrainerWithISmoothing::EbwDiscriminativeMixtureSetTrainerWithISmoothing(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c)
{}

EbwDiscriminativeMixtureSetTrainerWithISmoothing::~EbwDiscriminativeMixtureSetTrainerWithISmoothing()
{}

Mm::DiscriminativeMixtureSetEstimator*
EbwDiscriminativeMixtureSetTrainerWithISmoothing::createMixtureSetEstimator() const
{
    return new Mm::EbwDiscriminativeMixtureSetEstimatorWithISmoothing(config);
}
