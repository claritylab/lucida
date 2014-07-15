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
#include "Feature.hh"

using namespace Mm;

//=============================================================================================
void Feature::add(size_t newIndex, const Core::Ref<const Vector> &v)
{
    if (newIndex >= nStreams()) streams_.resize(newIndex + 1);
    Core::Ref<const Vector> &s(streams_[newIndex]);
    require_(!s);
    s = v;
}

//=============================================================================================
const std::string FeatureDescription::defaultName("feature-description");
const std::string FeatureDescription::nameDimension("dimension");
const std::string FeatureDescription::namePortName("port-name");

FeatureDescription::FeatureDescription(const std::string &name) :
    Precursor(name)
{}

FeatureDescription::FeatureDescription(const Core::Configurable &parent) :
    Precursor(prepareName(parent.fullName(), defaultName))
{}

FeatureDescription::FeatureDescription(const std::string &name, const Feature &feature) :
    Precursor(name)
{
    initialize(feature);
}

FeatureDescription::FeatureDescription(
    const Core::Configurable &parent, const Feature &feature) :
    Precursor(prepareName(parent.fullName(), defaultName))
{
    initialize(feature);
}

void FeatureDescription::initialize(const Feature &feature)
{
    for(size_t streamIndex = 0; streamIndex < feature.nStreams(); ++ streamIndex) {
	operator[](streamIndex).setValue(nameDimension, feature[streamIndex]->size());
     }
}
