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
#ifndef _FLF_PIVOT_ARC_CONFUSION_NETWORK_BUILDER_HH
#define _FLF_PIVOT_ARC_CONFUSION_NETWORK_BUILDER_HH

/**
   CN construction algorithm based on a direct arc clustering driven by a pivot element,
   for details see my thesis, chapter 4.4.2 (The Arc-Cluster CN Construction Algorithm)
**/

#include "FlfCore/Lattice.hh"
#include "ConfusionNetwork.hh"
#include "Network.hh"

namespace Flf {

	class PivotArcCnFactory : public ConfusionNetworkFactory {
		friend class ConfusionNetworkFactory;
	private:
		struct Internal;
		Internal *internal_;
	protected:
		PivotArcCnFactory(const Core::Configuration &config);
		PivotArcCnFactory();
	public:
		virtual ~PivotArcCnFactory();
		virtual void dump(std::ostream &os) const;
		virtual void build(ConstLatticeRef l, ConstFwdBwdRef fb);
		virtual void reset();

		virtual ConstConfusionNetworkRef getCn(ScoreId posteriorId, bool mapping) const;
		virtual std::pair<ConstConfusionNetworkRef, ConstLatticeRef> getNormalizedCn(ScoreId confidenceId, bool mapping) const;
	};

    NodeRef createPivotArcCnBuilderNode(const std::string &name, const Core::Configuration &config);

} // namespace

#endif // _FLF_PIVOT_ARC_CONFUSION_NETWORK_BUILDER_HH
