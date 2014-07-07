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
#ifndef _FLF_RESCORE_HH
#define _FLF_RESCORE_HH

#include "Bliss/Lexicon.hh"
#include "FlfCore/Lattice.hh"
#include "Network.hh"


namespace Flf {
    /**
     * Reduce many scores to a single score.
     **/
    // ConstLatticeRef reduce(ConstLatticeRef l, const ScoreIdList &fromIds, ScoreId toId, RescoreMode rescoreMode = RescoreModeClone);
    NodeRef createReduceScoresNode(const std::string &name, const Core::Configuration &config);


    /**
     * arithmetic operations nodes
     **/
    /**
     * f(x,c) = x + c
     **/
    ConstLatticeRef add(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode = RescoreModeClone);
    NodeRef createAddNode(const std::string &name, const Core::Configuration &config);
    /**
     * f(x,c) = c * x
     **/
    ConstLatticeRef multiply(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode = RescoreModeClone);
    NodeRef createMultiplyNode(const std::string &name, const Core::Configuration &config);
    /**
     * f(x,c) = exp(c * x)
     **/
    ConstLatticeRef exp(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode = RescoreModeClone);
    NodeRef createExpNode(const std::string &name, const Core::Configuration &config);
    /**
     * f(x,c) = c * log(x)
     **/
    ConstLatticeRef log(ConstLatticeRef l, ScoreId id, Score c, RescoreMode rescoreMode = RescoreModeClone);
    NodeRef createLogNode(const std::string &name, const Core::Configuration &config);


    /**
     * extend by fixed or lemma dependent penalties
     **/
    NodeRef createExtendByPenaltyNode(const std::string &name, const Core::Configuration &config);


    /**
     * extend by pronunciation score
     **/
    ConstLatticeRef extendByPronunciationScore(
	ConstLatticeRef l,
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpAlphabet,
	ScoreId id,  Score scale,
	RescoreMode rescoreMode = RescoreModeClone);
    NodeRef createExtendByPronunciationScoreNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_RESCORE_HH
