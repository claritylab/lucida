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
#include "DataExtractor.hh"
#include <Am/AcousticModel.hh>
#include <Speech/Module.hh>

using namespace Speech;


DataExtractor::DataExtractor(const Core::Configuration &c, bool loadFromFile) :
    Component(c),
    Precursor(c),
    statisticsChannel_(c, "statistics"),
    nRecordings_(0), nSegments_(0)
{
    setDataSource(Core::Ref<Speech::DataSource>(Speech::Module::instance().createDataSource(select("feature-extraction"), loadFromFile)));
    dataSource_->respondToDelayedErrors();
}

void DataExtractor::signOn(CorpusVisitor &corpusVisitor)
{
    corpusVisitor.signOn(dataSource_);
    Precursor::signOn(corpusVisitor);
}

void DataExtractor::enterCorpus(Bliss::Corpus *c)
{
    Precursor::enterCorpus(c);

    if (!c->level()) {
	nRecordings_ = nSegments_ = 0;
	nFrames_.clear();
    }

}

void DataExtractor::leaveCorpus(Bliss::Corpus *c)
{
    if (c->level() == 0 && statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("statistics");
	statisticsChannel_ << Core::XmlEmpty("recordings") + Core::XmlAttribute("number", nRecordings_);
	statisticsChannel_ << Core::XmlEmpty("segments") + Core::XmlAttribute("number", nSegments_);

	for(Flow::PortId portId = 0; portId < (Flow::PortId)nFrames_.size(); ++ portId) {
	    statisticsChannel_ << Core::XmlEmpty("frames")
		+ Core::XmlAttribute("port", portNames_[portId])
		+ Core::XmlAttribute("number", nFrames_[portId]);
	}
	statisticsChannel_ << Core::XmlClose("statistics");
    }

    Precursor::leaveCorpus(c);
}

void DataExtractor::enterRecording(Bliss::Recording *recording)
{
    Precursor::enterRecording(recording);
    nRecordings_ += 1;
}

void DataExtractor::enterSegment(Bliss::Segment *segment)
{
    Precursor::enterSegment(segment);
    nSegments_ += 1;
    dataSource_->initialize(segment);
}

void DataExtractor::leaveSegment(Bliss::Segment *segment)
{
    dataSource_->finalize();

    reportRealTime(dataSource_->realTime());

    const std::vector<size_t> &nFrames(dataSource_->nFrames());
    for(size_t i = nFrames_.size(); i < nFrames.size(); ++ i) {
	portNames_.push_back(dataSource_->outputName(i));
	nFrames_.push_back(0);
    }

    if (statisticsChannel_.isOpen()) {
	statisticsChannel_ << Core::XmlOpen("statistics");
	for(Flow::PortId portId = 0; portId < (Flow::PortId)nFrames.size(); ++ portId) {
	    nFrames_[portId] += nFrames[portId];

	    statisticsChannel_ << Core::XmlEmpty("frames")
		+ Core::XmlAttribute("port", portNames_[portId])
		+ Core::XmlAttribute("number", nFrames[portId]);
	}
	statisticsChannel_ << Core::XmlClose("statistics");
    }

    Precursor::leaveSegment(segment);
}

void DataExtractor::processSegment(Bliss::Segment* segment)
{
    while(dataSource()->getData());
}

// ====================================================================================================
void FeatureExtractor::processSegment(Bliss::Segment* segment)
{
    Core::Ref<Feature> feature;
    bool firstFeature = true;
    while(dataSource()->getData(feature)) {
	if (firstFeature) { // try to check the dimension only once for each segment
	    setFeatureDescription(Mm::FeatureDescription(*this, *feature));
	    firstFeature = false;
	}
	processFeature(feature);
    }
}
