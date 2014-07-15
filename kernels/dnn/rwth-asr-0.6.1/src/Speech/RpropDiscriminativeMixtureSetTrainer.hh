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
#ifndef _SPEECH_RPROP_DISCRIMINATIVE_MIXTURE_SET_TRAINER_HH
#define _SPEECH_RPROP_DISCRIMINATIVE_MIXTURE_SET_TRAINER_HH

#include "DiscriminativeMixtureSetTrainer.hh"

namespace Speech {

    /**
     * discriminative mixture set trainer: Rprop
     */
    class RpropDiscriminativeMixtureSetTrainer : public DiscriminativeMixtureSetTrainer
    {
	typedef DiscriminativeMixtureSetTrainer Precursor;
    protected:
	Mm::DiscriminativeMixtureSetEstimator* createMixtureSetEstimator() const;
    public:
	RpropDiscriminativeMixtureSetTrainer(const Core::Configuration &);
	virtual ~RpropDiscriminativeMixtureSetTrainer();
    };

    /**
     * Rprop with i-smoothing
     */
    class RpropDiscriminativeMixtureSetTrainerWithISmoothing :
	public DiscriminativeMixtureSetTrainer
    {
	typedef DiscriminativeMixtureSetTrainer Precursor;
    protected:
	Mm::DiscriminativeMixtureSetEstimator* createMixtureSetEstimator() const;
    public:
	RpropDiscriminativeMixtureSetTrainerWithISmoothing(const Core::Configuration &);
	virtual ~RpropDiscriminativeMixtureSetTrainerWithISmoothing();
    };


}

#endif // _SPEECH_RPROP_DISCRIMINATIVE_MIXTURE_SET_TRAINER_HH
