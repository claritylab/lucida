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
#include "PhonemeGraph.hh"

/**\file
 * \deprecated This file will soon become obsolete!
 */

using namespace Bliss;

// ===========================================================================
const PhonemeGraph::Edge *PhonemeGraphNode::findIncomingPhoneme(Phoneme::Id p) const {
    for (PhonemeGraph::const_IncomingEdgeIterator e = incomingEdges(); e.valid(); ++e) {
	if (e->phoneme() && e->phoneme()->id() == p)
	    return e.pointer();
    }
    return 0;
}

const PhonemeGraph::Edge *PhonemeGraphNode::findOutgoingPhoneme(Phoneme::Id p) const {
    for (PhonemeGraph::const_OutgoingEdgeIterator e = outgoingEdges(); e.valid(); ++e) {
	if (e->phoneme() && e->phoneme()->id() == p)
	    return e.pointer();
    }
    return 0;
}

// ===========================================================================
void PhonemeGraph::Drawer::drawEdge(const Edge &e, std::ostream &os) const {
    os << "[label=\"";
    if (e.phoneme()) {
	os << e.phoneme()->symbol();
    }
    if (e.pronunciation())
	os << "\\n"
	   << e.pronunciation()->lemmas().first->preferredOrthographicForm()
	   << "\\n/ "
	   << e.pronunciation()->format(phonemeInventory_)
	   << "/";
    os << "\"]";
};

void PhonemeGraph::Drawer::drawNode(const Node &n, std::ostream &os) const {
    os << "[label=\"";
    if (n.flags() & Node::wordBoundary)
	os << "WB";
    os << "\"]";
};

// ===========================================================================
PhonemeGraphBuilder::PhonemeGraphBuilder(
    const Core::Configuration &c,
    LexiconRef _lexicon) :
    Core::Component(c),
    lexicon_(_lexicon)
{
    optimize_ = true;
}

void PhonemeGraphBuilder::build(PhonemeGraph &pg, const LemmaGraph &lg) const {
    pg.clear();

    std::vector<PhonemeGraph::Node*> nn(lg.nNodeIds(), (PhonemeGraph::Node*)(0));
    LemmaGraph::const_NodeIterator lgn = lg.nodes();
    for (lgn.begin(); lgn.valid(); ++lgn) {
	pg.addNode(nn[lgn->id()] =
		   new PhonemeGraph::Node(PhonemeGraph::Node::wordBoundary));
    }
    pg.setInitial(nn[lg.initial()->id()]);
    pg.addFinal  (nn[lg.final  ()->id()]);

    LemmaGraph::const_EdgeIterator lge = lg.edges();
    for (lge.begin(); lge.valid(); ++lge) {
	if (lge->lemma()) {
	    Lemma::PronunciationIterator pron, pron_end;
	    for (Core::tie(pron, pron_end) = lge->lemma()->pronunciations(); pron != pron_end; ++pron)
		addPronunciation(pg, nn[lge->sourceId()],
				     nn[lge->targetId()], pron);
	} else {
	    pg.addEdge(new PhonemeGraph::Edge(0, 0),
		       nn[lge->sourceId()],
		       nn[lge->targetId()]);
	}
    }

    // remove duplicate pronunciation pointers
    PhonemeGraph::EdgeIterator pge = pg.edges();
    for (pge.begin(); pge.valid(); ++pge) {
	if (pge->source()->flags() & PhonemeGraph::Node::uniquePronunciation)
	    pge->setPronunciation(0);
    }
}

void PhonemeGraphBuilder::addPronunciation(
    PhonemeGraph &graph,
    PhonemeGraph::Node *from,
    PhonemeGraph::Node *to,
    const Pronunciation *pron) const
{
    int i, j ;
    PhonemeGraph::Node *ff, *tt;
    const PhonemeGraph::Edge *e;

    i = 0; ff = from;
    j = pron->length(); tt = to;

    // skip common prefix
    if (optimize_) {
	while (i < pron->length()) {
	    e = ff->findOutgoingPhoneme((*pron)[i]);
	    if (!e) break;
	    if (e->target() == tt) break;
	    if (e->target()->nIncomingEdges() > 1) break;
	    ff = e->target();
	    ++i;
	}
    }

    if (i == pron->length() && ff == to) {
	warning("Redundant pronunciation for lemma \"%s\" ignored.",
		pron->lemmas().first->preferredOrthographicForm().str());
	return;
    }

    // skip common suffix
    if (optimize_) {
	while (j > i) {
	    e = tt->findIncomingPhoneme((*pron)[j-1]);
	    if (!e) break;
	    if (e->source() == ff) break;
	    if (e->source()->nOutgoingEdges() > 1) break;
	    tt = e->source();
	    --j;
	}
    }

    // add the unique remaining middle-part
    if (i < j) {
	PhonemeGraph::Node *f, *t;
	PhonemeGraph::Node::Flags flags = PhonemeGraph::Node::Flags(
	    PhonemeGraph::Node::withinWord |
	    PhonemeGraph::Node::uniquePronunciation);
	for (f = ff ; i < j ; ++i) {
	    if (i < j-1) {
		graph.addNode(t = new PhonemeGraph::Node(flags));
	    } else
		t = tt;
	    const Phoneme *phon = lexicon_->phonemeInventory()->phoneme((*pron)[i]);
	    graph.addEdge(new PhonemeGraph::Edge(phon, pron), f, t);
	    f = t;
	}
    } else {
	// remaining middle part is empty, so add an epsilon transition
	graph.addEdge(new PhonemeGraph::Edge(0, pron), ff, tt);
    }

    // remove ambiguous pronunciation pointers
    while (ff != from) {
	verify(ff->nIncomingEdges() == 1);
	ff->setFlags(PhonemeGraph::Node::Flags(ff->flags() & ~PhonemeGraph::Node::uniquePronunciation));
	ff->incomingEdge()->setPronunciation(0);
	ff = ff->incomingEdge()->source();
    }
    while (tt != to) {
	verify(tt->nOutgoingEdges() == 1);
	tt->outgoingEdge()->setPronunciation(0);
	tt = tt->outgoingEdge()->target();
    }
}
