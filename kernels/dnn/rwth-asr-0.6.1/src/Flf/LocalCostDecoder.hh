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
#ifndef _FLF_LOCAL_COST_DECODER_HH
#define _FLF_LOCAL_COST_DECODER_HH

/**
   Implementation of local costs of the first and the second type,
   for details see my thesis, chapter 3.3.3 (The Bayes Risk Decoding Framework with Local Cost Functions)

   The implementations of the following Levenshtein distance approximations:
   - Symmetrically Normalized Frame Errors (arc and path normalization, see my thesis, chapter 4.2.2)
   - Local Alignment based Errors (see my thesis, chapter 4.3)

   Other local cost functions have separate implementations, see *ConfusionNetworkBuilder.hh
**/

#include "FlfCore/Lattice.hh"
#include "FwdBwd.hh"
#include "Network.hh"


namespace Flf {

    /*
    std::pair<ConstLatticeRef, Score> decodeByApproximatedRisk(
	ConstLatticeRef hypSpaceL, ConstLatticeRef sumSpaceL, ConstFwdBwdRef sumSpaceFb,
	Score wordPenalty = 0.0,
	ScoreId confidenceId = Semiring::InvalidId);

    ConstLatticeRef extendByApproximatedRisk(
	ConstLatticeRef l, ConstFwdBwdRef fb,
	Score wordPenalty = 0.0,
	ScoreId scoreId = Semiring::InvalidId, ScoreId confidenceId = Semiring::InvalidId,
	RescoreMode rescoreMode = RescoreModeClone);
    */

    NodeRef createLocalCostDecoderNode(const std::string &name, const Core::Configuration &config);

} // namespace

#endif // _FLF_LOCAL_COST_DECODER_HH
