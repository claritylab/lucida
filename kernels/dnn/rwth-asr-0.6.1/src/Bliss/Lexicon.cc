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
// $Id: Lexicon.cc 9621 2014-05-13 17:35:55Z golik $

#include "Fsa.hh"
#include "Lexicon.hh"
#include "LexiconParser.hh"
#include <Core/MD5.hh>
#include <Core/Utility.hh>
#include <Fsa/AlphabetUtility.hh>

using namespace Bliss;
using namespace Core;
using Core::tie;

// ===========================================================================
Lemma::Lemma() :
	Token(),
	pronunciations_(0)
{}

Lemma::~Lemma() {}

void Lemma::setName(Symbol symbol) {
	setSymbol(symbol);
}

u32 Lemma::nPronunciations() const {
	u32 result = 0;
	for (PronunciationIterator pi(pronunciations_); pi != PronunciationIterator(0); ++pi)
		++result;
	return result;
}

bool Lemma::hasPronunciation(const Pronunciation *pron) const {
	for (PronunciationIterator pi(pronunciations_); pi != PronunciationIterator(0); ++pi)
		if (pi->pronunciation() == pron)
			return true;
	return false;
}

// ===========================================================================
Pronunciation::Pronunciation(const Phoneme::Id *_phonemes) :
	lemmas_(0), phonemes_(_phonemes)
{}

Pronunciation::~Pronunciation() {}

u32 Pronunciation::length() const {
	u32 result = 0;
	for (const Phoneme::Id *ph = phonemes_; *ph != Phoneme::term; ++ph)
		++result;
	return result;
}

u32 Pronunciation::Hash::operator()(const Phoneme::Id *phs) const {
	u32 result = 0;
	for (const Phoneme::Id *ph = phs; *ph != Phoneme::term; ++ph)
		result = (result << 5 | result >> 27) ^ u32(*ph);
	return result;
}

u32 Pronunciation::Hash::operator()(const Pronunciation *pr) const {
	return operator()(pr->phonemes_);
}

bool Pronunciation::Equality::operator()(const Phoneme::Id *ll, const Phoneme::Id *rr) const {
	if (ll == rr) return true;
	while (*ll != Phoneme::term)
		if (*ll++ != *rr++) return false;
	return (*ll == *rr);
}

bool Pronunciation::Equality::operator()(const Pronunciation *l, const Pronunciation *r) const {
	return operator()(l->phonemes_, r->phonemes_);
}

u32 Pronunciation::nLemmas() const {
	u32 result = 0;
	for (LemmaIterator pi(lemmas_); pi != LemmaIterator(0); ++pi)
		++result;
	return result;
}

std::string Pronunciation::format(Core::Ref<const PhonemeInventory> phonemeInventory) const {
	require(phonemeInventory);
	std::string result;
	for (const Phoneme::Id *i = phonemes_ ; *i != Phoneme::term ; ++i) {
		if (i != phonemes_) result += utf8::blank;
		result.append(phonemeInventory->phoneme(*i)->symbol());
	}
	return result;
}

// ===========================================================================
ParameterString Lexicon::paramFilename(
	"file",
	"name of lexicon file to load");

struct Lexicon::Internal {
	// FSA adaptors
	Core::WeakRef<const LemmaAlphabet> lemmaAlphabet_;
	Core::WeakRef<const LemmaPronunciationAlphabet> lemmaPronunciationAlphabet_;
	Core::WeakRef<const LetterAlphabet> letterAlphabet_;
	Core::WeakRef<const SyntacticTokenAlphabet> syntacticTokenAlphabet_;
	Core::WeakRef<const EvaluationTokenAlphabet> evaluationTokenAlphabet_;

	// construction helpers
	struct PronunciationSuffix;
	class PronunciationSuffixMap;
};

Lexicon::Lexicon(const Configuration &c) :
	Component(c),
	symbolSequences_(symbols_),
	internal_(0)
{
	internal_ = new Internal;
}

Lexicon::~Lexicon() {
	for (PronunciationList::const_iterator p = pronunciations_.begin() ; p != pronunciations_.end() ; ++p)
		delete *p;
	delete internal_;
}

void Lexicon::load(const std::string &filename) {
	Core::MD5 md5;
	if (md5.updateFromFile(filename))
		dependency_.setValue(md5);
	else
		warning("could not derive md5 sum from file '%s'", filename.c_str());
	LexiconParser parser(config, this);
	log("reading lexicon from file") << " \"" << filename << "\" ...";
	if (parser.parseFile(filename.c_str()) != 0)
		error("Error while reading lexicon file.");
	log("dependency value: ") << dependency_.value();
}

LexiconRef Lexicon::create(const Configuration &c) {
	Lexicon *result = new Lexicon(c);
	result->load(paramFilename(c));
	if (result->hasFatalErrors()) {
		delete result;
		return LexiconRef();
	}
	result->logStatistics();
	return LexiconRef(result);
}

Lemma *Lexicon::newLemma() {
	Lemma *lemma = new Lemma;
	lemmas_.insert(lemma);
	return lemma;
}

Lemma *Lexicon::newLemma(const std::string &name) {
	require(isWhitespaceNormalized(name));
	require(!lemmas_[name]);
	Lemma *lemma = new Lemma;
	lemma->setName(symbols_[name]);
	lemmas_.add(lemma);
	return lemma;
}

void Lexicon::setOrthographicForms(Lemma *lemma, const std::vector<std::string> &orths) {
	require(lemma);
	for (std::vector<std::string>::const_iterator orth = orths.begin(); orth != orths.end(); ++orth) {
		require(isWhitespaceNormalized(*orth));
		const char *cc, *nc;
		for (cc = nc = orth->c_str(); *cc; cc = nc) {
			do ++nc; while (*nc && utf8::byteType(*nc) == utf8::multiByteTail);
			letter(std::string(cc, nc));
		}
	}
	lemma->setOrthographicForms(symbolSequences_.add(orths));
}

void Lexicon::setDefaultLemmaName(Lemma *lemma) {
	require(lemma);
	require(!lemma->hasName());
	require(lemma->nOrthographicForms());

	std::string name = lemma->preferredOrthographicForm();
	for (u32 i = 0; lemmas_[name];)
		name = Core::form(
			"%s [%d]",
			lemma->preferredOrthographicForm().str(),
			++i);

	verify(isWhitespaceNormalized(name));
	lemma->setName(symbols_[name]);
	lemmas_.link(lemma->name(), lemma);
}

const Letter *Lexicon::letter(const std::string &letter) const {
	Token *token = letters_[letter.c_str()];
	if (!token) {
		require(utf8::length(letter.c_str()) == 1);
		token = new Letter((*const_cast<SymbolSet*>(&symbols_))[letter]);
		const_cast<TokenInventory*>(&letters_)->add(token);
	}
	return static_cast<Letter*>(token);
}

void Lexicon::setPhonemeInventory(Core::Ref<const PhonemeInventory> pi) {
	require(!phonemeInventory_); // changing it could invalidate all pronunciaitions
	phonemeInventory_ = pi;
}

Pronunciation *Lexicon::getOrCreatePronunciation(const std::vector<Phoneme::Id> &phonemes) {
	require(phonemes.size() >= 1 && phonemes.back() == Phoneme::term);
	Pronunciation *pron = new Pronunciation(&*(phonemes.begin()));
	PronunciationMap::const_iterator it;
	bool isNew;
	tie(it, isNew) = pronunciationMap_.insert(pron);
	if (isNew) {
		phon_.start();
		for (std::vector<Phoneme::Id>::const_iterator i = phonemes.begin();
			 i != phonemes.end(); ++i) phon_.grow(*i);
		pron->phonemes_ = phon_.currentBegin();
		verify(phon_.currentEnd()[-1] == Phoneme::term);
		phon_.finish();
		pronunciations_.push_back(pron);
	} else {
		delete pron;
		pron = *it;
	}
	return pron;
}

void Lexicon::parsePronunciation(
	const std::string &phonStr,
	std::vector<Phoneme::Id> &phonemes) const
{
	require(phonemeInventory());
	const Phoneme *phoneme;
	std::string::size_type i, j;
	i = phonStr.find_first_not_of(utf8::whitespace);
	while (i != std::string::npos) {
		j = phonStr.find_first_of(utf8::whitespace, i) ;
		if (j == std::string::npos) j = phonStr.length();
		if ((phoneme = phonemeInventory()->phoneme(phonStr.substr(i, j-i)))) {
			phonemes.push_back(phoneme->id()) ;
		} else {
			error("ignoring unknown phoneme \"%s\"",
				  phonStr.substr(i, j-i).c_str());
		}
		i = phonStr.find_first_not_of(utf8::whitespace, j) ;
	}
}

Pronunciation *Lexicon::getPronunciation(const std::string &phon) {
	require(phonemeInventory());
	std::vector<Phoneme::Id> phonemes;
	parsePronunciation(phon, phonemes);
	phonemes.push_back(Phoneme::term);
	Pronunciation *pron = getOrCreatePronunciation(phonemes);
	return pron;
}

void Lexicon::addPronunciation(Lemma *lemma, Pronunciation *pron, f32 weight) {
	require(lemma);
	require(pron);
	require(weight >= 0.0);
	require(!lemma->hasPronunciation(pron));

	LemmaPronunciation *lp = lemmaPronunciations_.add(LemmaPronunciation(lemmaPronunciationsByIndex_.size()));
	lemmaPronunciationsByIndex_.push_back(lp);
	lp->lemma_					= lemma;
	lp->nextForThisLemma_		 = lemma->pronunciations_;
	lemma->pronunciations_		= lp;

	lp->pronunciation_			= pron;
	lp->nextForThisPronunciation_ = pron->lemmas_;
	pron->lemmas_				 = lp;

	lp->score_ = -(::log(weight));
}

void Lexicon::normalizePronunciationWeights(Lemma *lemma) {
	if (lemma->nPronunciations() == 0) return;
	f32 sum = 0.0;
	for (LemmaPronunciation *lp = lemma->pronunciations_; lp; lp = lp->nextForThisLemma_)
		sum += lp->pronunciationProbability();
	if (sum <= 0.0) {
		error("pronunciation probabilities for lemma %s cannot be normalized: sum of weights is %f",
			  lemma->preferredOrthographicForm().str(), sum);
		return;
	}
	for (LemmaPronunciation *lp = lemma->pronunciations_; lp; lp = lp->nextForThisLemma_)
		lp->setPronunciationProbability(lp->pronunciationProbability() / sum);
}

SyntacticToken *Lexicon::getOrCreateSyntacticToken(Symbol synt) {
	Token *token = syntacticTokens_[synt];
	if (!token) {
		token = new SyntacticToken(synt);
		syntacticTokens_.add(token);
	}
	return static_cast<SyntacticToken*>(token);
}

void Lexicon::setSyntacticTokenSequence(Lemma *lemma, const std::vector<std::string> &synt) {
	require(lemma);
	synts_.start();
	for (std::vector<std::string>::const_iterator i = synt.begin(); i != synt.end(); ++i) {
		SyntacticToken *token = getOrCreateSyntacticToken(symbols_[*i]);
		synts_.grow(token);
		token->addLemma(lemma);
	}
	SyntacticTokenSequence tokenSequence(synts_.currentBegin(), synts_.currentEnd());
	synts_.finish();
	lemma->setSyntacticTokenSequence(tokenSequence);
}

void Lexicon::setDefaultSyntacticToken(Lemma *lemma) {
	require(lemma);
	require(lemma->nOrthographicForms());
	SyntacticToken *token = getOrCreateSyntacticToken(lemma->preferredOrthographicForm());
	token->addLemma(lemma);
	const SyntacticToken **tp = synts_.add(token);
	SyntacticTokenSequence tokenSequence(tp, tp+1);
	lemma->setSyntacticTokenSequence(tokenSequence);
}

EvaluationToken *Lexicon::getOrCreateEvaluationToken(Symbol eval) {
	Token *token = evaluationTokens_[eval];
	if (!token) {
		token = new EvaluationToken(eval);
		evaluationTokens_.add(token);
	}
	return static_cast<EvaluationToken*>(token);
}

void Lexicon::addEvaluationTokenSequence(Lemma *lemma, const std::vector<std::string> &eval) {
	require(lemma);
	evals_.start();
	for (std::vector<std::string>::const_iterator i = eval.begin(); i != eval.end(); ++i) {
		EvaluationToken *token = getOrCreateEvaluationToken(symbols_[*i]);
		evals_.grow(token);
	}
	EvaluationTokenSequence tokenSequence(evals_.currentBegin(), evals_.currentEnd());
	evals_.finish();
	lemma->addEvaluationTokenSequence(tokenSequence);
}

void Lexicon::setDefaultEvaluationToken(Lemma *lemma) {
	require(lemma);
	require(lemma->nOrthographicForms());
	require(!lemma->nEvaluationTokenSequences());
	EvaluationToken *token = getOrCreateEvaluationToken(lemma->preferredOrthographicForm());
	const EvaluationToken **tp = evals_.add(token);
	EvaluationTokenSequence tokenSequence(tp, tp+1);
	lemma->addEvaluationTokenSequence(tokenSequence);
}

void Lexicon::defineSpecialLemma(const std::string &name, Lemma *lemma) {
	require(!specialLemma(name));
	require(lemma);
	specialLemmas_[name] = lemma;
}

const Lemma *Lexicon::specialLemma(const std::string &name) const {
	LemmaMap::const_iterator i = specialLemmas_.find(name) ;
	return (i == specialLemmas_.end()) ? 0 : i->second;
}

void Lexicon::writeXml(Core::XmlWriter &os) const {
	os << Core::XmlOpen("lexicon");
	if (phonemeInventory())
		phonemeInventory()->writeXml(os);
	LemmaIterator l, l_end;
	for (tie(l, l_end) = lemmas(); l != l_end; ++l) {
		const Lemma *lemma(*l);

		std::string specialName = "";
		for(LemmaMap::const_iterator i = specialLemmas_.begin();
		i != specialLemmas_.end(); ++i) {
				if(i->second->id() == lemma->id())
						specialName = i->first;
		}
		if(specialName.length() == 0)
				os << Core::XmlOpen("lemma");
		else
				os << Core::XmlOpen("lemma") + Core::XmlAttribute("special", specialName);

		const OrthographicFormList &ol(lemma->orthographicForms());
		for (OrthographicFormList::Iterator o = ol.begin() ; o != ol.end() ; ++o) {
			if (o->length())
				os << Core::XmlFull("orth", *o);
			else
				os << Core::XmlEmpty("orth");
		}

		Lemma::PronunciationIterator p, p_end;
		for (tie(p, p_end) = lemma->pronunciations() ; p != p_end ; ++p) {
			os << Core::XmlFull("phon", p->pronunciation()->format(phonemeInventory()))
				+ Core::XmlAttribute("weight", p->pronunciationProbability());
		}

		const SyntacticTokenSequence &sts(lemma->syntacticTokenSequence());
		if (sts.size()) {
			os << Core::XmlOpen("synt");
			for (SyntacticTokenSequence::Iterator st = sts.begin() ; st != sts.end() ; ++st) {
				os << Core::XmlFull("tok", (*st)->symbol());
			}
			os << Core::XmlClose("synt");
		} else {
			os << Core::XmlEmpty("synt");
		}

		Lemma::EvaluationTokenSequenceIterator e, e_end;
		for (tie(e, e_end) = lemma->evaluationTokenSequences(); e != e_end; ++e) {
			if (e->size()) {
				os << Core::XmlOpen("eval");
				for (EvaluationTokenSequence::Iterator et = e->begin() ; et != e->end() ; ++et) {
					os << Core::XmlFull("tok", (*et)->symbol());
				}
				os << Core::XmlClose("eval");
			} else {
				os << Core::XmlEmpty("eval");
			}
		}

		os << Core::XmlClose("lemma");
	}
	os << Core::XmlClose("lexicon");
}

// ===========================================================================
void Lexicon::logStatistics() const {
	Message msg(log("statistics:"));
    msg << "\nnumber of phonemes:                    " << ((phonemeInventory()) ? phonemeInventory()->nPhonemes() : 0)
	<< "\nnumber of lemmas:                      " << nLemmas()
	<< "\nnumber of lemma pronunciations:        " << nLemmaPronunciations()
	<< "\nnumber of distinct pronunciations:     " << nPronunciations()
	<< "\nnumber of distinct syntactic tokens:   " << nSyntacticTokens()
	<< "\nnumber of distinct evaluation tokens:  " << nEvaluationTokens();

	PronunciationIterator p, p_end;
	u32 nPhonemes = 0;
	for (tie(p, p_end) = pronunciations(); p != p_end; ++p)
		nPhonemes += (*p)->length();
	msg << "\naverage number of phonemes per pronunciation: " << f32(nPhonemes) / nPronunciations();
}

// ===========================================================================
Core::Ref<const LemmaAlphabet> Lexicon::lemmaAlphabet() const {
	Core::Ref<const LemmaAlphabet> result;
	if (internal_->lemmaAlphabet_)
		result = internal_->lemmaAlphabet_;
	else
		internal_->lemmaAlphabet_ = result = Core::ref(new LemmaAlphabet(LexiconRef(this)));
	return result;
}

Core::Ref<const LemmaPronunciationAlphabet> Lexicon::lemmaPronunciationAlphabet() const {
	Core::Ref<const LemmaPronunciationAlphabet> result;
	if (internal_->lemmaPronunciationAlphabet_)
		result = internal_->lemmaPronunciationAlphabet_;
	else
		internal_->lemmaPronunciationAlphabet_ = result = Core::ref(new LemmaPronunciationAlphabet(LexiconRef(this)));
	return result;
}

Core::Ref<const SyntacticTokenAlphabet> Lexicon::syntacticTokenAlphabet() const {
	Core::Ref<const SyntacticTokenAlphabet> result;
	if (internal_->syntacticTokenAlphabet_)
		result = internal_->syntacticTokenAlphabet_;
	else
		internal_->syntacticTokenAlphabet_ = result = Core::ref(new SyntacticTokenAlphabet(ref(this)));
	return result;
}

Core::Ref<const EvaluationTokenAlphabet> Lexicon::evaluationTokenAlphabet() const {
	Core::Ref<const EvaluationTokenAlphabet> result;
	if (internal_->evaluationTokenAlphabet_)
		result = internal_->evaluationTokenAlphabet_;
	else
		internal_->evaluationTokenAlphabet_ = result = Core::ref(new EvaluationTokenAlphabet(ref(this)));
	return result;
}

Core::Ref<const LetterAlphabet> Lexicon::letterAlphabet() const {
	Core::Ref<const LetterAlphabet> result;
	if (internal_->letterAlphabet_)
		result = internal_->letterAlphabet_;
	else
		internal_->letterAlphabet_ = result = Core::ref(new LetterAlphabet(ref(this)));
	return result;
}

/**
 * lemma-to-orthography transducer properties:
 * - the lemma label is the first input symbol
 * - output labels are (Unicode) letters
 * If @c shallIncludeVariants is true, all orthographic forms are
 * included.  Otherwise only the preferred (first) orthographic form
 * of each lemma is included.
 */
Core::Ref<Fsa::Automaton> Lexicon::createLemmaToOrthographyTransducer(
	bool shallIncludeVariants, bool onlyWithEvalToken) const
{
	Fsa::StaticAutomaton *result = new Fsa::StaticAutomaton;
	result->setType(Fsa::TypeTransducer);
	result->setSemiring(Fsa::TropicalSemiring);
	result->setInputAlphabet(lemmaAlphabet());
	result->setOutputAlphabet(letterAlphabet());

	Fsa::State *initial = result->newState();
	result->setInitialStateId(initial->id());
	result->setStateFinal(initial);
	Fsa::Weight one = Fsa::Weight(result->semiring()->one());
	for (LemmaIterator lemma = lemmas().first; lemma != lemmas().second; ++lemma) {
		const OrthographicFormList &forms((*lemma)->orthographicForms());
		OrthographicFormList::Iterator orth = forms.begin();
		OrthographicFormList::Iterator orth_end = (shallIncludeVariants) ? forms.end() : (forms.begin() + 1);
		if( onlyWithEvalToken &&
		    ((*lemma)->evaluationTokenSequences().first == (*lemma)->evaluationTokenSequences().second ||
			((*lemma)->evaluationTokenSequences().second == (*lemma)->evaluationTokenSequences().first+1 && (*lemma)->evaluationTokenSequences().first->isEpsilon())) )
		{
		    initial->newArc(initial->id(), one, (*lemma)->id(), Fsa::Epsilon);
		}else{
		for (; orth != orth_end; ++orth) {
			if (orth->length() > 0) {
				Fsa::State *from = initial;
				const char *cc, *nc;
				for (cc = nc = orth->str(); *cc; cc = nc) {
					do ++nc; while (*nc && utf8::byteType(*nc) == utf8::multiByteTail);
					Fsa::State *to  = (*nc) ? result->newState() : initial;
					Fsa::LabelId in = (from == initial) ? (*lemma)->id() : Fsa::Epsilon;
					Fsa::LabelId out = letter(std::string(cc, nc))->id();
					from->newArc(to->id(), one, in, out);
					from = to;
				}
			} else {
				initial->newArc(initial->id(), one, (*lemma)->id(), Fsa::Epsilon);
			    }
			}
		}
	}
	result->normalize();
	result->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, 0);
	return Core::ref(result);
}

// ---------------------------------------------------------------------------
struct Lexicon::Internal::PronunciationSuffix {
	const Phoneme::Id *phonemes;
	u32 homophoneIndex;

	struct Hash : Pronunciation::Hash {
		size_t operator() (const PronunciationSuffix &ps) const {
			return (size_t(Pronunciation::Hash::operator()(ps.phonemes)) << 2)
				^ size_t(ps.homophoneIndex);
		}
	};
	struct Equality : Pronunciation::Equality {
		bool operator() (const PronunciationSuffix &ll, const PronunciationSuffix &rr) const {
			return Pronunciation::Equality::operator()(ll.phonemes, rr.phonemes)
				&& (ll.homophoneIndex == rr.homophoneIndex);
		}
	};
};

class Lexicon::Internal:: PronunciationSuffixMap :
	public Core::hash_map< PronunciationSuffix,
			   Fsa::StateId,
			   PronunciationSuffix::Hash,
			   PronunciationSuffix::Equality>
{
public:
	Fsa::StateId lookup(const PronunciationSuffix &ps) {
		const_iterator ii = find(ps);
		if (ii == end())
			return Fsa::InvalidStateId;
		return ii->second;
	}
	void add(const PronunciationSuffix &ps, Fsa::StateId si) {
		insert(std::make_pair(ps, si));
	}
};

Core::Ref<PhonemeToLemmaPronunciationTransducer>
Lexicon::createPhonemeToLemmaPronunciationTransducer(
	bool shallDisambiguate) const
{
		return createPhonemeToLemmaPronunciationTransducer(phonemeInventory()->phonemeAlphabet()->nDisambiguators(), shallDisambiguate, false);
}


Core::Ref<PhonemeToLemmaPronunciationTransducer>
Lexicon::createPhonemeToLemmaPronunciationTransducer(
				size_t nDisambiguators, bool shallDisambiguate, bool markInitialPhonesAndSilence) const
{
	Core::Ref<const PhonemeAlphabet> pi = phonemeInventory()->phonemeAlphabet();
	Core::Ref<const LemmaPronunciationAlphabet> li = lemmaPronunciationAlphabet();

	Core::Ref<PhonemeToLemmaPronunciationTransducer> result = Core::ref(new PhonemeToLemmaPronunciationTransducer);
	result->setType(Fsa::TypeTransducer);
	result->setSemiring(Fsa::TropicalSemiring);
	result->setInputAlphabet(pi);
	result->setOutputAlphabet(li);

	Fsa::State *initial = result->newState();
	result->setInitialStateId(initial->id());
	result->setStateFinal(initial);
	Fsa::Weight weight = result->semiring()->one();
	Internal::PronunciationSuffixMap suffixState;
	Internal::PronunciationSuffix suffix;
	for (PronunciationIterator pron = pronunciations().first; pron != pronunciations().second; ++pron) {
		suffix.homophoneIndex = 0;
		for (Pronunciation::LemmaIterator l = (*pron)->lemmas().first; l != (*pron)->lemmas().second; ++l) {
			const LemmaPronunciation *lp = l;
			Fsa::LabelId out = lp->id();
			if ((*pron)->phonemes()[0] != Phoneme::term) {
				Fsa::State *from = initial;
				Fsa::State *next = from;
				for (suffix.phonemes = (*pron)->phonemes(); next && *suffix.phonemes != Phoneme::term;) {
					Fsa::LabelId in = *suffix.phonemes++;
					Fsa::StateId to = suffixState.lookup(suffix);
					if (to == Fsa::InvalidStateId) {
						next = result->newState();
						if (*suffix.phonemes == Phoneme::term) {
							next->newArc(initial->id(), weight, pi->disambiguator(suffix.homophoneIndex), Fsa::Epsilon);
						}
						to = next->id();
						suffixState.add(suffix, to);
					} else {
						next = 0;
					}
					from->newArc(to, weight, in, out);
					from = next;
					out = Fsa::Epsilon;
				}
			} else {
				warning("empty pronunciation of lemma '%s' added to transducer (may not be determinizable?)",
						l->lemma()->name().str());
				initial->newArc(initial->id(), weight, pi->disambiguator(suffix.homophoneIndex), out);
			}
			if (shallDisambiguate) suffix.homophoneIndex += 1;
		}
	}
    Fsa::LabelId nLexiconDisambiguators = pi->nDisambiguators();
	for (size_t d = 0; d < nDisambiguators; ++d)
		initial->newArc(initial->id(), weight, pi->disambiguator(nLexiconDisambiguators + d), li->disambiguator(d));
	if (markInitialPhonesAndSilence) {
		Fsa::LabelId initialPhoneAndSilenceOffset = Fsa::count(pi).maxLabelId_ + 1;
		for (Fsa::State::iterator a = initial->begin(); a != initial->end(); ++a)
			if ((a->input_ >= Fsa::FirstLabelId) && !pi->isDisambiguator(a->input_))
				a->input_ += initialPhoneAndSilenceOffset;
	}
	result->normalize();
	result->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, 0);
	return result;
}



/**
 * - disambiguator(1) between empty-pronunciation and corresponding non-empy-pronuncaition
 * - disambiguator(0) else
 **/
Core::Ref<PhonemeToLemmaPronunciationTransducer>
Lexicon::createPhonemeToLemmaPronunciationAndStickPunctuationTransducer() const
{
	Core::Ref<const PhonemeAlphabet> pi = phonemeInventory()->phonemeAlphabet();
	Core::Ref<const LemmaPronunciationAlphabet> li = lemmaPronunciationAlphabet();

	Core::Ref<PhonemeToLemmaPronunciationTransducer> result = Core::ref(new PhonemeToLemmaPronunciationTransducer);
	result->setType(Fsa::TypeTransducer);
	result->setSemiring(Fsa::TropicalSemiring);
	result->setInputAlphabet(pi);
	result->setOutputAlphabet(li);

	Fsa::State *initial = result->newState();
	result->setInitialStateId(initial->id());
	Fsa::Weight weight = result->semiring()->one();
	Internal::PronunciationSuffixMap suffixState;
	Internal::PronunciationSuffix suffix;
	Fsa::LabelId disambig0Id = pi->disambiguator(0);
	Fsa::LabelId disambig1Id = pi->disambiguator(1);

	Fsa::State *prePunctuation = result->newState();
	Fsa::State *postPunctuation = result->newState();

	Fsa::State *loop = result->newState();
	result->setStateFinal(loop);
	initial->newArc(loop->id(), weight, Fsa::Epsilon, Fsa::Epsilon);
	initial->newArc(prePunctuation->id(), weight, Fsa::Epsilon, Fsa::Epsilon);


	for (PronunciationIterator pron = pronunciations().first; pron != pronunciations().second; ++pron) {
		suffix.homophoneIndex = 0;
		for (Pronunciation::LemmaIterator l = (*pron)->lemmas().first; l != (*pron)->lemmas().second; ++l) {
			const LemmaPronunciation *lp = l;
			Fsa::LabelId out = lp->id();
			if ((*pron)->phonemes()[0] != Phoneme::term) {
				Fsa::State *from = loop;
				Fsa::State *next = from;
				for (suffix.phonemes = (*pron)->phonemes(); next && *suffix.phonemes != Phoneme::term;) {
					Fsa::LabelId in = *suffix.phonemes++;
					Fsa::StateId to = suffixState.lookup(suffix);
					if (to == Fsa::InvalidStateId) {
						next = result->newState();
						if (*suffix.phonemes == Phoneme::term) {
							next->newArc(loop->id(), weight, disambig0Id, Fsa::Epsilon);
							next->newArc(postPunctuation->id(), weight, disambig1Id, Fsa::Epsilon);
						}
						to = next->id();
						suffixState.add(suffix, to);
					} else {
						next = 0;
					}
					from->newArc(to, weight, in, out);
					from = next;
					out = Fsa::Epsilon;
				}
			} else {
				log("Lemma \"%s\" has empty pronunciation; assume punctuation mark",
					l->lemma()->name().str());
				postPunctuation->newArc(loop->id(), weight, disambig0Id, out);
				postPunctuation->newArc(prePunctuation->id(), weight, disambig0Id, out);
				prePunctuation->newArc(loop->id(), weight, disambig1Id, out);
			}
		}
	}
	result->normalize();
	result->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, 0);
	return result;
}

// ---------------------------------------------------------------------------
Core::Ref<LemmaPronunciationToLemmaTransducer> Lexicon::createLemmaPronunciationToLemmaTransducer(u32 nDisambiguators) const {
	Core::Ref<const LemmaPronunciationAlphabet> lpa = lemmaPronunciationAlphabet();
	Core::Ref<const LemmaAlphabet> li = lemmaAlphabet();
	LemmaPronunciationToLemmaTransducer *result = new LemmaPronunciationToLemmaTransducer;
	result->setType(Fsa::TypeTransducer);
	result->setSemiring(Fsa::TropicalSemiring);
	result->setInputAlphabet(lpa);
	result->setOutputAlphabet(lemmaAlphabet());

	Fsa::State *state = result->newState();
	result->setInitialStateId(state->id());
	result->setStateFinal(state);
	for (LemmaPronunciationList::const_iterator lp = lemmaPronunciationsByIndex_.begin(); lp != lemmaPronunciationsByIndex_.end(); ++lp) {
		state->newArc(state->id(), Fsa::Weight((*lp)->pronunciationScore()),
					  lpa->index(*lp), (*lp)->lemma()->id());
	}
	for (u32 l = 0; l < nDisambiguators; ++l)
			state->newArc(state->id(), result->semiring()->one(), lpa->disambiguator(l), li->disambiguator(l));
	result->normalize();
	result->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, 0);
	return Core::ref(result);
}


// ---------------------------------------------------------------------------
Core::Ref<LemmaToSyntacticTokenTransducer> Lexicon::createLemmaToSyntacticTokenTransducer(
	bool useEmptySyntacticTokenSequences, size_t nDisambiguators) const
{
	Core::Ref<const LemmaAlphabet> li = lemmaAlphabet();
	Core::Ref<const SyntacticTokenAlphabet> si = syntacticTokenAlphabet();

	LemmaToSyntacticTokenTransducer *result = new LemmaToSyntacticTokenTransducer;
	result->setType(Fsa::TypeTransducer);
	result->setSemiring(Fsa::TropicalSemiring);
	result->setInputAlphabet(li);
	result->setOutputAlphabet(si);

	Fsa::State *initial = result->newState();
	result->setInitialStateId(initial->id());
	result->setStateFinal(initial);
	for (LemmaIterator l = lemmas().first; l != lemmas().second; ++l) {
		const SyntacticTokenSequence &tokens = (*l)->syntacticTokenSequence();
		if (useEmptySyntacticTokenSequences && tokens.isEpsilon()) {
			initial->newArc(initial->id(), result->semiring()->one(), li->index(*l), Fsa::Epsilon);
		} else {
			Fsa::State *from = initial;
			for (u32 i = 0; i < tokens.length(); ++i) {
				Fsa::State *to  = (i < tokens.length() - 1) ? result->newState() : initial;
				Fsa::LabelId in = (i == 0) ? li->index(*l) : Fsa::Epsilon;
				from->newArc(to->id(),
							 Fsa::Weight(tokens[i]->classEmissionScore()),
							 in, si->index(tokens[i]));
				from = to;
			}
		}
	}
	for (size_t l = 0; l < nDisambiguators; ++l) {
		initial->newArc(initial->id(), result->semiring()->one(), li->disambiguator(l), si->disambiguator(l));
	}
	result->normalize();
	result->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, 0);
	return Core::ref(result);
}

Core::Ref<LemmaToEvaluationTokenTransducer> Lexicon::createLemmaToEvaluationTokenTransducer(bool allowAlternatives) const {
	Core::Ref<const LemmaAlphabet> li = lemmaAlphabet();
	Core::Ref<const EvaluationTokenAlphabet> ei = evaluationTokenAlphabet();

	LemmaToEvaluationTokenTransducer *result = new LemmaToEvaluationTokenTransducer;
	result->setType(Fsa::TypeTransducer);
	result->setSemiring(Fsa::TropicalSemiring);
	result->setInputAlphabet(li);
	result->setOutputAlphabet(ei);

	XmlWriter o(std::cout);
	Fsa::State *initial = result->newState();
	result->setInitialStateId(initial->id());
	result->setStateFinal(initial);
	for (LemmaIterator l = lemmas().first; l != lemmas().second; ++l) {
		Lemma::EvaluationTokenSequenceIterator e, e_end;
		for (tie(e, e_end) = (*l)->evaluationTokenSequences(); e != e_end; ++e) {
			if (e->isEpsilon()) {
				initial->newArc(initial->id(), result->semiring()->one(), li->index(*l), Fsa::Epsilon);
			} else {
				Fsa::State *from = initial;
				for (u32 i = 0; i < e->length(); ++i) {
					Fsa::State *to = (i < e->length() - 1) ? result->newState() : initial;
					from->newArc(
						to->id(),
						result->semiring()->one(),
						(i == 0) ? li->index(*l) : Fsa::Epsilon,
						ei->index((*e)[i]));
					from = to;
				}
			}
			if (!allowAlternatives) break;
		}
	}
	result->normalize();
	result->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, 0);
	return Core::ref(result);
}

Core::Ref<LemmaToEvaluationTokenTransducer> Lexicon::createLemmaToPreferredEvaluationTokenSequenceTransducer() const {
	return createLemmaToEvaluationTokenTransducer(false);
}
