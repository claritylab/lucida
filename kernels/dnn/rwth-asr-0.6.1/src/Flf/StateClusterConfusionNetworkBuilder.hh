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
#ifndef _FLF_STATE_CLUSTER_CONFUSION_NETWORK_HH
#define _FLF_STATE_CLUSTER_CONFUSION_NETWORK_HH

/**
   CN construction algorithm based on a state clustering,
   for details see my thesis, chapter 4.4.3 (The State-Cluster CN Construction Algorithm)
**/

#include "FlfCore/Lattice.hh"
#include "Network.hh"

namespace Flf {

    NodeRef createStateClusterCnBuilderNode(const std::string &name, const Core::Configuration &config);

} // namespace

#endif // _FLF_STATE_CLUSTER_CONFUSION_NETWORK_HH
