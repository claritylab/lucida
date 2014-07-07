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
#include "Utilities.hh"
#include <Core/XmlStream.hh>
#include <Fsa/Determinize.hh>
#include <Fsa/Rational.hh>

namespace Lattice {

    bool dumpWordBoundaries(
	Core::Ref<const WordBoundaries> wordBoundaries,
	Core::XmlWriter &xo)
    {
	xo.generateFormattingHints();
	if (xo) {
	    xo << Core::XmlOpen("word-boundaries")
		+ Core::XmlAttribute("size", wordBoundaries->size());
	    for (size_t i = 0; i < wordBoundaries->size(); ++ i) {
		xo << Core::XmlOpen("state")
		    + Core::XmlAttribute("id", i)
		    + Core::XmlAttribute("time", wordBoundaries->time(i))
		    + Core::XmlAttribute("transit-final", wordBoundaries->transit(i).final)
		    + Core::XmlAttribute("transit-initial", wordBoundaries->transit(i).initial);
		xo << Core::XmlClose("state");
	    }
	    xo << Core::XmlClose("word-boundaries");
	    return xo;
	}
	return false;
    }

    bool dumpWordBoundaries(
	Core::Ref<const WordBoundaries> wordBoundaries,
	const std::string &file)
    {
	if (file.empty()) return false;
	Core::TextOutputStream o(file);
	Core::XmlWriter xo(o);
	return dumpWordBoundaries(wordBoundaries, xo);
    }

}
