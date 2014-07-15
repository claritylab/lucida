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
#ifndef _LATTICE_ACCURACY_HH
#define _LATTICE_ACCURACY_HH

#include "Lattice.hh"
#include "Types.hh"
#include <Fsa/Types.hh>
#include <Core/Hash.hh>

namespace Speech {
    class PhonemeSequenceAlignmentGenerator;
    class Alignment;
    class Confidences;
}

namespace Bliss {
    class Evaluator;
    class OrthographicParser;
};

namespace Lattice {

    /**
     *  Calculate the exact word accuracies.
     *  @param orth is the reference orthography.
     *
     *  The resulting word lattice contains the same hypotheses as @param lattice
     *  but the topology may be different.
     */
    ConstWordLatticeRef getExactWordAccuracy(
	ConstWordLatticeRef lattice,
	const std::string &orth,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToLemma,
	Fsa::ConstAutomatonRef lemmaToEval);

    /**
     *  The same as getExactWordAccuracy() but for the phoneme accuracy.
     *  Note that the phoneme accuracies are represented on word level, i.e.
     *  the output is a _word_ lattice.
     */
    ConstWordLatticeRef getExactPhonemeAccuracy(
	ConstWordLatticeRef lattice,
	const std::string &orth,
	Bliss::OrthographicParser *orthToLemma,
	Fsa::ConstAutomatonRef lemmaPronToPhoneme,
	Fsa::ConstAutomatonRef lemmaToPhoneme);

    /**
     *  Calculate the approximate word accuracies after Povey.
     *  @param correct represents the reference hypotheses (aka numerator lattice).
     *  @param shortPauses defines label ids which are discarded for the accuracy
     *  like for example silence.
     *  By default the accuracies are calculated on the lemma
     *  pronunciations. Optionally the accuracies can be calculated
     *  on the lemmata, cf. @param useLemmata. This is useful if
     *  @param correct does not contain all pronunciation variants
     *  (at the moment the numerator lattices do not contain pronunciation
     *  variants).
     *
     *  @param correct must have confidences as arc weights.
     *
     *  The resulting word lattice has the same topology and
     *  identical state ids as the @param lattice.
     */
    ConstWordLatticeRef getApproximateWordAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	bool useLemmata = false);

    /**
     *  Calculate the approximate phone accuracies after Povey.
     *  @param correct represents the reference hypotheses (aka numerator lattice).
     *  @param shortPauses defines label ids which are discarded for the accuracy
     *  like for example silence.
     *  Remarks: 1) Unlike Povey the phone boundaries are not given.
     *           2) The phone accuracies of a word arc are accumulated
     *              and stored as arc weight.
     *
     *  @param correct must have confidences as arc weights.
     *
     *  The resulting word lattice has the same topology and
     *  identical state ids as the @param lattice.
     */
    ConstWordLatticeRef getApproximatePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator);

    /**
     *  Calculate the approximate phone accuracies after Povey
	(as in getApproximatePhoneAccuracy), but the accuracy is masked
	based on the provided confidence alignment.
     */
    ConstWordLatticeRef getApproximatePhoneAccuracyMask(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const Speech::Confidences &mask,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator);

    /**
     *  Calculate the frame phone accuracies after Wessel.
     *  @param correct represents the reference hypotheses (aka numerator lattice).
     *  @param shortPauses defines label ids which are discarded for the accuracy
     *  like for example silence.
     *  Remarks: 1) The phone accuracies of a word arc are accumulated
     *              and stored as arc weight.
     *
     *  The resulting word lattice has the same topology and
     *  identical state ids as the @param lattice.
     */
    ConstWordLatticeRef getFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator,
	f32 normalization);

    /**
     *  Calculate the frame state accuracies (aka state-based Hamming distance).
     *  @param correct represents the reference hypotheses (aka numerator lattice).
     *  Remarks: 1) The accuracies are accumulated and stored as word arc weight.
     *
     *  The resulting word lattice has the same topology and
     *  identical state ids as @param lattice.
     */
    ConstWordLatticeRef getFrameStateAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator);

    /**
     *  Calculate the soft frame phone accuracies for @param lattice after Zheng.
     *  @param correct represents the reference hypotheses (aka numerator lattice),
     *  which contains the posteriors.
     *  @param shortPauses defines label ids which are discarded for the accuracy
     *  like for example silence.
     *  Remarks: 1) The phone accuracies of a word arc are accumulated
     *              and stored as arc weight.
     *
     *  The resulting word lattice has the same topology and
     *  identical state ids as the @param lattice.
     */
    ConstWordLatticeRef getSoftFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator);

    ConstWordLatticeRef getSoftFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	const Speech::Alignment &forcedAlignment,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator);

    /**
     *  Calculate the weighted frame phone accuracies according to MCE on state level.
     *  @param correct represents the reference hypotheses (aka numerator lattice).
     *  @param shortPauses defines label ids which are discarded for the accuracy
     *  like for example silence.
     *  Remarks: 1) The phone accuracies of a word arc are accumulated
     *              and stored as arc weight.
     *
     *  The resulting word lattice has the same topology and
     *  identical state ids as the @param lattice.
     */
    ConstWordLatticeRef getWeightedFramePhoneAccuracy(
	ConstWordLatticeRef lattice,
	ConstWordLatticeRef correct,
	const ShortPauses &shortPauses,
	Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> alignmentGenerator,
	f32 beta, f32 margin = 0);

}

#endif // _LATTICE_ACCURACY_HH
