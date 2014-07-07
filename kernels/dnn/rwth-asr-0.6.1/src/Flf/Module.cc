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
#include <Search/Module.hh>
#include "Module.hh"
#include "Lexicon.hh"
#include "LatticeHandler.hh"

namespace Flf {

    Module_::Module_() : lexicon_(0), network_(0), processor_(0) {}
    Module_::~Module_() { delete lexicon_; }

    void Module_::init() {
	lexicon_ = new Lexicon(Core::Configuration(Core::Application::us()->getConfiguration(), "lexicon"));
    }

    const Lexicon *Module_::lexicon() {
	return lexicon_;
    }

    void Module_::setLexicon(Lexicon *l) {
	lexicon_ = l;
    }

    Network *Module_::network() {
	return network_;
    }

    void Module_::setNetwork(Network *n) {
	network_ = n;
    }

    Processor *Module_::processor() {
	return processor_;
    }

    void Module_::setProcessor(Processor *p) {
	processor_ = p;
    }

    Flf::LatticeHandler* Module_::createLatticeHandler(const Core::Configuration &c) const {
	return new Flf::LatticeHandler(c, Search::Module::instance().createLatticeHandler(c));
    }

} // namespace Flf
