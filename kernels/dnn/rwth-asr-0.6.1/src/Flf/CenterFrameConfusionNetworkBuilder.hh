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
#ifndef _FLF_CENTER_FRAME_CONFUSION_NETWORK_BUILDER_HH
#define _FLF_CENTER_FRAME_CONFUSION_NETWORK_BUILDER_HH

/**
   CN construction algorithm based on the selection of a frame representing the center of the next CN slot,
   for details see my thesis, chapter 4.4.4 (The Center-Frame CN Construction Algorithm)
**/

#include "FlfCore/Lattice.hh"
#include "ConfusionNetwork.hh"
#include "Network.hh"

namespace Flf {

	class CenterFrameCnFactory : public ConfusionNetworkFactory {
		friend class ConfusionNetworkFactory;
	private:
		struct Internal;
		Internal *internal_;
	private:
		CenterFrameCnFactory(const Core::Configuration &config);
		CenterFrameCnFactory();
	public:
		virtual ~CenterFrameCnFactory();
		virtual void dump(std::ostream &os) const;
		/*
		  fCN is used for initialization of the algorithm (see cc-file);
		  if not given it is calculated from lattice (and FB scores)
		*/
		virtual void build(ConstLatticeRef l, ConstFwdBwdRef fb = ConstFwdBwdRef());
		virtual void reset();
		virtual ConstConfusionNetworkRef getCn(ScoreId posteriorId, bool mapping) const;
		virtual std::pair<ConstConfusionNetworkRef, ConstLatticeRef> getNormalizedCn(ScoreId confidenceId, bool mapping) const;
	};


    NodeRef createCenterFrameCnBuilderNode(const std::string &name, const Core::Configuration &config);

} // namespace

#endif // _FLF_CENTER_FRAME_CONFUSION_NETWORK_BUILDER_HH
