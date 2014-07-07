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
#ifndef _SPEECH_ABSTRACT_SEGMENTWISE_TRAINER_HH
#define _SPEECH_ABSTRACT_SEGMENTWISE_TRAINER_HH

#include "LatticeSetProcessor.hh"
#include <Core/Parameter.hh>
#include <Lattice/Lattice.hh>

namespace Bliss {
    class OrthographicParser;
}

namespace Speech {

    /*
     * AbstractSegmentwiseTrainer: abstract base class
     * This is the interface for the discriminative
     * accumulation of segmentwise training criteria,
     * e.g. GHMMs/LHMMs with MPE, or CRF for tagging.
     * Each model and criterion corresponds with a
     * derived class.
     */
    class AbstractSegmentwiseTrainer : public LatticeSetProcessor
    {
	typedef LatticeSetProcessor Precursor;
    public:
	enum Criterion {
	    maximumMutualInformation,
	    weightedMaximumMutualInformation,
	    minimumClassificationError,
	    minimumError,
	    gisMinimumError,
	    logMinimumError,
	    contextPrior,
	    contextAccuracy,
	    weightedMinimumError,
	    legacyMinimumErrorWithISmoothing,
	    minimumErrorWithISmoothing,
	    logMinimumErrorWithISmoothing,
	    weightedMinimumErrorWithISmoothing,
	    corrective,
	    minimumLeastSquaredError,
	    plain};
	static Core::Choice choiceCriterion;
	static Core::ParameterChoice paramCriterion;
    private:
	static const Core::ParameterFloat paramWeightThreshold;
	static const Core::ParameterInt paramPosteriorTolerance;
	static Core::ParameterString paramLatticeName;
    private:
	Mm::Weight weightThreshold_;
	s32 posteriorTolerance_;
    protected:
	Bliss::OrthographicParser *orthToLemma_;
	Fsa::ConstAutomatonRef lemmaPronToLemma_;
	Fsa::ConstAutomatonRef lemmaToLemmaConfusion_;
	std::string part_;
    protected:
	Lattice::ConstWordLatticeRef extractNumerator(const std::string &orth, Lattice::ConstWordLatticeRef denominator) const;
	Lattice::ConstWordLatticeRef turnOffCompetingHypotheses(const std::string &orth, Lattice::ConstWordLatticeRef denominator);
	Mm::Weight weightThreshold() const { return weightThreshold_; }
    public:
	s32 posteriorTolerance() const { return posteriorTolerance_; }
    public:
	AbstractSegmentwiseTrainer(const Core::Configuration &);
	virtual ~AbstractSegmentwiseTrainer();

	/*
	 * Use this function for lexicon-based initialization.
	 * Lexicon must be global because otherwise pointer
	 * comparisons of the alphabets will fail etc.
	 */
	virtual void initialize(Bliss::LexiconRef);

    };

} // namespace Speech

#endif // _SPEECH_ABSTRACT_SEGMENTWISE_TRAINER_HH
