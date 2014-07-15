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
#include <strstream>
#include <Core/Application.hh>
#include "Basic.hh"
#include "Input.hh"
#include "Output.hh"
#include "Packed.hh"
#include "Python.hh"
#include "Static.hh"

namespace {

    class FsaLibrary : public Core::Application {
    public:
	FsaLibrary() {
	    setTitle("sprint-fsa-python");
	    setDefaultLoadConfigurationFile(false);
	    openLogging();
	}
	virtual ~FsaLibrary() {
	    closeLogging(false);
	}
	virtual int main(const std::vector<std::string> &arguments) {
	    return EXIT_SUCCESS;
	}
    } app;

}

const std::string info(Fsa::ConstAutomatonRef f, bool progress) {
    std::ostrstream o;
    Core::XmlWriter xo(o);
    Fsa::info(f, xo, progress);
    return o.str();
}

const std::string meminfo(Fsa::ConstAutomatonRef f) {
    std::ostrstream o;
    Core::XmlWriter xo(o);
    Fsa::memoryInfo(f, xo);
    return o.str();
}

const std::string draw(Fsa::ConstAutomatonRef f, bool dumpStates, bool progress) {
    std::ostrstream o;
    Fsa::drawDot(f, o, dumpStates, progress);
    return o.str();
}

Fsa::ConstAutomatonRef read(const std::string &file) {
    Fsa::StorageAutomaton *f;
    std::string tmp = file;
    if (std::string(tmp, 0, 7) == "packed:") {
	f = new Fsa::PackedAutomaton();
	tmp = std::string(tmp, 7);
    } else f = new Fsa::StaticAutomaton();
    if (Fsa::read(f, tmp)) return Fsa::ConstAutomatonRef(f);
    return Fsa::ConstAutomatonRef();
}
