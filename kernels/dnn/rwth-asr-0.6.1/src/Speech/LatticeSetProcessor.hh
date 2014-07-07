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
#ifndef _SPEECH_LATTICE_SET_PROCESSOR_HH
#define _SPEECH_LATTICE_SET_PROCESSOR_HH

#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include "CorpusProcessor.hh"
#include <Lattice/Lattice.hh>
#include "ModelCombination.hh"

namespace Bliss {
    class Evaluator;
}

namespace Speech {
    class SegmentwiseFeatureExtractor;
    class PhonemeSequenceAlignmentGenerator;
}

namespace Speech {

    /** LatticeSetProcessor is base class for algorithms processing lattice sets.
     *  Remark:
     *    instance of this class can be used for dry runs since it does not have
     *    any abstract functions.
     */
    class LatticeSetProcessor :
	virtual public Core::Component,
	public Core::ReferenceCounted
    {
    private:
	bool initialized_;
    protected:
	f64 timeProcessSegment_;
	Core::Ref<LatticeSetProcessor> processor_;
    public:
	LatticeSetProcessor(const Core::Configuration &);
	virtual ~LatticeSetProcessor() {}

	/** This lattice set processor is used for cascading.
	 */
	void setProcessor(Core::Ref<LatticeSetProcessor> processor) {
	    verify(!processor_); require(processor);
	    processor_ = processor;
	}

	/** Override this function to sign on to services of the corpus visitor.
	 *  Note: call the signOn function of your predecessor.
	 */
	virtual void signOn(CorpusVisitor &corpusVisitor) {
	    if (processor_) processor_->signOn(corpusVisitor);
	}

	/** Override this function to perform pre processing before the first segment of the corpus.
	 */
	virtual void enterCorpus(Bliss::Corpus *corpus) {
	    if (processor_) processor_->enterCorpus(corpus);
	}

	/** Override this function to perform post processing after the last segment of the corpus.
	 */
	virtual void leaveCorpus(Bliss::Corpus *corpus) {
	    if (processor_) processor_->leaveCorpus(corpus);
	}

	/** Override this function to perform pre processing before the first segment of the recording.
	 */
	virtual void enterRecording(Bliss::Recording *recording) {
	    if (processor_) processor_->enterRecording(recording);
	}

	/** Override this function to perform post processing after the last segment of the recording.
	 */
	virtual void leaveRecording(Bliss::Recording *recording) {
	    if (processor_) processor_->leaveRecording(recording);
	}

	/** Override this function to perform preparations before first aligned features arrive.
	 */
	virtual void enterSegment(Bliss::Segment *s) {
	    if (processor_) processor_->enterSegment(s);
	}
	/** Override this function to perform post processing after the last aligned features of the segment.
	 */
	virtual void leaveSegment(Bliss::Segment *s) {
	    if (processor_) processor_->leaveSegment(s);
	}
	/** Override this function to perform preparations before first features arrive.
	 */
	virtual void enterSpeechSegment(Bliss::SpeechSegment *s) {
	    if (processor_) processor_->enterSpeechSegment(s);
	}
	/** Override this function to perform post processing after the last features of the segment.
	 */
	virtual void leaveSpeechSegment(Bliss::SpeechSegment *s) {
	    if (processor_) processor_->leaveSpeechSegment(s);
	}

	/** Override this function to implement the processing of lattice sets.
	 */
	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	/**
	 *  Override this function to create the description of the output word lattices.
	 */
	virtual void getWordLatticeDescription(Lattice::WordLatticeDescription &) {}
	/**
	 *  Override this function to achieve the attributes of the input lattice set.
	 *  This function is called once before the first segment is processed.
	 */
	virtual void setWordLatticeDescription(const Lattice::WordLatticeDescription &) {}

	/**
	 *  Override this function to achieve the lexicon of the input lattice set.
	 */
	virtual void initialize(Bliss::LexiconRef lexicon) {
	    if (processor_) processor_->initialize(lexicon);
	}
	virtual void setSegmentwiseFeatureExtractor(Core::Ref<SegmentwiseFeatureExtractor>);
	virtual void setAlignmentGenerator(Core::Ref<PhonemeSequenceAlignmentGenerator>);

	virtual std::string name() const { return "unknown"; }

	virtual void logComputationTime() const;
    };

    /**
     *  root lattice set processor,
     *  i.e. it is both a corpus and a lattice set processor
     */
    class LatticeSetProcessorRoot :
	public CorpusProcessor,
	public LatticeSetProcessor
    {
	/** Precursor's functions are executed beforehand. */
	typedef LatticeSetProcessor Precursor;
    public:
	LatticeSetProcessorRoot(const Core::Configuration &);
	virtual ~LatticeSetProcessorRoot();

	virtual void signOn(CorpusVisitor &);
	virtual void enterCorpus(Bliss::Corpus *);
	virtual void leaveCorpus(Bliss::Corpus *);
	virtual void enterRecording(Bliss::Recording *);
	virtual void leaveRecording(Bliss::Recording *);
	virtual void enterSegment(Bliss::Segment *);
	virtual void leaveSegment(Bliss::Segment *);
	virtual void enterSpeechSegment(Bliss::SpeechSegment *);
	virtual void leaveSpeechSegment(Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef lexicon) { Precursor::initialize(lexicon); }
	void initialize();
    };

	class InfoLatticeProcessorNode : public LatticeSetProcessor
	{
	typedef LatticeSetProcessor Precursor;

	public:
	InfoLatticeProcessorNode(const Core::Configuration &);
	virtual ~InfoLatticeProcessorNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	};

    class DensityLatticeProcessorNode : public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	static const Core::ParameterBool paramShallEvaluateArcsPerSpokenWord;
	static const Core::ParameterBool paramShallEvaluateArcsPerTimeframe;
    private:
	bool shallEvaluateArcsPerSpokenWord_;
	bool shallEvaluateArcsPerTimeframe_;
    public:
	DensityLatticeProcessorNode(const Core::Configuration &);
	virtual ~DensityLatticeProcessorNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    class GerLatticeProcessorNode : public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	Bliss::Evaluator *evaluator_;
    public:
	GerLatticeProcessorNode(const Core::Configuration &);
	virtual ~GerLatticeProcessorNode();

	virtual void enterSpeechSegment(Bliss::SpeechSegment *);
	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual void initialize(Bliss::LexiconRef);
    };

    class LinearCombinationLatticeProcessorNode : public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    private:
	static const Core::ParameterStringVector paramOutputs;
	static Core::ParameterFloatVector paramScales;
    private:
	Core::StringHashMap<std::vector<Fsa::Weight> > scales_;
    private:
	void insert(const std::string &key, const std::vector<f64> &data);
	bool checkNParts(u32 nParts) const;
    public:
	LinearCombinationLatticeProcessorNode(const Core::Configuration &);
	virtual ~LinearCombinationLatticeProcessorNode() {}

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
	virtual std::string name() const { return "linear-combination"; }
    };

    /**
     * Cache node
     */
    class CacheNode :
	public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    public:
	CacheNode(const Core::Configuration &);
	virtual ~CacheNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    /**
     * Copy node
     */
    class CopyNode :
	public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    public:
	CopyNode(const Core::Configuration &);
	virtual ~CopyNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    /**
     * Partial node
     */
    class PartialNode :
	public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
	static const Core::ParameterInt paramInitial;
    private:
	Fsa::StateId initial_;
    public:
	PartialNode(const Core::Configuration &);
	virtual ~PartialNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

    /**
     * Empty node
     */
    class SkipEmptyLatticeNode :
	public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    public:
	SkipEmptyLatticeNode(const Core::Configuration &);
	virtual ~SkipEmptyLatticeNode();

	virtual void processWordLattice(Lattice::ConstWordLatticeRef, Bliss::SpeechSegment *);
    };

} // namespace Speech

#endif // _SPEECH_LATTICE_SET_PROCESSOR_HH
