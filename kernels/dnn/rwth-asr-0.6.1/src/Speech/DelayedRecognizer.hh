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
#ifndef _SPEECH_DELAYED_RECOGNIZER_HH
#define _SPEECH_DELAYED_RECOGNIZER_HH

#include <Speech/Recognizer.hh>

namespace Speech {


// Handles delayed feeding of features into a search-algorithm for buffered feature scorers and acoustic look-ahead
class RecognizerDelayHandler
{
public:
    RecognizerDelayHandler(Search::SearchAlgorithm* recognizer, Core::Ref <Am::AcousticModel> acousticModel, Core::Ref<Mm::ContextScorerCache> cache = Core::Ref<Mm::ContextScorerCache>());

    // Should be called whenever a new segment is started
    void reset();
    // Add a feature. The returned feature is another feature which was scored at this timeframe (may be zero if the buffer is currently being filled)
    Core::Ref<const Feature> add(Core::Ref<const Feature> f);
    // Flushes and scores features from the buffer. Returns the scored feature, or zero if the buffer is empty.
    Core::Ref<const Feature> flush();

private:
    void setLookAhead();
    void initializeBatchScorer();

    Search::SearchAlgorithm* recognizer_;
    Core::Ref <Am::AcousticModel> acousticModel_;
    // All features that are currently buffered in the feature scorer
    typedef std::deque<Core::Ref<const Feature> > FeatureBuffer;
    FeatureBuffer featureBuffer_;
    u32 bufferSize_, featureScorerOffset_;
    bool initializeScorer_;
    Core::Ref<Mm::ContextScorerCache> cache_;
};

/**
 * Recognizer with a delay in output, used for search algorithms
 * with an acoustic look-ahead.
 */
class DelayedRecognizer : public OfflineRecognizer
{
public:
    DelayedRecognizer(const Core::Configuration&);
    virtual ~DelayedRecognizer() {}

    virtual void signOn(CorpusVisitor &corpusVisitor);
    virtual void enterSpeechSegment(Bliss::SpeechSegment*);
    virtual void processFeature(Core::Ref<const Feature>);
    virtual void leaveSpeechSegment(Bliss::SpeechSegment*);

protected:
    RecognizerDelayHandler delay_;
};

} // namespace Speech

#endif /* _SPEECH_DELAYED_RECOGNIZER_HH */
