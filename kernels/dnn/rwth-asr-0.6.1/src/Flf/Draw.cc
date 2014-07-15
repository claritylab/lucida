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

#include "FlfCore/Ftl.hh"
#include "Draw.hh"

namespace Flf {

    /**
     * visualization
     **/
    bool drawDot(ConstLatticeRef l, std::ostream &o, Fsa::Hint hint, bool progress) {
	if (!l) {
	    Core::Application::us()->warning("Cannot draw lattice, because the lattice is empty.");
	    return false;
	} else
	    return FtlWrapper::drawDot(l, o, hint, progress);

    }

    bool drawDot(ConstLatticeRef l, const std::string &file, Fsa::Hint hint, bool progress) {
	if (!l) {
	    Core::Application::us()->warning("Cannot draw lattice, because the lattice is empty.");
	    return false;
	} else
	    return FtlWrapper::drawDot(l, file, hint, progress);
    }

} // namespace Flf
