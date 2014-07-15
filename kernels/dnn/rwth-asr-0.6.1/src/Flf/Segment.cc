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

#include "Segment.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    const std::string & Segment::segmentIdOrDie() const {
	if (!hasSegmentId())
	    Core::Application::us()->criticalError(
		"Segment: Segment id is not provided.");
	return segmentId();
    }

    void Segment::setBlissSpeechSegment(const Bliss::SpeechSegment *blissSpeechSegment) {
	blissSpeechSegment_ = blissSpeechSegment;
	if (blissSpeechSegment_->recording())
	    setAudioFilename(blissSpeechSegment_->recording()->audio());
	setStartTime(blissSpeechSegment_->start());
	setEndTime(blissSpeechSegment_->end());
	setTrack(blissSpeechSegment_->track());
	setOrthography(blissSpeechSegment_->orth());
	if (blissSpeechSegment_->speaker())
	    setSpeakerId(blissSpeechSegment_->speaker()->name());
	if (blissSpeechSegment_->condition())
	    setConditionId(blissSpeechSegment_->condition()->name());
	if (blissSpeechSegment_->recording())
	    setRecordingId(blissSpeechSegment_->recording()->name());
	setSegmentId(blissSpeechSegment_->fullName());
    }

    const Bliss::SpeechSegment * Segment::blissSpeechSegmentOrDie() const {
	if (!hasBlissSpeechSegment())
	    Core::Application::us()->criticalError(
		"Segment: Bliss speech segment is not provided.");
	return blissSpeechSegment();
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    s32 nFrames(f32 startTime, f32 endTime) {
	return s32(Core::round((endTime - startTime) * 100.0));
    }
    s32 nFrames(ConstSegmentRef segment) {
	verify(segment->hasStartTime() && segment->hasEndTime());
	return nFrames(segment->startTime(), segment->endTime());
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    std::ostream & printAsText(std::ostream &os, ConstSegmentRef segment) {
	if (segment) {
	    if (segment->hasSegmentId())
		os << segment->segmentId();
	    else if (segment->hasRecordingId())
		os << segment->recordingId();
	    else
		os << "segment";
	    os << " (";
	    if (segment->hasStartTime())
		os << segment->startTime();
	    else
		os << "-inf";
	    os << "-";
	    if (segment->hasEndTime())
		os << segment->endTime();
	    else
		os << "inf";
	    os << ")";
	} else
	    os << "# unknown (-inf-inf)";
	return os;
    }

    void printSegmentHeader(std::ostream &os, ConstSegmentRef segment, const std::string &head) {
	printAsText(os << head, segment) << std::endl;
    }
    // -------------------------------------------------------------------------

} // namespace Flf
