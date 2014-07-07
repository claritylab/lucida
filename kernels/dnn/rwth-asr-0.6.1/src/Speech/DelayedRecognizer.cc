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
#include <Speech/DelayedRecognizer.hh>

using namespace Speech;

DelayedRecognizer::DelayedRecognizer(const Core::Configuration &c) :
    Core::Component(c),
    OfflineRecognizer(c),
    delay_(recognizer_, acousticModel_)
{
}

void DelayedRecognizer::signOn(CorpusVisitor &corpusVisitor)
{
    // @todo: remove code duplication. this virtual method is misused
    // in order to check for lookAheadLength == 0 in OfflineLineRecognizer
    FeatureExtractor::signOn(corpusVisitor);
    acousticModel_->signOn(corpusVisitor);
}

void DelayedRecognizer::enterSpeechSegment(Bliss::SpeechSegment *segment)
{
    OfflineRecognizer::enterSpeechSegment(segment);
    delay_.reset();
}

void DelayedRecognizer::processFeature(Core::Ref<const Feature> f)
{
    Core::Ref<const Feature> currentFeature = delay_.add(f);
    if(currentFeature.get())
	processFeatureTimestamp(currentFeature->timestamp());
}

void DelayedRecognizer::leaveSpeechSegment(Bliss::SpeechSegment *segment)
{
    Core::Ref<const Feature> currentFeature = delay_.flush();
    while(currentFeature)
    {
	processFeatureTimestamp(currentFeature->timestamp());
	currentFeature = delay_.flush();
    }

    ensure(acousticModel_->featureScorer()->bufferEmpty());
    finishSegment(segment);
}

RecognizerDelayHandler::RecognizerDelayHandler(Search::SearchAlgorithm* recognizer, Core::Ref<Am::AcousticModel> acousticModel, Core::Ref<Mm::ContextScorerCache> cache) :
    recognizer_(recognizer),
    acousticModel_(acousticModel),
    bufferSize_(0),
    featureScorerOffset_(0),
    initializeScorer_(true),
    cache_(cache)
{
    verify(recognizer_ && acousticModel_);
    bufferSize_ = std::max(recognizer_->lookAheadLength(), acousticModel_->featureScorer()->bufferSize());
}

void RecognizerDelayHandler::reset() {
    initializeScorer_ = true;
    featureBuffer_.clear();
    while(!acousticModel_->featureScorer()->bufferEmpty())
	acousticModel_->featureScorer()->flush();
}

Core::Ref<const Feature> RecognizerDelayHandler::add(Core::Ref<const Feature> f) {
    if (featureBuffer_.size() < bufferSize_) {
	featureBuffer_.push_back(f);
	return Core::Ref<const Feature>();
    } else {
	featureBuffer_.push_back(f);
	Core::Ref<const Mm::ScaledFeatureScorer> scorer = acousticModel_->featureScorer();
	if (scorer->isBuffered() && initializeScorer_) {
	    initializeBatchScorer();
	    initializeScorer_ = false;
	}
	Core::Ref<const Feature> currentFeature = featureBuffer_.front();
	if (cache_.get()) {
	    Mm::CachedContextScorer cachedScorer = cache_->cacheScorer(currentFeature, scorer->nMixtures());
	    if (!cachedScorer->precached())
		cachedScorer->setScorer(scorer->getScorer(featureBuffer_[featureScorerOffset_]));
	    featureBuffer_.pop_front();
	    setLookAhead();
	    recognizer_->feed(cachedScorer);
	    cache_->uncacheScorer(currentFeature, cachedScorer);
	}else{
	    Mm::FeatureScorer::Scorer currentScorer = scorer->getScorer(featureBuffer_[featureScorerOffset_]);
	    featureBuffer_.pop_front();
	    setLookAhead();
	    recognizer_->feed(currentScorer);
	}
	return currentFeature;
    }
}

Core::Ref<const Feature> RecognizerDelayHandler::flush() {
    if(featureBuffer_.empty() || initializeScorer_)
	return Core::Ref<const Feature>();

    Core::Ref<const Mm::ScaledFeatureScorer> scorer = acousticModel_->featureScorer();

    bool allPreCached = (bool)cache_;
    if (cache_) {
	for (std::deque<Core::Ref<const Feature> >::iterator it = featureBuffer_.begin(); it != featureBuffer_.end(); ++it) {
	    Mm::CachedContextScorer cachedScorer = cache_->cacheScorer(*it, scorer->nMixtures());
	    if (!cachedScorer->precached())
		allPreCached = false;
	    cache_->uncacheScorer(*it, cachedScorer);
	}
    }

    Mm::FeatureScorer::Scorer currentScorer;
    if (!allPreCached) {
	if (featureBuffer_.size() > featureScorerOffset_) {
	    currentScorer = scorer->getScorer(featureBuffer_[featureScorerOffset_]);
	} else {
	    verify_(scorer->isBuffered());
	    currentScorer = scorer->flush();
	}
    }
    Core::Ref<const Feature> currentFeature = featureBuffer_.front();
    featureBuffer_.pop_front();
    if(cache_) {
	Mm::CachedContextScorer cachedScorer = cache_->cacheScorer(currentFeature, scorer->nMixtures());
	cachedScorer->setScorer(currentScorer);
	setLookAhead();
	recognizer_->feed(cachedScorer);
	cache_->uncacheScorer(currentFeature, cachedScorer);
    }else{
	setLookAhead();
	recognizer_->feed(currentScorer);
    }
    return currentFeature;
}

void RecognizerDelayHandler::setLookAhead() {
// @todo avoid copying the feature vectors, use vector< Core::Ref<FeatureVector> > instead.
    std::vector<Mm::FeatureVector> lah;
    for (FeatureBuffer::const_iterator f = featureBuffer_.begin(); f != featureBuffer_.end(); ++f)
	lah.push_back(*(*f)->mainStream());
    recognizer_->setLookAhead(lah);
}

void RecognizerDelayHandler::initializeBatchScorer() {
    Core::Ref<const Mm::ScaledFeatureScorer> scorer = acousticModel_->featureScorer();
    u32 pos = 0;
    while (!scorer->bufferFilled() && pos < (featureBuffer_.size() - 1)) {
	scorer->addFeature(featureBuffer_[pos]);
	++pos;
    }
    featureScorerOffset_ = pos;
    ensure(scorer->bufferFilled());
}
