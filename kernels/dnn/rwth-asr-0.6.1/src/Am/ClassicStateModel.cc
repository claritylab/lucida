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
#include "ClassicStateModel.hh"

#include <Core/Application.hh>
#include <Core/CompressedStream.hh>
#include <Core/Debug.hh>
#include <Core/Hash.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/Utility.hh>

using namespace Am;

#define DEBUG_ALLOPHONE_INFO 15

// ===========================================================================
const Core::ParameterInt Phonology::paramHistoryLength(
    "history-length",
    "maximum history length in context phonology",
    1);
const Core::ParameterInt Phonology::paramFutureLength(
    "future-length",
    "maximum future length in context phonology",
    1);
const Core::ParameterBool Phonology::paramCrossWord(
    "cross-word",
    "model context dependencies between words; at maximum one phoneme",
    true);

Phonology::Phonology(const Core::Configuration &config, Bliss::PhonemeInventoryRef pi):
    Precursor(pi, paramHistoryLength(config), paramFutureLength(config)),
    isCrossWord_(paramCrossWord(config)) {}

void Phonology::setCrossWord(bool isCrossWord) {
	    isCrossWord_ = isCrossWord;
}

// ===========================================================================

const u8 Allophone::isWithinPhone = 0;
const u8 Allophone::isInitialPhone = 1;
const u8 Allophone::isFinalPhone   = 2;

// ===========================================================================
const Core::ParameterString AllophoneAlphabet::paramAddFromFile(
    "add-from-file",
    "index all the allophones listed in <file>",
    "");
const Core::ParameterBool AllophoneAlphabet::paramAddFromLexicon(
    "add-from-lexicon",
    "index all allophones occuring in the current lexicon",
    true);
const Core::ParameterBool AllophoneAlphabet::paramAddAll(
    "add-all",
    "index all possible allophones",
    false);
const Core::ParameterString AllophoneAlphabet::paramStoreToFile(
    "store-to-file",
    "Before destruction, the allophone alphabet dumps itself to <file>",
    "");

AllophoneAlphabet::AllophoneAlphabet(const Core::Configuration &config, ConstPhonologyRef phonology, Bliss::LexiconRef lexicon) :
    Fsa::Alphabet(), Core::Component(config),
    phonology_(phonology),
    pi_(phonology->getPhonemeInventory()),
    isCrossWord_(phonology->isCrossWord()) {
    nDisambiguators_ = 0;
    std::string filename = paramAddFromFile(config);
    if (!filename.empty()) {
		load(filename);
		log("%d allophones after adding allophones from file \"%s\"",
			(int)allophones_.size(), filename.c_str());
    }
    if (paramAddFromLexicon(config)) {
		add(lexicon);
		log("%d allophones after adding allophones from lexicon",
			(int)allophones_.size());
    }
    if (paramAddAll(config)) {
		add(pi_);
		log("%d allophones after adding all allophones",
			(int)allophones_.size());
    }
    for(s32 p = 0; p < pi_->nPhonemes(); ++p)
    {
      std::string phoneme(pi_->phoneme(p+1)->symbol());
      /* names of phonemes must not conflict with allophone specification syntax, otherwise allophone-mapping may fail */
      if(phoneme.find("{") == std::string::npos && phoneme.find("}") == std::string::npos && phoneme.find("+") == std::string::npos && (phoneme.find("-") == std::string::npos || phoneme[0] !='['))
      {
	  // Phoneme name is OK
      }else{
	  error() << "bad phoneme name: '" << phoneme << "', must not contain any of the following characters: { } + .";
      }
    }

    setSilence(lexicon);
}

AllophoneAlphabet::~AllophoneAlphabet() {
    std::string filename = paramStoreToFile(config);
    if (!filename.empty()) {
	log("store %d allophones to \"%s\"",
	    (int)allophones_.size(), filename.c_str());
	store(filename);
    }
    clear();
}

std::pair<AllophoneAlphabet::AllophoneMap::iterator, bool> AllophoneAlphabet::insert(const Allophone *allo) const {
    std::pair<AllophoneMap::iterator, bool> insAllo =
	allophoneMap_.insert(std::make_pair(allo, allophones_.size()));
    if (insAllo.second) {
	allophones_.push_back(allo);
	verify(allophones_.size() < size_t(MaxId));
    } else
	delete allo;
    return insAllo;
}

std::pair<AllophoneAlphabet::AllophoneMap::iterator, bool> AllophoneAlphabet::insert(const Allophone &allo) const {
    return insert(new Allophone(allo));
}

void AllophoneAlphabet::clear() {
    allophoneMap_.clear();
    for (AllophoneList::const_iterator a = allophones_.begin(); a != allophones_.end(); ++a) delete *a;
    allophones_.clear();
    nDisambiguators_ = 0;
}

void AllophoneAlphabet::add(Core::Ref<const Bliss::PhonemeInventory> pi) {
    typedef std::vector<Bliss::Phoneme::Id> PhonemeIdList;
    PhonemeIdList phonemeIds, ciPhonemeIds;
    Bliss::PhonemeInventory::PhonemeIterator itPhoneme, endPhoneme;
    for (Core::tie(itPhoneme, endPhoneme) = pi->phonemes(); itPhoneme != endPhoneme; ++itPhoneme) {
	if (*itPhoneme == Bliss::Phoneme::term)
	    continue;
	if ((*itPhoneme)->isContextDependent())
	    phonemeIds.push_back((*itPhoneme)->id());
	else
	    ciPhonemeIds.push_back((*itPhoneme)->id());
    }

    typedef std::vector<PhonemeIdList::const_iterator> PhonemeIdIteratorList;
    PhonemeIdIteratorList historyIts, futureIts;

    Core::ProgressIndicator *progress = new Core::ProgressIndicator("index all allophones");
    progress->start();
    for (s32 i = 0; i <= phonology_->maximumFutureLength(); ++i) {
	for (s32 j = 0; j <= phonology_->maximumHistoryLength(); ++j) {
	    for (;;) {
		Phonology::SemiContext future, history;
		PhonemeIdIteratorList::iterator itIt;
		for (itIt = historyIts.begin(); itIt != historyIts.end(); ++itIt)
		    history.append(1, **itIt);
		for (itIt = futureIts.begin(); itIt != futureIts.end(); ++itIt)
		    future.append(1, **itIt);

		for (PhonemeIdList::iterator itP = phonemeIds.begin(); itP != phonemeIds.end(); ++itP) {
		    Allophone *allo = new Allophone(*itP, Allophone::isWithinPhone);
		    allo->setHistory(history);
		    allo->setFuture(future);
		    insert(allo);
		    progress->notify();
		}
		if ((j == 0) || (isCrossWord_ && (j <= 1))) {
		    for (PhonemeIdList::iterator itP = phonemeIds.begin(); itP != phonemeIds.end(); ++itP) {
			Allophone *allo = new Allophone(*itP, Allophone::isInitialPhone);
			allo->setHistory(history);
			allo->setFuture(future);
			insert(allo);
			progress->notify();
		    }
		}
		if ((i == 0) || (isCrossWord_ && (i <= 1))) {
		    for (PhonemeIdList::iterator itP = phonemeIds.begin(); itP != phonemeIds.end(); ++itP) {
			Allophone *allo = new Allophone(*itP, Allophone::isFinalPhone);
			allo->setHistory(history);
			allo->setFuture(future);
			insert(allo);
			progress->notify();
		    }
		}
		if (((i == 0) && (j == 0)) || (isCrossWord_ && ((i <= 1) && (j <= 1)))) {
		    for (PhonemeIdList::iterator itP = phonemeIds.begin(); itP != phonemeIds.end(); ++itP) {
			Allophone *allo = new Allophone(*itP, Allophone::isInitialPhone | Allophone::isFinalPhone);
			allo->setHistory(history);
			allo->setFuture(future);
			insert(allo);
			progress->notify();
		    }
		}

		for (itIt = historyIts.begin(); itIt != historyIts.end(); ++itIt)
		    if (++(*itIt) == phonemeIds.end())
			*itIt = phonemeIds.begin();
		    else
			break;
		if (itIt == historyIts.end()) {
		    for (itIt = futureIts.begin(); itIt != futureIts.end(); ++itIt)
			if (++(*itIt) == phonemeIds.end())
			    *itIt = phonemeIds.begin();
			else
			    break;
		    if (itIt == futureIts.end())
			break;
		}
	    }
	    historyIts.push_back(phonemeIds.begin());
	}
	historyIts.clear();
	futureIts.push_back(phonemeIds.begin());
    }
    for (PhonemeIdList::iterator itP = ciPhonemeIds.begin(); itP != ciPhonemeIds.end(); ++itP) {
	insert(new Allophone(*itP, Allophone::isWithinPhone));
	progress->notify();
	insert(new Allophone(*itP, Allophone::isInitialPhone));
	progress->notify();
	insert(new Allophone(*itP, Allophone::isFinalPhone));
	progress->notify();
	insert(new Allophone(*itP, Allophone::isInitialPhone | Allophone::isFinalPhone));
	progress->notify();
    }
    progress->finish();
    delete progress;
}

/*
  b: begin, c: central, e:end
*/
std::pair<AllophoneAlphabet::AllophoneMap::iterator, bool> AllophoneAlphabet::createAndInsert
(const std::vector<bool> &cd, const Bliss::Pronunciation &p, s32 b, s32 c, s32 e, s16 boundary) const {
    Allophone *allo = new Allophone(p[c], boundary);
    for (s32 i = c - 1; (i >= b); --i)
	phonology_->appendHistory(*allo, p[i]);
    for (s32 i = c + 1; (i <= e); ++i)
	phonology_->appendFuture(*allo, p[i]);
    return insert(allo);
}

void AllophoneAlphabet::add(Bliss::LexiconRef lexicon) {
    require(pi_);
    const s32 maxHistory = phonology_->maximumHistoryLength();
    const s32 maxFuture  = phonology_->maximumFutureLength();

    std::vector<bool> cd(pi_->nPhonemes()+1);
    Bliss::PhonemeInventory::PhonemeIterator itPhoneme, endPhoneme;
    for (Core::tie(itPhoneme, endPhoneme) = pi_->phonemes(); itPhoneme != endPhoneme; ++itPhoneme)
	cd[(*itPhoneme)->id()] = (*itPhoneme)->isContextDependent();

    AllophoneList leftCwAllophones;
    AllophoneList rightCwAllophones;
    AllophoneList leftRightCwAllophones;
    PhonemeIdSet initialPhonemeSet;
    PhonemeIdSet finalPhonemeSet;

    Core::ProgressIndicator *progress;
    progress = new Core::ProgressIndicator("index allophones from lexicon: within word");
    progress->start(lexicon->nPronunciations());
    std::pair<AllophoneMap::iterator, bool> insAllo;
    Bliss::Lexicon::PronunciationIterator lexicon_pron, lexicon_pron_end;
    for (Core::tie(lexicon_pron, lexicon_pron_end) = lexicon->pronunciations();
	 lexicon_pron != lexicon_pron_end; ++lexicon_pron) {
	const Bliss::Pronunciation &p(**lexicon_pron);
	s32 maxPronId = p.length() - 1;
	if (maxPronId < 0) continue;
	if (cd[p[0]]) initialPhonemeSet.insert(p[0]);
	if (cd[p[maxPronId]]) finalPhonemeSet.insert(p[maxPronId]);

	// split by cd/id
	std::vector<s32> contextChanges;
	for (s32 i = 1; i <= maxPronId; ++i)
	    if (cd[p[i-1]] != cd[p[i]])
		contextChanges.push_back(i-1);
	contextChanges.push_back(maxPronId);

	s32 firstPronId, lastPronId = -1;
	for (std::vector<s32>::const_iterator itLastPronId = contextChanges.begin();
	     itLastPronId != contextChanges.end(); ++itLastPronId) {
	    firstPronId = lastPronId + 1;
	    lastPronId = *itLastPronId;

	    // configure next segment
	    s32 historyLength, futureLength;
	    bool isLeftCw, isRightCw;
	    if (cd[p[firstPronId]]) {
		historyLength = maxHistory;
		futureLength  = maxFuture;
		isLeftCw  = (isCrossWord_ && (firstPronId == 0));
		isRightCw = (isCrossWord_ && (lastPronId == maxPronId));
	    } else {
		historyLength = 0;
		futureLength  = 0;
		isLeftCw  = false;
		isRightCw = false;
	    }
	    s16 initialFlag = (firstPronId == 0) ?
		Allophone::isInitialPhone : Allophone::isWithinPhone;
	    s16 finalFlag   = (lastPronId == maxPronId) ?
		Allophone::isFinalPhone   : Allophone::isWithinPhone;

	    // special treatment for segments of length one
	    if (firstPronId == lastPronId) {
		insAllo = createAndInsert(cd, p, firstPronId, firstPronId, firstPronId,
					  initialFlag | finalFlag);
		if (insAllo.second) {
		    if (isLeftCw && (historyLength > 0))
			leftCwAllophones.push_back(insAllo.first->first);
		    if (isRightCw && (futureLength > 0))
			rightCwAllophones.push_back(insAllo.first->first);
		    if (isLeftCw && isRightCw && (historyLength > 0) && (futureLength > 0))
			leftRightCwAllophones.push_back(insAllo.first->first);
		}
		continue;
	    }

	    // let's rock
	    s32 b = firstPronId, c = firstPronId, e = firstPronId + futureLength;
	    if (historyLength == 0) {
		// initial allophone
		insAllo = createAndInsert(cd, p, firstPronId, firstPronId, std::min(e, lastPronId),
					  initialFlag);
		if (insAllo.second && isRightCw && (e > lastPronId))
		    rightCwAllophones.push_back(insAllo.first->first);
		++b; ++c; ++e;
	    } else {
		// initial cw allophone
		insAllo = createAndInsert(cd, p, firstPronId, firstPronId, std::min(e, lastPronId),
					  initialFlag);
		if (insAllo.second) {
		    if (isLeftCw)
			leftCwAllophones.push_back(insAllo.first->first);
		    if ((e > lastPronId) && isRightCw) {
			if (isLeftCw)
			    leftRightCwAllophones.push_back(insAllo.first->first);
			rightCwAllophones.push_back(insAllo.first->first);
		    }
		}
		// left and left/right cw allophones
		for (++c, ++e; c < std::min(firstPronId + historyLength, lastPronId); ++c, ++e) {
		    insAllo = createAndInsert(cd, p, firstPronId, c, std::min(e, lastPronId),
					      Allophone::isWithinPhone);
		    if (insAllo.second) {
			if (isLeftCw)
			    leftCwAllophones.push_back(insAllo.first->first);
			if ((e > lastPronId) && isRightCw) {
			    if (isLeftCw)
				leftRightCwAllophones.push_back(insAllo.first->first);
			    rightCwAllophones.push_back(insAllo.first->first);
			}
		    }
		}
	    }
	    if (futureLength == 0) {
		// within allophones
		for (; c < lastPronId; ++b, ++c)
		    createAndInsert(cd, p, b, c, c, Allophone::isWithinPhone);
		// final allophone
		createAndInsert(cd, p, b, lastPronId, lastPronId, finalFlag);
	    } else {
		// within allophones
		for (; e <= lastPronId; ++b, ++c, ++e)
		    createAndInsert(cd, p, b, c, e, Allophone::isWithinPhone);
		// right cw allophones
		for (; c < lastPronId; ++b, ++c) {
		    insAllo = createAndInsert(cd, p, b, c, lastPronId, Allophone::isWithinPhone);
		    if (insAllo.second && isRightCw)
			rightCwAllophones.push_back(insAllo.first->first);
		}
		// final cw allophone
		insAllo = createAndInsert(cd, p, b, lastPronId, lastPronId, finalFlag);
		if (insAllo.second) {
		    if ((historyLength > lastPronId) && isLeftCw) {
			leftCwAllophones.push_back(insAllo.first->first);
			if (isRightCw)
			    leftRightCwAllophones.push_back(insAllo.first->first);
		    }
		    if (isRightCw)
			rightCwAllophones.push_back(insAllo.first->first);
		}
	    }
	}
	progress->notify();
    }
    progress->finish();
    delete progress;
    if (isCrossWord_) {
	typedef std::vector<Bliss::Phoneme::Id> PhonemeIdList;
	PhonemeIdList initialPhonemes(initialPhonemeSet.begin(), initialPhonemeSet.end());
	PhonemeIdList finalPhonemes(finalPhonemeSet.begin(), finalPhonemeSet.end());

	progress = new Core::ProgressIndicator("index allophones from lexicon: across word");
	progress->start();
	// left cw allophones
	for (AllophoneList::const_iterator itAllo = leftCwAllophones.begin();
	     itAllo != leftCwAllophones.end(); ++itAllo) {
	    const Allophone &allo(**itAllo);
	    for (PhonemeIdList::const_iterator itFinal = finalPhonemes.begin();
		 itFinal != finalPhonemes.end(); ++itFinal) {
		Allophone *cwAllo = new Allophone(allo);
		phonology_->appendHistory(*cwAllo, *itFinal);
		insert(cwAllo);
		progress->notify();
	    }
	}
	// right cw allophones
	for (AllophoneList::const_iterator itAllo = rightCwAllophones.begin();
	     itAllo != rightCwAllophones.end(); ++itAllo) {
	    const Allophone &allo(**itAllo);
	    for (PhonemeIdList::const_iterator itInitial = initialPhonemes.begin();
		 itInitial != initialPhonemes.end(); ++itInitial) {
		Allophone *cwAllo = new Allophone(allo);
		phonology_->appendFuture(*cwAllo, *itInitial);
		insert(cwAllo);
		progress->notify();
	    }
	}
	// left/right cw allophones
	for (AllophoneList::const_iterator itAllo = leftRightCwAllophones.begin();
	     itAllo != leftRightCwAllophones.end(); ++itAllo) {
	    const Allophone &allo(**itAllo);
	    for (PhonemeIdList::const_iterator itFinal = finalPhonemes.begin();
		 itFinal != finalPhonemes.end(); ++itFinal)
		for (PhonemeIdList::const_iterator itInitial = initialPhonemes.begin();
		     itInitial != initialPhonemes.end(); ++itInitial) {
		    Allophone *cwAllo = new Allophone(allo);
		    phonology_->appendHistory(*cwAllo, *itFinal);
		    phonology_->appendFuture(*cwAllo, *itInitial);
		    insert(cwAllo);
		    progress->notify();
		}
	}
	progress->finish();
	delete progress;
    }
}

void AllophoneAlphabet::load(const std::string &filename) {
    verify(!filename.empty());
    if (!allophones_.empty())
	warning("Indices of allophones in file \"%s\" seems NOT to match indices in current allophone alphabet",
		filename.c_str());
    Core::CompressedInputStream cis(filename);
    if(!cis.isOpen())
	error() << "Failed opening allophone file '" << filename << "' for reading";
    std::string line;
    Core::ProgressIndicator *progress = new Core::ProgressIndicator("index allophones from file");
    progress->start();
    while (cis) {
	std::getline(cis, line);
	Core::stripWhitespace(line);
	if (!line.empty() && (*line.c_str() != '#')) {
	    if (!insert(fromString(line)).second)
		warning("Indices of allophones in file \"%s\" seems NOT to match indices in current allophone alphabet; allophone %s does already exist",
			filename.c_str(), line.c_str());
	    progress->notify();
	}
    }
    progress->finish();
    delete progress;
}

void AllophoneAlphabet::store(const std::string &filename) {
    verify(!filename.empty());
    Core::CompressedOutputStream cos(filename);
    if(!cos.isOpen())
	error() << "Failed opening allophone file '" << filename << "' for writing";
    cos << "# Number of allophones: " << nClasses() << std::endl;
    //    cos << "# Number of disambiguators: " << nDisambiguators() << std::endl;
    for (AllophoneList::const_iterator itAllo = allophones_.begin();
	 itAllo != allophones_.end(); ++itAllo)
	cos << toString(**itAllo) << std::endl;
}

void AllophoneAlphabet::setSilence(Bliss::LexiconRef lexicon) {
    const Bliss::Lemma *silLemma = lexicon->specialLemma("silence");
    if (silLemma) {
	Bliss::Lemma::PronunciationIterator lemma_pron, lemma_pron_end;
	for (Core::tie(lemma_pron, lemma_pron_end) = silLemma->pronunciations();
	     lemma_pron != lemma_pron_end; ++lemma_pron) {
	    for (const Bliss::Phoneme::Id *itPhoneme = (*lemma_pron).pronunciation()->phonemes();
		 *itPhoneme != Bliss::Phoneme::term; ++itPhoneme)
		silPhonemes_.insert(*itPhoneme);
	}
    }
}

const Allophone * AllophoneAlphabet::allophone(Fsa::LabelId id) const {
    if(size_t(id & IdMask) >= allophones_.size())
    {
	std::cerr << "id too high: " << (id & IdMask) << " orig " << id << " limit " << allophones_.size() << std::endl;
	verify(0);
    }
    return allophones_[id & IdMask];
}

Fsa::LabelId AllophoneAlphabet::index(const Allophone *allo, bool create) const {
    AllophoneMap::const_iterator itAllo = allophoneMap_.find(allo);
    if (itAllo == allophoneMap_.end()) {
	if(!create)
	    return Fsa::InvalidLabelId;
	itAllo = insert(new Allophone(*allo)).first;
    }
    return itAllo->second;
}

bool AllophoneAlphabet::hasIndex(const Allophone *allo) const {
    return allophoneMap_.find(allo) != allophoneMap_.end();
}

std::string AllophoneAlphabet::symbol(Fsa::LabelId id) const {
    if ((Fsa::FirstLabelId <= id) && (id <= Fsa::LastLabelId)) {
	if (id & DisambiguatorFlag)
	    return Core::form("#%d", (id & DisambiguatorMask));
	else if (size_t(id & IdMask) < allophones_.size())
	    return toString(*allophones_[id & IdMask]);
	else {
	    warning("Symbol representation requested for unknown allophone id %d",
		    (id & IdMask));
	    return "";
	}
    } else
	return specialSymbol(id);
}

Fsa::LabelId AllophoneAlphabet::index(const std::string &symbol) const {
    Fsa::LabelId lid = specialIndex(symbol);
    if (lid == Fsa::InvalidLabelId)
	lid = index(fromString(symbol));
    return lid;
}

Allophone AllophoneAlphabet::fromString(const std::string &s) const {
    const Bliss::Phoneme *phoneme;
    const char *c, *d, *e, *f;
    c = d = s.c_str();
    for (; (*d != '{') && (*d != '\0'); ++d);
    verify(*d != '\0');
    phoneme = pi_->phoneme(std::string(c, std::string::size_type(d - c)));
    if(!phoneme)
	criticalError("unknown phoneme: '%s'", std::string(c, std::string::size_type(d - c)).c_str());
    Allophone allo(phoneme->id(), Allophone::isWithinPhone);
    c = d;
    for (; ((*d != '+') && (*d != '\0')); ++d);
    verify(*d != '\0');
    e = f = d;
    do {
	f = --e;
	for (; (*e != '-') && (*e != '{'); --e);
	std::string p(e + 1, std::string::size_type(f - e));
	if (p == "#") {
	    verify(allo.history().empty() && (*e == '{'));
	    break;
	}
	phoneme = pi_->phoneme(p);
	if(!phoneme)
	    criticalError() << "unknown phoneme: " << p << " taken from allophone " << s;
	phonology_->appendHistory(allo, phoneme->id());
    } while (*e != '{');
    c = e = f = d;
    for (; (*d != '}') && (*d != '\0'); ++d);
    verify(*d != '\0');
    do {
	f = ++e;
	for (; (*e != '-') && (*e != '}'); ++e);
	std::string p(f, std::string::size_type(e - f));
	if (p == "#") {
	    verify (allo.future().empty() && (*e == '}'));
	    break;
	}
	phoneme = pi_->phoneme(p);
	if(!phoneme)
	    criticalError() << "unknown phoneme: " << p << " taken from allophone " << s;
	phonology_->appendFuture(allo, phoneme->id());
    } while (*e != '}');
    c = ++d;
    for (; (*d != '\0'); ++d);
    std::string b(c, std::string::size_type(d - c));
    if (b.empty()) {
	// pass
    } else if (b == "@i") {
	allo.boundary = Allophone::isInitialPhone;
    } else if (b == "@f") {
	allo.boundary = Allophone::isFinalPhone;
    } else if (b == "@i@f") {
	allo.boundary = Allophone::isInitialPhone | Allophone::isFinalPhone;
    } else
	defect();
    return allo;
}

std::string AllophoneAlphabet::toString(const Allophone &allo) const {
    std::string result = allo.format(pi_);
    if (allo.boundary & Allophone::isInitialPhone) result += "@i";
    if (allo.boundary & Allophone::isFinalPhone)   result += "@f";
    return result;
}

bool AllophoneAlphabet::isDisambiguator(Fsa::LabelId id) const {
    return (Fsa::FirstLabelId <= id) && (id <= Fsa::LastLabelId)
	&& (id & DisambiguatorFlag);
}

Fsa::LabelId AllophoneAlphabet::disambiguator(Fsa::LabelId d) const {
    nDisambiguators_ = std::max(nDisambiguators_, d + 1);
    return d | DisambiguatorFlag;
}

AllophoneAlphabet::const_iterator AllophoneAlphabet::begin() const {
    return const_iterator(Fsa::ConstAlphabetRef(this), 0);
}

AllophoneAlphabet::const_iterator AllophoneAlphabet::end() const {
    return const_iterator(Fsa::ConstAlphabetRef(this), allophones_.size());
}

void AllophoneAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment()
       << allophones_.size() << " allophone labels, "
       << nDisambiguators_ << " disambiguation symbols"
       << Core::XmlCloseComment() << "\n";
    Precursor::writeXml(os);
}

void AllophoneAlphabet::dump(Core::XmlWriter &xml) const {
    xml << Core::XmlOpen("allophone-list")
	+ Core::XmlAttribute("n", allophones_.size());
    Fsa::LabelId idAllo = 0;
    for (AllophoneList::const_iterator itAllo = allophones_.begin();
	 itAllo != allophones_.end(); ++itAllo, ++idAllo)
	xml << Core::form("%9d. ", idAllo) << toString(**itAllo) << "\n";
    xml << Core::XmlClose("allophone-list")
	<< Core::XmlEmpty("disambiguator")
	+ Core::XmlAttribute("n", nDisambiguators_);
}

void AllophoneAlphabet::dumpPlain(std::ostream &os) const {
    for (AllophoneList::const_iterator itAllo = allophones_.begin();
	 itAllo != allophones_.end(); ++itAllo)
	os << toString(**itAllo) << std::endl;
}
// ===========================================================================


// ===========================================================================
AllophoneStateIterator::AllophoneStateIterator(
    ConstAllophoneStateAlphabetRef asAlphabet) :
    asAlphabet_(asAlphabet) {
    itAllo_ = asAlphabet_->allophoneAlphabet_->allophones().begin();
    endAllo_ = asAlphabet_->allophoneAlphabet_->allophones().end();
    resetState();
}

void AllophoneStateIterator::resetState() const {
    if (itAllo_ == endAllo_)
	endState_ = 0;
    else
	endState_ = asAlphabet_->hmmTopologies_->get((*itAllo_)->central())->nPhoneStates();
    state_ = 0;
}

Fsa::LabelId AllophoneStateIterator::id() const {
    return asAlphabet_->index(*itAllo_, state_);
}

AllophoneState AllophoneStateIterator::allophoneState() const {
    return asAlphabet_->allophoneState(*itAllo_, state_);
}
// ===========================================================================


// ===========================================================================
// AllophoneStateAlphabet::AllophoneStateAlphabet() {}

AllophoneStateAlphabet::AllophoneStateAlphabet(
    const Core::Configuration &config,
    ConstAllophoneAlphabetRef allophoneAlphabet,
    ClassicHmmTopologySetRef hmmTopologies) :
    Fsa::Alphabet(),
    Core::Component(config),
    allophoneAlphabet_(allophoneAlphabet),
    hmmTopologies_(hmmTopologies) {
}

Fsa::LabelId AllophoneStateAlphabet::nClasses() const {
    Fsa::LabelId nStates = 0;
    for (AllophoneAlphabet::AllophoneList::const_iterator itAllo = allophoneAlphabet_->allophones().begin();
	 itAllo != allophoneAlphabet_->allophones().end(); ++itAllo) {
	nStates += hmmTopologies_->get((*itAllo)->central())->nPhoneStates();
    }
    return nStates;
}

Fsa::LabelId AllophoneStateAlphabet::index(const AllophoneState &alloState) const {
    return allophoneAlphabet_->index(*alloState.allophone()) | alloState.state() << 26;
}

Fsa::LabelId AllophoneStateAlphabet::index(const Allophone *allo, s16 state, bool create) const {
    verify(allo && (state <= MaxStateId));
    Fsa::LabelId ret = allophoneAlphabet_->index(*allo, create);
    if(ret == Fsa::InvalidLabelId)
	return ret;
    else
	return ret | state << 26;
}

AllophoneState AllophoneStateAlphabet::allophoneState(Fsa::LabelId id) const {
    return AllophoneState(
	allophoneAlphabet_->allophone(id),
	(id & StateMask) >> 26);
}

AllophoneState AllophoneStateAlphabet::allophoneState(const Allophone *allo, s16 state) const {
    verify(allo && (state <= MaxStateId));
    return AllophoneState(allo, state);
}

std::pair<Fsa::LabelId, s16> AllophoneStateAlphabet::allophoneIndexAndState(Fsa::LabelId allophoneStateId) const {
    return std::make_pair(allophoneStateId & AllophoneAlphabet::IdMask, (allophoneStateId & StateMask) >> 26);
}

std::string AllophoneStateAlphabet::toString(const AllophoneState &alloState) const {
    return allophoneAlphabet_->toString(alloState)
	+ "." + Core::itoa(alloState.state());
}

Fsa::LabelId AllophoneStateAlphabet::index(const std::string &symbol, bool create) const {
    //
    // @todo: avoid hard coding lengths and special delimiter symbols
    // what about using a delimiter string or class instead of hard coding '.'?
    //
    // Example:
    // a{b+c}@i@f.10
    // <allo-center><allo-context-delimiter [{]><allo-left><allo-neighbor-delimiter [+]><allo-right><allo-context-delimiter [}]>[@i][@f]<state-delimiter [.]><state-number>

    // we search for last occurrence of '.' in order to allow more than 9 HMM states
    const size_t found=symbol.find_last_of(".");
    //
    // there must be at least one state-delimiter -
    // larger than 5 in order to use the '.' also as a phoneme symbol
    // e.g. .{.+.}.10
    //
    // @todo: would be better to check if a closing context-delimiter has been
    // found before the state-delimiter, as '5' depends again on other delimiter
    // lengths!
    ensure((found != std::string::npos) && (found > 5 )); // '5' depends again on other delimiter lengths
    DBG(DEBUG_ALLOPHONE_INFO) << VAR(symbol.substr(0, found)) << " " << VAR(symbol.substr(found+1)) << ENDDBG;
    Allophone allo = allophoneAlphabet_->fromString(symbol.substr(0, found));
    s16 state;
    if (!Core::strconv(symbol.substr(found+1), state))
	defect();
    return index(&allo, state, create);
}

std::string AllophoneStateAlphabet::symbol(Fsa::LabelId id) const {
    if ((Fsa::FirstLabelId <= id) && (id <= Fsa::LastLabelId)) {
	if (isDisambiguator(id))
	    return Core::form("#%d", (id & AllophoneAlphabet::DisambiguatorMask));
	else
	    return toString(allophoneState(id));
    } else
	return specialSymbol(id);
}

std::pair<AllophoneStateIterator, AllophoneStateIterator> AllophoneStateAlphabet::allophoneStates() const {
    AllophoneStateIterator begin(Core::ref(this));
    AllophoneStateIterator end(Core::ref(this));
    end.itAllo_ = end.endAllo_;
    end.state_ = end.endState_ = 0;
    return std::make_pair(begin, end);
}

AllophoneStateAlphabet::const_iterator AllophoneStateAlphabet::begin() const {
    return const_iterator(Fsa::ConstAlphabetRef(this), 0);
}

AllophoneStateAlphabet::const_iterator AllophoneStateAlphabet::end() const {
    return const_iterator(Fsa::ConstAlphabetRef(this), 0);
}

void AllophoneStateAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment()
       << nClasses() << " allophone state labels, "
	//       << nDisambiguators() << " disambiguation symbols"
       << Core::XmlCloseComment() << "\n";
}

namespace {
    struct IndexedAllophoneState {
	Fsa::LabelId id;
	Am::AllophoneState allophoneState;
	IndexedAllophoneState() {}
	IndexedAllophoneState(Fsa::LabelId id, Am::AllophoneState allophoneState) :
	    id(id), allophoneState(allophoneState) {}
	bool operator< (const IndexedAllophoneState &i) const
	    { return id < i.id; }
    };
    typedef std::vector<IndexedAllophoneState> IndexedAllophoneStateList;
} // namespace

void AllophoneStateAlphabet::dump(Core::XmlWriter &xml) const {
    IndexedAllophoneStateList indexedAllophoneStates;
    for (std::pair<AllophoneStateIterator, AllophoneStateIterator> it = allophoneStates();
	 it.first != it.second; ++it.first)
	indexedAllophoneStates.push_back(IndexedAllophoneState(it.first.id(), it.first.allophoneState()));
    std::sort(indexedAllophoneStates.begin(), indexedAllophoneStates.end());
    xml << Core::XmlOpen("allophone-state-list");
    for (IndexedAllophoneStateList::const_iterator it = indexedAllophoneStates.begin();
	 it != indexedAllophoneStates.end(); ++it)
	xml << Core::form("%9d. %s\n", it->id, toString(it->allophoneState).c_str());
    xml << Core::XmlClose("allophone-state-list")
	<< Core::XmlEmpty("disambiguator")
	+ Core::XmlAttribute("n", nDisambiguators());
}

void AllophoneStateAlphabet::dumpPlain(std::ostream &os) const {
    IndexedAllophoneStateList indexedAllophoneStates;
    for (std::pair<AllophoneStateIterator, AllophoneStateIterator> it = allophoneStates();
	 it.first != it.second; ++it.first)
	indexedAllophoneStates.push_back(IndexedAllophoneState(it.first.id(), it.first.allophoneState()));
    std::sort(indexedAllophoneStates.begin(), indexedAllophoneStates.end());
    for (IndexedAllophoneStateList::const_iterator it = indexedAllophoneStates.begin();
	 it != indexedAllophoneStates.end(); ++it)
	os << toString(it->allophoneState) << std::endl;
}
// ===========================================================================


// ===========================================================================
ClassicStateModel::ClassicStateModel(
    ConstPhonologyRef phonologyRef,
    ConstAllophoneAlphabetRef allophoneAlphabetRef,
    ConstAllophoneStateAlphabetRef allophoneStateAlphabetRef,
    HmmTopologySetRef hmmTopologySetRef,
    std::vector<std::string> conditions) :
    phonologyRef_(phonologyRef),
    piRef_(phonologyRef->getPhonemeInventory()),
    allophoneAlphabetRef_(allophoneAlphabetRef),
    allophoneStateAlphabetRef_(allophoneStateAlphabetRef),
    hmmTopologySetRef_(hmmTopologySetRef),
    conditions_(conditions) {
    verify(phonologyRef_ && allophoneAlphabetRef_  && allophoneStateAlphabetRef_ && hmmTopologySetRef_);
}

AllophoneState ClassicStateModel::allophoneState(const Allophone *allo, s16 state) const {
    return allophoneStateAlphabetRef_->allophoneState(allo, state);
}

const ClassicHmmTopology * ClassicStateModel::hmmTopology(const Allophone *allo) const {
    return hmmTopologySetRef_->get(allo->central());
}
// ===========================================================================
