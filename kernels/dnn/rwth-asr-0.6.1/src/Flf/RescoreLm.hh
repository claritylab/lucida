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
#ifndef _FLF_RESCORE_LM
#define _FLF_RESCORE_LM

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Lm {
    class LanguageModel;
}

namespace Bliss {
    class Lemma;
}

namespace Flf {
    /**
     * Performs a time-synchronous rescoring.
     * The input lattice may be a mesh lattice, in which case
     * a huge lattice may be expanded. Pruning must be used to
     * prevent the resulting lattice from exploding in that case.
     *
     * wordEndBeam and wordEndLimit are equivalent to word end pruning used during
     * standard decoding (wordEndBeam is relative to the LM scale)
     * */
    ConstLatticeRef decodeRescoreLm(ConstLatticeRef lat, Core::Ref<Lm::LanguageModel> lm,
				  float wordEndBeam = 20,
				  u32 wordEndLimit = 50000,
				  const std::vector<const Bliss::Lemma*>& prefix = std::vector<const Bliss::Lemma*>(),
				  const std::vector<const Bliss::Lemma*>& suffix = std::vector<const Bliss::Lemma*>());

    NodeRef createDecodeRescoreLmNode(const std::string &name, const Core::Configuration &config);
} // namespace Flf

#endif
