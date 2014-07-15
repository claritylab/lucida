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
#include "ClassicAcousticModel.hh"
#include "ClassicStateModel.hh"
#include "ClassicTransducerBuilder.hh"
#include "Module.hh"
#include <Fsa/Input.hh>
#include <Fsa/Output.hh>
#include <Fsa/Basic.hh>
#include <Fsa/Static.hh>

class TestApplication : public Core::Application {
public:
    TestApplication() : Core::Application() { setTitle("check"); }
    virtual std::string getUsage() const { return "short program to test Am features\n"; }
    int main(const std::vector<std::string> &arguments);
};

int TestApplication::main(const std::vector<std::string> &arguments) {
    INIT_MODULE(Am)

    Bliss::LexiconRef lexicon = Bliss::Lexicon::create(select("lexicon"));
    Core::Ref<Am::ClassicAcousticModel> am = Core::ref(new Am::ClassicAcousticModel(config, lexicon));
    am->allophoneAlphabet()->dump(log());

    return EXIT_SUCCESS;
}

APPLICATION(TestApplication)
