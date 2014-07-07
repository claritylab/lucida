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
#ifndef _SPEECH_SEGMENTWISE_GMM_TRAINER_HH
#define _SPEECH_SEGMENTWISE_GMM_TRAINER_HH

#include "AcousticSegmentwiseTrainer.hh"
#include "DiscriminativeMixtureSetTrainer.hh"
#include <Lattice/Accumulator.hh>

namespace Speech {

    typedef Lattice::CachedAcousticAccumulator<DiscriminativeMixtureSetTrainer> GmmAccumulator;

    /*
     * SegmentwiseGmmTrainer
     * This is the interface for the discriminative
     * GHMM training for different training criteria.
     * Each criterion corresponds with a separate derived
     * class.
     */
    class SegmentwiseGmmTrainer : public AcousticSegmentwiseTrainer<GmmAccumulator>
    {
	typedef AcousticSegmentwiseTrainer<GmmAccumulator> Precursor;
    private:
	Mm::FeatureDescription featureDescription_;
    protected:
	bool initialized_;
	DiscriminativeMixtureSetTrainer *mixtureSetTrainer_;
    protected:
	virtual void setFeatureDescription(const Mm::FeatureDescription &);
	virtual GmmAccumulator* createAcc();
	virtual GmmAccumulator* createNumAcc();
	virtual GmmAccumulator* createDenAcc();
	virtual GmmAccumulator* createMleAcc();

	void accumulateObjectiveFunction(f32);
	void initializeMixtureSetTrainer();
    public:
	SegmentwiseGmmTrainer(
	    const Core::Configuration &);
	virtual ~SegmentwiseGmmTrainer();

	virtual void leaveCorpus(Bliss::Corpus *);

	virtual void write() const { mixtureSetTrainer_->write(); }

	static SegmentwiseGmmTrainer*
	createSegmentwiseGmmTrainer(const Core::Configuration &);
    };


    /*
     * SegmentwiseGmmTrainer: risk based
     * All risk-based training criteria can be
     * optimized by this class.
     * Assumption: risk transducer is passed in
     * addition to total scores.
     * see Heigold et al., Modified MMI/MPE, ICML 2008, for a description of the transducer based gradient accumulation
     *
     */
    class MinimumErrorSegmentwiseGmmTrainer : public SegmentwiseGmmTrainer
    {
	typedef SegmentwiseGmmTrainer Precursor;
    public:
	MinimumErrorSegmentwiseGmmTrainer(const Core::Configuration &);
	virtual ~MinimumErrorSegmentwiseGmmTrainer() {}
	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };


} //namespace Speech

#endif // u_SPEECH_SEGMENTWISE_GMM_TRAINER_HH
