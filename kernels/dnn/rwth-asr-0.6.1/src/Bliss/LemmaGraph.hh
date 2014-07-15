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
// $Id: LemmaGraph.hh 6223 2006-11-14 17:01:38Z rybach $

#ifndef _BLISS_LEMMAGRAPH_HH
#define _BLISS_LEMMAGRAPH_HH

#include <Core/Component.hh>
#include <Core/Graph.hh>
#include "Orthography.hh"

/**\file
 * \deprecated This file will soon become obsolete!
 */

namespace Bliss {

    class LemmaGraphNode;
    class LemmaGraphEdge;

    /**
     * Lemma Graph (aka word graph).
     *
     * Each edge is labled with one lemma (or epsilon).
     **/

    class LemmaGraph : public Core::GraphWithInitialFinals<LemmaGraphNode, LemmaGraphEdge> {
	typedef LemmaGraph Self;
	typedef Core::Graph<LemmaGraphNode, LemmaGraphEdge> Precursor;
    public:
	void store(std::ostream&) const;
	void load(std::ostream&);

	class Drawer : public Core::Drawer<LemmaGraph> {
	public:
	    virtual void drawEdge(const Edge&, std::ostream&) const;
	};
    };

    class LemmaGraphNode : public LemmaGraph::NodeBase {};

    class LemmaGraphEdge : public LemmaGraph::EdgeBase {
	const Lemma *lemma_;
    public:
	LemmaGraphEdge(const Lemma *lemma) : lemma_(lemma) {}
	const Lemma *lemma() const { return lemma_; }
    };

    class LemmaGraphBuilder : public OrthographicParser::Handler {
    private:
	LemmaGraph &net_;
    public:
	LemmaGraphBuilder(LemmaGraph&);
	virtual void initialize(Core::Ref<const Lexicon>);
	virtual OrthographicParser::Node newNode();
	virtual void newEdge(OrthographicParser::Node from, OrthographicParser::Node to, const Lemma *lemma);
	virtual void finalize(OrthographicParser::Node intial, OrthographicParser::Node final);
    };

} // namespace Bliss

#endif // _BLISS_LEMMAGRAPH_HH
