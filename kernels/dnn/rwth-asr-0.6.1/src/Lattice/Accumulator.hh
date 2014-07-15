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
#ifndef _LATTICE_ACCUMULATOR_HH
#define _LATTICE_ACCUMULATOR_HH

#include "Lattice.hh"
#include <Speech/Alignment.hh>
#include <Speech/PhonemeSequenceAlignmentGenerator.hh>
#include <Speech/DiscriminativeMixtureSetTrainer.hh>
#include <Mm/Types.hh>
#include <Mm/DensityToWeightMap.hh>

namespace Lattice {

    /**
     * BaseAccumulator
     */
    template <class Trainer>
    class BaseAccumulator : public DfsState
    {
    public:
	typedef Mm::DensityToWeightMap PosteriorsAndDensities;
    protected:
	Mm::Weight weightThreshold_;
	Trainer *trainer_;
    protected:
	virtual void accumulate(Core::Ref<const Mm::Feature::Vector> f, Mm::MixtureIndex m, Mm::Weight w) {
	    defect();
	}
	virtual void accumulate(Core::Ref<const Mm::Feature::Vector> f, const PosteriorsAndDensities &p) {
	    defect();
	}
	/*
	virtual void accumulate(Core::Ref<const Sparse::Feature::SparseVector> sf, Mm::MixtureIndex m, Mm::Weight w) {
	    defect();
	}
	*/
	virtual void reset() {}
    public:
	BaseAccumulator(Trainer *, Mm::Weight);
	virtual ~BaseAccumulator() {}

	virtual void setFsa(Fsa::ConstAutomatonRef fsa) {
	    fsa_ = fsa;
	}
	void setWordBoundaries(Core::Ref<const WordBoundaries> wordBoundaries) {
	    wordBoundaries_ = wordBoundaries;
	}
	void work() {
	    reset();
	    dfs();
	}
    };

    /**
     * AcousticAccumulator
     */
    template <class Trainer>
    class AcousticAccumulator : public BaseAccumulator<Trainer>
    {
	typedef BaseAccumulator<Trainer> Precursor;
    protected:
	typedef Core::Ref<Speech::PhonemeSequenceAlignmentGenerator> AlignmentGeneratorRef;
	typedef Speech::Alignment Alignment;
	typedef Speech::TimeframeIndex TimeframeIndex;
	typedef Speech::ConstSegmentwiseFeaturesRef ConstSegmentwiseFeaturesRef;
    protected:
	AlignmentGeneratorRef alignmentGenerator_;
	Core::Ref<const Am::AcousticModel> acousticModel_;
	ConstSegmentwiseFeaturesRef features_;
	ConstSegmentwiseFeaturesRef accumulationFeatures_;
    protected:
	virtual const Alignment* getAlignment(Fsa::ConstStateRef from, const Fsa::Arc &a);
	virtual void process(TimeframeIndex t, Mm::MixtureIndex m, Mm::Weight w) {
	    this->accumulate((*accumulationFeatures_)[t]->mainStream(), m, w);
	}
	virtual void reset() {}
    public:
	AcousticAccumulator(
	    ConstSegmentwiseFeaturesRef,
	    AlignmentGeneratorRef, Trainer *,
	    Mm::Weight, Core::Ref<const Am::AcousticModel>);
	virtual ~AcousticAccumulator() {}

	virtual void discoverState(Fsa::ConstStateRef sp);
	void setAccumulationFeatures(ConstSegmentwiseFeaturesRef accumulationFeatures) {
	    accumulationFeatures_ = accumulationFeatures;
	}
    };

    /**
     * CachedAcousticAccumulator
     */
    struct Key {
	Speech::TimeframeIndex t;
	Mm::MixtureIndex m;
	Key(Speech::TimeframeIndex _t, Mm::MixtureIndex _m) : t(_t), m(_m) {}
    };
    struct KeyHash
    {
	size_t operator() (const Key &k) const {
	    return ((k.t & 0x0000ffff) | (k.m << 16));
	}
    };
    struct KeyEquality
    {
	bool operator() (const Key &lhs, const Key &rhs) const {
	    return (lhs.t == rhs.t) and (lhs.m == rhs.m);
	}
    };
    class Collector : public Core::hash_map<Key, Mm::Weight, KeyHash, KeyEquality>
    {
    public:
	Collector() {}

	void collect(const Key &key, Mm::Weight w) {
	    if (find(key) == end()) {
		(*this)[key] = w;
	    } else {
		(*this)[key] += w;
	    }
	}
    };

    template <class Trainer>
    class CachedAcousticAccumulator : public AcousticAccumulator<Trainer>
    {
	typedef AcousticAccumulator<Trainer> Precursor;
    protected:
	Collector collector_;
    protected:
	virtual void process(typename Precursor::TimeframeIndex, Mm::MixtureIndex, Mm::Weight);
	virtual void reset() { collector_.clear(); }
    public:
	CachedAcousticAccumulator(
	    typename Precursor::ConstSegmentwiseFeaturesRef,
	    typename Precursor::AlignmentGeneratorRef, Trainer *,
	    Mm::Weight, Core::Ref<const Am::AcousticModel>);
	virtual ~CachedAcousticAccumulator() {}
	virtual void finish();
    };


#include "Accumulator.tcc"

} // namespace Lattice

#endif //_LATTICE_ACCUMULATOR_HH
