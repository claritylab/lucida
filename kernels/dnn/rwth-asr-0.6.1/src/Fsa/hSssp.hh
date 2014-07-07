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
#ifndef _H_FSA_SSSP_HH
#define _H_FSA_SSSP_HH

#include "Types.hh"
#include <Core/Vector.hh>

namespace Ftl {

    /*
     * TODO:
     * - make state potentials reference counted?
     */
    template<class Weight>
    class StatePotentials : public Core::Vector<Weight> {
	typedef Core::Vector<Weight> Precursor;
    public:
	StatePotentials() : Precursor() {}
	StatePotentials(size_t size) : Precursor(size) {}
	StatePotentials(size_t size, const Weight &t) : Precursor(size, t) {}
    };

    template<class _Automaton>
    struct SsspArcFilter {
	SsspArcFilter() {}
	virtual ~SsspArcFilter() {}
	typedef typename _Automaton::Arc _Arc;
	virtual bool operator() (const _Arc &a) const { return true; }
    };

    template<class _Automaton>
    class IsInputLabel : public SsspArcFilter<_Automaton> {
	typedef SsspArcFilter<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Arc _Arc;
    private:
	Fsa::LabelId label_;
    public:
	IsInputLabel(Fsa::LabelId label) : label_(label) {}
	virtual bool operator() (const _Arc &a) const { return (a.input() == label_); }
    };

    template<class _Automaton>
    class AreBothLabels : public SsspArcFilter<_Automaton> {
	typedef SsspArcFilter<_Automaton> Precursor;
    public:
	typedef typename _Automaton::Arc _Arc;
    private:
	Fsa::LabelId label_;
    public:
	AreBothLabels(Fsa::LabelId label) : label_(label) {}
	virtual bool operator() (const _Arc &a) const {
	    return (a.input() == label_) && (a.output() == label_);
	}
    };
} // namespace Ftl

#endif // _H_FSA_SSSP_HH
