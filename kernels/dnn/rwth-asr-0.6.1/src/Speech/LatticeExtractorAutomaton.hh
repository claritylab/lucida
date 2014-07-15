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
#ifndef _SPEECH_LATTICE_EXTRACTOR_AUTOMATON_HH
#define _SPEECH_LATTICE_EXTRACTOR_AUTOMATON_HH

namespace Speech {
namespace LatticeExtratorInternal {

    /* @todo: same as Speech/LatticeRescorerAutomaton.hh ? */

   /*
   * LatticeRescorerAutomaton: base class
   */
    class LatticeRescorerAutomaton : public Lattice::ModifyWordLattice
    {
	typedef Lattice::ModifyWordLattice Precursor;
    protected:
	virtual Fsa::Weight score(Fsa::StateId s, const Fsa::Arc &a) const = 0;
    public:
	LatticeRescorerAutomaton(Lattice::ConstWordLatticeRef lattice) :
	    Precursor(lattice) {
	    setProperties(Fsa::PropertySortedByWeight, Fsa::PropertyNone);
	}
	virtual ~LatticeRescorerAutomaton() {}

	virtual void modifyState(Fsa::State *sp) const {
	    if (sp->isFinal()) sp->weight_ = semiring()->one();
	    for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++ a)
		a->weight_ = score(sp->id(), *a);
	}
    };

    /*
     * LatticeRescorerAutomaton: with cache
     */
    class CachedLatticeRescorerAutomaton : public LatticeRescorerAutomaton
    {
	typedef LatticeRescorerAutomaton Precursor;
	struct Key {
	    std::string str;

	    Key(Fsa::LabelId input,
		const Lattice::WordBoundary &wbl,
		const Lattice::WordBoundary &wbr) {
		str = Core::form("%d|%d|%d|%d|%d",
				 input,
				 wbl.time(),
				 wbr.time(),
				 wbl.transit().final,
				 wbr.transit().initial);
	    }
	};
	typedef Core::StringHashMap<Fsa::Weight> Scores;
    private:
	mutable Scores cache_;
    public:
	CachedLatticeRescorerAutomaton(Lattice::ConstWordLatticeRef lattice) :
	    Precursor(lattice) {}
	virtual ~CachedLatticeRescorerAutomaton() {}

	virtual void modifyState(Fsa::State *sp) const {
	    if (sp->isFinal()) sp->weight_ = semiring()->one();
	    for (Fsa::State::iterator a = sp->begin(); a != sp->end(); ++ a) {
		const Key key(
		    a->input(),
		    (*wordBoundaries_)[sp->id()],
		    (*wordBoundaries_)[fsa_->getState(a->target())->id()]);
		Scores::const_iterator it = cache_.find(key.str);
		if (it == cache_.end())
		    cache_.insert(std::make_pair(key.str, score(sp->id(), *a)));
		a->weight_ = cache_[key.str];
	    }
	}
    };

}
}

#endif // _SPEECH_LATTICE_EXTRACTOR_AUTOMATON_HH
