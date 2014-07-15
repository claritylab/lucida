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
#include "AcousticModelAdaptor.hh"
#include <Mm/Module.hh>
using namespace Am;


MixtureSetAdaptor::MixtureSetAdaptor(const Core::Configuration &c,
				     Core::Ref<AcousticModel> toAdapt):
    Component(c), Precursor(c, toAdapt)
{
    Core::Ref<Mm::MixtureSet> mixtureSet = Mm::Module::instance().readMixtureSet(select("mixture-set"));
    if (!mixtureSet || !setMixtureSet(mixtureSet))
	criticalError("failed to initialize mixture set.");
}

MixtureSetAdaptor::~MixtureSetAdaptor() {}

bool MixtureSetAdaptor::setMixtureSet(const Core::Ref<Mm::MixtureSet> mixtureSet)
{
    require(mixtureSet);

    Core::Ref<Mm::ScaledFeatureScorer> featureScorer =
	Mm::Module::instance().createScaledFeatureScorer(
	    select("mixture-set"),
	    Core::Ref<Mm::AbstractMixtureSet>(mixtureSet));
    if (!featureScorer) {
	error("Could not create feature scorer.");
	return false;
    }
    if (!toAdapt_->setFeatureScorer(featureScorer))
	return false;
    mixtureSet_ = mixtureSet;
    return true;
}

Core::Ref<Mm::MixtureSet> MixtureSetAdaptor::unadaptedMixtureSet() const
{
    return Mm::Module::instance().readMixtureSet(select("mixture-set"));
}
