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
// $Id: Fsa.hh 8249 2011-05-06 11:57:02Z rybach $

#ifndef _BLISS_FSA_HH
#define _BLISS_FSA_HH

#include <sstream>
#include <Bliss/Lexicon.hh>
#include <Fsa/Alphabet.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/Static.hh>
#include <Fsa/Types.hh>

namespace Bliss {


	class TokenAlphabet : public Fsa::Alphabet {
	protected:
		LexiconRef lexicon_;
	private:
		const TokenInventory &tokens_;
		mutable u32 nDisambiguators_;
	protected:
		friend class Lexicon;
		TokenAlphabet(const TokenInventory&);
		TokenAlphabet(LexiconRef, const TokenInventory&);
	public:
		virtual ~TokenAlphabet();
		virtual Fsa::LabelId index(const std::string&) const;
		virtual std::string symbol(Fsa::LabelId) const;
		virtual const_iterator end() const;
		virtual void writeXml(Core::XmlWriter &os) const;
		virtual void describe(Core::XmlWriter &os) const;

		Fsa::LabelId index(const Token *tt) const {
			return (tt) ? Fsa::LabelId(tt->id()) : Fsa::Epsilon;
		}
		const Token* token(Fsa::LabelId ii) const {
			require_(ii == Fsa::Epsilon || (ii >= 0 && u32(ii) < tokens_.size()));
			return (ii != Fsa::Epsilon) ? tokens_[ii] : 0;
		}

		u32 nDisambiguators() const;
		Fsa::LabelId disambiguator(u32) const;
		virtual bool isDisambiguator(Fsa::LabelId) const;
	};

	/**
	 * PhonemeAlphabet as FSA alphabet.
	 *
	 * In addition to the phoneme in the PhonemeAlphabet, the
	 * PhonemeAlphabet virtually contains a variable number
	 * disambiguation symbols.  These are needed to create
	 * determinizable lexicon transducers, even when homophones are
	 * present.
	 */
	class PhonemeAlphabet : public TokenAlphabet {
	private:
		friend class PhonemeInventory;
		Core::Ref<const PhonemeInventory> pi_;
		PhonemeAlphabet(Core::Ref<const PhonemeInventory>);
	public:
		Core::Ref<const PhonemeInventory> phonemeInventory() const { return pi_; }

		const Phoneme *phoneme(Fsa::LabelId id) const {
			if (id == Fsa::Epsilon) return 0;
			return pi_->phoneme(id);
		}
		virtual Fsa::LabelId index(const std::string&) const;
		virtual std::string symbol(Fsa::LabelId) const;
		virtual const_iterator begin() const;
		virtual const_iterator end() const;
		virtual void writeXml(Core::XmlWriter &os) const;

		Fsa::LabelId index(const Token *tt) const { return TokenAlphabet::index(tt); }
	};

	/** Adaptor: Lexicon as FSA alphabet of lemmata. */
	class LemmaAlphabet : public TokenAlphabet {
	private:
		friend class Lexicon;
		LemmaAlphabet(LexiconRef);
	public:
		const Lemma *lemma(Fsa::LabelId id) const {
			return static_cast<const Lemma*>(token(id));
		}
		virtual void describe(Core::XmlWriter&) const;
	};

	/** Adaptor: Lexicon as FSA alphabet of lemma-pronunciations. */
	class LemmaPronunciationAlphabet : public Fsa::Alphabet {
	private:
		LexiconRef lexicon_;
		friend class Lexicon;
		mutable u32 nDisambiguators_;
		LemmaPronunciationAlphabet(LexiconRef l) : lexicon_(l), nDisambiguators_(0) {}
	public:
		Fsa::LabelId index(const LemmaPronunciation *l) const {
			return (l) ? Fsa::LabelId(l->id()) : Fsa::Epsilon;
		}
		const LemmaPronunciation* lemmaPronunciation(Fsa::LabelId id) const {
			if (id == Fsa::Epsilon) return 0;
			require_(id >= 0 && size_t(id) < lexicon_->lemmaPronunciationsByIndex_.size());
			return lexicon_->lemmaPronunciationsByIndex_[id];
		}

		virtual const_iterator end() const {
			return const_iterator(Fsa::ConstAlphabetRef(this),
					lexicon_->lemmaPronunciationsByIndex_.size() + nDisambiguators_);
		}
		virtual Fsa::LabelId index(const std::string&) const;
		virtual std::string symbol(Fsa::LabelId) const;
		virtual void writeXml(Core::XmlWriter &os) const;

		u32 nDisambiguators() const;
		Fsa::LabelId disambiguator(u32) const;
		virtual bool isDisambiguator(Fsa::LabelId) const;

	};

	/** Adaptor: Lexicon as FSA alphabet of syntactic tokens */
	class SyntacticTokenAlphabet : public TokenAlphabet {
	private:
		friend class Lexicon;
		SyntacticTokenAlphabet(LexiconRef);
	public:
		const SyntacticToken* syntacticToken(Fsa::LabelId ii) const {
			return static_cast<const SyntacticToken*>(token(ii));
		}
		virtual void describe(Core::XmlWriter &os) const;
	};

	/** Adaptor: Lexicon as FSA alphabet of evaluation tokens */
	class EvaluationTokenAlphabet : public TokenAlphabet {
	private:
		LexiconRef lexicon_;
		friend class Lexicon;
		EvaluationTokenAlphabet(LexiconRef);
	public:
		const EvaluationToken* evaluationToken(Fsa::LabelId ii) const {
			return static_cast<const EvaluationToken*>(token(ii));
		}
		virtual void describe(Core::XmlWriter &os) const;
	};

	/** Alphabet of single letters/characters. */
	class LetterAlphabet : public TokenAlphabet {
	private:
		friend class Lexicon;
		LetterAlphabet(LexiconRef);
	public:
		const Letter* letter(Fsa::LabelId ii) const {
			return static_cast<const Letter*>(token(ii));
		}
		virtual void describe(Core::XmlWriter &os) const;
	};

	// ===========================================================================
	class LemmaAcceptor : public Fsa::StaticAutomaton {
	public:
		LemmaAcceptor(LexiconRef lexicon) {
			setType(Fsa::TypeAcceptor);
			setSemiring(Fsa::TropicalSemiring);
			if (lexicon) setInputAlphabet(lexicon->lemmaAlphabet());
		}
		const LemmaAlphabet* lemmaAlphabet() const {
			return dynamic_cast<const LemmaAlphabet*>(getInputAlphabet().get());
		}
	};

	class LemmaPronunciationAcceptor : public Fsa::StaticAutomaton {
	public:
		LemmaPronunciationAcceptor(LexiconRef lexicon) {
			setType(Fsa::TypeAcceptor);
			setSemiring(Fsa::TropicalSemiring);
			if (lexicon) setInputAlphabet(lexicon->lemmaPronunciationAlphabet());
		}
		const LemmaPronunciationAlphabet* lemmaPronunciationAlphabet() const {
			return dynamic_cast<const LemmaPronunciationAlphabet*>(getInputAlphabet().get());
		}
	};

	class PhonemeToLemmaPronunciationTransducer : public Fsa::StaticAutomaton {
	public:
		const PhonemeAlphabet* phonemeAlphabet() const {
			return dynamic_cast<const PhonemeAlphabet*>(getInputAlphabet().get());
		}
		const LemmaPronunciationAlphabet* lemmaPronunciationAlphabet() const {
			return dynamic_cast<const LemmaPronunciationAlphabet*>(getOutputAlphabet().get());
		}
	};

	class LemmaPronunciationToLemmaTransducer : public Fsa::StaticAutomaton {
	public:
		const LemmaAlphabet* lemmaAlphabet() const {
			return dynamic_cast<const LemmaAlphabet*>(getOutputAlphabet().get());
		}
	};

	class LemmaToSyntacticTokenTransducer : public Fsa::StaticAutomaton {
	public:
		const LemmaAlphabet* lemmaAlphabet() const {
			return dynamic_cast<const LemmaAlphabet*>(getInputAlphabet().get());
		}
		const SyntacticTokenAlphabet* syntacticTokenAlphabet() const {
			return dynamic_cast<const SyntacticTokenAlphabet*>(getOutputAlphabet().get());
		}
	};

	class LemmaToEvaluationTokenTransducer : public Fsa::StaticAutomaton {
	public:
		const LemmaAlphabet* lemmaAlphabet() const {
			return dynamic_cast<const LemmaAlphabet*>(getInputAlphabet().get());
		}
		const EvaluationTokenAlphabet* evaluationTokenAlphabet() const {
			return dynamic_cast<const EvaluationTokenAlphabet*>(getOutputAlphabet().get());
		}
	};

} // namespace Bliss

#endif //_BLISS_FSA_HH
