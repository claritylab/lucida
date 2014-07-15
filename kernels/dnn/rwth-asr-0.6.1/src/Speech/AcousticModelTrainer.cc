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
#include <Am/Module.hh>
#include <Core/Hash.hh>
#include <Core/ProgressIndicator.hh>
#include <Core/TextStream.hh>
#include "AcousticModelTrainer.hh"

using namespace Speech;

// ===========================================================================
AcousticModelTrainer::AcousticModelTrainer(const Core::Configuration &c, Am::AcousticModel::Mode mode) :
    Component(c),
    Precursor(c)
{
    lexicon_ = Bliss::Lexicon::create(select("lexicon"));
    if (!lexicon_) criticalError("Failed to initialize lexicon.");

    acousticModel_ = Am::Module::instance().createAcousticModel(select("acoustic-model"), lexicon_, mode);
    if (!acousticModel_) criticalError("Failed to initialize acoustic model.");
}

AcousticModelTrainer::~AcousticModelTrainer()
{}

void AcousticModelTrainer::signOn(CorpusVisitor &corpusVisitor)
{
    Precursor::signOn(corpusVisitor);
    acousticModel_->signOn(corpusVisitor);
}
// ===========================================================================
TextDependentMixtureSetTrainer::TextDependentMixtureSetTrainer(const Core::Configuration &c) :
    Core::Component(c),
    AcousticModelTrainer(c, Am::AcousticModel::noEmissions),
    MlMixtureSetTrainer(c),
    featureDescription_(*this),
    initialized_(false)
{}

void TextDependentMixtureSetTrainer::setFeatureDescription(const Mm::FeatureDescription &description)
{
    if (!initialized_) {
	featureDescription_ = description;

	size_t dimension;
	featureDescription_.mainStream().getValue(Mm::FeatureDescription::nameDimension, dimension);

	initializeAccumulation(acousticModel()->nEmissions(), dimension);
	initialized_ = true;
    } else {
	if (featureDescription_ != description) {
	    criticalError("Change of features is not allowed.");
	}
    }
    AcousticModelTrainer::setFeatureDescription(description);
}


// ===========================================================================
// TdcSumAccumulator

const Core::ParameterString TdcSumAccumulator::paramSumFile(
    "sum-file",
    "name of generated sum file to be used with the TDC software");
const Core::ParameterString TdcSumAccumulator::paramOldSumFile(
    "old-sum-file",
    "name of generated sum file to be used with the TDC software");
const Core::ParameterString TdcSumAccumulator::paramNewSumFile(
    "new-sum-file",
    "name of generated sum file to be used with the TDC software");
const Core::ParameterStringVector TdcSumAccumulator::paramSumFilesToCombine(
    "sum-files-to-combine",
    "combine TDC sum files with current accumulator ", " ", 0);

struct TdcSumAccumulator::Statistics {
    struct Accumulator {
	Mm::Weight count;
	std::vector<Mm::Sum> sum, sumOfSquares;
	Accumulator(Mm::ComponentIndex d) :
	    count(0), sum(d, 0.0), sumOfSquares(d, 0.0) {}
	void reset() { count = 0; sum.clear(); sumOfSquares.clear(); }
	Accumulator & operator= (const Accumulator &accu) {
	    count = accu.count;
	    sum = accu.sum;
	    sumOfSquares = accu.sumOfSquares;
	    return *this;
	}
	Accumulator & operator+= (const Accumulator &accu) {
	    verify((sum.size() == accu.sum.size()) && (sumOfSquares.size() == accu.sumOfSquares.size()));
	    count += accu.count;
	    {
		std::vector<Mm::Sum>::iterator itSum = sum.begin();
		for (std::vector<Mm::Sum>::const_iterator itAdd = accu.sum.begin(), endAdd = accu.sum.end(); itAdd != endAdd; ++itSum, ++itAdd)
		    *itSum += *itAdd;
	    }
	    {
		std::vector<Mm::Sum>::iterator itSumOfSquares = sumOfSquares.begin();
		for (std::vector<Mm::Sum>::const_iterator itAdd = accu.sumOfSquares.begin(), endAdd = accu.sumOfSquares.end(); itAdd != endAdd; ++itSumOfSquares, ++itAdd)
		    *itSumOfSquares += *itAdd;
	    }
	    return *this;
	}
    };
    typedef Core::hash_map<Fsa::LabelId, Accumulator> AccumulatorMap;
    AccumulatorMap accumulators;
};

TdcSumAccumulator::TdcSumAccumulator(const Core::Configuration &c) :
    Core::Component(c),
    AcousticModelTrainer(c, Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTying),
    Legacy::PhoneticDecisionTreeBase(c, acousticModel()->phonemeInventory()) {
    statistics_ = new Statistics();
}

TdcSumAccumulator::~TdcSumAccumulator() {
    delete statistics_;
}

/**
 * \todo estimation sum files for boundary styles other than 'super-pos-dep'
 */

char TdcSumAccumulator::allophoneBoundaryToBoundaryFlag(u8 allophoneBoundary) const {
    switch (boundaryStyle_) {
    case noPosDep:
	return ' ';
    case posDep:
	if (allophoneBoundary == Am::Allophone::isWithinPhone) {
	    return 'W';
	} else if ((allophoneBoundary == Am::Allophone::isInitialPhone) ||
		   (allophoneBoundary == Am::Allophone::isFinalPhone) ||
		   (allophoneBoundary == (Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone))) {
	    return 'X';
	} else {
	    error("unknown boundary flag %d", allophoneBoundary);
	    return 'W';
	}
	break;
    case superPosDep:
	if (allophoneBoundary == Am::Allophone::isWithinPhone) {
	    return 'W';
	} else if (allophoneBoundary == Am::Allophone::isInitialPhone) {
	    return 'Y';
	} else if (allophoneBoundary == Am::Allophone::isFinalPhone) {
	    return 'Z';
	} else if (allophoneBoundary == (Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone)) {
	    return 'X';
	} else {
	    error("unknown boundary flag %d", allophoneBoundary);
	    return 'W';
	}
    default:
	defect();
	return ' ';
    }
}

u8 TdcSumAccumulator::boundaryFlagToAllophoneBoundary(char c) const {
    switch (boundaryStyle_) {
    case noPosDep:
	switch (c) {
	case 'W':
	    return Am::Allophone::isWithinPhone;
	default:
	    error("unknown boundary flag \"%c\"", c);
	    return 0;
	}
    case posDep:
	switch (c) {
	case 'W':
	    return Am::Allophone::isWithinPhone;
	case 'X':
	    return (Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone);
	default:
	    error("unknown boundary flag \"%c\"", c);
	    return 0;
	}
    case superPosDep:
	switch (c) {
	case 'W':
	    return Am::Allophone::isWithinPhone;
	case 'Y':
	    return Am::Allophone::isInitialPhone;
	case 'Z':
	    return Am::Allophone::isFinalPhone;
	case 'X':
	    return (Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone);
	default:
	    error("unknown boundary flag \"%c\"", c);
	    return 0;
	}
    default:
	defect();
	return 0;
    }
}

void TdcSumAccumulator::quantizeCountsToIntegers() {
    for (Statistics::AccumulatorMap::iterator i = statistics_->accumulators.begin(); i != statistics_->accumulators.end(); ++i) {
	Statistics::Accumulator &accu = i->second;
	Mm::Weight quantizedCount = Core::floor(accu.count);
	Mm::Weight l = quantizedCount / accu.count;
	accu.count = quantizedCount;
	for (Mm::ComponentIndex c = 0; c < accu.sum.size(); ++c) {
	    accu.sum[c] *= l;
	    accu.sumOfSquares[c] *= l;
	}
   }
}

void TdcSumAccumulator::reset() {
    statistics_->accumulators.clear();
}

void TdcSumAccumulator::processAlignedFeature(Core::Ref<const Feature> f, Am::AllophoneStateIndex e, Mm::Weight w) {
    const Feature::Vector &feature = *f->mainStream();
    if(w != 0.0) {
    Statistics::AccumulatorMap::iterator i = statistics_->accumulators.find(e);
    if (i == statistics_->accumulators.end())
	i = statistics_->accumulators.insert(std::make_pair(e, Statistics::Accumulator(feature.size()))).first;
    Statistics::Accumulator &accu = i->second;
    accu.count += w;
    require(feature.size() == accu.sum.size());
    verify(feature.size() == accu.sumOfSquares.size());
    for (Mm::ComponentIndex d = 0; d < feature.size(); ++d) {
	accu.sum[d] += w * feature[d];
	accu.sumOfSquares[d] += w * feature[d] * feature[d];
    }
   }
}

void TdcSumAccumulator::loadSumFile(std::string filename) {
    if (filename.empty())
	filename = paramSumFile(config);
    addSumFile(filename);
}

void TdcSumAccumulator::writeSumFile(std::string filename) {
    quantizeCountsToIntegers();

    if (filename.empty())
	filename = paramSumFile(config);
    Core::TextOutputStream os(filename);
    if (os.good())
	log("Writing %zd allophone states to sum file \"%s\" for use with TDC ...",
	    statistics_->accumulators.size(), filename.c_str());
    else {
	error("Failed to open sum file \"%s\" for writing", filename.c_str());
	return;
    }

    Am::ConstAllophoneStateAlphabetRef allophoneStateAlphabet = acousticModel()->allophoneStateAlphabet();

    Core::ProgressIndicator p("writing sum file", "phone states");
    p.start(statistics_->accumulators.size());
    for (Statistics::AccumulatorMap::const_iterator i = statistics_->accumulators.begin(); i != statistics_->accumulators.end(); ++i) {
	const Am::AllophoneState allophoneState = allophoneStateAlphabet->allophoneState(i->first);
	const Statistics::Accumulator &accu = i->second;

	os << allophoneState.allophone()->format(pi_) << ' '
	   << s32(allophoneState.state()) << ' '
	   << allophoneBoundaryToBoundaryFlag(allophoneState.allophone()->boundary) << ' '
	   << u32(accu.count) << ":\n";
	for (Mm::ComponentIndex c = 0; c < accu.sum.size(); ++c)
	    os << c << ' '
	       << accu.sum[c] << ' '
	       << accu.sumOfSquares[c] << '\n';
	p.notify();
    }
    p.finish();
}

namespace {
    std::string nextString(const char * &c) {
	for (; ::isspace(*c); ++c);
	const char *b = c;
	for (; !::isspace(*c) && (*c != '\0'); ++c);
	return std::string(b, std::string::size_type(c - b));
    }

    s32 nextInt(const char * &c) {
	char *n;
	s32 result = ::strtol(c, &n, 10);
	if (c == n)
	    Core::Application::us()->error("Failed to convert integer \"%s\"", nextString(c).c_str());
	c = n;
	return result;
    }

    f64 nextFloat(const char * &c) {
	char *n;
	f64 result = ::strtod(c, &n);
	if (c == n)
	    Core::Application::us()->error("Failed to convert integer \"%s\"", nextString(c).c_str());
	c = n;
	return result;
    }
} // namespace

void TdcSumAccumulator::addSumFile(const std::string &filename) {
    Core::TextInputStream is(filename);
    if (is.good())
	log("Add TDC sum file \"%s\" ...", filename.c_str());
    else {
	error("Failed to open sum file \"%s\" for reading", filename.c_str());
	return;
    }
    Core::Ref<const Am::AcousticModel> am = acousticModel();
    Am::ConstAllophoneAlphabetRef allophoneAlphabet = acousticModel()->allophoneAlphabet();
    Am::ConstAllophoneStateAlphabetRef allophoneStateAlphabet = acousticModel()->allophoneStateAlphabet();

    Statistics::Accumulator accu(0);
    accu.sum.reserve(256); accu.sumOfSquares.reserve(256);
    std::pair<Fsa::LabelId, Statistics::Accumulator> allophoneStateAccu = std::make_pair(Fsa::InvalidLabelId, Statistics::Accumulator(0));

    Core::ProgressIndicator p("reading sum file", "estimated phone states");
    if (statistics_->accumulators.empty()) {
	s32 estimatedSize = allophoneStateAlphabet->nClasses() / 3;
	statistics_->accumulators.resize(estimatedSize);
	p.start(estimatedSize);
    } else
	p.start(statistics_->accumulators.size());
    std::string s;
    while (Core::getline(is, s) != EOF) {
	Core::stripWhitespace(s);
	if (!s.empty())
	    break;
    }
    ensure(!s.empty());
    while (!s.empty()) {
	const char *c = s.c_str();
	Am::Allophone allophone = allophoneAlphabet->fromString(nextString(c));
	s16 state = nextInt(c);
	for (; ::isspace(*c); ++c); ensure(*c != '\0');
	allophone.boundary = boundaryFlagToAllophoneBoundary(*c); ++c;
	allophoneStateAccu.first = allophoneStateAlphabet->index(&allophone, state);
	accu.count = nextInt(c);
	s32 i = 0;
	for (;;) {
	    s.clear();
	    while (Core::getline(is, s) != EOF) {
		Core::stripWhitespace(s);
		if (!s.empty())
		    break;
	    }
	    if (s.empty())
		break;
	    char *n;
	    s32 j = ::strtol(s.c_str(), &n, 10);
	    if (!::isspace(*n))
		break;
	    ensure(i == j);
	    c = n;
	    accu.sum.push_back(nextFloat(c));
	    accu.sumOfSquares.push_back(nextFloat(c));
	    ++i;
	}
	Statistics::Accumulator &target = statistics_->accumulators.insert(allophoneStateAccu).first->second;
	if (target.count == 0)
	    target = accu;
	else
	    target += accu;
	accu.reset();
	p.notify();
    }
    p.finish();
}

void TdcSumAccumulator::addSumFiles(const std::vector<std::string> &filenames) {
    for (std::vector<std::string>::const_iterator itFilename = filenames.begin(); itFilename != filenames.end(); ++itFilename)
	addSumFile(*itFilename);
}
