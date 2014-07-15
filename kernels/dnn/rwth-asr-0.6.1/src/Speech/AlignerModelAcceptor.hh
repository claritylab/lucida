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
#ifndef _SPEECH_ALIGNER_MODEL_ACCEPTOR_HH
#define _SPEECH_ALIGNER_MODEL_ACCEPTOR_HH

#include "CorpusProcessor.hh"
#include "AllophoneStateGraphBuilder.hh"
#include <Fsa/Semiring.hh>
#include <Fsa/Static.hh>
#include <Am/AcousticModel.hh>
#include <Am/ClassicStateModel.hh>

namespace Speech {
    class AllophoneStateGraphBuilder;
    class FsaCache;
};

namespace Speech {

    typedef f32 AlignerScore;
    typedef Fsa::StaticAutomaton AlignerModelAcceptor;

    /**
     *  AlignerModelAcceptorGenerator
     */
    class AlignerModelAcceptorGenerator : public CorpusProcessor {
	typedef CorpusProcessor Precursor;
    private:
	AllophoneStateGraphBuilder *allophoneStateGraphBuilder_;
	FsaCache *cache_;
    public:
	AlignerModelAcceptorGenerator(const Core::Configuration&);
	virtual ~AlignerModelAcceptorGenerator();

	void enterSpeechSegment(Bliss::SpeechSegment*);
    };

} // namespace Speech

#endif // _SPEECH_ALIGNING_FEATURE_EXTRACTOR_HH
