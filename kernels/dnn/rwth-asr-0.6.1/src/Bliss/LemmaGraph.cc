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
#include "LemmaGraph.hh"

#include <Core/StringUtilities.hh>

using namespace Bliss;

/**\file
 * \deprecated This file will soon become obsolete!
 */

void LemmaGraph::store(std::ostream &os) const {

}

void LemmaGraph::load(std::ostream &os) {

}

void LemmaGraph::Drawer::drawEdge
(const Edge &e, std::ostream &os) const {
    if (e.lemma())
	os << "[label=\"" << e.lemma()->preferredOrthographicForm() << "\"]";
}

LemmaGraphBuilder::LemmaGraphBuilder(LemmaGraph &net) :
    net_(net)
{}

void LemmaGraphBuilder::initialize(Core::Ref<const Lexicon> lexicon) {
    net_.clear();
}

OrthographicParser::Node LemmaGraphBuilder::newNode() {
    return net_.newNode();
}

void LemmaGraphBuilder::newEdge(OrthographicParser::Node from, OrthographicParser::Node to, const Lemma *lemma) {
    net_.addEdge(new LemmaGraph::Edge(lemma),
		 static_cast<LemmaGraph::Node*>(from),
		 static_cast<LemmaGraph::Node*>(to));
}

void LemmaGraphBuilder::finalize(OrthographicParser::Node initial, OrthographicParser::Node final) {
    net_.setInitial(static_cast<LemmaGraph::Node*>(initial));
    net_.addFinal  (static_cast<LemmaGraph::Node*>(final));
}
