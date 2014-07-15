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
#ifndef _FLF_CORPUS_PROCESSOR_HH
#define _FLF_CORPUS_PROCESSOR_HH

#include <Speech/CorpusVisitor.hh>
#include <Speech/CorpusProcessor.hh>

#include "Processor.hh"
#include "Segment.hh"


namespace Flf {

    class SpeechSegmentNode;

    class CorpusProcessor : public Processor, public Speech::CorpusProcessor {
    private:
	typedef std::vector<SpeechSegmentNode*> SpeechSegmentNodeList;
    private:
	Speech::CorpusVisitor *corpusVisitor_;
	SpeechSegmentNodeList *speechSegNodes_;
	bool good_;
    private:
	CorpusProcessor(const Core::Configuration &config,
			Network *network, SpeechSegmentNodeList *speechSegNodes);
    public:
	virtual ~CorpusProcessor();
	virtual bool init(const std::vector<std::string> &arguments);
	virtual void run();
	void processSpeechSegment(Bliss::SpeechSegment *segment);

	static Processor* create(const Core::Configuration &config, Network *network);
    };

    /*
      Port 0: ConstSegmentRef
      Port 1: void* (const Bliss::SpeechSegment*)
    */
    class SpeechSegmentNode : public Node {
    private:
	const Bliss::SpeechSegment *blissSpeechSegment_;
	ConstSegmentRef segment_;
    public:
	SpeechSegmentNode(const std::string &name, const Core::Configuration &config);
	virtual void init(const std::vector<std::string> &arguments);
	virtual void sync();
	bool synced() const { return !blissSpeechSegment_ ; }
	virtual bool good() { return true; }
	void setSpeechSegment(Bliss::SpeechSegment *blissSpeechSegment)
	    { blissSpeechSegment_ = blissSpeechSegment; }
	virtual ConstSegmentRef sendSegment(Port to);
	virtual const void * sendData(Port to);
    };
    NodeRef createSpeechSegmentNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_CORPUS_PROCESSOR_HH
