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
#include <Bliss/CorpusDescription.hh>
#include <Core/Application.hh>
#include <Speech/CorpusVisitor.hh>

#include "CorpusProcessor.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    Processor* CorpusProcessor::create(const Core::Configuration &config, Network *network) {
	SpeechSegmentNodeList *speechSegNodes = new SpeechSegmentNodeList();
	for (Network::NodeList::iterator it = network->nodes().begin();
	     it != network->nodes().end(); ++it) {
	    SpeechSegmentNode *speechSegNode = dynamic_cast<SpeechSegmentNode*>(it->get());
	    if (speechSegNode)
		speechSegNodes->push_back(speechSegNode);
	}
	if (speechSegNodes->empty()) {
	    delete speechSegNodes;
	    return 0;
	} else {
	    Core::Application::us()->log("CorpusProcessor: Process network.");
	    return new CorpusProcessor(config, network, speechSegNodes);
	}
    }

    CorpusProcessor::CorpusProcessor(
	const Core::Configuration &config,
	Network *network, SpeechSegmentNodeList *speechSegNodes) :
	Core::Component(config),
	Processor(config, network),
	Speech::CorpusProcessor(config),
	corpusVisitor_(0),
	speechSegNodes_(speechSegNodes),
	good_(true) {
    }

    CorpusProcessor::~CorpusProcessor() {
	delete speechSegNodes_;
	delete corpusVisitor_;
    }

    bool CorpusProcessor::init(const std::vector<std::string> &arguments) {
	corpusVisitor_ = new Speech::CorpusVisitor(config);
	signOn(*corpusVisitor_);
	return Processor::init(arguments);
    }

    void CorpusProcessor::run() {
	Bliss::CorpusDescription corpusDescription(select("corpus"));
	corpusDescription.accept(corpusVisitor_);
    }

    void CorpusProcessor::processSpeechSegment(Bliss::SpeechSegment *segment) {
	if (!good_)
	    criticalError("At least one source node is out of data.");
	for (SpeechSegmentNodeList::iterator it = speechSegNodes_->begin();
	     it != speechSegNodes_->end(); ++it)
	    (*it)->setSpeechSegment(segment);

	bool repeat = true;
	while (repeat)
	{
	    repeat = false;
	crawler_->reset();
	network_->pull();
	good_ = network_->sync(*crawler_);
	    for (SpeechSegmentNodeList::iterator it = speechSegNodes_->begin();
	     it != speechSegNodes_->end(); ++it)
		 if (!(*it)->synced())
		 {
		     log() << "repeating segment";
		     repeat = true;
		 }
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    SpeechSegmentNode::SpeechSegmentNode(const std::string &name, const Core::Configuration &config) :
	Node(name, config), blissSpeechSegment_(0) {}

    void SpeechSegmentNode::init(const std::vector<std::string> &arguments) {
	if (!in().empty())
	    error("SpeechSegmentNode: Do not expect incoming links.");
    }

    void SpeechSegmentNode::sync() {
	blissSpeechSegment_ = 0;
	segment_.reset();
    }

    ConstSegmentRef SpeechSegmentNode::sendSegment(Port to) {
	verify(to == 0);
	if (!segment_) {
	    if (!blissSpeechSegment_)
		criticalError("SpeechSegmentNode: Bliss speech segment not set");
	    segment_ = ConstSegmentRef(new Segment(blissSpeechSegment_));
	}
	return segment_;
    }

    const void * SpeechSegmentNode::sendData(Port to) {
	verify(to == 1);
	if (!blissSpeechSegment_)
	    criticalError("SpeechSegmentNode: Bliss speech segment not set");
	return blissSpeechSegment_;
    }

    NodeRef createSpeechSegmentNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new SpeechSegmentNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
