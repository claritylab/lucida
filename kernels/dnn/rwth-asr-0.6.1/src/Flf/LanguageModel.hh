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
#ifndef _FLF_LANGUAGE_MODEL_HH
#define _FLF_LANGUAGE_MODEL_HH

#include <Lm/LanguageModel.hh>
#include <Lm/ScaledLanguageModel.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"


namespace Flf {

    typedef Core::Ref<const Lm::LanguageModel> ConstLanguageModelRef;
    typedef Core::Ref<const Lm::ScaledLanguageModel> ConstScaledLanguageModelRef;


    /**
     * composition of lattice and lm
     *
     * For more information please read Lm/Compose.hh.
     *
     * The result is almost equvivalent to the lm-rescorer implementation in
     * LanguageModelRescorer.hh; a slight difference concers the syntax-emission-probabilities
     * (see the comments in Compose.cc).
     *
     * Additional features:
     * - the result of the composition has exactly as many final states as the input lattice
     **/

    ConstLatticeRef composeWithLm(ConstLatticeRef l, ConstLanguageModelRef lm, ScoreId id,
				  Score scale = Semiring::DefaultScale, bool forceSentenceEnd = false);

    NodeRef createComposeWithLmNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_LANGUAGE_MODEL_HH
