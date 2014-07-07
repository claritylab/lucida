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
		#include <Speech/Types.hh>

using namespace Lattice;

/**
 * BaseAccumulator
 */
template <class Trainer>
BaseAccumulator<Trainer>::BaseAccumulator(
    Trainer *trainer,
    Mm::Weight weightThreshold)
    :
    weightThreshold_(weightThreshold),
    trainer_(trainer)
{}

/**
 * AcousticAccumulator
 */
template <class Trainer>
AcousticAccumulator<Trainer>::AcousticAccumulator(
    ConstSegmentwiseFeaturesRef features,
    AlignmentGeneratorRef alignmentGenerator,
    Trainer *trainer,
    Mm::Weight weightThreshold,
    Core::Ref<const Am::AcousticModel> acousticModel) :
    Precursor(trainer, weightThreshold),
    alignmentGenerator_(alignmentGenerator),
    acousticModel_(acousticModel),
    features_(features),
    accumulationFeatures_(features)
{}

template <class Trainer>
const Speech::Alignment* AcousticAccumulator<Trainer>::getAlignment(
    Fsa::ConstStateRef from, const Fsa::Arc &a)
{
    require(this->wordBoundaries_);
    TimeframeIndex begtime = this->wordBoundaries_->time(from->id());
    if (begtime == Speech::InvalidTimeframeIndex) {
	return 0;
    }
    const Bliss::LemmaPronunciationAlphabet*
	alphabet = dynamic_cast<const Bliss::LemmaPronunciationAlphabet*>(
	    this->fsa_->getInputAlphabet().get());
    Bliss::Phoneme::Id leftContext(this->wordBoundaries_->transit(from->id()).final);
    const Bliss::LemmaPronunciation *pronunciation = alphabet->lemmaPronunciation(a.input());
    if (!pronunciation) {
	return 0;
    }
    TimeframeIndex endtime = this->wordBoundaries_->time(this->fsa_->getState(a.target())->id());
    verify_(endtime != Speech::InvalidTimeframeIndex);
    Bliss::Coarticulated<Bliss::LemmaPronunciation> coarticulatedPronunciation(
	*pronunciation, leftContext,
	this->wordBoundaries_->transit(this->fsa_->getState(a.target())->id()).initial);
    return alignmentGenerator_->getAlignment(coarticulatedPronunciation, begtime, endtime);
}

template <class Trainer>
void AcousticAccumulator<Trainer>::discoverState(Fsa::ConstStateRef sp)
{
    for (Fsa::State::const_iterator a = sp->begin(); a != sp->end(); ++ a) {
	const Alignment *alignment = getAlignment(sp, *a);
	if (alignment) {
	    f32 weight = f32(a->weight());
	    if (weight > this->weightThreshold_) {
	        std::vector<Speech::AlignmentItem>::const_iterator al = alignment->begin();
	        for (; al != alignment->end(); ++ al) {
		    process(al->time, acousticModel_->emissionIndex(al->emission), weight);
	        }
	    }
        }
    }
}

/**
 * CachedAcousticAccumulator
 */
template <class Trainer>
CachedAcousticAccumulator<Trainer>::CachedAcousticAccumulator(
    typename Precursor::ConstSegmentwiseFeaturesRef features,
    typename Precursor::AlignmentGeneratorRef alignmentGenerator,
    Trainer *trainer,
    Mm::Weight weightThreshold,
    Core::Ref<const Am::AcousticModel> acousticModel) :
    Precursor(features, alignmentGenerator, trainer, weightThreshold, acousticModel)
{}

template <class Trainer>
void CachedAcousticAccumulator<Trainer>::process(
    typename Precursor::TimeframeIndex t,
    Mm::MixtureIndex m,
    Mm::Weight w)
{
    collector_.collect(Key(t, m), w);
}

template <class Trainer>
void CachedAcousticAccumulator<Trainer>::finish()
{
    for (Collector::const_iterator c = collector_.begin(); c != collector_.end(); ++ c) {
        Core::Ref<const Speech::Feature> af = (*this->accumulationFeatures_)[c->first.t];
	this->accumulate(af->mainStream(), c->first.m, c->second);
    }
}
