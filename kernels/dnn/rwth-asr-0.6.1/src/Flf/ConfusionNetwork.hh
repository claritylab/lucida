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
#ifndef _FLF_CONFUSION_NETWORK_HH
#define _FLF_CONFUSION_NETWORK_HH

#include "FlfCore/Lattice.hh"
#include "FwdBwd.hh"
#include "Network.hh"

namespace Flf {

	class ConfusionNetworkFactory;
	typedef Core::Ref<ConfusionNetworkFactory> ConfusionNetworkFactoryRef;
	class ConfusionNetworkFactory : public Core::ReferenceCounted {
	public:
		typedef enum CnAlgorithm {
			CnAlgorithmPivotArcClustering,
			CnAlgorithmStateClustering,
			CnAlgorithmCenterFrame
		} CnAlgorithmType;
	public:
		virtual ~ConfusionNetworkFactory();

		virtual void dump(std::ostream &os) const;
		/*
		  Perform the arc clustering
		*/
		virtual void build(ConstLatticeRef l, ConstFwdBwdRef fb = ConstFwdBwdRef()) = 0;
		/*
		  Reset the arc clustering
		*/
		virtual void reset() = 0;
		/*
		  Get a sausage, i.e. the slots are not normalized
		*/
		virtual ConstConfusionNetworkRef getCn(
			ScoreId posteriorId = Semiring::InvalidId, bool mapping = false) const = 0;
		/*
		  Get a CN, i.e. the slots are normalized, and
		  get the CN decoding result.
		*/
		virtual std::pair<ConstConfusionNetworkRef, ConstLatticeRef> getNormalizedCn(
			ScoreId confidenceId = Semiring::InvalidId, bool mapping = false) const = 0;

		/*
		  Create CN factory
		*/
		static ConfusionNetworkFactoryRef create(const Core::Configuration &config);

		/*
		  Create CN factory using the given algorithm with default parameters
		*/
		static ConfusionNetworkFactoryRef create(CnAlgorithmType cnAlgorithm);
	};




    /**
     * Build a confusion network from a lattice with "sausage" topology.
     * Remark: a linear lattice has a "sausage" topology.
     **/
    ConstConfusionNetworkRef sausageLattice2cn(ConstLatticeRef l, bool keepEpsArcs = true);

    /**
     * Convert a CN into a lattice with "sausage" topology.
     * If approximateBoundaryTimes is set, then the boundary times in the lattice
     * are approximated by inheriting them from the adjacent confusion network arc with highest
     * probability. Otherwise boundary times are omitted.
     **/
    ConstLatticeRef cn2lattice(ConstConfusionNetworkRef cn, bool approximateBoundaryTimes = false);

    /**
     * Normalizes an arbitrary CN.
     * Requires a dimension storing posteriror probabilities, i.e.
     * a dimension defining a slot-wise prob. distribution, where
     * missing prob. mass is assigned to the eps-arc.
     * Each slot in the resulting normalized CN
     * - is sorted by word label
     * - each word label occurs at most once
     * - the slot-wise sum over dimension "posterirorId" equals one
     **/
    ConstConfusionNetworkRef normalizeCn(ConstConfusionNetworkRef cn, ScoreId posterirorId);
    bool isNormalizeCn(ConstConfusionNetworkRef cn, ScoreId posteriorId);

    /**
     * Decodes a normalized CN;
     * if the CN is not normalized, a valid posteriroId is required and the CN is normalized beforehand.
     **/
    ConstLatticeRef decodeCn(ConstConfusionNetworkRef cn, ScoreId posterirorId = Semiring::InvalidId);

    NodeRef createCnDecoderNode(const std::string &name, const Core::Configuration &config);


    /**
     * CN features
     **/
    struct CnFeatureConfiguration {
		// CN
		ConstConfusionNetworkRef cn;
		ScoreId cnPosteriorId; // required for posterior distribution based features like confidence, score, and entropy
		// composition with CN
		bool compose;
		bool duplicateOutput;
		// lattice features
		ConstSemiringRef semiring;
		ScoreId confidenceId;
		ScoreId scoreId;
		ScoreId errId;
		bool oracleOutput;
		ScoreId slotEntropyId;
		ScoreId slotId;
		ScoreId nonEpsSlotId;
		Score epsSlotThreshold;
		CnFeatureConfiguration();
    };

    /**
     * Add CN features to lattice
     * Optionally, perform the "composition" of the lattice and the CN derived from the lattice:
     * each path in the resulting lattice has the same length
     **/
    ConstLatticeRef composeAndAddCnFeatures(ConstLatticeRef l, const CnFeatureConfiguration &cnConfig);
    NodeRef createCnFeatureNode(const std::string &name, const Core::Configuration &config);

} // namespace

#endif // _FLF_CONFUSION_NETWORK_HH
