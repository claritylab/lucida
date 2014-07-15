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
#include <cstring>
#include <Core/Application.hh>
#include <Core/CompressedStream.hh>
#include <Core/Unicode.hh>
#include <Core/Utility.hh>

#include "tInput.hh"
#include "Alphabet.hh"
#include "Input.hh"
#include "Output.hh"
#include "Packed.hh"
#include "Resources.hh"
#include "Static.hh"
#include "Storage.hh"
#include "Types.hh"



namespace Fsa {
    ConstAutomatonRef read(const std::string &argument, ConstSemiringRef semiring) {
	StorageAutomaton *f;
	std::string tmp = argument;
	if (std::string(tmp, 0, 7) == "packed:") {
	    f = new PackedAutomaton();
	    tmp = std::string(tmp, 7);
	} else f = new StaticAutomaton();
	f->setSemiring(semiring);
	if (!read(f, tmp)) {
	    Core::Application::us()->error("could not load fsa '%s'.", tmp.c_str());
	    delete f;
	    return ConstAutomatonRef();
	}
	return ConstAutomatonRef(f);
    }

    bool read(StorageAutomaton *f, const std::string &file)
    { return Ftl::read<Automaton>(getResources(), f, file); }

    bool read(StorageAutomaton *f, const std::string &format, std::istream &i)
    { return Ftl::read<Automaton>(getResources(), f, format, i); }

    bool readAtt(StorageAutomaton *f, std::istream &i)
    { return Ftl::readAtt<Automaton>(getResources(), f, i); }

    bool readBinary(StorageAutomaton *f, std::istream &i)
    { return Ftl::readBinary<Automaton>(getResources(), f, i); }

    bool readLinear(StorageAutomaton *f, std::istream &i)
    { return Ftl::readLinear<Automaton>(getResources(), f, i); }

    bool readXml(StorageAutomaton *f, std::istream &i)
    { return Ftl::readXml<Automaton>(getResources(), f, i); }


} // namespace Fsa
