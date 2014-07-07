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
#include "tDraw.hh"
#include "tOutput.hh"
#include "Resources.hh"
#include "Output.hh"

namespace Fsa {
    /**
     * storage
     **/
    bool write(      ConstAutomatonRef f, const std::string &format, std::ostream &o, StoredComponents what, bool progress)
    { return Ftl::write<Automaton>(getResources(), f, format, o, what, progress); }

    bool write(      ConstAutomatonRef f, const std::string &file, StoredComponents what, bool progress)
    { return Ftl::write<Automaton>(getResources(), f, file, what, progress); }

    bool writeAtt(   ConstAutomatonRef f, std::ostream &o, StoredComponents what, bool progress)
    { return Ftl::writeAtt<Automaton>(getResources(), f, o, what, progress); }

    bool writeAtt(   ConstAutomatonRef f, const std::string &file, StoredComponents what, bool progress)
    { return Ftl::writeAtt<Automaton>(getResources(), f, file, what, progress); }

    bool writeBinary(ConstAutomatonRef f, std::ostream &o, StoredComponents what, bool progress)
    { return Ftl::writeBinary<Automaton>(getResources(), f, o, what, progress); }

    bool writeBinary(ConstAutomatonRef f, const std::string &file, StoredComponents what, bool progress)
    { return Ftl::writeBinary<Automaton>(getResources(), f, file, what, progress); }

    bool writeLinear(ConstAutomatonRef f, std::ostream &o, StoredComponents what, bool progress, bool printAll)
    { return Ftl::writeLinear<Automaton>(getResources(), f, o, what, progress, printAll); }

    bool writeLinear(ConstAutomatonRef f, const std::string &file, StoredComponents what, bool progress)
    { return Ftl::writeLinear<Automaton>(getResources(), f, file, what, progress); }

    bool writeXml(   ConstAutomatonRef f, std::ostream &o, StoredComponents what, bool progress)
    { return Ftl::writeXml<Automaton>(getResources(), f, o, what, progress); }

    bool writeXml(   ConstAutomatonRef f, const std::string &file, StoredComponents what, bool progress)
    { return Ftl::writeXml<Automaton>(getResources(), f, file, what, progress); }

    bool writeTrXml(  ConstAutomatonRef f, std::ostream &o, StoredComponents what, bool progress)
    { return Ftl::writeTrXml<Automaton>(getResources(), f, o, what, progress); }

    bool writeTrXml(  ConstAutomatonRef f, const std::string &file, StoredComponents what, bool progress)
    { return Ftl::writeTrXml<Automaton>(getResources(), f, file, what, progress); }




    /**
     * visualization
     **/
    bool drawDot(ConstAutomatonRef f, std::ostream &o, Fsa::Hint hint, bool progress)
    { return Ftl::drawDot<Automaton>(f, o, hint, progress); }
    bool drawDot(ConstAutomatonRef f, const std::string &file, Fsa::Hint hint, bool progress)
    { return Ftl::drawDot<Automaton>(f, file, hint, progress); }
} // namespace Fsa
