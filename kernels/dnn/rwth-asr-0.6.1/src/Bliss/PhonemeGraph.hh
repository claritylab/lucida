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
// $Id: PhonemeGraph.hh 6223 2006-11-14 17:01:38Z rybach $

#ifndef _BLISS_PHONEMEGRAPH_HH
#define _BLISS_PHONEMEGRAPH_HH

#include <Core/Component.hh>
#include <Core/Graph.hh>
#include "Lexicon.hh"
#include "LemmaGraph.hh"

/**\file
 * \deprecated This file will soon become obsolete!
 */

namespace Bliss {

    class PhonemeGraphNode;
    class PhonemeGraphEdge;

    /**
     * Phoneme Graph (aka pronunciation network).
     *
     * Each edge is labled with one phoneme (or epsilon).
     **/

    class PhonemeGraph :
	public Core::GraphWithInitialFinals<PhonemeGraphNode, PhonemeGraphEdge>
    {
	typedef PhonemeGraph Self;
	typedef Core::Graph<PhonemeGraphNode, PhonemeGraphEdge> Precursor;
    public:
	class Drawer : public Core::Drawer<Precursor> {
	    Core::Ref<const PhonemeInventory> phonemeInventory_;
	public:
	    Drawer(Core::Ref<const PhonemeInventory> pi) : phonemeInventory_(pi) {}
	    virtual ~Drawer() {}
	    virtual void drawEdge(const Edge&, std::ostream&) const;
	    virtual void drawNode(const Node&, std::ostream&) const;
	};
    };

    class PhonemeGraphNodeInfo {
    public:
	enum Flags {
	    withinWord   = 0,
	    wordBoundary = 1,
	    uniquePronunciation = 2  // used during construction
	};
    private:
	Flags flags_;
    public:
	Flags flags() const { return flags_; }
	void setFlags(Flags _flags) { flags_ = _flags; }
	PhonemeGraphNodeInfo(Flags f) : flags_(f) {}
	PhonemeGraphNodeInfo(const PhonemeGraphNodeInfo &o) :
	    flags_(o.flags_) {}
    };

    class PhonemeGraphNode :
	public PhonemeGraph::NodeBase,
	public PhonemeGraphNodeInfo
    {
    public:
	PhonemeGraphNode(const PhonemeGraphNodeInfo &o) :
	    PhonemeGraphNodeInfo(o) {}
	const PhonemeGraph::Edge *findIncomingPhoneme(Phoneme::Id) const;
	const PhonemeGraph::Edge *findOutgoingPhoneme(Phoneme::Id) const;
    };

    class PhonemeGraphEdgeInfo {
    private:
	const Phoneme       *phoneme_;
	const Pronunciation *pronunciation_;
    public:
	const Phoneme       *phoneme()       const { return phoneme_; }
	const Pronunciation *pronunciation() const { return pronunciation_; }

	void setPronunciation(const Pronunciation *pron) {
	    pronunciation_ = pron;
	}

	PhonemeGraphEdgeInfo(const Phoneme *phon, const Pronunciation *pron) :
	    phoneme_(phon), pronunciation_(pron) {}
    };

    class PhonemeGraphEdge :
	public PhonemeGraph::EdgeBase,
	public PhonemeGraphEdgeInfo
    {
    public:
	PhonemeGraphEdge(const Phoneme *phon, const Pronunciation *pron) :
	    PhonemeGraphEdgeInfo(phon, pron) {}
    };

    /**
     * Converter from LemmaGraph to PhonemeGraph.
     */

    class PhonemeGraphBuilder :
	public Core::Component
    {
    private:
	LexiconRef lexicon_;
	bool optimize_;
    public:
	PhonemeGraphBuilder(const Core::Configuration &c, LexiconRef);

	void addPronunciation(PhonemeGraph &graph,
			      PhonemeGraph::Node *from,
			      PhonemeGraph::Node *to,
			      const Pronunciation *pron) const;

	/**
	 * Create a PhonemeGraph based on a given LemmaGraph.
	 *
	 * For each edge in the lemma graph all possible
	 * pronunciations are added to the phoneme graph.  The
	 * resulting phoneme graph is somewhat optimized by sharing
	 * phoneme transitions between pronunciations (with common
	 * prefix or suffix), but is not guaranteed to be minimal (in
	 * the strict sense).  The phoneme graph may contain epsilon
	 * transitions.  Since lemmata may have empty pronunciations,
	 * a full-fledged epsilon removal algorithm would be needed to
	 * avoid this.  Therefore any of these properties
	 * (epsilon-free, deterministic, minimal) require additional
	 * processing.
	 * @param out the phoneme graph the be constructed; any nodes
	 * or edges already present will be cleared.
	 * @param in the lemma graph to convert
	 */
	void build(PhonemeGraph &out, const LemmaGraph &in) const;
    };

} // namespace Bliss

#endif // _BLISS_PHONEMEGRAPH_HH
