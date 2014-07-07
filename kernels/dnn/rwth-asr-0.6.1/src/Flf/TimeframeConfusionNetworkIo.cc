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
#include <Core/XmlBuilder.hh>
#include <Core/XmlParser.hh>
#include <Flow/Attributes.hh>
#include <Flow/Data.hh>
#include <Flow/DataAdaptor.hh>
#include <Speech/Alignment.hh>

#include "Lexicon.hh"
#include "TimeframeConfusionNetwork.hh"
#include "TimeframeConfusionNetworkIo.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    /**
     * Read/Write Posterior Cns
     *
     **/
    namespace {
	/**
	 * Iterate through posterior cn,
	 * sort slot w.r.t. probability.
	 **/
	class PosteriorCnSortingIterator {
	private:
	    struct ProbabilityWeakOrder {
		bool operator() (const PosteriorCn::Arc &a1, const PosteriorCn::Arc &a2) const {
		    return a1.score > a2.score;
		}
	    };
	private:
	    ConstPosteriorCnRef cn_;
	    PosteriorCn::Slot slot_;
	    PosteriorCn::const_iterator it_, end_;
	private:
	    PosteriorCnSortingIterator(ConstPosteriorCnRef cn) :
		cn_(cn), slot_(0) { verify(cn); }

	    void setSlot() {
		if (it_ != end_) {
		    slot_ = *it_;
		    std::sort(slot_.begin(), slot_.end(), ProbabilityWeakOrder());
		} else
		    slot_.clear();
	    }

	public:
	    const PosteriorCn::Slot & operator* () const
		{ return slot_; }
	    bool operator!= (const PosteriorCnSortingIterator &it) const
		{ return it_ != it.it_; }
	    void operator++ ()
		{ verify(it_ != end_); ++it_; setSlot(); }
	public:
	    static PosteriorCnSortingIterator begin(ConstPosteriorCnRef cn) {
		PosteriorCnSortingIterator cnIt(cn);
		cnIt.it_ = cn->begin(); cnIt.end_ = cn->end();
		cnIt.setSlot();
		return cnIt;
	    }

	    static PosteriorCnSortingIterator end(ConstPosteriorCnRef cn) {
		PosteriorCnSortingIterator cnIt(cn);
		cnIt.it_ = cnIt.end_ = cn->end();
		return cnIt;
	    }
	};
    } // namespace

    /**
      Text-Format:
      #word   probability

      # slot 1
      <word> <probability>
      ...
    **/
    void writePosteriorCnAsText(std::ostream &os, ConstPosteriorCnRef cn, ConstSegmentRef segment) {
	if (segment)
	    printSegmentHeader(os, segment);
	if (!cn) return;
	Fsa::ConstAlphabetRef alphabet = cn->alphabet;
	u32 i = 0;
	for (PosteriorCnSortingIterator itSlot = PosteriorCnSortingIterator::begin(cn),
		 endSlot = PosteriorCnSortingIterator::end(cn); itSlot != endSlot; ++itSlot, ++i) {
	    os << "# slot " << (i + 1) << std::endl;
	    const PosteriorCn::Slot &slot = *itSlot;
	    for (PosteriorCn::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
		os << ((itArc->label == Fsa::Epsilon) ? "@" : alphabet->symbol(itArc->label)) << "\t"
		   << itArc->score << std::endl;
	}
    }

    /**
      XML-Format:
      <posterior-cn n="#slots" version="1.0">
	<head>
	  <alphabet name="..."/>
	</head>
	<slot n="#words">
	  <word p="..."> ... </word>
	  ...
	</slot>
	...
      </posterior-cn>

      <posterior-cn version="1.1">
	<head>
	  <alphabet name="..."/>
	</head>
	<body [n="#slots"]>
	  <slot [n="#arcs"]>
	    <arc> <word> ... </word> <probability> ... </probability> </arc>
	    ...
	  </slot>
	  ...
	</body>
      </posterior-cn>

      Words have to be sorted in decreasing order of p!
    **/
    namespace {
	class PosteriorCnXmlParser : public Core::XmlSchemaParser {
	    typedef PosteriorCnXmlParser Self;
	    typedef Core::XmlSchemaParser Precursor;
	private:
	    PosteriorCn *cn_;
	    Lexicon::SymbolMap symbolMap_;
	    std::string cdata_;
	    Fsa::LabelId label_;
	    Probability probability_;
	private:
	    void resetArc() {
		label_ = Fsa::Epsilon;
		probability_ = 1.0;
	    }

	    void setSymbolMap() {
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

	    void alphabet(const Core::XmlAttributes atts) {
		const char *c = atts["name"];
		if (!c)
		    parser()->criticalError("\"alphabet\" tag requires \"name\" attribute.");
		std::string s;
		Core::strconv(std::string(c), s);
		cn_->alphabet = Lexicon::us()->alphabet(Lexicon::us()->alphabetId(s, true));
	    }

	    void endHeader() {
		setSymbolMap();
	    }

	    void startBody(const Core::XmlAttributes atts) {
		verify(cn_->alphabet);
		const char *c = atts["n"];
		if (c) {
		    s32 n;
		    if (Core::strconv(std::string(c), n))
			cn_->reserve(n);
		}
	    }

	    void startSlot(const Core::XmlAttributes atts) {
		cn_->push_back(PosteriorCn::Slot());
		const char *c = atts["n"];
		if (c) {
		    s32 n;
		    if (Core::strconv(std::string(c), n))
			cn_->back().reserve(n);
		}
	    }

	    void endSlot() {
		std::sort(cn_->back().begin(), cn_->back().end());
	    }

	    void endArc() {
		cn_->back().push_back(PosteriorCn::Arc(label_, probability_));
		resetArc();
		cdata_.clear();
	    }

	    void endWord() {
		std::string word;
		if (!Core::strconv(cdata_, word))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		label_ = symbolMap_.index(word);
		cdata_.clear();
	    }

	    void endProbability() {
		if (!Core::strconv(cdata_, probability_))
		    parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
		cdata_.clear();
	    }

	public:
	    PosteriorCnXmlParser(const Core::Configuration &config) :
		Precursor(config), cn_(0) {
		Core::XmlMixedElement *headerElement = new Core::XmlMixedElementRelay(
		    "head", this, 0, endHandler(&Self::endHeader), 0,
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "alphabet", this, startHandler(&Self::alphabet), 0, 0,
				  XML_NO_MORE_CHILDREN)),
		    XML_NO_MORE_CHILDREN);
		Core::XmlMixedElement *bodyElement = new Core::XmlMixedElementRelay(
		    "body", this, startHandler(&Self::startBody), 0, 0,
		    XML_CHILD(new Core::XmlMixedElementRelay(
				  "slot", this, startHandler(&Self::startSlot), endHandler(&Self::endSlot), 0,
				  XML_CHILD(new Core::XmlMixedElementRelay(
						"arc", this, 0, endHandler(&Self::endArc), 0,
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "word", this, 0, endHandler(&Self::endWord), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_CHILD(new Core::XmlMixedElementRelay(
							      "probability", this, 0, endHandler(&Self::endProbability), charactersHandler(&Self::appendCdata),
							      XML_NO_MORE_CHILDREN)),
						XML_NO_MORE_CHILDREN)),
				  XML_NO_MORE_CHILDREN)),
		    XML_NO_MORE_CHILDREN);
		Core::XmlRegularElementRelay *rootElement = new Core::XmlRegularElementRelay(
		    "posterior-cn", this, startHandler(&Self::startRoot), 0);
		rootElement->addTransition(0, 1, headerElement);
		rootElement->addTransition(1, 2, bodyElement);
		rootElement->addTransition(0, 2, bodyElement);
		rootElement->addFinalState(1);
		rootElement->addFinalState(2);
		setRoot(rootElement);
	    }

	    bool parse(PosteriorCn *cn, std::istream &i) {
		verify(cn);
		cn_ = cn;
		setSymbolMap();
		resetArc();
		return (Precursor::parseStream(i) == 0);
	    }
	};
	PosteriorCnXmlParser * parser_ = 0;
    } // namespace

    ConstPosteriorCnRef readPosteriorCnFromXml(std::istream &is, PosteriorCnRef cn) {
	if (!parser_)
	    parser_ = new PosteriorCnXmlParser(Core::Configuration(Core::Application::us()->getConfiguration(), "posterior-cn-parser"));
	if (!cn) {
	    cn = PosteriorCnRef(new PosteriorCn);
	    cn->alphabet = Lexicon::us()->alphabet(Lexicon::LemmaAlphabetId);
	}
	parser_->parse(cn.get(), is);
	return cn;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void writePosteriorCnAsXml(Core::XmlWriter &xml, ConstPosteriorCnRef cn, ConstSegmentRef segment, u32 ioFlag) {
	if (!cn) return;
	{
	    Core::XmlOpen xmlCnOpen("posterior-cn");
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
	    xml << Core::XmlClose("head");
	}
	if (ioFlag & IoFlag::WriteBody) {
	    Fsa::ConstAlphabetRef alphabet = cn->alphabet;
	    xml << (Core::XmlOpen("body") + Core::XmlAttribute("n", cn->size()));
	    for (PosteriorCnSortingIterator itSlot = PosteriorCnSortingIterator::begin(cn),
		     endSlot = PosteriorCnSortingIterator::end(cn); itSlot != endSlot; ++itSlot) {
		const PosteriorCn::Slot &slot = *itSlot;
		xml << Core::XmlOpen("slot") + Core::XmlAttribute("n", slot.size());
		for (PosteriorCn::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
		    xml << Core::XmlOpen("arc")
			<< Core::XmlFull("word", alphabet->symbol(itArc->label))
			<< Core::XmlFull("probability", itArc->score)
			<< Core::XmlClose("arc");
		xml << Core::XmlClose("slot");
	    }
	    xml << Core::XmlClose("body");
	}
	{
	    xml << Core::XmlClose("posterior-cn");
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void writePosteriorCnAsFlowAlignment(Flow::Cache &cache, ConstPosteriorCnRef cn, ConstSegmentRef segment) {
	Flow::CacheWriter *cacheWriter = cache.newWriter(segment->segmentId());
	verify(cacheWriter && cacheWriter->isOpen());
	Flow::Attributes *flowAttr = new Flow::Attributes;
	Core::Ref<const Flow::Attributes> flowAttrRef = Core::Ref<const Flow::Attributes>(flowAttr);
	flowAttr->set("datatype", "flow-alignment");
	Flow::DataAdaptor<Speech::Alignment> *flowAlignment = new Flow::DataAdaptor<Speech::Alignment>();
	Flow::DataPtr<Flow::Data> flowAlignmentRef = Flow::DataPtr<Flow::Data>(flowAlignment);
	flowAlignment->setStartTime(segment->startTime());
	flowAlignment->setEndTime(segment->endTime());
	Speech::Alignment &alignment = flowAlignment->data();
	Time t = 0;
	for (PosteriorCnSortingIterator itSlot = PosteriorCnSortingIterator::begin(cn),
		 endSlot = PosteriorCnSortingIterator::end(cn); itSlot != endSlot; ++itSlot, ++t) {
	    const PosteriorCn::Slot &slot = *itSlot;
	    for (PosteriorCn::Slot::const_iterator itArc = slot.begin(), endArc = slot.end(); itArc != endArc; ++itArc)
		if ((Fsa::FirstLabelId <= itArc->label) && (itArc->label <= Fsa::LastLabelId))
		    alignment.push_back(Speech::AlignmentItem(t, itArc->label, itArc->score));
	}
	alignment.sortItems();
	cacheWriter->putAttributes(flowAttrRef);
	cacheWriter->putData(flowAlignment);
	delete cacheWriter;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class DumpPosteriorCnNode : public Node {
    public:
	static const Core::ParameterFloat paramThreshold;
    private:
	Core::Channel dumpChannel_;
	IoFormat::PosteriorCnFormat format_;
	bool hasCn_;
	ConstPosteriorCnRef cn_;

    private:
	void dump(std::ostream &os, ConstPosteriorCnRef cn) {
	    ConstSegmentRef segment = connected(1) ? requestSegment(1) : ConstSegmentRef();
	    switch (format_) {
	    case IoFormat::PosteriorCnFormatText: {
		writePosteriorCnAsText(os, cn, segment);
		os << std::endl;
		break; }
	    case IoFormat::PosteriorCnFormatXml: {
		Core::XmlWriter xml(os);
		xml.generateFormattingHints();
		writePosteriorCnAsXml(xml, cn, segment);
		break; }
	    default:
		defect();
	    }
	}

	ConstPosteriorCnRef getCn() {
	    if (hasCn_)
		return cn_;
	    cn_ = requestPosteriorCn(0);
	    hasCn_ = true;
	    if (dumpChannel_.isOpen())
		dump(dumpChannel_, cn_);
	    else
		dump(clog(), cn_);
	    return cn_;
	}

    public:
	DumpPosteriorCnNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config),
	    dumpChannel_(config, "dump") {}
	virtual ~DumpPosteriorCnNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("Need a posterior CN as input at port 0");
	    format_ = IoFormat::PosteriorCnFormat(IoFormat::paramPosteriorCnFormat(config, IoFormat::PosteriorCnFormatXml));
	    hasCn_ = false;
	    if (dumpChannel_.isOpen() && (format_ == IoFormat::PosteriorCnFormatXml)) {
		Core::XmlWriter xml(dumpChannel_);
		xml.generateFormattingHints();
		xml << Core::XmlOpen("dump") + Core::XmlAttribute("type", "posterior-CN");
	    }
	}

	virtual void finalize() {
	    if (dumpChannel_.isOpen() && (format_ == IoFormat::PosteriorCnFormatXml)) {
		Core::XmlWriter xml(dumpChannel_);
		xml.generateFormattingHints();
		xml << Core::XmlClose("dump");
	    }
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    ConstPosteriorCnRef cn = getCn();
	    switch (to) {
	    case 0:
		return posteriorCn2lattice(cn);
	    case 1:
		defect();
		return ConstLatticeRef();
	    default:
		return ConstLatticeRef();
	    }
	}

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    verify(to == 1);
	    ConstPosteriorCnRef cn = getCn();
	    return cn;
	}

	virtual void sync() {
	    cn_.reset();
	    hasCn_ = false;
	}

    };
    NodeRef createDumpPosteriorCnNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DumpPosteriorCnNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
