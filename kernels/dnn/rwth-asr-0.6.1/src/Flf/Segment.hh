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
#ifndef _FLF_SEGMENT_HH
#define _FLF_SEGMENT_HH

#include <Bliss/CorpusDescription.hh>
#include <Core/ReferenceCounting.hh>

namespace Flf {

    class Segment : public Core::ReferenceCounted {
    private:
	std::string audioFilename_;
	bool hasAudioFilename_;
	f32 startTime_;
	bool hasStartTime_;
	f32 endTime_;
	bool hasEndTime_;
	u32 track_;
	bool hasTrack_;
	std::string orthography_;
	bool hasOrthography_;
	std::string speakerId_;
	bool hasSpeakerId_;
	std::string conditionId_;
	bool hasConditionId_;
	std::string recordingId_;
	bool hasRecordingId_;
	std::string segmentId_;
	bool hasSegmentId_;

	const Bliss::SpeechSegment *blissSpeechSegment_;

    public:
	Segment() :
	    audioFilename_(),
	    hasAudioFilename_(false),
	    startTime_(Core::Type<f32>::min),
	    hasStartTime_(false),
	    endTime_(Core::Type<f32>::max),
	    hasEndTime_(false),
	    track_(0),
	    hasTrack_(false),
	    orthography_(),
	    hasOrthography_(false),
	    speakerId_(),
	    hasSpeakerId_(false),
	    conditionId_(),
	    hasConditionId_(false),
	    recordingId_(),
	    hasRecordingId_(false),
	    segmentId_(),
	    hasSegmentId_(false),
	    blissSpeechSegment_(0) {}

	Segment(const Bliss::SpeechSegment *blissSpeechSegment) {
	    setBlissSpeechSegment(blissSpeechSegment);
	}

	bool hasAudioFilename() const {
	    return hasAudioFilename_;
	}
	const std::string & audioFilename() const {
	    return audioFilename_;
	}
	void setAudioFilename(const std::string &audioFilename) {
	    audioFilename_ = audioFilename;
	    hasAudioFilename_ = true;
	}

	bool hasStartTime() const {
	    return hasStartTime_;
	}
	f32 startTime() const {
	    return startTime_;
	}
	void setStartTime(f32 startTime) {
	    startTime_ = startTime;
	    hasStartTime_ = true;
	}

	bool hasEndTime() const {
	    return hasEndTime_;
	}
	f32 endTime() const {
	    return endTime_;
	}
	void setEndTime(f32 endTime) {
	    endTime_ = endTime;
	    hasEndTime_ = true;
	}

	bool hasTrack() const {
	    return hasTrack_;
	}
	u32 track() const {
	    return track_;
	}
	void setTrack(u32 track) {
	    track_ = track;
	    hasTrack_ = true;
	}

	bool hasOrthography() const {
	    return hasOrthography_;
	}
	const std::string & orthography() const {
	    return orthography_;
	}
	void setOrthography(const std::string &orthography) {
	    orthography_ = orthography;
	    hasOrthography_ = true;
	}

	bool hasSpeakerId() const {
	    return hasSpeakerId_;
	}
	const std::string & speakerId() const {
	    return speakerId_;
	}
	void setSpeakerId(const std::string &speakerId) {
	    speakerId_ = speakerId;
	    hasSpeakerId_ = true;
	}

	bool hasConditionId() const {
	    return hasConditionId_;
	}
	const std::string & conditionId() const {
	    return conditionId_;
	}
	void setConditionId(const std::string &conditionId) {
	    conditionId_ = conditionId;
	    hasConditionId_ = true;
	}

	bool hasRecordingId() const {
	    return hasRecordingId_;
	}
	const std::string & recordingId() const {
	    return recordingId_;
	}
	void setRecordingId(const std::string &recordingId) {
	    recordingId_ = recordingId;
	    hasRecordingId_ = true;
	}

	bool hasSegmentId() const {
	    return hasSegmentId_;
	}
	const std::string & segmentId() const {
	    return segmentId_;
	}
	void setSegmentId(const std::string &segmentId) {
	    segmentId_ = segmentId;
	    hasSegmentId_ = true;
	}
	const std::string & segmentIdOrDie() const;

	bool hasBlissSpeechSegment() const {
	    return blissSpeechSegment_;
	}
	const Bliss::SpeechSegment * blissSpeechSegment() const {
	    return blissSpeechSegment_;
	}
	void setBlissSpeechSegment(const Bliss::SpeechSegment *blissSpeechSegment);
	const Bliss::SpeechSegment * blissSpeechSegmentOrDie() const;
    };
    typedef Core::Ref<Segment> SegmentRef;
    typedef Core::Ref<const Segment> ConstSegmentRef;


    s32 nFrames(f32 startTime, f32 endTime);
    s32 nFrames(ConstSegmentRef segment);


    // Prints a single line of the following form:
    // # <segment-name> (<start-time>-<end-time>)
    // or, if the segment provides no information,
    // # unknown (-inf-inf)
    std::ostream & printAsText(std::ostream &os, ConstSegmentRef segment);
    void printSegmentHeader(std::ostream &os, ConstSegmentRef segment, const std::string &head = "# ");

} // namespace Flf

#endif // _FLF_SEGMENT_HH
