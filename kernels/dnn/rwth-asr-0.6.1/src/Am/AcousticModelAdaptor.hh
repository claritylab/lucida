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
#ifndef _AM_ACOUSTIC_MODEL_ADAPTOR_HH
#define _AM_ACOUSTIC_MODEL_ADAPTOR_HH

#include "AcousticModel.hh"

namespace Am {

    /**
     * Interface class for acoustic model adaptors
     */
    class AcousticModelAdaptor :
	public virtual Core::Component,
	public Core::ReferenceCounted
    {
    protected:
	Core::Ref<AcousticModel> toAdapt_;
    public:
	AcousticModelAdaptor(const Core::Configuration &c, Core::Ref<AcousticModel> toAdapt) :
	    Component(c), toAdapt_(toAdapt) { require(toAdapt_); }
	virtual ~AcousticModelAdaptor() {}

	Core::Ref<const AcousticModel> acousticModel() const { return toAdapt_; }
    };

    /**
     * MixtureSetAdaptor
     */
    class MixtureSetAdaptor : public AcousticModelAdaptor {
	typedef AcousticModelAdaptor Precursor;
    protected:
	Core::Ref<Mm::MixtureSet> mixtureSet_;
    public:
	MixtureSetAdaptor(const Core::Configuration&, Core::Ref<AcousticModel>);
	virtual ~MixtureSetAdaptor();

	virtual const Core::Ref<Mm::MixtureSet> mixtureSet() const { return mixtureSet_; }
	virtual bool setMixtureSet(const Core::Ref<Mm::MixtureSet>);
	virtual Core::Ref<Mm::MixtureSet> unadaptedMixtureSet() const;
    };

} // namespace Am

#endif //_AM_ACOUSTIC_MODEL_ADAPTOR_HH
