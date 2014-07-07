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
#include <Core/Vector.hh>

#include "Basic.hh"
#include "Traverse.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    void TraverseState::traverseDfs() {
	Core::Vector<bool> visited;
	Core::Vector<Fsa::StateId> S;
	S.push_back(l->initialStateId());
	while (!S.empty()) {
	    Fsa::StateId sid(S.back());
	    S.pop_back();
	    visited.grow(sid, false);
	    if (visited[sid])
		continue;
	    visited[sid] = true;
	    ConstStateRef sr = l->getState(sid);
	    exploreState(sr);
	    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a) {
		exploreArc(sr, *a);
		S.push_back(a->target());
	    }
	}
    }

    void TraverseState::traverseInTopologicalOrder() {
	if (!sortTopologically(l))
	    Core::Application::us()->criticalError(
		"\"%s\" has no topological order", l->describe().c_str());
	for (StateMap::const_iterator itSid = l->getTopologicalSort()->begin(),
		 endSid = l->getTopologicalSort()->end(); itSid != endSid; ++itSid) {
	    ConstStateRef sr = l->getState(*itSid);
	    exploreState(sr);
	    for (State::const_iterator a = sr->begin(); a != sr->end(); ++a)
		exploreArc(sr, *a);
	}
    }

    void TraverseState::traverse() {
	if (l->getTopologicalSort())
	    traverseInTopologicalOrder();
	else
	    traverseDfs();
    }
    // -------------------------------------------------------------------------

} // namespace Flf
