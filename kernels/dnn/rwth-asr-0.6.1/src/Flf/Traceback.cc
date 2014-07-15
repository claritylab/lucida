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
#include <Modules.hh>
#include <Bliss/CorpusDescription.hh>
#include <Core/Choice.hh>
#include <Core/Directory.hh>
#include <Core/Parameter.hh>
#include <Core/StringUtilities.hh>
#include <Core/XmlStream.hh>
#include <Search/Search.hh>

#include "FlfCore/Basic.hh"
#include "Best.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "Traceback.hh"
#include "Segment.hh"


namespace Flf {

#ifndef MODULE_FLF_EXT
    class LatticeAlignmentBuilder : public Core::ReferenceCounted {};
    typedef Core::Ref<LatticeAlignmentBuilder> LatticeAlignmentBuilderRef;
    class LatticeAlignment : public Core::ReferenceCounted {};
    typedef Core::Ref<const LatticeAlignment> ConstLatticeAlignmentRef;
#endif

    // -------------------------------------------------------------------------
    class DumpTracebackNode : public FilterNode {
	typedef FilterNode Precursor;
    public:
	static const Core::Choice TracebackFormatChoice;
	static const Core::ParameterChoice paramTracebackFormat;

	static const Core::ParameterBool paramDumpOrthography;
	static const Core::ParameterBool paramDumpCoarticulation;
	static const Core::ParameterBool paramDumpNoises;
	static const Core::ParameterString paramNoisePrefix;
	static const Core::ParameterBool paramDumpNonWord;
	static const Core::ParameterBool paramDumpEps;
	static const Core::ParameterString paramNonWordSymbol;
	static const Core::ParameterStringVector paramScoreKeys;
	static const Core::ParameterBool paramDumpType;
	static const Core::ParameterBool paramDumpPhonemeAlignment;
	static const Core::ParameterBool paramDumpSubwordAlignment;

    private:
	Core::Channel dump_;
	Core::XmlWriter *dumpXmlWriter_;
	const Bliss::Lemma *unkLemma_;
	const Bliss::LemmaPronunciation *unkLemmaPron_;
	ConstSegmentRef segment_;
	s32 format_;

	bool dumpOrthography_;
	bool dumpCoarticulation_;
	bool dumpNonWord_;
	bool dumpEps_;
	bool dumpNoises_;
	std::string nonWordSymbol_;
	std::string epsSymbol_;
	std::string noisePrefix_;
	KeyList scoreKeys_;
	bool dumpType_;
	bool dumpPhonemeAlignment_;
	bool dumpSubwordAlignment_;
	LatticeAlignmentBuilderRef alignmentBuilder_;

    protected:
	void dumpBlissTraceback(ConstLatticeRef l, Core::XmlWriter &os) {
	    if (l && (l->initialStateId() != Fsa::InvalidStateId)) {
		Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet =
		    Lexicon::us()->lemmaPronunciationAlphabet();
		Search::SearchAlgorithm::Traceback traceback;
		Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());
		if (alphabetId != Lexicon::LemmaPronunciationAlphabetId)
		    warning("DumpTracebackNode: Input alphabet of \"%s\" "
			    "is not lemma pronunciation; map alphabet",
			    l->describe().c_str());
		const Boundaries &boundaries = *l->getBoundaries();
		const ScoreList &scales = l->semiring()->scales();
		ConstStateRef initialSr = l->getState(l->initialStateId());
		for (Fsa::StateId i = 1, n = initialSr->nArcs(); i <= n; ++i) {
		    ConstLatticeRef p;
		    ConstStateRef sr;
		    if (l->hasProperty(Fsa::PropertyLinear)) {
			verify(n == 1);
			p = l;
			sr = initialSr;
		    } else if (alphabetId == Lexicon::LemmaPronunciationAlphabetId) {
			p = l;
			sr = p->getState(i);
		    } else {
			p = best(mapInput(partial(l, i), MapToLemmaPronunciation));
			sr = p->getState(p->initialStateId());
		    }
		    traceback.clear();
		    traceback.push_back(
			Search::SearchAlgorithm::TracebackItem(
			    0,
			    0,
			    Search::SearchAlgorithm::ScoreVector(0.0, 0),
			    Search::SearchAlgorithm::TracebackItem::Transit()));
		    Score score = Semiring::One;
		    for (; sr->hasArcs(); sr = p->getState(sr->begin()->target())) {
			verify(sr->nArcs() == 1);
			const Arc &a = *sr->begin();
			score += a.weight()->project(scales);
			const Bliss::LemmaPronunciation *lemmaPron = 0;
			if ((Fsa::FirstLabelId <= a.input()) && (a.input() <= Fsa::LastLabelId)) {
			    lemmaPron = lpAlphabet->lemmaPronunciation(a.input());
			} else if (a.input() != Fsa::Epsilon) {
			    require(unkLemmaPron_);
			    lemmaPron = unkLemmaPron_;
			}
			if (lemmaPron) {
			    const Boundary &b = boundaries.get(a.target());
			    traceback.push_back(
				Search::SearchAlgorithm::TracebackItem(
				    lemmaPron,
				    b.time(),
				    Search::SearchAlgorithm::ScoreVector(score, 0),
				    Search::SearchAlgorithm::TracebackItem::Transit()));
			}
		    }
		    verify(sr->isFinal());
		    score += sr->weight()->project(scales);
		    const Boundary &b = boundaries.get(sr->id());
		    traceback.push_back(
			Search::SearchAlgorithm::TracebackItem(
			    0,
			    b.time(),
			    Search::SearchAlgorithm::ScoreVector(score, 0),
			    Search::SearchAlgorithm::TracebackItem::Transit()));
		    Core::XmlOpen tracebackOpen("traceback");
		    tracebackOpen + Core::XmlAttribute("source", "recognized");
		    tracebackOpen + Core::XmlAttribute("n", i);
		    if (segment_) {
			if (segment_->hasSegmentId())
			    tracebackOpen + Core::XmlAttribute("name", segment_->segmentId());
			if (segment_->hasTrack())
			    tracebackOpen + Core::XmlAttribute("track", segment_->track());
			if (segment_->hasStartTime())
			    tracebackOpen + Core::XmlAttribute("begin", segment_->startTime());
			if (segment_->hasEndTime())
			    tracebackOpen + Core::XmlAttribute("end", segment_->endTime());
		    }
		    os << tracebackOpen;
		    traceback.write(os, Lexicon::us()->phonemeInventory());
		    os << Core::XmlClose("traceback");
		}
	    } else {
		Core::XmlEmpty tracebackEmpty("traceback");
		tracebackEmpty + Core::XmlAttribute("source", "recognized");
		tracebackEmpty + Core::XmlAttribute("n", 0);
		if (segment_) {
		    if (segment_->hasSegmentId())
			tracebackEmpty + Core::XmlAttribute("name", segment_->segmentId());
		    if (segment_->hasTrack())
			tracebackEmpty + Core::XmlAttribute("track", segment_->track());
		    if (segment_->hasStartTime())
			tracebackEmpty + Core::XmlAttribute("begin", segment_->startTime());
		    if (segment_->hasEndTime())
			tracebackEmpty + Core::XmlAttribute("end", segment_->endTime());
		}
		os << tracebackEmpty;
	    }
	}

	struct CtmPrinter {
	    typedef enum {
		TypeInvalid,
		TypeLexical,
		TypeSubLexicalBegin,
		TypeSubLexicalWithin,
		TypeSubLexicalEnd,
		TypeNonLexical,
		TypeHesitation,
		TypePhone
	    } Type;
	    static const Core::Choice TypeChoice;

	    std::ostream &os;
	    std::string name;
	    u32 track;
	    ScoreIdList scoreIds;
	    std::string tail;

	    CtmPrinter(std::ostream &os) : os(os), name("unknown"), track(1), scoreIds() {}
	    void printHeader(ConstSegmentRef segment) {
		printAsText(os << ";; ", segment) << tail << std::endl;
	    }

	    void print(f32 start, f32 duration, const std::string &word, const Scores &scores, Type type = TypeInvalid) {
		os << name
		   << " " << track
		   << " " << Core::form("%.3f", start)
		   << " " << Core::form("%.3f", duration)
		   << " " << word;
		for (ScoreIdList::const_iterator itId = scoreIds.begin(); itId != scoreIds.end(); ++itId)
		    os << Core::form(" %.4f", scores[*itId]);
		if (type != TypeInvalid)
		    os << " " << TypeChoice[type];
		os << tail << std::endl;
	    }

	};


	void dumpCtmTraceback(ConstLatticeRef l, std::ostream &os) {
	    CtmPrinter cp(os);
	    f32 begin =  0.0;
	    f32 end = Core::Type<f32>::max;
	    if (segment_) {
		if (segment_->hasRecordingId())
		    cp.name = segment_->recordingId();
		else if (segment_->hasAudioFilename())
		    cp.name = Core::stripExtension(Core::baseName(segment_->audioFilename()));
		else if (segment_->hasSegmentId())
		    cp.name = segment_->segmentId();
		if (segment_->hasTrack())
		    cp.track = segment_->track() + 1;
		if (segment_->hasStartTime())
		    begin =  segment_->startTime();
		if (segment_->hasEndTime())
		    end =  segment_->endTime();
	    }
	    if (l && (l->initialStateId() != Fsa::InvalidStateId)) {
		Lexicon::AlphabetId alphabetId = Lexicon::us()->alphabetId(l->getInputAlphabet());
		Lexicon::LemmaAlphabetRef lAlphabet;
		Lexicon::LemmaPronunciationAlphabetRef lpAlphabet;
		Fsa::ConstAlphabetRef alphabet;
		LabelMapRef nonWordToEpsilonMap;
		bool mapToLemma = false;
		switch (alphabetId) {
		case Lexicon::LemmaAlphabetId:
		    alphabet = lAlphabet = Lexicon::us()->lemmaAlphabet();
		    nonWordToEpsilonMap = LabelMap::createNonWordToEpsilonMap(Lexicon::LemmaAlphabetId);
		    break;
		case Lexicon::LemmaPronunciationAlphabetId:
		    alphabet = lpAlphabet = Lexicon::us()->lemmaPronunciationAlphabet();
		    nonWordToEpsilonMap = LabelMap::createNonWordToEpsilonMap(Lexicon::LemmaPronunciationAlphabetId);
		    break;
		default:
		    if (dumpPhonemeAlignment_ || dumpSubwordAlignment_)
			criticalError("Acoustic alignment requires lemma or lemma-pronunciation as input, not %s",
				      Lexicon::us()->alphabetName(alphabetId).c_str());
		    if (dumpOrthography_) {
			alphabet = lAlphabet = Lexicon::us()->lemmaAlphabet();
			nonWordToEpsilonMap = LabelMap::createNonWordToEpsilonMap(Lexicon::LemmaAlphabetId);
			mapToLemma = true;
		    } else
			alphabet = l->inputAlphabet();
		}
		ConstSemiringRef semiring = l->semiring();
		for (KeyList::const_iterator itKey = scoreKeys_.begin(); itKey != scoreKeys_.end(); ++itKey) {
		    cp.scoreIds.push_back(semiring->id(*itKey));
		    if (cp.scoreIds.back() == Semiring::InvalidId)
			criticalError("DumpTracebackNode: Dimension \"%s\" does not exist.",
				      itKey->c_str());
		}
		const Boundaries &boundaries = *l->getBoundaries();
		ConstLatticeAlignmentRef latticeAlignment;
		if (dumpPhonemeAlignment_ || dumpSubwordAlignment_) {
		    latticeAlignment = getLatticeAlignment(l);
		}
		ConstStateRef initialSr = l->getState(l->initialStateId());
		if (!l->hasProperty(Fsa::PropertyLinear))
		    printAsText(os << ";; ", segment_) << " [1.." << initialSr->nArcs() << "]-best" << std::endl;
		for (Fsa::StateId i = 1, n = initialSr->nArcs(); i <= n; ++i) {
		    ConstLatticeRef p;
		    ConstStateRef sr;
		    if (l->hasProperty(Fsa::PropertyLinear)) {
			verify(n == 1);
			p = l;
			sr = initialSr;
			cp.tail = "";
		    } else {
			if (!mapToLemma) {
			    p = l;
			    sr = p->getState(i);
			} else {
			    p = best(mapInput(partial(l, i), MapToLemmaPronunciation));
			    sr = p->getState(p->initialStateId());
			}
			cp.tail = Core::form(" %d-best", i);
		    }
		    cp.printHeader(segment_);
		    for (; sr->hasArcs(); sr = p->getState(sr->begin()->target())) {
			verify(sr->nArcs() == 1);
			const Arc &a = *sr->begin();
			const Boundary &leftBoundary = boundaries.get(sr->id()), &rightBoundary = boundaries.get(a.target());
			f32 wordBegin = f32(leftBoundary.time()) / 100.00;
			f32 wordEnd   = f32(rightBoundary.time()) / 100.00;
			if (wordBegin < wordEnd) {
			    if (lAlphabet || lpAlphabet) {
				std::string word;
				CtmPrinter::Type type = CtmPrinter::TypeInvalid;
				if ((Fsa::FirstLabelId <= a.input()) && (a.input() <= Fsa::LastLabelId)) {
				    if (dumpOrthography_) {
					word = (lAlphabet) ?
					    std::string(lAlphabet->lemma(a.input())->preferredOrthographicForm()) :
					    lpAlphabet->lemmaPronunciation(a.input())->lemma()->preferredOrthographicForm();
				    } else
					word = alphabet->symbol(a.input());
				    if (dumpCoarticulation_) {
					word = Core::form("/%s/%c %s %c/%s/",
							  ((leftBoundary.transit().initial == Bliss::Phoneme::term) ? "#" :
							   Lexicon::us()->phonemeInventory()->phonemeAlphabet()->symbol(leftBoundary.transit().initial).c_str()),
							  ((leftBoundary.transit().boundary == AcrossWordBoundary) ? '|' : '+'),
							  word.c_str(),
							  ((rightBoundary.transit().boundary == AcrossWordBoundary) ? '|' : '+'),
							  ((rightBoundary.transit().initial == Bliss::Phoneme::term) ? "#" :
							   Lexicon::us()->phonemeInventory()->phonemeAlphabet()->symbol(rightBoundary.transit().final).c_str()));
				    }
				    if ((*nonWordToEpsilonMap)[a.input()].empty()) {
					if (leftBoundary.transit().boundary == AcrossWordBoundary) {
					    if (rightBoundary.transit().boundary == AcrossWordBoundary)
						type = CtmPrinter::TypeLexical;
					    else
						type = CtmPrinter::TypeSubLexicalBegin;
					} else {
					    if (rightBoundary.transit().boundary == AcrossWordBoundary)
						type = CtmPrinter::TypeSubLexicalEnd;
					    else
						type = CtmPrinter::TypeSubLexicalWithin;
					}
				    } else if (dumpNonWord_) {
					verify_((*nonWordToEpsilonMap)[a.input()].front().label == Fsa::Epsilon);
					type = CtmPrinter::TypeNonLexical;
				    }
				} else if (dumpEps_) {
				    word = epsSymbol_;
				    type = CtmPrinter::TypeNonLexical;
				}
				// check whether orthography is a noise
				if (!dumpNoises_ && Core::startsWith(word, noisePrefix_)){
				    continue;
				}
				// dump
				if (type != CtmPrinter::TypeInvalid) {
				    f32 absoluteBegin = begin + wordBegin, duration = wordEnd - wordBegin;
				    cp.print(absoluteBegin, duration, word, *a.weight(), (dumpType_ ? type : CtmPrinter::TypeInvalid));
				    if (dumpPhonemeAlignment_) {
					if (!dumpPhonemeAlignment(cp, latticeAlignment, sr, absoluteBegin, duration))
					    warning() << "No phoneme alignment available for \"" << absoluteBegin << " " << duration << " " << word << "\"";
				    }
				    if (dumpSubwordAlignment_) {
					if (!dumpSubwordAlignment(cp, latticeAlignment, sr, absoluteBegin, duration))
					    warning() << "No sub-word alignment available for \"" << absoluteBegin << " " << duration << " " << word << "\"";
				    }
				}
			    } else {
				if ((Fsa::FirstLabelId <= a.input()) && (a.input() <= Fsa::LastLabelId))
				    cp.print(begin + wordBegin, wordEnd - wordBegin, alphabet->symbol(a.input()), *a.weight());
				else if (dumpEps_)
				    cp.print(begin + wordBegin, wordEnd - wordBegin, epsSymbol_, *a.weight());
			    }
			}
		    }
		    // check (last) wordEnd against end
		}
	    } else
		cp.printHeader(segment_);
	}

	ConstLatticeAlignmentRef getLatticeAlignment(ConstLatticeRef l) {
#if 1
	    criticalError("Acoustic alignment requires module FLF_EXT");
	    return ConstLatticeAlignmentRef();
#endif
	}

	bool dumpPhonemeAlignment(CtmPrinter &cp, ConstLatticeAlignmentRef latticeAlignment, ConstStateRef sr, f32 absoluteBegin, f32 duration) {
	    return false;
	}

	bool dumpSubwordAlignment(CtmPrinter &cp, ConstLatticeAlignmentRef latticeAlignment, ConstStateRef sr, f32 absoluteBegin, f32 duration) {
	    return false;
	}

	void dumpOrthTraceback(ConstLatticeRef l, Core::XmlWriter &os) {
	    if (l && (l->initialStateId() != Fsa::InvalidStateId)) {
		ConstStateRef initialSr = l->getState(l->initialStateId());
		verify(initialSr->nArcs() == 1);
		verify(l->hasProperty(Fsa::PropertyLinear));

		Fsa::ConstAlphabetRef alphabet = l->getInputAlphabet();

		ConstLatticeRef p = l;
		os << Core::XmlOpen("orth");
		for (ConstStateRef sr = initialSr; sr->hasArcs(); sr = p->getState(sr->begin()->target())) {
		    verify(sr->nArcs() == 1);
		    const Arc &a = *sr->begin();
		    if ((Fsa::FirstLabelId <= a.input()) && (a.input() <= Fsa::LastLabelId))
			os << std::string(alphabet->symbol(a.input())) << " ";
		}
		os << Core::XmlClose("orth");
	    }
	}

	void dumpTraceback(ConstLatticeRef l, std::ostream &os) {
	    if (l && (l->initialStateId() != Fsa::InvalidStateId)) {
		require(l->getBoundaries()->valid());
	    }
	    switch (format_) {
	    case 0: {
		Core::XmlWriter xml(os);
		xml.generateFormattingHints();
		dumpBlissTraceback(l, xml);
		break; }
	    case 1:
		dumpCtmTraceback(l, os);
		break;
	    case 2: {
		Core::XmlWriter xml(os);
		xml.generateFormattingHints();
		dumpOrthTraceback(l, xml);
		break; }
	    default:
		defect();
	    }
	}

	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (connected(1))
		segment_ = requestSegment(1);
	    if (dump_ && dump_.isOpen()) {
		dumpTraceback(l, dump_);
	    } else {
		dumpTraceback(l, clog());
	    }
	    return l;
	}
    public:
	DumpTracebackNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), dump_(config, "dump"), dumpXmlWriter_(0) {}
	virtual ~DumpTracebackNode() {
	    delete dumpXmlWriter_;
	}

	virtual void init(const std::vector<std::string> &arguments) {
	    unkLemma_ = (Lexicon::us()->unkLemmaId() == Fsa::InvalidLabelId) ?
		0 : Lexicon::us()->lemmaAlphabet()->lemma(Lexicon::us()->unkLemmaId());
	    unkLemmaPron_ = (Lexicon::us()->unkLemmaPronunciationId() == Fsa::InvalidLabelId) ?
		0 : Lexicon::us()->lemmaPronunciationAlphabet()->
		lemmaPronunciation(Lexicon::us()->unkLemmaPronunciationId());
	    format_ = paramTracebackFormat(config);
	    switch (format_) {
	    case 0:
		break;
	    case 1: {
		const Core::Configuration ctmConfig = select("ctm");
		dumpOrthography_ = paramDumpOrthography(ctmConfig);
		dumpCoarticulation_ = paramDumpCoarticulation(ctmConfig);
		dumpNonWord_ = paramDumpNonWord(ctmConfig);
		dumpNoises_ = paramDumpNoises(ctmConfig);
		dumpEps_ = paramDumpEps(ctmConfig, dumpNonWord_);
		nonWordSymbol_ = paramNonWordSymbol(ctmConfig);
		noisePrefix_ = paramNoisePrefix(ctmConfig);
		epsSymbol_ = nonWordSymbol_.empty() ? "!NULL" : nonWordSymbol_;
		scoreKeys_ = paramScoreKeys(ctmConfig);
		dumpType_ = paramDumpType(ctmConfig);
		dumpPhonemeAlignment_ = paramDumpPhonemeAlignment(ctmConfig);
		dumpSubwordAlignment_ = paramDumpSubwordAlignment(ctmConfig);
		if (dumpPhonemeAlignment_ || dumpSubwordAlignment_) {
		    createAlignmentBuilder(ctmConfig);
		}
		if (dump_ && dump_.isOpen()) {
		    dump_ << ";; <name> <track> <start> <duration> <word>";
		    for (KeyList::const_iterator itKey = scoreKeys_.begin(); itKey != scoreKeys_.end(); ++itKey)
			dump_ << " <" << *itKey << ">";
		    if (dumpType_)
			dump_ << " <type>";
		    dump_ << " [<n-best>]" << std::endl;
		}
		break; }
	    case 2:
		break;
	    default:
		defect();
	    }
	}

	void createAlignmentBuilder(const Core::Configuration &ctmConfig) {
#if 1
	    criticalError("lattice alignment requires module FLF_EXT.");
#endif
	}
    };
    const Core::Choice DumpTracebackNode::CtmPrinter::TypeChoice(
	"invalid",        TypeInvalid,
	"lex",            TypeLexical,
	"sub-lex-begin",  TypeSubLexicalBegin,
	"sub-lex-within", TypeSubLexicalWithin,
	"sub-lex-end",    TypeSubLexicalEnd,
	"non-lex",        TypeNonLexical,
	"fp",             TypeHesitation,
	"phone",          TypePhone,
	Core::Choice::endMark());
    const Core::Choice DumpTracebackNode::TracebackFormatChoice = Core::Choice(
	"bliss",  0,
	"ctm",    1,
	"corpus", 2,
	Core::Choice::endMark());
    const Core::ParameterChoice DumpTracebackNode::paramTracebackFormat(
	"format",
	&DumpTracebackNode::TracebackFormatChoice,
	"traceback format",
	1);
    const Core::ParameterBool DumpTracebackNode::paramDumpOrthography(
	"dump-orthography",
	"dump preferred orthography instead of lattice input alphabet",
	true);
    const Core::ParameterBool DumpTracebackNode::paramDumpCoarticulation(
	"dump-coarticulation",
	"dump coarticualtion",
	false);
    const Core::ParameterBool DumpTracebackNode::paramDumpNonWord(
	"dump-non-word",
	"dump non word",
	false);
    const Core::ParameterBool DumpTracebackNode::paramDumpEps(
	"dump-eps",
	"dump epsilon",
	false);
    const Core::ParameterBool DumpTracebackNode::paramDumpNoises(
	"dump-noises",
	"dump noises",
	true);
    const Core::ParameterString DumpTracebackNode::paramNonWordSymbol(
	"non-word-symbol",
	"replace symbol for non-words like silence or noises and for epsilons",
	"");
    const Core::ParameterString DumpTracebackNode::paramNoisePrefix(
	"noise-prefix",
	"lemmas with orthography that have this prefix are regarded as a noise lemma",
	"[");
    const Core::ParameterStringVector DumpTracebackNode::paramScoreKeys(
	"scores",
	"dimension of scores to be dumped; default is no scores",
	"");
    const Core::ParameterBool DumpTracebackNode::paramDumpType(
	"dump-type",
	"dump type",
	false);
    const Core::ParameterBool DumpTracebackNode::paramDumpPhonemeAlignment(
	"dump-phoneme-alignment",
	"dump phoneme alignment",
	false);
    const Core::ParameterBool DumpTracebackNode::paramDumpSubwordAlignment(
	"dump-subword-alignment",
	"dump subword alignment",
	false);
    NodeRef createDumpTracebackNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DumpTracebackNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
