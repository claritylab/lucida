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
#ifndef _T_FSA_DRAW_HH
#define _T_FSA_DRAW_HH

#include <iostream>

#include "Types.hh"

namespace Ftl {
    /**
     * visualization
     **/
    template<class _Automaton>
    class DrawDotDfsState;

    template<class _Automaton>
    class DotDrawer {
    private:
	friend class DrawDotDfsState<_Automaton>;
	std::ostream &os_;
	Fsa::Hint hints_;
	bool progress_;
    public:
	DotDrawer(std::ostream&, Fsa::Hint hints = Fsa::HintNone, bool progress = false);
	void setHint(Fsa::Hint hint)    { hints_ |= hint;  }
	void unsetHint(Fsa::Hint hint)  { hints_ &= ~hint; }
	void progress(bool bb)          { progress_ = bb;  }
	bool draw(typename _Automaton::ConstRef);
    };

    template<class _Automaton>
    bool drawDot(typename _Automaton::ConstRef f, std::ostream &o, Fsa::Hint hint = Fsa::HintNone, bool progress = false);
    template<class _Automaton>
    bool drawDot(typename _Automaton::ConstRef f, const std::string &file, Fsa::Hint hint = Fsa::HintNone, bool progress = false);
} // namespace Ftl

#include "tDraw.cc"

#endif // _T_FSA_DRAW_HH
