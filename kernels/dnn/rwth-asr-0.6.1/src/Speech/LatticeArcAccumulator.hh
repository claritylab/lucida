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
#ifndef _SPEECH_LATTICE_ARC_ACCUMULATOR_HH
#define _SPEECH_LATTICE_ARC_ACCUMULATOR_HH

#include <Bliss/Fsa.hh>
#include <Core/ObjectCache.hh>
#include <Core/Parameter.hh>
#include <Fsa/Dfs.hh>
#include "LatticeNodes.hh"

namespace Speech {

    /** LemmaPronunciationId_hash_map */
    class LemmaPronunciationId_hash_map : public Core::HashMap<Fsa::LabelId,f32> {
    public:
	void init(Bliss::LexiconRef lexicon);
	void read(const std::string &filename);
	void write(const std::string &filename);
    private:
	Bliss::LexiconRef lexicon;
    };

    /** LatticeArcAccumulatorNode */
    class LatticeArcAccumulatorNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	static Core::ParameterString paramLexiconFilename;
	static Core::ParameterString paramEncoding;
	static Core::ParameterString paramPronunciationStatsFilenameRead;
	static Core::ParameterString paramPronunciationStatsFilenameWrite;
	static const Core::ParameterFloat paramMinimumPronWeight;
	static const Core::ParameterFloat paramTauWeight;
	f32 tau_;
	f32 minimalPronWeight_;
    protected:
	std::string lexiconFilename_;
	std::string pronunciationStatsFilenameRead_;
	std::string pronunciationStatsFilenameWrite_;
	std::string encoding_;
	ModelCombinationRef modelCombination_;
	bool needInit_;
	LemmaPronunciationId_hash_map *arcAccumulator_;
    public:
	static std::string filterName() { return "lattice-arc-accumulator"; }
	LatticeArcAccumulatorNode(const Core::Configuration&);
	virtual ~LatticeArcAccumulatorNode();
	virtual bool setParameter(const std::string &name, const std::string &value) { return false; }
	bool updateWeightsMap(Core::hash_map<Fsa::LabelId, f32> &counts,
			      Core::hash_map<Fsa::LabelId, f32> &weights,
			      f32 tau = 1.0);
	virtual bool configure();
	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "model-combination" ? 1 : 0; }
	virtual bool work(Flow::PortId);
    };


    class ArcWeightAccumulator: public Fsa::DfsState {
    private:
	Core::hash_map<Fsa::LabelId, f32>* Sums_;
    public:
	ArcWeightAccumulator(Fsa::ConstAutomatonRef f, Core::hash_map<Fsa::LabelId, f32>* sums) : Fsa::DfsState(f), Sums_(sums) {
	    dfs();
	}
	virtual ~ArcWeightAccumulator() {}

	void accumulateArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	    f32 weight = f32(a.weight());
	    if(weight > Core::Type<f32>::epsilon) {
		if((*Sums_).count(a.input()) > 0)
		    (*Sums_)[a.input()] += weight;
		else
		    (*Sums_)[a.input()] = weight;
	    }
	}

	virtual void exploreTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	    accumulateArc(from, a);
	}
	virtual void exploreNonTreeArc(Fsa::ConstStateRef from, const Fsa::Arc &a) {
	    accumulateArc(from, a);
	}
    };
}

#endif // _SPEECH_LATTICE_ARC_ACCUMULATOR_HH
