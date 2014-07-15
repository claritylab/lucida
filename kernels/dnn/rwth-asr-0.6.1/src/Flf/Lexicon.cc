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
#include <Core/Application.hh>
#include <Bliss/Lexicon.hh>
#include <Bliss/Symbol.hh>
#include <Core/Choice.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Traverse.hh"
#include "Lexicon.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    class UnknownAlphabet : public Fsa::Alphabet {
    public:
	virtual std::string symbol(Fsa::LabelId index) const {
	    switch (index) {
	    case Fsa::Epsilon:
		return "*EPS*";
	    case Fsa::Any:
		return "*ANY*";
	    case Fsa::Failure:
		return "*FAIL*";
	    case Fsa::Else:
		return "*ELSE*";
	    default:
		return Core::form("%d", index);
	    }
	}
	virtual Fsa::LabelId index(const std::string &symbol) const {
	    Fsa::LabelId id;
	    if (!Core::strconv(symbol, id)) {
		if (symbol == "*EPS*")
		    return Fsa::Epsilon;
		else if (symbol == "*ANY*")
		    return Fsa::Any;
		else if (symbol == "*FAIL*")
		    return Fsa::Failure;
		else if (symbol == "*ELSE*")
		    return Fsa::Else;
		else
		    Core::Application::us()->error(
			"Failed to convert \"%s\"; it is not a valid integer", symbol.c_str());
		return Fsa::InvalidLabelId;
	    }
	    return id;
	}
	virtual const_iterator begin() const {
	    return const_iterator(Core::Ref<const Fsa::Alphabet>(this), 0);
	}
	virtual const_iterator end() const {
	    return const_iterator(Core::Ref<const Fsa::Alphabet>(this), 0);
	}
    };

    typedef Bliss::Lemma Lemma;
    typedef Bliss::LemmaPronunciation LemmaPronunciation;
    typedef Bliss::Lemma::LemmaPronunciationRange LemmaPronunciationRange;

    LexiconRef Lexicon::us_ = LexiconRef();

    const Core::ParameterString Lexicon::paramFile(
	"file",
	"file",
	"");

    const Core::ParameterBool Lexicon::paramNormalizePronunciationWeights(
	"normalize-pronunciation",
	"normalize pronunciation weights/scores",
	true);

    const Core::ParameterBool Lexicon::paramReadOnly(
	"read-only",
	"do not dynamically add unknown tokens",
	false);

    Lexicon::Lexicon(const Core::Configuration &config) :
	Precursor(config) {
	verify(!us_);
	us_ = LexiconRef(this);
	us_->acquireReference();
	if (!paramFile(config).empty()) {
	    load(paramFile(config));
	    for (LemmaIterator l = lemmas().first; l != lemmas().second; ++l) {
		const Lemma *lemma = *l;
		for (Bliss::OrthographicFormList::Iterator o = lemma->orthographicForms().begin(),
			 o_end = lemma->orthographicForms().end(); o != o_end; ++o){
		    if (!orthographyMap_.insert(std::make_pair(std::string(*o), const_cast<Lemma*>(lemma))).second)
			warning("Orthography \"%s\" occurs twice; a lookup for the orthography will map to lemma \"%s\".",
				o->str(), lemma->symbol().str());
		}
	    }
	    // hash orthographies
	} else
	    us_->setPhonemeInventory(Core::Ref<Bliss::PhonemeInventory>(new Bliss::PhonemeInventory));
	unknownAlphabet_ = Fsa::ConstAlphabetRef(new UnknownAlphabet);
	phonemeInventory_ = const_cast<Bliss::PhonemeInventory*>(phonemeInventory().get());
	normalizePronunciationWeights_ = paramNormalizePronunciationWeights(config);
	isReadOnly_ = paramReadOnly(config);
	if (!isReadOnly_)
	    insertionChannel_ = new Core::XmlChannel(config, "insertion");
	else
	    insertionChannel_ = 0;
	nLemmaUpdates_ = 0;
	emptyPron_ = Precursor::getOrCreatePronunciation(
	    std::vector<Bliss::Phoneme::Id>(1, Bliss::Phoneme::term));
	initSpecialLemmas();
    }

    Lexicon::~Lexicon() {
	delete insertionChannel_;
    }

    void Lexicon::initSpecialLemmas() {
	/*
	  silence lemma (pronunciation)
	*/
	{
	    const Lemma *siLemma = Precursor::specialLemma("silence");
	    if (!siLemma) siLemma = Precursor::specialLemma("sil");
	    if (siLemma) {
		// sil lemma
		siLemmaId_ = siLemma->id();
		// sil lemma pronunciation
		LemmaPronunciationRange unkPronRange = siLemma->pronunciations();
		if (unkPronRange.first != unkPronRange.second)
		    siLemmaPronunciationId_ = unkPronRange.first->id();
		else
		    siLemmaPronunciationId_ = Fsa::InvalidLabelId;
	    } else
		siLemmaId_ = siLemmaPronunciationId_ = Fsa::InvalidLabelId;
	}
	/*
	  unknown lemma (pronunciation)
	*/
	{
	    const Lemma *unkLemma = Precursor::specialLemma("unknown");
	    if (!unkLemma) unkLemma = Precursor::specialLemma("unk");
	    if (unkLemma) {
		// unk lemma
		unkLemmaId_ = unkLemma->id();
		// unk lemma pronunciation
		LemmaPronunciationRange unkPronRange = unkLemma->pronunciations();
		if (unkPronRange.first != unkPronRange.second)
		    unkLemmaPronunciationId_ = unkPronRange.first->id();
		else
		    unkLemmaPronunciationId_ = Fsa::InvalidLabelId;
	    } else
		unkLemmaId_ = unkLemmaPronunciationId_ = Fsa::InvalidLabelId;
	}
	/*
	  sentence end lemma (pronunciation)
	*/
	{
	    const Lemma *sentenceEndLemma = Precursor::specialLemma("sentence-end");
	    if (!sentenceEndLemma) sentenceEndLemma = Precursor::specialLemma("sentence-boundary");
	    if (sentenceEndLemma) {
		// sentence end lemma
		sentenceEndLemmaId_ = sentenceEndLemma->id();
		// unk lemma pronunciation
		LemmaPronunciationRange sentenceEndPronRange = sentenceEndLemma->pronunciations();
		if (sentenceEndPronRange.first != sentenceEndPronRange.second)
		    sentenceEndLemmaPronunciationId_ = sentenceEndPronRange.first->id();
		else
		    sentenceEndLemmaPronunciationId_ = Fsa::InvalidLabelId;
	    } else
		sentenceEndLemmaId_ = sentenceEndLemmaPronunciationId_ = Fsa::InvalidLabelId;
	}
    }

    Lexicon::ConstLemmaPtrList Lexicon::nonWordLemmas() {
	ConstLemmaPtrList _nonWordLemmas;
	for (std::pair<LemmaIterator, LemmaIterator> lRange = lemmas(); lRange.first != lRange.second; ++lRange.first) {
	    const Bliss::Lemma *lemma = *lRange.first;
	    std::pair<Bliss::Lemma::EvaluationTokenSequenceIterator, Bliss::Lemma::EvaluationTokenSequenceIterator> eRange = lemma->evaluationTokenSequences();
	    if (eRange.first != eRange.second) {
		for (; eRange.first != eRange.second; ++eRange.first)
		    if (eRange.first->isEpsilon()) {
			_nonWordLemmas.push_back(lemma);
			break;
		    }
	    } else
		_nonWordLemmas.push_back(lemma);
	}
	return _nonWordLemmas;
    }

    Lexicon::ConstLemmaPronunciationPtrList Lexicon::nonWordLemmaPronunciations() {
	ConstLemmaPtrList _nonWordLemmas = nonWordLemmas();
	ConstLemmaPronunciationPtrList _nonWordLemmaProns;
	for (ConstLemmaPtrList::const_iterator itNonWordLemma = _nonWordLemmas.begin(); itNonWordLemma != _nonWordLemmas.end(); ++itNonWordLemma)
	    for (Bliss::Lemma::LemmaPronunciationRange lpRange = (*itNonWordLemma)->pronunciations(); lpRange.first != lpRange.second; ++lpRange.first)
		_nonWordLemmaProns.push_back(lpRange.first);
	return _nonWordLemmaProns;
    }

    LabelIdList Lexicon::nonWordLemmaIds() {
	ConstLemmaPtrList _nonWordLemmas = nonWordLemmas();
	LabelIdList _nonWordLemmaIds(_nonWordLemmas.size());
	LabelIdList::iterator itNonWordLemmaId = _nonWordLemmaIds.begin();
	for (ConstLemmaPtrList::const_iterator itNonWordLemma = _nonWordLemmas.begin(); itNonWordLemma != _nonWordLemmas.end();
	     ++itNonWordLemma, ++itNonWordLemmaId) *itNonWordLemmaId = (*itNonWordLemma)->id();
	return _nonWordLemmaIds;
    }

    Fsa::LabelId Lexicon::unknownId(const std::string &id) {
	return unknownAlphabet_->index(id);
    }

    Fsa::LabelId Lexicon::phonemeId(const std::string &id) {
	Fsa::LabelId label = phonemeInventory_->phonemeAlphabet()->index(id);
	if ((label != Fsa::InvalidLabelId) || isReadOnly_)
	    return label;
	Bliss::Phoneme *phoneme = phonemeInventory_->newPhoneme();
	phonemeInventory_->assignSymbol(phoneme, id);
	if (insertionChannel_->isOpen())
	    *insertionChannel_ << (Core::XmlFull("new-phoneme", phoneme->symbol().str())
				  + Core::XmlAttribute("id", phoneme->id()));
	return phoneme->id();
    }

    std::pair<Lemma*, bool> Lexicon::getLemma(const std::string &orth) {
	LemmaMap::const_iterator it = orthographyMap_.find(orth);
	if (it != orthographyMap_.end())
	    return std::make_pair(it->second, true);
	if (isReadOnly_)
	    return std::make_pair((Lemma*)0, true);
	Lemma *_lemma = Precursor::newLemma(orth);
	Precursor::setOrthographicForms(_lemma, std::vector<std::string>(1, orth));
	orthographyMap_.insert(std::make_pair(orth, _lemma));
	return std::make_pair(_lemma, false);
    }

    void Lexicon::updateLemma(Lemma *_lemma) {
	require(_lemma);
	if (!_lemma->hasName())
	    Precursor::setDefaultLemmaName(_lemma);
	if (!_lemma->hasSyntacticTokenSequence())
	    Precursor::setDefaultSyntacticToken(_lemma);
	if (!_lemma->hasEvaluationTokenSequence())
	    Precursor::setDefaultEvaluationToken(_lemma);
	if(normalizePronunciationWeights_)
	    Precursor::normalizePronunciationWeights(_lemma);
	phonemeToLemmaPronunciationTransducer_.reset();
	lemmaPronunciationToLemmaTransducer_.reset();
	lemmaToSyntacticTokenTransducer_.reset();
	lemmaToEvaluationTokenTransducer_.reset();
	lemmaToPreferredEvaluationTokenSequenceTransducer_.reset();
	++nLemmaUpdates_;
    }

    Fsa::LabelId Lexicon::lemmaId(const std::string &id) {
	Fsa::LabelId label = lemmaAlphabet()->specialIndex(id);
	//Fsa::LabelId label = lemmaAlphabet()->index(id);
	if (label != Fsa::InvalidLabelId)
	    return label;
	std::pair<Lemma*, bool> lb = getLemma(id);
	if (!lb.first) {
	    // dbg
	    error("lemma not found in lexicon: '%s'\n"
		  "possible reason: '%s' is unknown and lexicon option 'read-only' is 'true'",
		  id.c_str(), id.c_str());
	    defect();
	    return Fsa::InvalidLabelId;
	}
	if (!lb.second) {
	    updateLemma(lb.first);
	    if (insertionChannel_->isOpen())
		*insertionChannel_ << (Core::XmlFull("new-lemma", lb.first->symbol().str())
				       + Core::XmlAttribute("id", lb.first->id()));
	}
	return lb.first->id();
    }

    Bliss::Pronunciation * Lexicon::getPronunciation(const std::string &phons) {
	std::vector<Bliss::Phoneme::Id> phonemes;
	std::string::size_type i, j;
	i = phons.find_first_not_of(utf8::whitespace) ;
	while (i != std::string::npos) {
	    j = phons.find_first_of(utf8::whitespace, i) ;
	    if (j == std::string::npos) j = phons.length();
	    Fsa::LabelId label = phonemeId(phons.substr(i, j-i));
	    if (label == Fsa::InvalidLabelId)
		criticalError("Invalid phoneme symbol \"%s\".",
			      phons.substr(i, j-i).c_str());
	    phonemes.push_back(label);
	    i = phons.find_first_not_of(utf8::whitespace, j) ;
	}
	phonemes.push_back(Bliss::Phoneme::term);
	Bliss::Pronunciation *pron = Precursor::getOrCreatePronunciation(phonemes);
	return pron;
    }

    Fsa::LabelId Lexicon::lemmaPronunciationId(const std::string &id) {
	Fsa::LabelId label = lemmaPronunciationAlphabet()->specialIndex(id);
	if (label != Fsa::InvalidLabelId)
	    return label;
	// Parse
	std::string::size_type i = id.find("   /");
	std::string::size_type j = i + 4;
	if (i == std::string::npos) { // for backward compatibility
	    i = id.find("/");
	    j = i + 1;
	}
	std::string::size_type k = id.rfind("/");
	if ((i == std::string::npos) || (k == std::string::npos) || (k < j)) {
	    criticalError("Invalid lemma pronunciation symbol \"%s\".",
			  id.c_str());

	    //dbg
	    defect();

	    return Fsa::InvalidLabelId;
	}
	std::string orth = id.substr(0, i);
	std::string phon = id.substr(j, k-j);
	// Get lemma and pronunciation
	Lemma *_lemma = getLemma(orth).first;
	//Lemma *_lemma = const_cast<Lemma*>(lemmaAlphabet()->lemma(lemmaId(id)));
	if (!_lemma) {

	    //dbg
	    defect();

	    return Fsa::InvalidLabelId;
	}
	for (LemmaPronunciationRange lpRange = _lemma->pronunciations();
	     lpRange.first != lpRange.second; ++lpRange.first) {
	    if (phon == lpRange.first->pronunciation()->format(phonemeInventory()))
		return lpRange.first->id();
	}
	if (isReadOnly_) {

	    //dbg
	    defect();

	    return Fsa::InvalidLabelId;
	}
	Bliss::Pronunciation *pron = getPronunciation(phon);
	Precursor::addPronunciation(_lemma, pron, 1.0);
	updateLemma(_lemma);
	for (LemmaPronunciationRange lpRange = _lemma->pronunciations();
	     lpRange.first != lpRange.second; ++lpRange.first) {
	    if (phon == lpRange.first->pronunciation()->format(phonemeInventory())) {
		if (insertionChannel_->isOpen())
		    *insertionChannel_ << (Core::XmlFull(
					       "new-lemma-pronunciation",
					       lemmaPronunciationAlphabet()->symbol(lpRange.first->id()))
					   + Core::XmlAttribute("id", lpRange.first->id()));
		return lpRange.first->id();
	    }
	}

	// dbg
	defect();

	return Fsa::InvalidLabelId;
    }

    Fsa::LabelId Lexicon::lemmaPronunciationId(const std::string &orth, s32 variant) {
	Fsa::LabelId label = lemmaPronunciationAlphabet()->specialIndex(orth);
	if (label != Fsa::InvalidLabelId) {
	    if (variant != -1)
		warning("Expected variant -1 for special symbol \"%s\", found %d;"
			"pronunciation variants special symbols have no pronunciations.",
			orth.c_str(), variant);
	    return label;
	}
	const Lemma *_lemma = getLemma(orth).first;
	if (!_lemma) {
	    criticalError("orthography not found in lexicon: ") << orth;
	    //dbg
	    defect();

	    return Fsa::InvalidLabelId;
	}
	const LemmaPronunciation *_lemmaPron = lemmaPronunciation(_lemma, variant);
	if (!_lemmaPron) {

	    //dbg
	    defect();

	    return Fsa::InvalidLabelId;
	}
	return _lemmaPron->id();
    }

    Fsa::LabelId Lexicon::syntacticTokenId(const std::string &id) {
	Fsa::LabelId label = syntacticTokenAlphabet()->index(id);
	if ((label != Fsa::InvalidLabelId) || isReadOnly_)
	    return label;
	std::pair<Lemma*, bool> lb = getLemma(id);
	if (lb.second) {
	    criticalError(
		"Could not add syntactic token \"%s\", "
		"because a lemma with the same orthography but a different syntactic token does already exist",
		id.c_str());
	    return Fsa::InvalidLabelId;
	}
	Precursor::setSyntacticTokenSequence(lb.first, std::vector<std::string>(1, id));
	updateLemma(lb.first);
	label = syntacticTokenAlphabet()->index(id);
	verify(label != Fsa::InvalidLabelId);
	if (insertionChannel_->isOpen())
	    *insertionChannel_ << (Core::XmlFull("new-syntactic-token", syntacticTokenAlphabet()->symbol(label))
				   + Core::XmlAttribute("id", label));
	return label;
    }

    Fsa::LabelId Lexicon::evaluationTokenId(const std::string &id) {
	Fsa::LabelId label = evaluationTokenAlphabet()->index(id);
	if ((label != Fsa::InvalidLabelId) || isReadOnly_)
	    return label;
	Lemma *_lemma = getLemma(id).first;
	Precursor::addEvaluationTokenSequence(_lemma, std::vector<std::string>(1, id));
	updateLemma(_lemma);
	label = evaluationTokenAlphabet()->index(id);
	verify(label != Fsa::InvalidLabelId);
	if (insertionChannel_->isOpen())
	    *insertionChannel_ << (Core::XmlFull("new-evaluation-token", evaluationTokenAlphabet()->symbol(label))
				   + Core::XmlAttribute("id", label));
	return label;
    }

    const LemmaPronunciation * Lexicon::lemmaPronunciation(const Lemma *_lemma, s32 variant) {
	require(_lemma);
	LemmaPronunciationRange lpRange;
	if (_lemma->nPronunciations() > 0) {
	    lpRange = _lemma->pronunciations();
	    if (variant == -1) {
		for (; lpRange.first != lpRange.second; ++lpRange.first)
		    if (lpRange.first->pronunciation()->length() == 0)
			return lpRange.first;
	    } else {
		verify(variant >= 0);
		if (variant < (s32)_lemma->nPronunciations())
		    for (s32 i = 0; i < variant; ++i, ++lpRange.first);
		return lpRange.first;
	    }
	}
	if (isReadOnly_)
	    return 0;
	Precursor::addPronunciation(const_cast<Lemma *>(_lemma), emptyPron_, 1.0);
	updateLemma(const_cast<Lemma *>(_lemma));
	lpRange = _lemma->pronunciations();
	verify(lpRange.first != lpRange.second);
	if (insertionChannel_->isOpen())
	    *insertionChannel_ << (Core::XmlFull(
				       "new-lemma-pronunciation",
				       lemmaPronunciationAlphabet()->symbol(lpRange.first->id()))
				   + Core::XmlAttribute("id", lpRange.first->id()));
	return lpRange.first;
    }

    std::pair<const Lemma*, s32> Lexicon::lemmaPronunciationVariant(const Bliss::LemmaPronunciation *lp) {
	require(lp);
	std::pair<const Lemma*, s32> lv(lp->lemma(), -1);
	if (lp->pronunciation()->length() > 0) {
	    s32 v = 0;
	    for (LemmaPronunciationRange lpRange = lv.first->pronunciations();
		 lpRange.first != lpRange.second; ++lpRange.first, ++v)
		if (lpRange.first->id() == lp->id())
		    break;
	    verify(v < (s32)lv.first->nPronunciations());
	    lv.second = v;
	}
	return lv;
    }

    Lexicon::SymbolMap::SymbolMap(Fsa::ConstAlphabetRef alphabet, IndexFcn indexFcn) :
	lexicon_(Lexicon::us().get()), index_(indexFcn), alphabet_(alphabet) {}

    void Lexicon::SymbolMap::indices(const std::string &str, std::vector<Fsa::LabelId> &ids) const {
	const char *b = str.c_str(), *c;
	for (; (*b != '\0') && ::isspace(*b); ++b);
	while (*b != '\0') {
	    for (c = b + 1; (*c != '\0') && !::isspace(*c); ++c);
	    ids.push_back(index(std::string(b, c - b)));
	    for (b = c; (*b != '\0') && ::isspace(*b); ++b);
	}
    }

    Lexicon::SymbolMap Lexicon::symbolMap(AlphabetId alphabetId) {
	switch (alphabetId) {
	case UnknownAlphabetId:
	    return SymbolMap(unknownAlphabet(), &Lexicon::unknownId);
	case LemmaAlphabetId:
	    return SymbolMap(lemmaAlphabet(), &Lexicon::lemmaId);
	case LemmaPronunciationAlphabetId:
	    return SymbolMap(lemmaPronunciationAlphabet(), &Lexicon::lemmaPronunciationId);
	case SyntacticTokenAlphabetId:
	    return SymbolMap(syntacticTokenAlphabet(), &Lexicon::syntacticTokenId);
	case EvaluationTokenAlphabetId:
	    return SymbolMap(evaluationTokenAlphabet(), &Lexicon::evaluationTokenId);
	case PhonemeAlphabetId:
	    return SymbolMap(phonemeInventory()->phonemeAlphabet(), &Lexicon::phonemeId);
	default:
	    return SymbolMap();
	}
    }

    const Core::Choice Lexicon::AlphabetNameMap(
	"unknown",             UnknownAlphabetId,
	"phoneme",             PhonemeAlphabetId,
	"lemma",               LemmaAlphabetId,
	"lemma-pronunciation", LemmaPronunciationAlphabetId,
	"syntax",              SyntacticTokenAlphabetId,
	"evaluation",          EvaluationTokenAlphabetId,
	Core::Choice::endMark());

    Lexicon::AlphabetId Lexicon::alphabetId(const std::string &name, bool dieOnFailure) {
	Core::Choice::Value c = AlphabetNameMap[name];
	if (c == Core::Choice::IllegalValue) {
	    if (dieOnFailure)
		criticalError("Lexicon provides no alphabet \"%s\".", name.c_str());
	    return InvalidAlphabetId;
	} else
	    return AlphabetId(c);
    }

    Lexicon::AlphabetId Lexicon::alphabetId(Fsa::ConstAlphabetRef alphabet, bool dieOnFailure) {
	if (alphabet == lemmaAlphabet()) {
	    return LemmaAlphabetId;
	} else if (alphabet == lemmaPronunciationAlphabet()) {
	    return LemmaPronunciationAlphabetId;
	} else if (alphabet == syntacticTokenAlphabet()) {
	    return SyntacticTokenAlphabetId;
	} else if (alphabet == evaluationTokenAlphabet()) {
	    return EvaluationTokenAlphabetId;
	} else if (alphabet == phonemeInventory()->phonemeAlphabet()) {
	    return PhonemeAlphabetId;
	} else if (alphabet == unknownAlphabet()) {
	    return UnknownAlphabetId;
	} else {
	    if (dieOnFailure)
		criticalError("Alphabet \"%p\" is not one of the lexicon's alphabets.", alphabet.get());
	    return InvalidAlphabetId;
	}
    }

    Fsa::ConstAlphabetRef Lexicon::alphabet(AlphabetId alphabetId) {
	switch (alphabetId) {
	case LemmaAlphabetId:
	    return lemmaAlphabet();
	case LemmaPronunciationAlphabetId:
	    return lemmaPronunciationAlphabet();
	case SyntacticTokenAlphabetId:
	    return syntacticTokenAlphabet();
	case EvaluationTokenAlphabetId:
	    return evaluationTokenAlphabet();
	case PhonemeAlphabetId:
	    return phonemeInventory()->phonemeAlphabet();
	case UnknownAlphabetId:
	    return unknownAlphabet();
	default:
	    return Fsa::ConstAlphabetRef();
	}
    }

    const std::string & Lexicon::alphabetName(AlphabetId id) {
	return AlphabetNameMap[id];
    }

    Lexicon::AlphabetMap::AlphabetMap(Fsa::ConstAlphabetRef from, Fsa::ConstAlphabetRef to) :
	lexicon_(Lexicon::us().get()),
	unkLabel_(Fsa::InvalidLabelId),
	from_(from), to_(to) {
	ensure(from_ && to_);
    }

    Fsa::LabelId Lexicon::AlphabetMap::operator[] (Fsa::LabelId label) {
	if ((label < Fsa::FirstLabelId) || (Fsa::LastLabelId < label))
	    return label; // special label ids are fixed
	Fsa::LabelId target;
	if (label < Fsa::LabelId(size())) {
	    target = Precursor::operator[](label);
	    if (target != Unmapped) return target;
	} else
	    grow(label, Unmapped);
	target = Precursor::operator[](label) = index(label);
	return target;
    }

    class LemmaAlphabetMap : public Lexicon::AlphabetMap {
    protected:
	virtual Fsa::LabelId index(Fsa::LabelId label) {
	    return lexicon_->lemmaId(from()->symbol(label));
	}
    public:
	LemmaAlphabetMap(Fsa::ConstAlphabetRef from) :
	    AlphabetMap(from, Lexicon::us()->lemmaAlphabet()) {
	    unkLabel_ = lexicon_->unkLemmaId();
	}
    };

    class LemmaPronunciationAlphabetMap : public Lexicon::AlphabetMap {
    protected:
	virtual Fsa::LabelId index(Fsa::LabelId label) {
	    return lexicon_->lemmaPronunciationId(from()->symbol(label));
	}
    public:
	LemmaPronunciationAlphabetMap(Fsa::ConstAlphabetRef from) :
	    AlphabetMap(from, Lexicon::us()->lemmaPronunciationAlphabet()) {
	    unkLabel_ = lexicon_->unkLemmaPronunciationId();
	}
    };

    class SyntacticTokenAlphabetMap : public Lexicon::AlphabetMap {
    protected:
	virtual Fsa::LabelId index(Fsa::LabelId label) {
	    return lexicon_->syntacticTokenId(from()->symbol(label));
	}
    public:
	SyntacticTokenAlphabetMap(Fsa::ConstAlphabetRef from) :
	    AlphabetMap(from, Lexicon::us()->syntacticTokenAlphabet()) {}
    };

    class EvaluationTokenAlphabetMap : public Lexicon::AlphabetMap {
    protected:
	virtual Fsa::LabelId index(Fsa::LabelId label) {
	    return lexicon_->evaluationTokenId(from()->symbol(label));
	}
    public:
	EvaluationTokenAlphabetMap(Fsa::ConstAlphabetRef from) :
	    AlphabetMap(from, Lexicon::us()->evaluationTokenAlphabet()) {}
    };

    class PhonemeAlphabetMap : public Lexicon::AlphabetMap {
    protected:
	virtual Fsa::LabelId index(Fsa::LabelId label) {
	    return lexicon_->phonemeId(from()->symbol(label));
	}
    public:
	PhonemeAlphabetMap(Fsa::ConstAlphabetRef from) :
	    AlphabetMap(from, Lexicon::us()->phonemeInventory()->phonemeAlphabet()) {}
    };

    class UnknownAlphabetMap : public Lexicon::AlphabetMap {
    protected:
	virtual Fsa::LabelId index(Fsa::LabelId label) {
	    return label;
	}
    public:
	UnknownAlphabetMap(Fsa::ConstAlphabetRef from) :
	    AlphabetMap(from, Lexicon::us()->unknownAlphabet()) {}
    };

    const Fsa::LabelId Lexicon::AlphabetMap::Unmapped = -2;
    Lexicon::AlphabetMapRef Lexicon::alphabetMap(Fsa::ConstAlphabetRef from, AlphabetId id) {
	switch (id) {
	case LemmaAlphabetId:
	    return AlphabetMapRef(new LemmaAlphabetMap(from));
	case LemmaPronunciationAlphabetId:
	    return AlphabetMapRef(new LemmaPronunciationAlphabetMap(from));
	case SyntacticTokenAlphabetId:
	    return AlphabetMapRef(new SyntacticTokenAlphabetMap(from));
	case EvaluationTokenAlphabetId:
	    return AlphabetMapRef(new EvaluationTokenAlphabetMap(from));
	case PhonemeAlphabetId:
	    return AlphabetMapRef(new PhonemeAlphabetMap(from));
	case UnknownAlphabetId:
	    return AlphabetMapRef(new UnknownAlphabetMap(from));
	default:
	    return AlphabetMapRef();
	}
    }

    Lexicon::PhonemeToLemmaPronunciationTransducerRef Lexicon::phonemeToLemmaPronunciationTransducer() {
	if (!phonemeToLemmaPronunciationTransducer_)
	    phonemeToLemmaPronunciationTransducer_ =
		Precursor::createPhonemeToLemmaPronunciationAndStickPunctuationTransducer();
	return phonemeToLemmaPronunciationTransducer_;
    }

    Lexicon::LemmaPronunciationToLemmaTransducerRef Lexicon::lemmaPronunciationToLemmaTransducer() {
	if (!lemmaPronunciationToLemmaTransducer_)
	    lemmaPronunciationToLemmaTransducer_ =
		Precursor::createLemmaPronunciationToLemmaTransducer();
	return lemmaPronunciationToLemmaTransducer_;
    }

    Lexicon::LemmaToSyntacticTokenTransducerRef Lexicon::lemmaToSyntacticTokenTransducer() {
	if (!lemmaToSyntacticTokenTransducer_)
	    lemmaToSyntacticTokenTransducer_ =
		Precursor::createLemmaToSyntacticTokenTransducer();
	return lemmaToSyntacticTokenTransducer_;
    }

    Lexicon::LemmaToEvaluationTokenTransducerRef Lexicon::lemmaToEvaluationTokenTransducer() {
	if (!lemmaToEvaluationTokenTransducer_)
	    lemmaToEvaluationTokenTransducer_ =
		Precursor::createLemmaToEvaluationTokenTransducer();
	return lemmaToEvaluationTokenTransducer_;
    }

    Lexicon::LemmaToEvaluationTokenTransducerRef Lexicon::lemmaToPreferredEvaluationTokenSequenceTransducer() {
	if (!lemmaToPreferredEvaluationTokenSequenceTransducer_)
	    lemmaToPreferredEvaluationTokenSequenceTransducer_ =
		Precursor::createLemmaToPreferredEvaluationTokenSequenceTransducer();
	return lemmaToPreferredEvaluationTokenSequenceTransducer_;
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    class WordListExtractorNode : public FilterNode {
    protected:
	class WordListExtractor : public TraverseState {
	private:
	    Core::Vector<bool> &hasLid_;
	protected:
	    virtual void exploreArc(ConstStateRef from, const Arc &a) {
		Fsa::LabelId lid = a.input();
		if ((Fsa::FirstLabelId <= lid) && (lid <= Fsa::LastLabelId)) {
		    hasLid_.grow(lid, false);
		    hasLid_[lid] = true;
		}
	    }
	public:
	    WordListExtractor(ConstLatticeRef l, Core::Vector<bool> &hasLid) :
		TraverseState(l), hasLid_(hasLid) {
		traverse();
	    }
	    virtual ~WordListExtractor() {}
	};

    private:
	Core::Channel dump_;
	Core::Vector<bool> hasLid_;
	Fsa::ConstAlphabetRef alphabet_;

    private:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!l)
		return ConstLatticeRef();
	    if (!alphabet_)
		alphabet_ = l->getInputAlphabet();
	    else
		verify(Lexicon::us()->alphabetId(alphabet_) == Lexicon::us()->alphabetId(l->getInputAlphabet()));
	    WordListExtractor extractAlphabet(l, hasLid_);
	    return l;
	}
    public:
	WordListExtractorNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config), dump_(config, "dump"), hasLid_(0) {}
	~WordListExtractorNode() {}

	virtual void init(const std::vector<std::string> &arguments) {}

	virtual void finalize() {
	    if (!alphabet_)
		return;
	    for (u32 lid = 0; lid < hasLid_.size(); ++lid)
		if (hasLid_[lid])
		    dump_ << alphabet_->symbol(lid) << std::endl;
	}
    };
    NodeRef createWordListExtractorNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new WordListExtractorNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    EditDistance::EditDistance(Fsa::ConstAlphabetRef alphabet, const Bliss::TokenAlphabet *toks) :
	alphabet_(alphabet), toks_(toks) {
	lengthD_ = 256; D_ = new Cost[lengthD_];
    }

    EditDistance::~EditDistance() {
	lengthD_ = 0; delete D_;
    }

    EditDistance::Cost EditDistance::operator() (Fsa::LabelId label1, Fsa::LabelId label2) const {
	if (label1 == label2) return 0;
	const Bliss::Token *tok1 = toks_->token(label1), *tok2 = toks_->token(label2);
	if (tok1 == tok2) return 0;
	if (!tok1) return tok2->symbol().length();
	if (!tok2) return tok1->symbol().length();
	Cost l1 = tok1->symbol().length();
	const Bliss::Symbol::Char *begin_c1 = tok1->symbol().str();
	const Bliss::Symbol::Char *end_c1 = begin_c1 + l1;
	verify(*end_c1 == '\0');
	if (lengthD_ <= l1)
	    { delete [] D_; lengthD_ = l1 + 1; D_ = new Cost[lengthD_]; }
	Cost *d = D_; for (Cost l = 0; l <= l1; ++l, ++d) *d = l;
	for (const Bliss::Symbol::Char *c2 = tok2->symbol().str(); *c2 != '\0'; ++c2) {
	    d = D_;
	    Cost p = *d + 1; ++d;
	    for (const Bliss::Symbol::Char *c1 = begin_c1; c1 != end_c1; ++c1, ++d) {
		verify_((d != D_) && (d != D_ + lengthD_));
		Cost n = std::min(*(d - 1) + ((*c1 == *c2) ? 0 : 1), std::min(*d, p) + 1);
		*(d - 1) = p; p = n;
	    }
	    *(d - 1) = p;
	}
	return *(D_ + l1);
    }

    Score EditDistance::cost(Fsa::LabelId label1, Fsa::LabelId label2) const {
	return Score(operator()(label1, label2));
    }

    Score EditDistance::normCost(Fsa::LabelId label1, Fsa::LabelId label2) const {
	EditDistance::Cost dist = operator()(label1, label2);
	if (dist == 0)
	    return 0.0;
	const Bliss::Token *tok1 = toks_->token(label1), *tok2 = toks_->token(label2);
	EditDistance::Cost norm = 0;
	if (tok1) norm += tok1->symbol().length();
	if (tok2) norm += tok2->symbol().length();
	verify(norm > 0);
	return Score(dist) / Score(norm);
    }

    ConstEditDistanceRef EditDistance::create(Lexicon::AlphabetId alphabetId) {
	Fsa::ConstAlphabetRef alphabet = Lexicon::us()->alphabet(alphabetId);
	switch (alphabetId) {
	case Lexicon::LemmaAlphabetId:
	    return ConstEditDistanceRef(new EditDistance(alphabet, Lexicon::us()->lemmaAlphabet().get()));
	case Lexicon::LemmaPronunciationAlphabetId:
	    Core::Application::us()->criticalError(
		"Edit distance calculation is not supported for alphabet \"%s\".",
		Lexicon::us()->alphabetName(alphabetId).c_str());
	    return ConstEditDistanceRef();
	case Lexicon::SyntacticTokenAlphabetId:
	    return ConstEditDistanceRef(new EditDistance(alphabet, Lexicon::us()->syntacticTokenAlphabet().get()));
	case Lexicon::EvaluationTokenAlphabetId:
	    return ConstEditDistanceRef(new EditDistance(alphabet, Lexicon::us()->evaluationTokenAlphabet().get()));
	case Lexicon::PhonemeAlphabetId:
	    return ConstEditDistanceRef(new EditDistance(alphabet, Lexicon::us()->phonemeInventory()->phonemeAlphabet().get()));
	default:
	    return ConstEditDistanceRef();
	}
    }
    // -------------------------------------------------------------------------

} // namespace Flf
