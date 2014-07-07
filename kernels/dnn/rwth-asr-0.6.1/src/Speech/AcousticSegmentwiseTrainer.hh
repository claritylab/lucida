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
#ifndef _SPEECH_ACOUSTIC_SEGMENTWISE_TRAINER_HH
#define _SPEECH_ACOUSTIC_SEGMENTWISE_TRAINER_HH

#include "AbstractSegmentwiseTrainer.hh"
#include "DataExtractor.hh"
#include "PhonemeSequenceAlignmentGenerator.hh"
#include <Mm/Feature.hh>

namespace Speech {

    /*
     * AbstractAcousticSegmentwiseTrainer
     * This is the interface for the discriminative
     * acoustic model training, e.g. GHMMs/LHMMs with
     * MPE. Each model and criterion corresponds with
     * a derived class.
     * This class implements general stuff like
     * feature and alignment administration.
     */
    class AbstractAcousticSegmentwiseTrainer :
	public AbstractSegmentwiseTrainer
    {
	typedef AbstractSegmentwiseTrainer Precursor;
     public:
	enum ModelType { gaussianMixture, maximumEntropy, neuralNetwork };
	static Core::Choice choiceModelType;
	static Core::ParameterChoice paramModelType;
    private:
	static Core::ParameterString paramPortName;
	static Core::ParameterString paramSparsePortName;
	static Core::ParameterString paramAccumulationPortName;
	static Core::ParameterString paramAccumulationSparsePortName;
	static Core::ParameterBool paramSinglePrecision;
    private:
	Flow::PortId portId_;
	Flow::PortId sparsePortId_;
	/*
	 * Features for posterior calculation and accumulation
	 * can be different (e.g. estimation of linear
	 * transformations). This can be configured via
	 * these ports.
	 */
	Flow::PortId accumulationPortId_;
	Flow::PortId accumulationSparsePortId_;
    protected:
	Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor_;
	Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator_;
    protected:
	Core::Ref<SegmentwiseFeatureExtractor> segmentwiseFeatureExtractor() const { return segmentwiseFeatureExtractor_; }
	Core::Ref<PhonemeSequenceAlignmentGenerator> alignmentGenerator() const { return alignmentGenerator_; }
	Core::Ref<Am::AcousticModel> acousticModel() const { return alignmentGenerator()->acousticModel(); }
	virtual void setFeatureDescription(const Mm::FeatureDescription &) = 0;
	Flow::PortId portId() const {
	    return portId_;
	}
	Flow::PortId sparsePortId() const {
	    return sparsePortId_;
	}
	Flow::PortId accumulationPortId() const {
	    return accumulationPortId_;
	}
	Flow::PortId accumulationSparsePortId() const {
	    return accumulationSparsePortId_;
	}
	ConstSegmentwiseFeaturesRef features(Flow::PortId portId, Flow::PortId sparsePortId) const;
	ConstSegmentwiseFeaturesRef features() const {
	    return features(portId(), sparsePortId());
	}
	ConstSegmentwiseFeaturesRef accumulationFeatures() const {
	    return features(accumulationPortId(), accumulationSparsePortId());
	}
    public:
	AbstractAcousticSegmentwiseTrainer(const Core::Configuration &);
	virtual ~AbstractAcousticSegmentwiseTrainer();

	/*
	 * This is the main function to write.
	 * The accumulation is model- and criterion-dependent.
	 */
	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void setSegmentwiseFeatureExtractor(Core::Ref<Speech::SegmentwiseFeatureExtractor>);
	virtual void setAlignmentGenerator(Core::Ref<Speech::PhonemeSequenceAlignmentGenerator>);

	static AbstractAcousticSegmentwiseTrainer* createAbstractAcousticSegmentwiseTrainer(const Core::Configuration &);
    };

    /*
     * AcousticSegmentwiseTrainer
     * Controls the main flow of calling
     * the accumulate-function of the corresponding
     * trainer/accumulator. The accumulation is
     * split into denominator and numerator accumulation.
     */
    template <class A>
    class AcousticSegmentwiseTrainer :
	public AbstractAcousticSegmentwiseTrainer
    {
	typedef AbstractAcousticSegmentwiseTrainer Precursor;
    private:
	const A *acc_;
    protected:
	void resetAcc() { delete acc_; acc_ = 0; }
	const A* acc() { if (!acc_) acc_ = createAcc(); return acc_; }
	virtual A* createAcc() = 0;
	virtual A* createNumAcc() = 0;
	virtual A* createDenAcc() = 0;
	virtual A* createMleAcc() = 0;

	void accumulateNumerator(Fsa::ConstAutomatonRef fsa, Core::Ref<const Lattice::WordBoundaries> wb) {
	    A *acc = createNumAcc();
	    acc->setWordBoundaries(wb);
	    acc->setFsa(fsa);
	    acc->work();
	    delete acc;
	}
	void accumulateDenominator(Fsa::ConstAutomatonRef fsa, Core::Ref<const Lattice::WordBoundaries> wb) {
	    A *acc = createDenAcc();
	    acc->setWordBoundaries(wb);
	    acc->setFsa(fsa);
	    acc->work();
	    delete acc;
	}
	void accumulateMle(Fsa::ConstAutomatonRef fsa, Core::Ref<const Lattice::WordBoundaries> wb) {
	    A *acc = createMleAcc();
	    acc->setWordBoundaries(wb);
	    acc->setFsa(fsa);
	    acc->work();
	    delete acc;
	}
    public:
	AcousticSegmentwiseTrainer(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c), acc_(0) {}
	virtual ~AcousticSegmentwiseTrainer() { delete acc_; }
    };

} // namespace Speech

#endif // _SPEECH_ACOUSTIC_SEGMENTWISE_TRAINER_HH
