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
#ifndef _LM_SCALED_LANGUAGE_MODEL_HH
#define _LM_SCALED_LANGUAGE_MODEL_HH

#include "LanguageModel.hh"
#include <Mc/Component.hh>
#include <Fsa/Arithmetic.hh>

namespace Lm {

    /** Base class for scaled language models
     *  @see Mc::Component.
     */
    class ScaledLanguageModel :
	public LanguageModel,
	public Mc::Component
    {
    protected:
	ScaledLanguageModel(const Core::Configuration &c, const Bliss::LexiconRef l) :
	    Core::Component(c), LanguageModel(c, l), Mc::Component(c) {}
    public:
	virtual ~ScaledLanguageModel() {}

	virtual void getDependencies(Core::DependencySet &) const;

	virtual Core::Ref<const LanguageModel> unscaled() const = 0;
    };

    /** Scaling of language model scores
     *  Wrapper class which multiplies the scores of an arbitrary language model object
     *  by predefined scale.
     */
    class LanguageModelScaling : public ScaledLanguageModel {
	typedef ScaledLanguageModel Precursor;
    private:
	Core::Ref<LanguageModel> languageModel_;
    public:
	LanguageModelScaling(const Core::Configuration &, Core::Ref<LanguageModel>);
	virtual ~LanguageModelScaling();

	virtual void init() { languageModel_->init(); }

	virtual void getDependencies(Core::DependencySet &) const;

	virtual Core::Ref<const LanguageModel> unscaled() const {
	    return languageModel_;
	}

	virtual Fsa::ConstAutomatonRef getFsa() const {
	    return Fsa::multiply(languageModel_->getFsa(), Fsa::Weight(scale()));
	}

	virtual History startHistory() const { return languageModel_->startHistory(); }
	virtual History extendedHistory(const History &h, Token w) const {
	    return languageModel_->extendedHistory(h, w);
	}
	virtual History reducedHistory(const History &h, u32 limit) const {
	    return languageModel_->reducedHistory(h, limit);
	}
	virtual std::string formatHistory(const History &h) const {
	    return languageModel_->formatHistory(h);
	}

	virtual Score score(const History &h, Token w) const {
	    return scale() * languageModel_->score(h, w);
	}
	virtual Score sentenceEndScore(const History &h) const {
	    return scale() * languageModel_->sentenceEndScore(h);
	}

	virtual Score sentenceBeginScore() const {
	    return scale() * languageModel_->sentenceBeginScore();
	}

	virtual CompiledBatchRequest* compileBatchRequest(const BatchRequest &r, Score scale = 1.0) const {
	    return languageModel_->compileBatchRequest(r, scale * Mc::Component::scale());
	}
	virtual void getBatch(const History &h,
			      const CompiledBatchRequest *r,
			      std::vector<f32> &result) const {
	    return languageModel_->getBatch(h, r, result);
	}
    };

    /** Language model score convenience function for syntactic tokens. */
    inline void addSyntacticTokenScore(
	Core::Ref<const LanguageModel> lm,
	Score lmScale,
	const Bliss::SyntacticToken *st,
	Score syntaxEmissionScale,
	const History &history, Score &score)
    {
	score += lmScale * lm->score(history, st);
	// p(class | synt) might be a separeted part of model combination
	// thus it might deserve a separeted scaling factor
	score += syntaxEmissionScale * st->classEmissionScore();
    }

    inline void addSyntacticTokenScore(
	Core::Ref<const ScaledLanguageModel> lm,
	const Bliss::SyntacticToken *st,
	Score syntaxEmissionScale,
	const History &history, Score &score)
    {
	addSyntacticTokenScore(
	    lm->unscaled(), lm->scale(),
	    st, syntaxEmissionScale,
	    history, score);
    }

    /** Language model score convenience function for lemmas. */
    inline void addLemmaScore(
	Core::Ref<const LanguageModel> lm,
	Score lmScale,
	const Bliss::Lemma *lemma,
	Score syntaxEmissionScale,
	History &history, Score &score)
    {
	require(lemma);
	const Bliss::SyntacticTokenSequence tokenSequence(lemma->syntacticTokenSequence());
	for (u32 ti = 0; ti < tokenSequence.length(); ++ti) {
	    const Bliss::SyntacticToken *st = tokenSequence[ti];
	    addSyntacticTokenScore(lm, lmScale, st, syntaxEmissionScale, history, score);
	    history = lm->extendedHistory(history, st);
	}
    }

    /** Language model score convenience function for lemmas. */
    inline void addLemmaScoreOmitExtension(
	Core::Ref<const LanguageModel> lm,
	Score lmScale,
	const Bliss::Lemma *lemma,
	Score syntaxEmissionScale,
	const History &history, Score &score)
    {
	require(lemma);
	const Bliss::SyntacticTokenSequence tokenSequence(lemma->syntacticTokenSequence());
	if (tokenSequence.length() == 0)
	{
	    return;
	}else if (tokenSequence.length() == 1)
	{
	    addSyntacticTokenScore(lm, lmScale, tokenSequence[0], syntaxEmissionScale, history, score);
	}else{
	    Lm::History h = history;
	    u32 last = tokenSequence.length()-1;
	    for (u32 ti = 0; ti < tokenSequence.length(); ++ti) {
		const Bliss::SyntacticToken *st = tokenSequence[ti];
		addSyntacticTokenScore(lm, lmScale, st, syntaxEmissionScale, h, score);
		if(ti != last)
		    h = lm->extendedHistory(h, st);
	    }
	}
    }

    inline void addLemmaScore(
	Core::Ref<const ScaledLanguageModel> lm,
	const Bliss::Lemma *lemma,
	Score syntaxEmissionScale,
	History &history, Score &score)
    {
	addLemmaScore(
	    lm->unscaled(), lm->scale(),
	    lemma, syntaxEmissionScale,
	    history, score);
    }

    inline void addLemmaScoreOmitExtension(
	Core::Ref<const ScaledLanguageModel> lm,
	const Bliss::Lemma *lemma,
	Score syntaxEmissionScale,
	const History &history, Score &score)
    {
	addLemmaScoreOmitExtension(
	    lm->unscaled(), lm->scale(),
	    lemma, syntaxEmissionScale,
	    history, score);
    }

    /** Language model score convenience function for lemma-pronunciations. */
    inline void addLemmaPronunciationScore(
	Core::Ref<const LanguageModel> lm,
	Score lmScale,
	const Bliss::LemmaPronunciation *pronunciation,
	Score wpScale,
	Score syntaxEmissionScale,
	History &history, Score &score)
    {
	require(pronunciation);
	addLemmaScore(lm, lmScale, pronunciation->lemma(), syntaxEmissionScale, history, score);
	score += wpScale * pronunciation->pronunciationScore();
    }

    /** Language model score convenience function for lemma-pronunciations. */
    inline void addLemmaPronunciationScoreOmitExtension(
	Core::Ref<const ScaledLanguageModel> lm,
	const Bliss::LemmaPronunciation *pronunciation, Score wpScale, Score syntaxEmissionScale,
	const History &history, Score &score)
    {
	require(pronunciation);
	addLemmaScoreOmitExtension(lm, pronunciation->lemma(), syntaxEmissionScale, history, score);
	score += wpScale * pronunciation->pronunciationScore();
    }

    /** Language model score convenience function for lemma-pronunciations. */
    inline void addLemmaPronunciationScore(
	Core::Ref<const ScaledLanguageModel> lm,
	const Bliss::LemmaPronunciation *pronunciation, Score wpScale, Score syntaxEmissionScale,
	History &history, Score &score)
    {
	require(pronunciation);
	addLemmaScore(lm, pronunciation->lemma(), syntaxEmissionScale, history, score);
	score += wpScale * pronunciation->pronunciationScore();
    }

} // namespace Lm

#endif // _LM_SCALED_LANGUAGE_MODEL_HH
