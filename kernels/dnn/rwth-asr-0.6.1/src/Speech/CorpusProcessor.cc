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
#include <Flow/Types.hh>
#include "CorpusProcessor.hh"

using namespace Speech;


CorpusProcessor::CorpusProcessor(const Core::Configuration &c) :
    Component(c),
    channelTimer_(c, "real-time-factor")
{}


CorpusProcessor::~CorpusProcessor()
{}


void CorpusProcessor::signOn(CorpusVisitor &corpusVisitor)
{
    corpusVisitor.signOn(this);
}

void CorpusProcessor::enterCorpus(Bliss::Corpus*) {}
void CorpusProcessor::leaveCorpus(Bliss::Corpus*) {}
void CorpusProcessor::enterRecording(Bliss::Recording*) {}
void CorpusProcessor::leaveRecording(Bliss::Recording*) {}

void CorpusProcessor::enterSegment(Bliss::Segment*) {
    timer_.start();
}

void CorpusProcessor::enterSpeechSegment(Bliss::SpeechSegment *speechSegment) {
    enterSegment(speechSegment);
}

void CorpusProcessor::processSegment(Bliss::Segment*) {}

void CorpusProcessor::processSpeechSegment(Bliss::SpeechSegment *speechSegment) {
    processSegment(speechSegment);
}

void CorpusProcessor::reportRealTime(Flow::Time realTime) {
    timer_.stop();
    if (channelTimer_.isOpen()) {
	timer_.write(channelTimer_);
	channelTimer_
	    << Core::XmlFull("real-time", realTime)
	    << Core::XmlFull("real-time-factor", timer_.user() / realTime)
	     + Core::XmlAttribute("reference", "user time")
	    << Core::XmlFull("real-time-factor", timer_.elapsed() / realTime)
	     + Core::XmlAttribute("reference", "elapsed time");
    }
}

void CorpusProcessor::leaveSegment(Bliss::Segment *s)
{
    if (timer_.isRunning()) {
	timer_.stop();
	timer_.write(channelTimer_);
    }
}

void CorpusProcessor::leaveSpeechSegment(Bliss::SpeechSegment *speechSegment) {
    leaveSegment(speechSegment);
}
