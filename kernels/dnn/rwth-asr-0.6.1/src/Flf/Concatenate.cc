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
#include "Core/Application.hh"
#include "Core/Hash.hh"
#include "Core/ReferenceCounting.hh"

#include "FlfCore/Traverse.hh"
#include "Archive.hh"
#include "Concatenate.hh"
#include "Miscellaneous.hh"
#include "Module.hh"

#include "FlfCore/Ftl.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class RecordingSegmentation : public std::vector<ConstSegmentRef>, public Core::ReferenceCounted {
    public:
	typedef std::vector<ConstSegmentRef> Precursor;
    public:
	Flf::ConstSegmentRef desc;
    public:
	RecordingSegmentation(Flf::ConstSegmentRef desc) : desc(desc) {}
    };
    typedef Core::Ref<RecordingSegmentation> RecordingSegmentationRef;

    struct BoundedSegment {
	Flf::ConstSegmentRef segment;
	Time offset;
	s32 startFrame, endFrame;
	BoundedSegment() :
	    segment(), offset(0), startFrame(Core::Type<s32>::max), endFrame(Core::Type<s32>::max) {}
    };
    typedef std::vector<BoundedSegment> BoundedSegmentList;
    s32 startFrame, endFrame;


    /**
     * Distribute recording time over segments:
     * In case of overlaps, the begining of the next next segment is cut
     **/
    BoundedSegmentList buildBoundedSegments(const RecordingSegmentation &recSeg) {
	BoundedSegmentList boundedSegs(recSeg.size());
	f32 recStartTime = recSeg.desc->startTime(), recEndTime = recSeg.desc->endTime();
	switch (recSeg.size()) {
	case 0:
	    break;
	case 1:
	    verify((recStartTime < recSeg.front()->endTime()) && (recSeg.back()->startTime() < recEndTime));
	    boundedSegs[0].segment = recSeg[0];
	    boundedSegs[0].offset = 0;
	    boundedSegs[0].startFrame = -nFrames(recStartTime, recSeg[0]->startTime());
	    boundedSegs[0].endFrame = nFrames(recSeg[0]->startTime(), recEndTime);
	    break;
	default:
	    verify((recStartTime < recSeg.front()->endTime()) && (recSeg.back()->startTime() < recEndTime));
	    RecordingSegmentation::const_iterator itSegL = recSeg.begin(), itSeg = recSeg.begin() + 1, itSegR = recSeg.begin() + 2;
	    BoundedSegmentList::iterator itBoundedSegL = boundedSegs.begin(), itBoundedSeg = boundedSegs.begin() + 1;
	    itBoundedSegL->segment = *itSegL;
	    itBoundedSegL->offset = 0;
	    itBoundedSegL->startFrame = -nFrames(recStartTime, (*itSegL)->startTime());
	    itBoundedSegL->endFrame = nFrames((*itSegL)->startTime(), (*itSegL)->endTime());
	    for (; itSegR != recSeg.end(); ++itSeg, ++itSegR) {
		if ((*itSeg)->endTime() <= (*itSegL)->endTime()) {
		    Core::Application::us()->warning(
			"Segment \"%s\" lies in \"%s\", discard first segment.",
			(*itSeg)->segmentIdOrDie().c_str(), (*itSegL)->segmentIdOrDie().c_str());
		} else {
		    itBoundedSeg->segment = *itSeg;
		    itBoundedSeg->offset = itBoundedSegL->offset + (itBoundedSegL->endFrame - itBoundedSegL->startFrame);
		    itBoundedSeg->startFrame = -(nFrames(recStartTime, (*itSeg)->startTime()) - itBoundedSeg->offset);
		    itBoundedSeg->endFrame = nFrames((*itSeg)->startTime(), (*itSeg)->endTime());
		    itSegL = itSeg;
		    ++itBoundedSegL, ++itBoundedSeg;
		}
	    }
	    if ((*itSeg)->endTime() <= (*itSegL)->endTime()) {
		Core::Application::us()->warning(
		    "Segment \"%s\" lies in \"%s\", discard first segment.",
		    (*itSeg)->segmentIdOrDie().c_str(), (*itSegL)->segmentIdOrDie().c_str());
		--itBoundedSeg;
	    } else {
		itBoundedSeg->segment = *itSeg;
		itBoundedSeg->offset = itBoundedSegL->offset + (itBoundedSegL->endFrame - itBoundedSegL->startFrame);
		itBoundedSeg->startFrame = -(nFrames(recStartTime, (*itSeg)->startTime()) - itBoundedSeg->offset);
		itBoundedSeg->endFrame = nFrames((*itSeg)->startTime(), recEndTime);
	    }
	    boundedSegs.erase(itBoundedSeg + 1, boundedSegs.end());
	    break;
	}
	return boundedSegs;
    }

    void dumpBoundedSegments(std::ostream &os, const BoundedSegmentList &boundedSegs) {
	os << "[segment name]  [relative start-frame - relative end-frame]  [offset to recording start]\n";
	os << "----------------------------------------------------------------------------------------\n";
	for (BoundedSegmentList::const_iterator it = boundedSegs.begin(); it != boundedSegs.end(); ++it)
	    os << it->segment->segmentId()
	       << "  " << it->startFrame << "-" << it->endFrame
	       << "  " << it->offset << "\n";
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class RecordingSegmentationSet {
    public:
	typedef std::vector<RecordingSegmentationRef> RecordingSegmentationList;
	typedef RecordingSegmentationList::const_iterator const_iterator;
	typedef Core::hash_map<std::string, RecordingSegmentationRef, Core::StringHash> RecordingSegmentationMap;
    private:
	RecordingSegmentationList list_;
	RecordingSegmentationMap map_;
    public:
	RecordingSegmentationSet() {}
	bool insertRecording(Flf::ConstSegmentRef desc) {
	    verify(desc->hasRecordingId() && desc->hasStartTime() && desc->hasEndTime());
	    RecordingSegmentationRef rec = RecordingSegmentationRef(new RecordingSegmentation(desc));
	    if (map_.insert(std::make_pair(desc->recordingId(), rec)).second) {
		list_.push_back(rec);
		return true;
	    } else
		return false;
	}
	bool addSegment(Flf::ConstSegmentRef desc) {
	    verify(desc->hasRecordingId() && desc->hasStartTime() && desc->hasEndTime());
	    RecordingSegmentationMap::iterator it = map_.find(desc->recordingId());
	    if (it != map_.end()) {
		it->second->push_back(desc);
		return true;
	    } else
		return false;
	}
	const_iterator begin() const
	    { return list_.begin(); }
	 const_iterator end() const
	    { return list_.end(); }
    };

    class ConcatenateNode : public Node {
	friend class Network;
    private:
	Core::Channel dump_;
	RecordingSegmentationSet recSegSet_;
	RecordingSegmentationSet::const_iterator itRecSegSet_, endRecSegSet_;
    protected:
	bool init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	    if (!connected(0) || !connected(1))
		criticalError("Need segment sources at port 0 and port 1.");
	    if (!crawler.init(in()[0].node, arguments) || !crawler.init(in()[1].node, arguments))
		return false;
	    NetworkCrawlerRef recSegCrawler = Module::instance().processor()->newCrawler();
	    for (bool _good = true; _good; ) {
		ConstSegmentRef segment = requestSegment(1);
		if (!recSegSet_.insertRecording(segment))
		    criticalError("Could not add recording \"%s\", "
				  "because a recording with the same name does already exist.",
				  segment->recordingId().c_str());
		_good = recSegCrawler->sync(in()[1].node);
		recSegCrawler->reset();
	    }
	    for (bool _good = true; _good; ) {
		ConstSegmentRef segment = requestSegment(0);
		if (!recSegSet_.addSegment(segment))
		    warning("Discard segment \"%s\", "
			    "because no recording name \"%s\" exists.",
			    segment->segmentId().c_str(),
			    segment->recordingId().c_str());
		_good = recSegCrawler->sync(in()[0].node);
		recSegCrawler->reset();
	    }
	    itRecSegSet_ = recSegSet_.begin();
	    endRecSegSet_ =  recSegSet_.end();
	    if (itRecSegSet_ == endRecSegSet_)
		return false;
	    if (dump_.isOpen()) {
		for (RecordingSegmentationSet::const_iterator itRecSeg = recSegSet_.begin(); itRecSeg != recSegSet_.end(); ++itRecSeg) {
		    const RecordingSegmentation &recSeg = **itRecSeg;
		    dump_ << "Recording [" << recSeg.desc->recordingId() << ":"
			  << Core::form("%.2f-%.2f", recSeg.desc->startTime(), recSeg.desc->endTime()) << "]" << std::endl;
		    for (RecordingSegmentation::const_iterator itSeg = recSeg.begin(); itSeg != recSeg.end(); ++itSeg)
			dump_ << "\tSegment [" << (*itSeg)->segmentId() << ":"
			      << Core::form("%.2f-%.2f", (*itSeg)->startTime(), (*itSeg)->endTime()) << "]" << std::endl;
		    dump_ << std::endl;
		}
	    }
	    init(arguments);
	    return good();
	}

	bool sync_(NetworkCrawler &crawler) {
	    ++itRecSegSet_;
	    if (itRecSegSet_ == endRecSegSet_)
		return false;
	    sync();
	    return good();
	}

	RecordingSegmentationRef recordingSegmentation() {
	    verify(itRecSegSet_ != endRecSegSet_);
	    return *itRecSegSet_;
	}

    public:
	ConcatenateNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), dump_(config, "dump") {}
	virtual ~ConcatenateNode() {}

	virtual ConstSegmentRef sendSegment(Port to) {
	    verify(to == 1);
	    return recordingSegmentation()->desc;
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ConcatenateLatticesNode : public ConcatenateNode {
	friend class Network;
	typedef ConcatenateNode Precursor;
    public:
	static const Core::ParameterBool paramForceSentenceEndLabels;

	class ConcatenatedLatticeBuilder {
	public:
	    class Concatenator;
	    friend class Concatenator;
	    class Concatenator : public TraverseState {
	    private:
		StaticLattice *s_;
		StaticBoundaries *b_;
		Fsa::StateId &sMaxSid_;
		Fsa::StateId &sFinalSid_;
		Time &sEndTime_;

		Core::Vector<Fsa::StateId> sidMap_;
		Fsa::StateId initialSid_;
		Time offset_;

	    protected:
		virtual void exploreState(ConstStateRef sr) {
		    verify((sr->id() < sidMap_.size()) && (sidMap_[sr->id()] != Fsa::InvalidStateId));
		    Fsa::StateId sid = sidMap_[sr->id()];
		    State *sp = 0;
		    if (sr->id() == initialSid_) {
			sp = s_->fastState(sid);
			verify(!sp->hasArcs());
			*sp = *sr; sp->setId(sid);
		    } else {
			sp = new State(*sr); sp->setId(sid);
			verify(!s_->hasState(sp->id()));
			s_->setState(sp);
			const Boundary &boundary = l->boundary(sr->id());
			b_->set(sid, Boundary(boundary.time() + offset_, boundary.transit()));
		    }
		    if (sp->isFinal()) {
			// final state is last state and final state has no weight
			verify(!sp->hasArcs() && (l->semiring()->compare(sp->weight(), l->semiring()->one()) == 0));
			sp->unsetFinal();
			// unique final state
			verify(sFinalSid_ == Fsa::InvalidStateId);
			sFinalSid_ = sid;
			sEndTime_ = b_->get(sid).time();
		    }
		    for (State::iterator a = sp->begin(), a_end = sp->end(); a != a_end; ++a) {
			Fsa::StateId targetSid = a->target();
			sidMap_.grow(targetSid, Fsa::InvalidStateId);
			if (sidMap_[targetSid] == Fsa::InvalidStateId)
			    sidMap_[targetSid] = ++sMaxSid_;
			a->target_ = sidMap_[targetSid];
		    }
		}

	    public:
		Concatenator(ConcatenatedLatticeBuilder *builder, ConstLatticeRef l) :
		    TraverseState(l),
		    s_(builder->s_), b_(builder->b_),
		    sMaxSid_(builder->maxSid_), sFinalSid_(builder->finalSid_), sEndTime_(builder->endTime_) {
		    initialSid_ = l->initialStateId();
		    sidMap_.grow(initialSid_, Fsa::InvalidStateId);
		    sidMap_[initialSid_] = sFinalSid_;
		    offset_ = sEndTime_;
		    sFinalSid_ = Fsa::InvalidStateId;
		    traverse();
		}
	    };

	private:
	    StaticLattice *s_;
	    StaticBoundaries *b_;
	    Fsa::StateId maxSid_;
	    Fsa::StateId finalSid_;
	    Time endTime_;

	    ConstLatticeRef sRef;
	    ScoresRef bestSum;


	protected:
	    void initialize(ConstLatticeRef l) {
		s_ = new StaticLattice;
		b_ = new StaticBoundaries;
		s_->setBoundaries(ConstBoundariesRef(b_));
		s_->setType(l->type());
		s_->setSemiring(l->semiring());
		s_->setInputAlphabet(l->getInputAlphabet());
		if (l->type() != Fsa::TypeAcceptor)
		    s_->setOutputAlphabet(l->getOutputAlphabet());
		s_->addProperties(Fsa::PropertyAcyclic);
		s_->setInitialStateId(0);
		s_->setState(new State(0));
		b_->set(0, Boundary(0));
		maxSid_ = 0;
		finalSid_ = 0;
		endTime_ = 0;
	    }
	public:
	    ConcatenatedLatticeBuilder() :
		s_(0), b_(0), maxSid_(0), finalSid_(0), endTime_(0) {}
	    ~ConcatenatedLatticeBuilder() {
		delete s_;
	    }
	    void concatenate(ConstLatticeRef l) {
		if (!s_)
		    initialize(l);
		Concatenator c(this, l);
		verify(finalSid_ != Fsa::InvalidStateId);
	    }
	    ConstLatticeRef get() {
		if (s_) {
		    s_->fastState(finalSid_)->setFinal(s_->semiring()->one());
		    ConstLatticeRef l = ConstLatticeRef(s_);
		    s_ = 0; b_ = 0;
		    maxSid_ = 0; finalSid_ = Fsa::InvalidStateId; endTime_ = 0;
		    return l;
		} else
		    return ConstLatticeRef();
	    }
	    Time endTime() const
		{ return endTime_; }
	};

    private:
	LatticeArchiveReader *reader_;
	bool forceSentenceEndLabels_;
	ConstLatticeRef buffer_;
	bool isValid_;
    public:
	ConcatenateLatticesNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), reader_(0) {}
	virtual ~ConcatenateLatticesNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    std::string readerPath = Archive::paramPath(config);
	    reader_ = LatticeArchive::getReader(config);
	    if (!reader_ || reader_->hasFatalErrors())
		criticalError("%s: failed to open lattice archive \"%s\" for reading.",
			      name.c_str(), readerPath.c_str());
	    else
		log("Lattice archive \"%s\" is open for reading.", readerPath.c_str());
	    forceSentenceEndLabels_ = paramForceSentenceEndLabels(config);
	    isValid_ = false;
	}

	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}

	virtual void finalize() {
	    reader_->close();
	    delete reader_;
	    reader_ = 0;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    verify(to == 0);
	    if (isValid_)
		return buffer_;
	    RecordingSegmentationRef recSeg = recordingSegmentation();
	    BoundedSegmentList boundedSegs = buildBoundedSegments(*recSeg);
	    {
		Core::Component::Message msg(log());
		msg << "Recording \"" << recSeg->desc->recordingId() << ":"
		    << recSeg->desc->startTime() << "-" << recSeg->desc->endTime() << "\"\n"
		    << "  duration=" << (recSeg->desc->endTime() - recSeg->desc->startTime()) << "sec\n";
		dumpBoundedSegments(msg, boundedSegs);
	    }
	    if (boundedSegs.empty())
		return ConstLatticeRef();
	    ConcatenatedLatticeBuilder builder;
	    s32 pending = 0;
	    for (BoundedSegmentList::iterator it = boundedSegs.begin(); it != boundedSegs.end(); ++it) {
		ConstLatticeRef l = reader_->get(it->segment->segmentId());
		if (!l) {
		    pending += it->endFrame - it->startFrame;
		    verify(pending >= 0);
		} else {
		    l = fit(l, it->startFrame - pending, it->endFrame, forceSentenceEndLabels_);
		    verify(it->offset == builder.endTime());
		    builder.concatenate(l);
		}
	    }
	    buffer_ = builder.get();
	    isValid_ = true;
	    return buffer_;
	}
    };
    const Core::ParameterBool ConcatenateLatticesNode::paramForceSentenceEndLabels(
	"force-sentence-end-labels",
	"put sentence end labels in between concatenated lattices",
	false);
    NodeRef createConcatenateLatticesNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ConcatenateLatticesNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    NodeRef createConcatenateCnsNode(const std::string &name, const Core::Configuration &config) {
	Core::Application::us()->criticalError("Not yet implemented.");
	return NodeRef();
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class ConcatenateFCnsNode : public ConcatenateNode {
	friend class Network;
	typedef ConcatenateNode Precursor;
    public:
    private:
	PosteriorCnArchiveReader *reader_;
	ConstPosteriorCnRef buffer_;
	PosteriorCn::Slot epsSlot_;
	bool isValid_;
    public:
	ConcatenateFCnsNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), reader_(0) {}
	virtual ~ConcatenateFCnsNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    std::string readerPath = Archive::paramPath(config);
	    reader_ = PosteriorCnArchive::getReader(config);
	    if (!reader_ || reader_->hasFatalErrors())
		criticalError("%s: failed to open fCN archive \"%s\" for reading.",
			      name.c_str(), readerPath.c_str());
	    else
		log("fCN archive \"%s\" is open for reading.", readerPath.c_str());
	    epsSlot_.push_back(PosteriorCn::Arc(Fsa::Epsilon, 1.0));
	    isValid_ = false;
	}

	virtual void sync() {
	    buffer_.reset();
	    isValid_ = false;
	}

	virtual void finalize() {
	    reader_->close();
	    delete reader_;
	    reader_ = 0;
	}

	virtual ConstPosteriorCnRef sendPosteriorCn(Port to) {
	    verify(to == 0);
	    if (isValid_)
		return buffer_;
	    RecordingSegmentationRef recSeg = recordingSegmentation();
	    BoundedSegmentList boundedSegs = buildBoundedSegments(*recSeg);
	    s32 nRecFrames = nFrames(recSeg->desc->startTime(), recSeg->desc->endTime());
	    {
		Core::Component::Message msg(log());
		msg << "Recording \"" << recSeg->desc->recordingId() << ":"
		    << recSeg->desc->startTime() << "-" << recSeg->desc->endTime() << "\"\n"
		    << "  #frames=" << nRecFrames << "\n";
		dumpBoundedSegments(msg, boundedSegs);
	    }
	    if (boundedSegs.empty())
		return ConstPosteriorCnRef();

	    PosteriorCn *concatCn = new PosteriorCn;
	    concatCn->reserve(nRecFrames);
	    for (BoundedSegmentList::iterator it = boundedSegs.begin(); it != boundedSegs.end(); ++it) {
		ConstPosteriorCnRef cn = reader_->get(it->segment->segmentId());
		concatCn->alphabet = cn->alphabet;
		verify(concatCn->size() == it->offset);
		s32 t = it->startFrame, end = it->endFrame;
		PosteriorCn::const_iterator itSlot = cn->begin(), endSlot = cn->end();
		for (; t < 0; ++t)
		    concatCn->push_back(epsSlot_);
		for (; (t < end) && (itSlot != endSlot); ++t, ++itSlot)
		    concatCn->push_back(*itSlot);
		for (; t < end; ++t)
		    concatCn->push_back(epsSlot_);
	    }
	    verify(concatCn->size() == size_t(nRecFrames));
	    buffer_ = ConstPosteriorCnRef(concatCn);
	    isValid_ = true;
	    return buffer_;
	}
    };
    NodeRef createConcatenateFCnsNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new ConcatenateFCnsNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
