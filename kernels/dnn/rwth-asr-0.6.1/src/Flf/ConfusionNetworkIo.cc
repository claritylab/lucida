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
#include <Core/Application.hh>
#include <Core/Utility.hh>
#include <Core/XmlBuilder.hh>

#include "ConfusionNetwork.hh"
#include "ConfusionNetworkIo.hh"
#include "Lexicon.hh"

namespace Flf {


    namespace {
	/**
	 * Iterate through cn.
	 * If cn is normalized, then the slot is sorted in descending order by scores[posteriorId].
	 **/
	class CnSortingIterator {
	private:
	    struct ProbabilityWeakOrder {
		ScoreId posteriorId;
		ProbabilityWeakOrder(ScoreId posteriorId) : posteriorId(posteriorId) {}
		bool operator() (const ConfusionNetwork::Arc &a1, const ConfusionNetwork::Arc &a2) const {
		    return a1.scores->get(posteriorId) > a2.scores->get(posteriorId);
		}
	    };
	private:
	    ConstConfusionNetworkRef cn_;
	    ScoreId posteriorId_;
	    bool isNormalized_;
	    const ConfusionNetwork::Slot *slot_;
	    ConfusionNetwork::Slot sortedSlot_;
	    ConfusionNetwork::const_iterator it_, end_;
	private:
	    CnSortingIterator(ConstConfusionNetworkRef cn) :
		cn_(cn), posteriorId_(Semiring::InvalidId), slot_(0) {
		verify(cn);
		if (cn->isNormalized()) {
		    posteriorId_ = cn->normalizedProperties->posteriorId;
		    verify(posteriorId_ != Semiring::InvalidId);
		}
	    }

	    void setSlot() {
		if (it_ != end_) {
		    if (posteriorId_ != Semiring::InvalidId) {
			sortedSlot_ = *it_;
			std::sort(sortedSlot_.begin(), sortedSlot_.end(), ProbabilityWeakOrder(posteriorId_));
			slot_ = &sortedSlot_;
		    } else
			slot_ = &*it_;
		} else
		    slot_ = 0;
	    }

	public:
	    const ConfusionNetwork::Slot & operator* () const
		{ return *slot_; }
	    bool operator!= (const CnSortingIterator &it) const
		{ return it_ != it.it_; }
	    void operator++ ()
		{ verify(it_ != end_); ++it_; setSlot(); }
	public:
	    static CnSortingIterator begin(ConstConfusionNetworkRef cn) {
		CnSortingIterator cnIt(cn);
		cnIt.it_ = cn->begin(); cnIt.end_ = cn->end();
		cnIt.setSlot();
		return cnIt;
	    }
	    static CnSortingIterator end(ConstConfusionNetworkRef cn) {
		CnSortingIterator cnIt(cn);
		cnIt.it_ = cnIt.end_ = cn->end();
		return cnIt;
	    }
	};
    } // namespace


    // -------------------------------------------------------------------------
    /**
     * Read/Write CNs
     *
     **/
    /**
      Text-Format:
      # oracle
      #word   begin    duration    <key[0]>/<scale[0]>   <key[1]>/<scale[0]> ...
      # slot
      #word   begin    duration    <key[0]>/<scale[0]>   <key[1]>/<scale[0]> ...

      # oracle 1
      <word>  <begin>  <duration>  <score[0]>  <score[1]> ...
      # slot 1
      <word>  <begin>  <duration>  <score[0]>  <score[1]> ...
      ...

      Attention: oracle is optional
    **/
    namespace {
	void writeConfusionNetworkHeaderAsText(std::ostream &os, Fsa::ConstAlphabetRef alphabet, ConstSemiringRef semiring) {
	    os << "#word\tbegin\tduration";
	    for (u32 i = 0; i < semiring->size(); ++i)
		os << "\t" << semiring->keys()[i] << "/" << semiring->scales()[i];
	    os << std::endl;
	}

	void writeConfusionNetworkArcAsText(std::ostream &os, Fsa::ConstAlphabetRef alphabet, ConstSemiringRef semiring, const ConfusionNetwork::Arc &arc) {
	    if (arc.begin != Speech::InvalidTimeframeIndex)
		os << arc.begin;
	    else
		os << "inf";
	    if (arc.duration != Speech::InvalidTimeframeIndex)
		os << "\t" << arc.duration;
	    else
		os << "\t" << "inf";
	    os << "\t" << ((arc.label == Fsa::Epsilon) ?
			   "@" : alphabet->symbol(arc.label));
	    for (Scores::const_iterator itScore = arc.scores->begin(),
		     endScore = arc.scores->begin() + semiring->size();
		 itScore != endScore; ++itScore) os << "\t" << *itScore;
	    os << std::endl;
	}
    } // namespace

    void writeConfusionNetworkAsText(std::ostream &os, ConstConfusionNetworkRef cn, ConstSegmentRef segment) {
	if (segment)
	    printSegmentHeader(os, segment);
	if (!cn) return;
	if (cn->isNormalized())
	    os << "# Normalized CN (posterior key is \"" << cn->semiring->key(cn->normalizedProperties->posteriorId) << "\")." << std::endl;
	if (cn->isNBestAlignment())
	    os << "# " << cn->nBestAlignmentProperties->n << "-best CN" << std::endl;
	Fsa::ConstAlphabetRef alphabet = cn->alphabet;
	ConstSemiringRef semiring = cn->semiring;
	ConfusionNetwork::ConstOracleAlignmentPropertiesRef oracleProps;
	if (cn->isOracleAlignment()) {
	    os << "oracle" << std::endl;
	    oracleProps = cn->oracleAlignmentProperties;
	    writeConfusionNetworkHeaderAsText(os, alphabet, oracleProps->semiring);
	}
	os << "slot" << std::endl;
	writeConfusionNetworkHeaderAsText(os, alphabet, semiring);
	u32 i = 0;
	for (CnSortingIterator itSlot = CnSortingIterator::begin(cn),
		 endSlot = CnSortingIterator::end(cn); itSlot != endSlot; ++itSlot, ++i) {
	    if (oracleProps) {
		os << "# oracle " << (i + 1) << std::endl;
		writeConfusionNetworkArcAsText(os, alphabet, oracleProps->semiring, oracleProps->alignment[i]);
	    }
	    os << "# slot " << (i + 1) << std::endl;
	    const ConfusionNetwork::Slot &slot = *itSlot;
	    for (ConfusionNetwork::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
		writeConfusionNetworkArcAsText(os, alphabet, semiring, *itArc);
	}
    }

    /**
      XML-Format:
      <cn n="#slots" version="1.0">
	<head>
	...
	</head>
	<slot n="#arcs">
	...
	</slot>
      </cn>

      <cn version="1.1">
	<head>
	  <alphabet name="..."/>
	  <semiring> ... </semiring>
	  [<normalized posterior-key="..."/>]
	  [<n-best n="..."/>]
	  [<oracle> <semiring> ... </semiring> </oracle>]
	</head>
	<body [n="#slots"]>
	  <slot [n="#arcs"]>
	    [<oracle> <word> ... </word> <begin> ... </begin> <end> ... </end> <from> ... </from> <to> ... </to> <score> ... </score> ... </oracle>]
	    <arc> <word> ... </word> <begin> ... </begin> <end> ... </end> <from> ... </from> <to> ... </to> <score> ... </score> ... </arc>
	    ...
	  </slot>
	  ...
	</body>
      </cn>
    **/
    // -------------------------------------------------------------------------
    namespace {
	class ConfusionNetworkXmlParser : public Core::XmlSchemaParser {
	    typedef ConfusionNetworkXmlParser Self;
	    typedef Core::XmlSchemaParser Precursor;
	private:
	    ConfusionNetwork *cn_;

	    Semiring::XmlElement *semiringXmlElement_;
	    Semiring::XmlElement *oracleSemiringXmlElement_;
	    std::string cdata_;

	    Lexicon::SymbolMap symbolMap_;
	    ConstSemiringRef semiring_;
	    ConstSemiringRef oracleSemiring_;
	    Key normalizedPosteriorKey_;
	    ConfusionNetwork::OracleAlignmentProperties::OracleAlignment *oracleAlignment_;

	    Fsa::LabelId label_;
	    Time begin_;
	    Time duration_;
	    u32 from_, to_;
	    ScoresRef scores_;
	    Scores::iterator itScore_, endScore_;

	private:
	    void resetArc() {
		label_ = Fsa::Epsilon;
		begin_ = Speech::InvalidTimeframeIndex;
		duration_ = 0;
		from_ = to_ = Core::Type<u32>::max;
		scores_.reset();
	    }

	    void init() {
		semiring_ = cn_->semiring;
		symbolMap_ = Lexicon::us()->symbolMap(Lexicon::us()->alphabetId(cn_->alphabet));
	    }

	    void appendCdata(const char *ch, int len) {
		cdata_.append((char*)(ch), len);
	    }

	    void startRoot(const Core::XmlAttributes atts) {
		const char *c = atts["version"];
		if (c) {
		    f32 version;
		    if (Core::strconv(std::string(c), version)) {
			if (version < 1.1 - 0.0001)
			    parser()->warning("Support only version 1.1, found %f.", version);
		    } else
			parser()->warning("Failed to parse version: \"%s\"", c);
		}
	    }

	    void endRoot() {
		if (oracleAlignment_)
		    if (cn_->size() != oracleAlignment_->size())
			parser()->criticalError("CN and oracle alignment must have same length (%zu != %zu)",
						cn_->size(), oracleAlignment_->size());
		oracleAlignment_ = 0;
	    }

	    void alphabet(const Core::XmlAttributes atts) {
		const char *c = atts["name"];
		if (!c)
		    parser()->criticalError("\"alphabet\" tag requires \"name\" attribute.");
		std::string s;
		Core::strconv(std::string(c), s);
		cn_->alphabet = Lexicon::us()->alphabet(Lexicon::us()->alphabetId(s, true));
	    }

	    void normalized(const Core::XmlAttributes atts) {
		const char *c = atts["posterior-key"];
		if (!c)
		    parser()->criticalError("\"normalized\" tag requires \"posterior-key\" attribute.");
		Core::strconv(std::string(c), normalizedPosteriorKey_);
	    }

	    void nBest(const Core::XmlAttributes atts) {
		const char *c = atts["n"];
		if (!c)
		    parser()->criticalError("\"n-best\" tag requires \"n\" attribute.");
		u32 n;
		if (!Core::strconv(std::string(c), n))
		    parser()->criticalError("Failed to convert \"%s\"", c);
		cn_->nBestAlignmentProperties= ConfusionNetwork::ConstNBestAlignmentPropertiesRef(
		    new ConfusionNetwork::NBestAlignmentProperties(n));
	    }

	    void endOracle() {
		ConfusionNetwork::OracleAlignmentProperties *oracleAlignmentProperties =
		    new ConfusionNetwork::OracleAlignmentProperties;
		oracleSemiring_ = oracleAlignmentProperties->semiring = oracleSemiringXmlElement_->semiring();
		oracleAlignment_ = &oracleAlignmentProperties->alignment;
		oracleAlignment_->reserve(cn_->size());
		cn_->oracleAlignmentProperties = ConfusionNetwork::ConstOracleAlignmentPropertiesRef(oracleAlignmentProperties);
	    }

	    void startHeader(const Core::XmlAttributes atts) {
		normalizedPosteriorKey_.clear();
		oracleAlignment_ = 0;
		cn_->normalizedProperties.reset();
		cn_->nBestAlignmentProperties.reset();
		cn_->oracleAlignmentProperties.reset();
	    }

	    void endHeader() {
		cn_->semiring = semiringXmlElement_->semiring();
		if (!normalizedPosteriorKey_.empty()) {
		    ScoreId posteriorId = cn_->semiring->id(normalizedPosteriorKey_);
		    if (posteriorId == Semiring::InvalidId)
			parser()->criticalError("Semiring \"%s\" has no dimension labeled \"%s\".",
						cn_->semiring->name().c_str(), normalizedPosteriorKey_.c_str());
		    cn_->normalizedProperties = ConfusionNetwork::ConstNormalizedPropertiesRef(
			new ConfusionNetwork::NormalizedProperties(posteriorId));
		}
		init();
	    }

	    void startBody(const Core::XmlAttributes atts) {
		verify(cn_->alphabet && cn_->semiring);
		const char *c = atts["n"];
		if (c) {
		    s32 n;
		    if (Core::strconv(std::string(c), n))
			cn_->reserve(n);
		}
	    }

	    void startSlot(const Core::XmlAttributes atts) {
		cn_->push_back(ConfusionNetwork::Slot());
		const char *c = atts["n"];
		if (c) {
		    s32 n;
		    if (Core::strconv(std::string(c), n))
			cn_->back().reserve(n);
		}
	    }

	    void endSlot() {
		if (cn_->isNormalized())
		    std::sort(cn_->back().begin(), cn_->back().end());
	    }

	    void startArc(const Core::XmlAttributes atts) {
		scores_ = semiring_->clone(semiring_->one());
		itScore_ = scores_->begin();
		endScore_ = itScore_ + semiring_->size();
	    }

	    void endArc() {
		cn_->back().push_back(ConfusionNetwork::Arc(label_, scores_, begin_, duration_, from_, to_));
		resetArc();
	    }

	    void startOracleArc(const Core::XmlAttributes atts) {
		verify(oracleSemiring_);
		scores_ = oracleSemiring_->clone(semiring_->one());
		itScore_ = scores_->begin();
		endScore_ = itScore_ + oracleSemiring_->size();
	    }

	    void endOracleArc() {
		verify(oracleAlignment_);
		oracleAlignment_->push_back(ConfusionNetwork::Arc(label_, scores_, begin_, duration_, from_, to_));
		resetArc();
	    }

	    void endWord() {
		std::string word;
		if (!Core::strconv(cdata_, word))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		label_ = symbolMap_.index(word);
		cdata_.clear();
	    }

	    void endBegin() {
		f32 f;
		if (!Core::strconv(cdata_, f))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		cdata_.clear();
		begin_ = Time(Core::round(f * 100.0));
	    }

	    void endDuration() {
		f32 f;
		if (!Core::strconv(cdata_, f))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		cdata_.clear();
		duration_ = Time(Core::round(f * 100.0));
	    }

	    void endFrom() {
		if (!Core::strconv(cdata_, from_))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		cdata_.clear();
	    }

	    void endTo() {
		if (!Core::strconv(cdata_, to_))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		cdata_.clear();
	    }

	    void endScore() {
		if (itScore_ == endScore_)
		    parser()->criticalError("Semiring has \"%s\" only %zu dimensions, but more scores found.",
					    semiring_->name().c_str(), semiring_->size());
		if (!Core::strconv(cdata_, *itScore_))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		++itScore_;
		cdata_.clear();
	    }

	public:
	    ConfusionNetworkXmlParser(const Core::Configuration &config) :
		Precursor(config), cn_(0) {
		oracleAlignment_ = 0;
		semiringXmlElement_ = Semiring::xmlElement(this);
		oracleSemiringXmlElement_ = Semiring::xmlElement(this);
		Core::XmlMixedElement *headerElement = new Core::XmlMixedElementRelay(
		    "head", this, startHandler(&Self::startHeader), endHandler(&Self::endHeader), 0,
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "alphabet", this, startHandler(&Self::alphabet), 0, 0,
				  XML_NO_MORE_CHILDREN)),
		    XML_CHILD(semiringXmlElement_),
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "normalized", this, startHandler(&Self::normalized), 0, 0,
				  XML_NO_MORE_CHILDREN)),
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "n-best", this, startHandler(&Self::nBest), 0, 0,
				  XML_NO_MORE_CHILDREN)),
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "oracle", this, 0, endHandler(&Self::endOracle), 0,
				  XML_CHILD(oracleSemiringXmlElement_),
				  XML_NO_MORE_CHILDREN)),
		    XML_NO_MORE_CHILDREN);
		Core::XmlMixedElement *bodyElement = new Core::XmlMixedElementRelay(
		    "body", this, startHandler(&Self::startBody), 0, 0,
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "slot", this, startHandler(&Self::startSlot), endHandler(&Self::endSlot), 0,
				  XML_CHILD(new Core::XmlMixedElementRelay(
						"oracle", this, startHandler(&Self::startOracleArc), endHandler(&Self::endOracleArc), 0,
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "word", this, 0, endHandler(&Self::endWord), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "begin", this, 0, endHandler(&Self::endBegin), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "duration", this, 0, endHandler(&Self::endDuration), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "from", this, 0, endHandler(&Self::endFrom), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "to", this, 0, endHandler(&Self::endTo), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "score", this, 0, endHandler(&Self::endScore), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_NO_MORE_CHILDREN)),
				  XML_CHILD(new Core::XmlMixedElementRelay(
						"arc", this, startHandler(&Self::startArc), endHandler(&Self::endArc), 0,
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "word", this, 0, endHandler(&Self::endWord), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "begin", this, 0, endHandler(&Self::endBegin), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "duration", this, 0, endHandler(&Self::endDuration), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "from", this, 0, endHandler(&Self::endFrom), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "to", this, 0, endHandler(&Self::endTo), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "score", this, 0, endHandler(&Self::endScore), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_NO_MORE_CHILDREN)),
				  XML_NO_MORE_CHILDREN)),
		    XML_NO_MORE_CHILDREN);
		Core::XmlRegularElementRelay *rootElement = new Core::XmlRegularElementRelay(
		    "cn", this, startHandler(&Self::startRoot), 0);
		rootElement->addTransition(0, 1, headerElement);
		rootElement->addTransition(1, 2, bodyElement);
		rootElement->addTransition(0, 2, bodyElement);
		rootElement->addFinalState(1);
		rootElement->addFinalState(2);
		setRoot(rootElement);
	    }

	    bool parse(ConfusionNetwork *cn, std::istream &i) {
		verify(cn);
		cn_ = cn;
		init();
		resetArc();
		return (Precursor::parseStream(i) == 0);
	    }
	};
	ConfusionNetworkXmlParser * parser_ = 0;

	ConstSemiringRef emptySemiring_ = Semiring::create(Fsa::SemiringTypeTropical, 0);
    } // namespace

    ConstConfusionNetworkRef readConfusionNetworkFromXml(std::istream &is, ConfusionNetworkRef cn) {
	if (!parser_)
	    parser_ = new ConfusionNetworkXmlParser(Core::Configuration(Core::Application::us()->getConfiguration(), "cn-parser"));
	if (!cn) {
	    cn = ConfusionNetworkRef(new ConfusionNetwork);
	    cn->alphabet = Lexicon::us()->alphabet(Lexicon::LemmaAlphabetId);
	    cn->semiring = emptySemiring_;
	}
	parser_->parse(cn.get(), is);
	return cn;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	void writeConfusionNetworkArcContentAsXml(Core::XmlWriter &xml, const ConfusionNetwork::Arc arc, Fsa::ConstAlphabetRef alphabet, ConstSemiringRef semiring) {
	    xml << Core::XmlFull("word", alphabet->symbol(arc.label).c_str());
	    if (arc.begin != Speech::InvalidTimeframeIndex)
		xml << Core::XmlFull("begin", Core::form("%.3f", (f32(arc.begin) / 100.0)))
		    << Core::XmlFull("duration", Core::form("%.3f", (f32(arc.duration) / 100.0)));
	    if ((arc.from != Core::Type<u32>::max) && (arc.to != Core::Type<u32>::max))
		xml << Core::XmlFull("from", arc.from)
		    << Core::XmlFull("to", arc.to);
	    for (Scores::const_iterator itScore = arc.scores->begin(),
		     endScore = arc.scores->begin() + semiring->size();
		 itScore != endScore; ++itScore) xml << Core::XmlFull("score", *itScore);
	}
    } // namespace Flf

    void writeConfusionNetworkAsXml(Core::XmlWriter &xml, ConstConfusionNetworkRef cn, ConstSegmentRef segment, u32 ioFlag) {
	if (!cn) return;
	{
	    Core::XmlOpen xmlCnOpen("cn");
	    xmlCnOpen + Core::XmlAttribute("version", "1.1");
	    if (segment) {
		if (segment->hasSegmentId())
		    xmlCnOpen + Core::XmlAttribute("name", segment->segmentId());
		if (segment->hasRecordingId())
		    xmlCnOpen + Core::XmlAttribute("recording", segment->recordingId());
		if (segment->hasStartTime() && segment->hasEndTime())
		    xmlCnOpen
			+ Core::XmlAttribute("start", Core::form("%.3f", segment->startTime()))
			+ Core::XmlAttribute("end", Core::form("%.3f", segment->endTime()));
	    }
	    xml << xmlCnOpen;
	}
	if (ioFlag & IoFlag::WriteHead) {
	    xml << Core::XmlOpen("head");
	    xml << Core::XmlEmpty("alphabet")
		+ Core::XmlAttribute("name", Lexicon::us()->alphabetName(Lexicon::us()->alphabetId(cn->alphabet)).c_str());
	    cn->semiring->write(xml);
	    if (cn->isNormalized())
		xml << Core::XmlEmpty("normalized")
		    + Core::XmlAttribute("posterior-key", cn->semiring->key(cn->normalizedProperties->posteriorId));
	    if (cn->isNBestAlignment())
		xml << Core::XmlEmpty("n-best")
		    + Core::XmlAttribute("n", cn->nBestAlignmentProperties->n);
	    if (cn->isOracleAlignment()) {
		xml << Core::XmlOpen("oracle");
		cn->oracleAlignmentProperties->semiring->write(xml);
		xml << Core::XmlClose("oracle");
	    }
	    xml << Core::XmlClose("head");
	}
	if (ioFlag & IoFlag::WriteBody) {
	    Fsa::ConstAlphabetRef alphabet = cn->alphabet;
	    ConstSemiringRef semiring = cn->semiring;
	    ConstSemiringRef oracleSemiring;
	    const ConfusionNetwork::OracleAlignmentProperties::OracleAlignment *oracleAlignment = 0;
	    if (cn->isOracleAlignment()) {
		oracleSemiring = cn->oracleAlignmentProperties->semiring;
		oracleAlignment = &cn->oracleAlignmentProperties->alignment;
	    }
	    xml << (Core::XmlOpen("body") + Core::XmlAttribute("n", cn->size()));
	    u32 i = 0;
	    for (CnSortingIterator itSlot = CnSortingIterator::begin(cn),
		     endSlot = CnSortingIterator::end(cn); itSlot != endSlot; ++itSlot, ++i) {
		const ConfusionNetwork::Slot &slot = *itSlot;
		xml << Core::XmlOpen("slot") + Core::XmlAttribute("n", slot.size());
		if (oracleAlignment) {
		    xml << Core::XmlOpen("oracle");
		    writeConfusionNetworkArcContentAsXml(xml, (*oracleAlignment)[i], alphabet, oracleSemiring);
		    xml << Core::XmlClose("oracle");
		}
		for (ConfusionNetwork::Slot::const_iterator itArc = slot.begin(), endArc = slot.end();
		     itArc != endArc; ++itArc) {
		    xml << Core::XmlOpen("arc");
		    writeConfusionNetworkArcContentAsXml(xml, *itArc, alphabet, semiring);
		    xml << Core::XmlClose("arc");
		}
		xml << Core::XmlClose("slot");
	    }
	    xml << Core::XmlClose("body");
	}
	{
	    xml << Core::XmlClose("cn");
	}
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class DumpConfusionNetworkNode : public Node {
    private:
	Core::Channel dumpChannel_;
	IoFormat::CnFormat format_;
	bool hasCn_;
	ConstConfusionNetworkRef cn_;

    private:
	void dump(std::ostream &os, ConstConfusionNetworkRef cn) {
	    ConstSegmentRef segment = connected(1) ? requestSegment(1) : ConstSegmentRef();
	    switch (format_) {
	    case IoFormat::CnFormatText: {
		writeConfusionNetworkAsText(os, cn, segment);
		os << std::endl;
		break; }
	    case IoFormat::CnFormatXml: {
		Core::XmlWriter xml(os);
		xml.generateFormattingHints();
		writeConfusionNetworkAsXml(xml, cn, segment);
		break; }
	    default:
		defect();
	    }
	}

	ConstConfusionNetworkRef getCn() {
	    if (hasCn_)
		return cn_;
	    cn_ = requestCn(0);
	    hasCn_ = true;
	    if (dumpChannel_.isOpen())
		dump(dumpChannel_, cn_);
	    else
		dump(clog(), cn_);
	    return cn_;
	}

    public:
	DumpConfusionNetworkNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config),
	    dumpChannel_(config, "dump") {}
	virtual ~DumpConfusionNetworkNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("Need a confusion network as input at port 0");
	    format_ = IoFormat::CnFormat(IoFormat::paramCnFormat(config, IoFormat::CnFormatXml));
	    hasCn_ = false;
	    if (dumpChannel_.isOpen() && (format_ == IoFormat::CnFormatXml)) {
		Core::XmlWriter xml(dumpChannel_);
		xml.generateFormattingHints();
		xml << Core::XmlOpen("dump") + Core::XmlAttribute("type", "CN");
	    }
	}

	virtual void finalize() {
	    if (dumpChannel_.isOpen() && (format_ == IoFormat::CnFormatXml)) {
		Core::XmlWriter xml(dumpChannel_);
		xml.generateFormattingHints();
		xml << Core::XmlClose("dump");
	    }
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    ConstConfusionNetworkRef cn = getCn();
	    switch (to) {
	    case 0:
		return cn2lattice(cn);
	    case 1:
		defect();
		return ConstLatticeRef();
	    default:
		return ConstLatticeRef();
	    }
	}

	virtual ConstConfusionNetworkRef sendCn(Port to) {
	    verify(to == 1);
	    ConstConfusionNetworkRef cn = getCn();
	    return cn;
	}

	virtual void sync() {
	    cn_.reset();
	    hasCn_ = false;
	}

    };
    NodeRef createDumpConfusionNetworkNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DumpConfusionNetworkNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
