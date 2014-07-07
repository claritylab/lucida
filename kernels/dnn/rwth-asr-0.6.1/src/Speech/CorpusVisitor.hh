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
#ifndef _SPEECH_CORPUS_VISITOR_HH
#define _SPEECH_CORPUS_VISITOR_HH

#include <Bliss/CorpusKey.hh>
#include "DataSource.hh"
#include <Core/ReferenceCounting.hh>
#include <Bliss/CorpusDescription.hh>
#include <Flow/Network.hh>

namespace Speech {

    class CorpusProcessor;

    /**
     * CorpusVisitor traverses the corpus description
     * and configures the registered objects.
     * Following types can be registered (sign on):
     *  - CorpusProcessor
     *  - DataSource
     *  - CorpusKey
     */

    class CorpusVisitor :
	public Core::Component,
	public Bliss::CorpusVisitor
    {
    private:
	std::vector<Core::Ref<Bliss::CorpusKey> > corpusKeys_;
	std::vector<Core::Ref<DataSource> > dataSources_;
	std::vector<CorpusProcessor*> corpusProcessors_;

	size_t recordingIndex_;
	size_t segmentIndex_;
    private:
	template<class Reference>
	void signOn(std::vector<Reference> &v, Reference r) {
	    require(!isSignedOn(v, r));
	    v.push_back(r);
	}
	template<class Reference>
	bool isSignedOn(const std::vector<Reference> &v, Reference r) const {
	    return std::find(v.begin(), v.end(), r) != v.end();
	}
    protected:
	virtual void enterCorpus(Bliss::Corpus*);
	virtual void leaveCorpus(Bliss::Corpus*);
	virtual void enterRecording(Bliss::Recording*);
	virtual void leaveRecording(Bliss::Recording*);
	virtual void visitSegment(Bliss::Segment*);
	virtual void visitSpeechSegment(Bliss::SpeechSegment*);
    public:
	CorpusVisitor(const Core::Configuration &c);

	virtual ~CorpusVisitor() {}

	void signOn(CorpusProcessor* p) { signOn(corpusProcessors_, p); }
	void signOn(Core::Ref<DataSource> d) { signOn(dataSources_, d); }
	void signOn(Core::Ref<Bliss::CorpusKey> k) { signOn(corpusKeys_, k); }

	void clearRegistrations();

	bool isSignedOn(Core::Ref<DataSource> d) const { return this->isSignedOn(dataSources_, d); }
    };

} // namespace Speech

#endif // _SPEECH_CORPUS_VISITOR_HH
