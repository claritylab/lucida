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
// $Id: check.cc 4878 2005-07-14 12:59:05Z heigold $

/**
 *
 */

#include <Bliss/Lexicon.hh>
#include <Core/Application.hh>
#include <Fsa/Archive.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Best.hh>
#include <Fsa/Output.hh>
#include <Fsa/Project.hh>
#include <Speech/ModelCombination.hh>

#include "Archive.hh"

using namespace Core;


// ===========================================================================
// Application

class TestApplication :
    public Application
{
public:
    std::string getUsage() const {
	return "short program";
    }

    int main(const std::vector<std::string> &arguments) {
	Speech::ModelCombination modelCombination(
	    select("model-combination"), Speech::ModelCombination::useLexicon);
	modelCombination.load();

	Lattice::ArchiveReader * archiveReader = Lattice::Archive::openForReading(
	    select("lattice-archive"),
	    modelCombination.lexicon());
	verify(archiveReader);

	for (Lattice::ArchiveReader::const_iterator it = archiveReader->files();
	     it; ++it) {
	    if ((it.name().at(it.name().size()-1) == '~'))
		continue;
	    if (it.name() == Lattice::Archive::latticeConfigFilename)
		continue;
	    if (it.name() == Fsa::ArchiveReader::alphabetFilename)
		continue;
	    log("read \"%s\"", it.name().c_str());
	    Fsa::ConstAutomatonRef f =
		archiveReader->get(it.name())->part(Lattice::WordLattice::acousticFsa);

	    //DEBUG
	    // Fsa::drawDot(Fsa::projectOutput(f), log());

	    Fsa::ConstAutomatonRef best = Fsa::projectOutput(Fsa::best(f));
	    Fsa::writeLinear(best, (XmlWriter&)log("score: %f\n", f32(Fsa::bestscore(best))));
	}

	delete archiveReader;
	return 0;
    }
} app; // <- You have to create ONE instance of the application

APPLICATION
