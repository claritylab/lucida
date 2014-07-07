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
#include <Bliss/SegmentOrdering.hh>
#include <Core/Application.hh>
#include <Core/CompressedStream.hh>

using namespace Bliss;

class SegmentOrderingVisitor::CorpusCopy  : public Corpus
{
public:
	CorpusCopy(const Corpus &c) : Corpus(c) {}
	void setParentCorpus(Corpus *corpus) {
		setParent(corpus);
	}
};

class SegmentOrderingVisitor::RecordingCopy : public Recording {
public:
	RecordingCopy(const Recording &r) : Recording(r) {}
	void setParentCorpus(Corpus *corpus) {
		setParent(corpus);
	}
};

SegmentOrderingVisitor::~SegmentOrderingVisitor()
{
	for (SpeakerMap::const_iterator s = speakers_.begin(); s != speakers_.end(); ++s)
		delete s->second;
	for (ConditionMap::const_iterator c = conditions_.begin(); c != conditions_.end(); ++c)
		delete c->second;
	for (SegmentMap::const_iterator s = segments_.begin(); s != segments_.end(); ++s)
		delete s->second;
	for (std::vector<Recording*>::const_iterator r = recordings_.begin(); r != recordings_.end(); ++r)
		delete *r;
	for (std::vector<Corpus*>::const_iterator c = corpus_.begin(); c != corpus_.end(); ++c)
		delete *c;
}


template<class T>
inline const std::string SegmentOrderingVisitor::_getName(const T* entry) {
	if (shortNameLookup_) {
		return entry->name();
	} else {
		return entry->fullName();
	}
}

void SegmentOrderingVisitor::addSegment(Segment *segment)
{
	verify(!recordings_.empty());
	segment->setRecording(recordings_.back());
	if (segment->condition())
		updateCondition(segment);
	std::string name = _getName(segment);
	if (segments_.find(name) != segments_.end()){
	    Core::Application::us()->error("can not add segment, because it is already present in segment list: ") << name;
	}
	segments_.insert(SegmentMap::value_type(name, segment));
}

template<class T, class C>
const T* SegmentOrderingVisitor::updateSegmentData(
		Segment *segment, const T *entry, C &map)
{
	if (entry->parent() != curCorpus_.front().first) {
		typename C::const_iterator c = map.find(_getName(entry));
		if (c != map.end()) {
			entry = c->second;
		} else {
			T *add = new T(*entry);
			if (entry->parent() == curRecording_) {
				add->setParent(recordings_.back());
			} else if (entry->parent() == curSegment_) {
				add->setParent(segment);
			} else {
				for (CorpusMap::const_reverse_iterator i = curCorpus_.rbegin();
						i != curCorpus_.rend(); ++i) {
					if (entry->parent() == i->first) {
						add->setParent(i->second);
						break;
					}
				}
			}
			map.insert(std::make_pair(_getName(entry), add));
			entry = add;
		}
	}
	return entry;
}

void SegmentOrderingVisitor::updateCondition(Segment *segment)
{
	const AcousticCondition *cond = segment->condition();
	verify(cond);
	cond = updateSegmentData(segment, cond, conditions_);
	segment->setCondition(cond);
}

void SegmentOrderingVisitor::updateSpeaker(SpeechSegment *segment)
{
	const Speaker *speaker = segment->speaker();
	verify(speaker);
	speaker = updateSegmentData(segment, speaker, speakers_);
	segment->setSpeaker(speaker);
}

void SegmentOrderingVisitor::setSegmentList(const std::string &filename)
{
	if (filename.empty()) return;
	Core::CompressedInputStream *cis = new Core::CompressedInputStream(filename.c_str());
	Core::TextInputStream is(cis);
	if (!is)
		Core::Application::us()->criticalError("Failed to open segment list \"%s\".", filename.c_str());
	std::string s;
	while (Core::getline(is, s) != EOF) {
		if ((s.size() == 0) || (s.at(0) == '#'))
			continue;
		Core::stripWhitespace(s);
		segmentList_.push_back(s);
	}
}

void SegmentOrderingVisitor::visitSpeechSegment(SpeechSegment *s) {
	curSegment_ = s;
	SpeechSegment *segment = new SpeechSegment(*s);
	if (s->speaker())
		updateSpeaker(segment);
	addSegment(segment);
}

void SegmentOrderingVisitor::enterRecording(Recording *r) {
	RecordingCopy *recording = new RecordingCopy(*r);
	recording->setParentCorpus(curCorpus_.back().second);
	recordings_.push_back(recording);
	curRecording_ = r;
}

void SegmentOrderingVisitor::enterCorpus(Corpus *c) {
	if (!curCorpus_.empty()) {
		// subcorpus
		CorpusCopy *corpus = new CorpusCopy(*c);
		corpus->setParentCorpus(curCorpus_.back().second);
		corpus_.push_back(corpus);
		curCorpus_.push_back(std::make_pair(c, corpus));
	} else {
		// root corpus
		curCorpus_.push_back(std::make_pair(c, c));
	}
}


void SegmentOrderingVisitor::leaveCorpus(Corpus *corpus)
{
	curCorpus_.pop_back();
	if (!curCorpus_.empty()) {
		// not the root corpus
		return;
	}
	Recording *curRecording = 0;
	Corpus *curCorpus = 0;
	for (std::vector<std::string>::const_iterator name = segmentList_.begin(); name != segmentList_.end(); ++name) {
		SegmentMap::const_iterator i = segments_.find(*name);
		if (i == segments_.end()) {
			Core::Application::us()->error("segment '%s' not found", name->c_str());
			continue;
		}
		Segment *segment = i->second;
		if (segment->recording() != curRecording) {
			if (curRecording)
				visitor_->leaveRecording(curRecording);
			curRecording = segment->recording();
			Corpus *recCorpus = static_cast<Corpus*>(curRecording->parent());
			if (recCorpus != curCorpus) {
				if (curCorpus) {
					visitor_->leaveCorpus(curCorpus);
				} else if (!corpus_.empty()) {
					// enter the root corpus
					visitor_->enterCorpus(corpus);
				}
				curCorpus = recCorpus;
				visitor_->enterCorpus(curCorpus);
			}
			visitor_->enterRecording(segment->recording());
		}
		segment->accept(visitor_);
	}
	if (curRecording)
		visitor_->leaveRecording(curRecording);
	if (curCorpus)
		visitor_->leaveCorpus(curCorpus);
	if (curCorpus != corpus)
		visitor_->leaveCorpus(corpus);
}
