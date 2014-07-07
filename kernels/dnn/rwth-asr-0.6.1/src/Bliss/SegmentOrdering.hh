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
#ifndef _BLISS_SEGMENT_ORDERING_HH
#define _BLISS_SEGMENT_ORDERING_HH

#include <Bliss/CorpusDescription.hh>

namespace Bliss {

/**
 * Changes the order of processed segments according to a given
 * segment id list (full segment names).
 * This visitor needs to make a copy of each sub-corpus, recording, and
 * segment object, because they are immediately deleted by
 * the CorpusDescriptionParser
 */
class SegmentOrderingVisitor : public CorpusVisitor
{
public:
	SegmentOrderingVisitor()
	: visitor_(0), curSegment_(0), curRecording_(0), shortNameLookup_(false) {}
	SegmentOrderingVisitor(bool shortNameLookup)
	: visitor_(0), curSegment_(0), curRecording_(0), shortNameLookup_(shortNameLookup) {}

	virtual ~SegmentOrderingVisitor();

	void setVisitor(CorpusVisitor *v) { visitor_ = v; }

	void setSegmentList(const std::string &filename);
	virtual void enterRecording(Recording *r);
	virtual void enterCorpus(Corpus *c);
	virtual void leaveCorpus(Corpus *corpus);
	virtual void visitSegment(Segment *s) {
		curSegment_ = s;
		Segment *segment = new Segment(*s);
		addSegment(segment);
	}
	virtual void visitSpeechSegment(SpeechSegment *s);
private:
	class CorpusCopy;
	class RecordingCopy;

	void addSegment(Segment *segment);
	template<class T, class C>
	const T* updateSegmentData(Segment *segment, const T *entry, C &map);
	void updateCondition(Segment *segment);
	void updateSpeaker(SpeechSegment *segment);
	template<class T> const std::string _getName(const T* entry);

	typedef Core::StringHashMap<Segment*> SegmentMap;
	typedef Core::StringHashMap<Speaker*> SpeakerMap;
	typedef Core::StringHashMap<AcousticCondition*> ConditionMap;
	typedef std::vector< std::pair<Corpus*, Corpus*> > CorpusMap;
	CorpusVisitor *visitor_;
	std::vector<Recording*> recordings_;
	std::vector<Corpus*> corpus_;
	CorpusMap curCorpus_;
	SpeakerMap speakers_;
	ConditionMap conditions_;
	const Segment *curSegment_;
	const Recording *curRecording_;
	SegmentMap segments_;
	std::vector<std::string> segmentList_;
	bool shortNameLookup_;
};


} // namespace Bliss

#endif // _BLISS_SEGMENT_ORDERING_HH
