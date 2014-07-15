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
#ifndef _SPEECH_DISCRIMINATIVE_MIXTURE_SET_TRAINER_HH
#define _SPEECH_DISCRIMINATIVE_MIXTURE_SET_TRAINER_HH

#include "MixtureSetTrainer.hh"
#include <Mm/DiscriminativeMixtureSetEstimator.hh>
#include <Mm/ConvertMixtureSetEstimator.hh>

namespace Speech
{

    /**
     * discriminative mixture set trainer
     */
    class DiscriminativeMixtureSetTrainer : public MixtureSetTrainer
    {
	typedef MixtureSetTrainer Precursor;
    public:
	DiscriminativeMixtureSetTrainer(const Core::Configuration &);
	virtual ~DiscriminativeMixtureSetTrainer();

	void accumulateDenominator(Core::Ref<const Feature::Vector> f, Mm::MixtureIndex m, Mm::Weight w);
	void accumulateObjectiveFunction(Mm::Score f);

	static DiscriminativeMixtureSetTrainer* createDiscriminativeMixtureSetTrainer(
	    const Core::Configuration &, bool iSmoothing);
    };


    /**
     * Mixture set estimator to convert from one mixture set estimator to
     * a maximum likelihood mixture set estimator.
     * This is useful in the context of gradient-based optimization methods
     * where the previous mixture set is required for re-estimation,
     * like for example in discriminative training.
     */
    class ConvertMixtureSetTrainer : public MlMixtureSetTrainer
    {
	typedef MlMixtureSetTrainer Precursor;
    protected:
	virtual Mm::ConvertMixtureSetEstimator* createMixtureSetEstimator() const;
    public:
	ConvertMixtureSetTrainer(const Core::Configuration &);
	virtual ~ConvertMixtureSetTrainer();

	virtual void read();
    };

}

#endif // _SPEECH_DISCRIMINATIVE_MIXTURE_SET_TRAINER_HH
