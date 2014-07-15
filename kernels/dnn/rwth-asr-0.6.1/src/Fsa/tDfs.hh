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
#ifndef _T_FSA_DFS_HH
#define _T_FSA_DFS_HH

#include <Core/ProgressIndicator.hh>
#include "Types.hh"
#include <Core/Vector.hh>

namespace Ftl {

    /**
     * Due to the principle of on-demand computation, exploring
     * automata becomes an important issue as states are not directly
     * accessable. Fsa provides this general implementation of
     * depth-first search and makes intense use of it itself. Details
     * of the algorithm can be looked up in (Cormen et al.). It stores
     * state information in a single vector. DfsState uses the design
     * pattern of a state. Intended use is as follows:
     *
     * 1. derive a specialized depth-first search state from DfsState
     * 2. overload callback methods appropriately
     * 3. create an instance of your state and pass automaton to it
     * 4. call dfs to process your automaton
     *
     * The use of the state design pattern allows that information
     * gathered upon traversal of the automaton is being kept after
     * traversal.
     **/
    template<class _Automaton>
    class DfsState {
    public:
	typedef typename _Automaton::Arc _Arc;
	typedef typename _Automaton::State _State;
	typedef typename _Automaton::ConstStateRef _ConstStateRef;
	typedef typename _Automaton::ConstRef _ConstAutomatonRef;
	/**
	 * The possible colors of a state.
	 **/
	typedef Fsa::StateId Color;
	static const Color White = 0xc0000000; // unprocessed
	static const Color Gray  = 0x80000000; // discovered
	static const Color Black = 0x40000000; // finished

    private:
	void setColor(Fsa::StateId s, Color c) { info_[s] = (info_[s] & Fsa::StateIdMask) | c; }
	void setPredecessor(Fsa::StateId s, Fsa::StateId p) { info_[s] = (info_[s] & ~Fsa::StateIdMask) | p; }

    protected:
	_ConstAutomatonRef fsa_;
	Core::Vector<Fsa::StateId> info_;

    public:
	DfsState(_ConstAutomatonRef f) : fsa_(f) {}
	virtual ~DfsState() {}

	/**
	 * Called upon first discovery of a state.
	 * @param sp reference to state
	 **/
	virtual void discoverState(_ConstStateRef sp) {}

	/**
	 * Called after all subtrees starting at a previously
	 * discovered state have been processed.
	 * @param s index of the state
	 **/
	virtual void finishState(Fsa::StateId s) {}

	/**
	 * Called upon discovery of a tree arc.
	 * Tree arcs never close a loop.  For this reason they are
	 * "forward" arcs.
	 * @param from reference to the start state
	 * @param a reference to the arc
	 **/
	virtual void exploreTreeArc(_ConstStateRef from, const _Arc &a) {}

	/**
	 * Called upon discovery of a non-tree arc.
	 * Non-tree arcs may close a loop, but this is not
	 * necessarily the case.  To be precise, a loop occurs if the
	 * target state is grey.  Such arcs are called "backward"
	 * arcs.  Trivial loops (source state is target state) are
	 * also non tree arcs.
	 * @param from reference to the start state
	 * @param a reference to the arc
	 **/
	virtual void exploreNonTreeArc(_ConstStateRef from, const _Arc &a) {}

	/**
	 * Called after all subtrees starting at a previously
	 * explored arc have been processed. Note: This function
	 * is called only within the recursive traversal.
	 * @param from reference to the start state
	 * @param a reference to the arc
	 */
	virtual void finishArc(_ConstStateRef from, const _Arc &a) {}

	/**
	 * Called after all states reachable from the initial state
	 * have been processed.
	 **/
	virtual void finish() {}

	/**
	 * Request the highest state id of the automaton.
	 * @return the state id
	 **/
	Fsa::StateId maxStateId() const { return info_.size() - 1; }

	Fsa::StateId size() const { return info_.size(); }

	/**
	 * Request the color of a state.
	 * @param s the state id
	 * @return the color of the state (see above)
	 **/
	Color color(Fsa::StateId s) const { return info_[s] & ~Fsa::StateIdMask; }

	/**
	 * Request the predecessor of a state.
	 * @param s the state id
	 * @return the predecessor of a state
	 **/
	Fsa::StateId predecessor(Fsa::StateId s) const { return info_[s] & Fsa::StateIdMask; }

	/**
	 * Initiate searching the automaton.
	 * @param p a pointer to a progress indicator
	 **/
	void dfs(Core::ProgressIndicator *p = 0);

	/**
	 * Initiate searching the automaton.
	 * Unlike dfs(), recursiveDfs() visits the arc in the same order
	 * like a recursive depth first search.
	 * @param p a pointer to a progress indicator
	 **/
	void recursiveDfs(Core::ProgressIndicator *p = 0);
    };

} // namespace Ftl

#include "tDfs.cc"

#endif // _T_FSA_DFS_HH
