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
#ifndef _SPEECH_RECOGNIZER_HH
#define _SPEECH_RECOGNIZER_HH

#include <Modules.hh>
#include "DataExtractor.hh"
#include <Search/Search.hh>
#include <Core/Choice.hh>
#include <Lattice/Archive.hh>
#include <Mm/Types.hh>

namespace Bliss {
    class Evaluator;
    class Lexicon;
    class OrthographicParser;
}
namespace Am {
    class AcousticModel;
}

namespace Search {
class LatticeHandler;
}

namespace Speech {

    class ModelCombination;


    class Recognizer : public virtual Core::Component {
    public:

	static const Core::Choice searchTypeChoice_;
	static const Core::ParameterChoice paramSearch;
    protected:
	typedef Search::SearchAlgorithm::Traceback Traceback;
	Core::Ref<const Bliss::Lexicon> lexicon_;
	Core::Ref<Am::AcousticModel> acousticModel_;
	Search::SearchAlgorithm *recognizer_;
    protected:
	void initializeRecognizer(Am::AcousticModel::Mode acousticModelMode);
	void initializeRecognizer(const Speech::ModelCombination&);
	virtual void createRecognizer();
    public:
	Recognizer(const Core::Configuration&);
	virtual ~Recognizer();
    };


    class OfflineRecognizer :
	public FeatureExtractor,
	public Recognizer
    {
	typedef FeatureExtractor Precursor;
    private:
	static const Core::ParameterBool paramStoreLattices;
	static const Core::ParameterBool paramStoreTracebacks;
	static const Core::ParameterBool paramTimeConditionedLattice;
	static const Core::ParameterString paramLayerName;
	static const Core::ParameterFloat paramPartialResultInterval;
	static const Core::ParameterBool paramEvaluteResult;
	static const Core::ParameterBool paramNoDependencyCheck;

	std::vector<Flow::Timestamp> featureTimes_;
	Flow::Time partialResultInterval_;
	Flow::Time lastPartialResult_;
	Traceback traceback_;

    protected:
	bool shouldEvaluateResult_, shouldStoreLattice_;
	Bliss::Evaluator *evaluator_;
	Search::LatticeHandler *latticeHandler_;
	Lattice::ArchiveWriter *tracebackArchiveWriter_;
	Core::XmlChannel tracebackChannel_;
	bool  timeConditionedLattice_;
	std::string layerName_;

	void processResult(Bliss::SpeechSegment *s);
	void logTraceback(const Traceback &);
	void addPartialToTraceback(Traceback &partialTraceback);
	void processFeatureTimestamp(const Flow::Timestamp &timestamp);
	void finishSegment(Bliss::SpeechSegment *segment);

    private:
	bool noDependencyCheck_;

    public:
	OfflineRecognizer(const Core::Configuration&,
			  Am::AcousticModel::Mode = Am::AcousticModel::complete);
	virtual ~OfflineRecognizer();

	virtual void signOn(CorpusVisitor &corpusVisitor);
	virtual void processResultAndLogStatistics(Bliss::SpeechSegment *s);
	virtual void enterSpeechSegment(Bliss::SpeechSegment*);
	virtual void leaveSpeechSegment(Bliss::SpeechSegment*);
	virtual void leaveSegment(Bliss::Segment*);

	virtual void processFeature(Core::Ref<const Feature>);
	virtual void setFeatureDescription(const Mm::FeatureDescription &);
    };

    class ConstrainedOfflineRecognizer : public OfflineRecognizer {
	typedef OfflineRecognizer Precursor;
    public:
	static const Core::ParameterBool paramUseLanguageModel;
	static const Core::ParameterFloat paramScale;
	static const Core::ParameterString paramFsaPrefix;
    private:
	Lattice::ArchiveReader *latticeArchiveReader_;
	Fsa::ConstAutomatonRef lemmaPronunciationToLemmaTransducer_;
	Fsa::ConstAutomatonRef lemmaToSyntacticTokenTransducer_;
	Fsa::ConstAutomatonRef lmFsa_;
	Fsa::Weight scale_;
	const std::string fsaPrefix_;
    public:
	ConstrainedOfflineRecognizer(const Core::Configuration&,
				     Am::AcousticModel::Mode = Am::AcousticModel::complete);
	virtual ~ConstrainedOfflineRecognizer();

	virtual void enterSpeechSegment(Bliss::SpeechSegment*);
    };
}
#endif //_SPEECH_RECOGNIZER_HH
