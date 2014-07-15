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
#include "tInput.hh"
#include "tOutput.hh"
#include "Resources.hh"

namespace Fsa {
    namespace { Resources *r = 0; }
    Resources & getResources() {
	if (!r) {
	    r = new Resources(Core::Application::us()->getConfiguration());
	    for (Core::Choice::const_iterator it = SemiringTypeChoice.begin();
		 it != SemiringTypeChoice.end(); ++it) {
		ConstSemiringRef semiring = getSemiring(SemiringType(it->value()));
		r->registerSemiring(semiring);
		r->registerSemiring(Fsa::SemiringType(it->value()), semiring);
	    }
	    r->registerSemiring(TropicalSemiring, true);
	    r->registerFormat(
		new Resources::Format(
		    "att",
		    "AT&T's ascii based format",
		    Ftl::readAtt<Automaton>, Ftl::writeAtt<Automaton>));
	    r->registerFormat(
		new Resources::Format(
		    "bin",
		    "binary format",
		    Ftl::readBinary<Automaton>, Ftl::writeBinary<Automaton>));
	    r->registerFormat(
		new Resources::Format(
		    "lin",
		    "ascii based format, only for linear automatons",
		    Ftl::readLinear<Automaton>, Ftl::writeLinear<Automaton>));
	    r->registerFormat(
		new Resources::Format(
		    "xml",
		    "xml based standard format",
		    Ftl::readXml<Automaton>, Ftl::writeXml<Automaton>), true);
	    r->registerFormat(
		new Resources::Format(
		    "trxml",
		    "xml based format used by the translators group",
		    0, Ftl::writeTrXml<Automaton>));
	}
	return *r;
    }

} // namespace Fsa
