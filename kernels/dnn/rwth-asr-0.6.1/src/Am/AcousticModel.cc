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
#include "AcousticModel.hh"
#include "Utilities.hh"

using namespace Am;

// ===========================================================================
const AcousticModel::Mode AcousticModel::complete = 0x0;
const AcousticModel::Mode AcousticModel::noEmissions = 0x1;
const AcousticModel::Mode AcousticModel::noStateTying = 0x2;
const AcousticModel::Mode AcousticModel::noStateTransition = 0x4;

bool AcousticModel::isCompatible(const Mm::FeatureDescription &description) const
{
    Core::DependencySet dependencies;
    getDependencies(dependencies);

    Core::DependencySet d;
    description.getDependencies(d);
    Core::DependencySet featureDependencies;
    featureDependencies.add(name(), d);

    if (!dependencies.satisfies(featureDependencies)) {
	warning("Feature mismatch between acoustic model and feature extraction.");
	return false;
    }
    return true;
}

bool AcousticModel::isWeakCompatible(const Mm::FeatureDescription& description) const
{
    Core::DependencySet dependencies;
    getDependencies(dependencies);

    Core::DependencySet d;
    description.getDependencies(d);
    Core::DependencySet featureDependencies;
    featureDependencies.add(name(), d);

    if (!dependencies.weak_satisfies(featureDependencies)) {
	error("Feature mismatch between acoustic model and feature extraction even in weak check.");
	return false;
    }
    return true;
}


bool AcousticModel::setKey(const std::string &key) { return true; };

Core::Ref<const EmissionAlphabet> AcousticModel::emissionAlphabet() const {
    Core::Ref<const EmissionAlphabet> result;
    if (emissionAlphabet_)
	result = emissionAlphabet_;
    else
	emissionAlphabet_ = result = Core::ref(new EmissionAlphabet(nEmissions()));
    return result;
}

// ===========================================================================
TransducerBuilder::TransducerBuilder() {
}

TransducerBuilder::~TransducerBuilder() {}
