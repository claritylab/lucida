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
// $Id: CorpusVisitor.cc 6997 2009-01-29 19:30:27Z rybach $

#include <Core/XmlStream.hh>
#include <Flow/Network.hh>
#include "CorpusProcessor.hh"
#include "CorpusVisitor.hh"

using namespace Core;
using namespace Speech;


/** Parameter adaptor functions and functors for passing corpus section parameters to
 *  DatasSource objects,
 */
class DataSourceParameterAdaptor {
    std::vector<Core::Ref<DataSource> > &dataSources_;
public:

    DataSourceParameterAdaptor(std::vector<Core::Ref<DataSource> > &dataSources) :
	dataSources_(dataSources) {}

    void set(const std::string &name, const std::string &value) {
	std::vector<Core::Ref<DataSource> >::iterator dataSource;
	for(dataSource = dataSources_.begin(); dataSource != dataSources_.end(); ++ dataSource)
	    (*dataSource)->setParameter(name, value);
    }

    void clear(const std::string &name) {
	std::vector<Core::Ref<DataSource> >::iterator dataSource;
	for(dataSource = dataSources_.begin(); dataSource != dataSources_.end(); ++ dataSource)
	    (*dataSource)->setParameter(name, "");
    }
};

/** Parameter adaptor functions and functors for passing corpus section parameters to
 *  CorpusKeys objects.
 */
class StringExpressionAdaptor {
    std::vector<Core::Ref<Bliss::CorpusKey> > &corpusKeys_;
public:
    StringExpressionAdaptor(std::vector<Core::Ref<Bliss::CorpusKey> > &corpusKeys) :
	corpusKeys_(corpusKeys) {}

    void set(const std::string &name, const std::string &value) {
	std::vector<Core::Ref<Bliss::CorpusKey> >::iterator corpusKey;
	for(corpusKey = corpusKeys_.begin(); corpusKey != corpusKeys_.end(); ++ corpusKey)
	    (*corpusKey)->setVariable(name, value);
    }

    void clear(const std::string &name) {
	std::vector<Core::Ref<Bliss::CorpusKey> >::iterator corpusKey;
	for(corpusKey = corpusKeys_.begin(); corpusKey != corpusKeys_.end(); ++ corpusKey)
	    (*corpusKey)->clear(name);
    }
};

template<class ParameterAdaptor>
void setParameter(size_t index, Bliss::Recording *recording, ParameterAdaptor parameterAdaptor)
{
    std::string inputFile(recording->video());
    if (inputFile.empty()) inputFile = recording->audio();
    parameterAdaptor.set("input-file", inputFile);
    if (!recording->audio().empty()) {
	parameterAdaptor.set("input-audio-file", recording->audio());
	parameterAdaptor.set("input-audio-name", recording->name());
    }
    if (!recording->video().empty()) {
	parameterAdaptor.set("input-video-file", recording->video());
	parameterAdaptor.set("input-video-name", recording->name());
    }
    parameterAdaptor.set("recording-index", Core::form("%zd", index));
}

template<class ParameterAdaptor>
void setParameter(size_t index, Bliss::Segment *segment, ParameterAdaptor parameterAdaptor)
{
    parameterAdaptor.set("id", segment->fullName());
    parameterAdaptor.set("segment-index", Core::form("%zd", index));
    parameterAdaptor.set("segment-type", std::string(Bliss::Segment::typeId[segment->type()]));
    parameterAdaptor.set("acoustic-condition", segment->condition() ? segment->condition()->name() : "");
    parameterAdaptor.set("start-time", Core::form("%g", segment->start()));
    parameterAdaptor.set("end-time", Core::form("%g", segment->end()));
    parameterAdaptor.set("track", Core::form("%d", segment->track()));

    // disassemble segment fullname: .../segment-1/segment-0 and corpus-0/corpus-1/...
    parameterAdaptor.set("segment", segment->fullName());
    parameterAdaptor.set("segment-0", segment->name());
    u32 segmentLevel = 1;
    Bliss::CorpusSection *corpusSection = segment->parent();
    while(corpusSection) {
	parameterAdaptor.set("corpus-" + Core::form("%d", corpusSection->level()),
			     corpusSection->name());
	parameterAdaptor.set("segment-" + Core::form("%d", segmentLevel),
			     corpusSection->name());
	corpusSection = corpusSection->parent();
	segmentLevel ++;
    }

}

template<class ParameterAdaptor>
void clearParameter(Bliss::Segment *segment, ParameterAdaptor parameterAdaptor)
{
}

template<class ParameterAdaptor>
void setParameter(size_t index, Bliss::SpeechSegment *speechSegment, ParameterAdaptor parameterAdaptor)
{
    setParameter(index, (Bliss::Segment*)speechSegment, parameterAdaptor);
    if (speechSegment->speaker() != 0) {
	parameterAdaptor.set("speaker", speechSegment->speaker()->name());
	parameterAdaptor.set("gender", Bliss::Speaker::genderId[speechSegment->speaker()->gender()]);
    }
    parameterAdaptor.set("orthography", speechSegment->orth());
}

template<class ParameterAdaptor>
void clearParameter(Bliss::SpeechSegment *speechSegment, ParameterAdaptor parameterAdaptor)
{
    parameterAdaptor.clear("speaker");
    parameterAdaptor.clear("gender");
    parameterAdaptor.clear("orthography");
    clearParameter((Bliss::Segment*)speechSegment, parameterAdaptor);
}

// CorpusVisitor
////////////////

CorpusVisitor::CorpusVisitor(const Core::Configuration &c) :
    Core::Component(c),
    recordingIndex_(0),
    segmentIndex_(0)
{}

void CorpusVisitor::enterCorpus(Bliss::Corpus *corpus)
{
    Bliss::CorpusVisitor::enterCorpus(corpus);
    recordingIndex_ = 0;
    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->enterCorpus(corpus);
}

void CorpusVisitor::leaveCorpus(Bliss::Corpus *corpus)
{
    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->leaveCorpus(corpus);

    Bliss::CorpusVisitor::leaveCorpus(corpus);
}

void CorpusVisitor::enterRecording(Bliss::Recording *recording)
{
    Bliss::CorpusVisitor::enterRecording(recording);
    segmentIndex_ = 0;

    setParameter(recordingIndex_, recording, DataSourceParameterAdaptor(dataSources_));
    setParameter(recordingIndex_, recording, StringExpressionAdaptor(corpusKeys_));

    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->enterRecording(recording);
}

void CorpusVisitor::leaveRecording(Bliss::Recording *recording)
{
    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->leaveRecording(recording);

    ++ recordingIndex_;
    Bliss::CorpusVisitor::leaveRecording(recording);
}

void CorpusVisitor::visitSegment(Bliss::Segment *segment)
{
    setParameter(segmentIndex_, segment, DataSourceParameterAdaptor(dataSources_));
    setParameter(segmentIndex_, segment, StringExpressionAdaptor(corpusKeys_));

    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->enterSegment(segment);
    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->processSegment(segment);
    for(size_t i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->leaveSegment(segment);
    ++ segmentIndex_;
}

void CorpusVisitor::visitSpeechSegment(Bliss::SpeechSegment *speechSegment)
{
    setParameter(segmentIndex_, speechSegment, DataSourceParameterAdaptor(dataSources_));
    setParameter(segmentIndex_, speechSegment, StringExpressionAdaptor(corpusKeys_));

    size_t i;
    for(i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->enterSpeechSegment(speechSegment);
    for(i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->processSpeechSegment(speechSegment);
    for(i = 0; i < corpusProcessors_.size(); ++ i)
	corpusProcessors_[i]->leaveSpeechSegment(speechSegment);

    clearParameter(speechSegment, DataSourceParameterAdaptor(dataSources_));
    clearParameter(speechSegment, StringExpressionAdaptor(corpusKeys_));
    ++ segmentIndex_;
}

void CorpusVisitor::clearRegistrations()
{
    corpusKeys_.clear();
    dataSources_.clear();
    corpusProcessors_.clear();
}
