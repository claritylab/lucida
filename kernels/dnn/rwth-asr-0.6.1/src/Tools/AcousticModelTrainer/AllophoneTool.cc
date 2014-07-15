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
#include <Am/ClassicAcousticModel.hh>
#include <Am/ClassicStateModel.hh>
#include <Am/ClassicDecisionTree.hh>
#include <Bliss/Lexicon.hh>
#include <Core/Application.hh>
#include <Core/Component.hh>


class AllophoneTool : public Core::Application {
protected:
    // dynamic loading of state tying
    Am::ClassicStateTyingRef getStateTying(Core::Ref<Am::ClassicAcousticModel> am);
public:
    AllophoneTool() : Core::Application() { setTitle("allophone-tool"); }
    virtual std::string getUsage() const { return "Information about allophones and allophone to mixture mapping."; }
    int main(const std::vector<std::string> &arguments);
};

Am::ClassicStateTyingRef AllophoneTool::getStateTying(Core::Ref<Am::ClassicAcousticModel> am) {
    Am::ClassicStateTyingRef stateTying = am->stateTying();
    if (!stateTying) {
	am->load(Am::AcousticModel::noEmissions | Am::AcousticModel::noStateTransition);
	stateTying = am->stateTying();
    }
    return stateTying;
}

int AllophoneTool::main(const std::vector<std::string> &arguments) {
    Bliss::LexiconRef lexicon = Bliss::Lexicon::create(select("lexicon"));
    Core::Ref<Am::ClassicAcousticModel> am = Core::ref(new Am::ClassicAcousticModel(select("acoustic-model"), lexicon));
    {
	Core::Channel dump(config, "dump-allophone-properties");
	if (dump.isOpen()) {
	    Core::XmlWriter xml(dump);
	    xml.generateFormattingHints();
	    Am::PropertyMap propertyMap(am->stateModel());
	    propertyMap.writeXml(xml);
	}
    }
    {
	Core::Channel dump(config, "dump-allophones");
	if (dump.isOpen())
	    am->allophoneAlphabet()->dumpPlain(dump);
    }
    {
	Core::Channel dump(config, "dump-allophone-states");
	if (dump.isOpen())
	    am->allophoneStateAlphabet()->dumpPlain(dump);
    }
    {
	Core::Channel dump(config, "dump-state-tying");
	if (dump.isOpen()) {
	    Am::ClassicStateTyingRef stateTying = getStateTying(am);
	    verify(stateTying);
	    stateTying->dumpStateTying(dump);
	}
    }

#warning DEPRECATED!!! because of duplicated code
    {
	Core::Channel dump(config, "dump-classify");
	if (dump.isOpen()) {

	    warning("Deprecated: use *.dump-state-tying.channel=...");

	    Am::ClassicStateTyingRef stateTying = getStateTying(am);
	    Core::XmlWriter os(dump);
	    os.generateFormattingHints();
	    os << Core::XmlOpen("allophone-state-mapping");
	    Am::ConstAllophoneStateAlphabetRef alloStateAlphabet = am->allophoneStateAlphabet();
	    for (std::pair<Am::AllophoneStateIterator, Am::AllophoneStateIterator> it = alloStateAlphabet->allophoneStates();
		 it.first != it.second; ++it.first)
		os << Core::form("%-32s %9d %9d\n",
				 alloStateAlphabet->toString(it.first.allophoneState()).c_str(), it.first.id(),
				 stateTying->classify(it.first.allophoneState()));
	    os << Core::XmlClose("allophone-state-mapping");
	}
    }

    return EXIT_SUCCESS;
}

APPLICATION(AllophoneTool)
