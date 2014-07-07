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
#include "Phonology.hh"

#include <algorithm>
#include <set>
#include <map>
#include <Core/StringUtilities.hh>

using namespace Bliss;

// ===========================================================================
bool ContextPhonology::SemiContext::empty() const {
    if (Precursor::empty()) {
	return true;
    } else if ((*begin()) == Phoneme::term) {
	// The 'term' element cannot be followed by not 'term' element.
	verify(std::find_if(begin(), end(),
			    std::bind2nd(std::not_equal_to<Phoneme::Id>(), Phoneme::term)) == end());
	return true;
    }
    return false;
}

u32 ContextPhonology::SemiContext::Hash::operator() (const SemiContext &sc) const {
    u32 h = 0;
    for (const_iterator i = sc.begin(); i != sc.end(); ++i)
	h = ((h << 7) | (h >> 25)) ^ u32(*i);
    return h;
}

// ===========================================================================
u32 ContextPhonology::Context::Hash::operator() (const Context &c) const {
    u32 h1 = sch(c.history), h2 = sch(c.future);
    return ((h1 << 16) | (h1 >> 16)) ^ h2;
}

// ===========================================================================
ContextPhonology::ContextPhonology(
    Core::Ref<const PhonemeInventory> pi,
    u32 maximumHistoryLength,
    u32 maximumFutureLength) :
    pi_(pi),
    maximumHistoryLength_(maximumHistoryLength),
    maximumFutureLength_(maximumFutureLength)
{}

void ContextPhonology::push(SemiContext &ctxt, Phoneme::Id p, s32 maximumLength) {
    ctxt.insert(ctxt.begin(), p);
    if (ctxt.size() > size_t(maximumLength))
	ctxt.erase(maximumLength);
}
void ContextPhonology::pushHistory(ContextPhonology::Context &a, Phoneme::Id p) const {
    push(a.history, p, maximumHistoryLength_);
}
void ContextPhonology::pushHistory(ContextPhonology::Allophone &a, Phoneme::Id p) const {
    pushHistory(a.context_, p);
}
void ContextPhonology::pushFuture(ContextPhonology::Context &a, Phoneme::Id p) const {
    push(a.future, p, maximumFutureLength_);
}
void ContextPhonology::pushFuture(ContextPhonology::Allophone &a, Phoneme::Id p) const {
    pushFuture(a.context_, p);
}

void ContextPhonology::append(SemiContext &ctxt, Phoneme::Id p, s32 maximumLength) {
    if (ctxt.size() < size_t(maximumLength))
	ctxt.append(1, p);
}
void ContextPhonology::appendHistory(ContextPhonology::Context &a, Phoneme::Id p) const {
    append(a.history, p, maximumHistoryLength_);
}
void ContextPhonology::appendHistory(ContextPhonology::Allophone &a, Phoneme::Id p) const {
    appendHistory(a.context_, p);
}
void ContextPhonology::appendFuture(ContextPhonology::Context &a, Phoneme::Id p) const {
    append(a.future, p, maximumFutureLength_);
}
void ContextPhonology::appendFuture(ContextPhonology::Allophone &a, Phoneme::Id p) const {
    appendFuture(a.context_, p);
}

ContextPhonology::Allophone
ContextPhonology::allophone(const Pronunciation &p, int i) const {
    require(0 <= i && u32(i) < p.length());
    SemiContext history, future;
    if (pi_->phoneme(p[i])->isContextDependent()) {
	for (int j = i - 1; j >= std::max(i - maximumHistoryLength_, 0); --j) {
	    if (!pi_->phoneme(p[j])->isContextDependent()) break;
	    history.insert(history.begin(), p[j]);
	}
	int j_end = std::min(i + maximumFutureLength_ + 1, int(p.length()));
	for (int j = i + 1; j < j_end;  ++j) {
	    if (!pi_->phoneme(p[j])->isContextDependent()) break;
	    future.append(1, p[j]);
	}
    }
    return Allophone(p[i], history, future);
}

ContextPhonology::Allophone
ContextPhonology::operator() (const Pronunciation &p, int i) const {
    return allophone(p, i);
}

u32 ContextPhonology::Allophone::Hash::operator() (const ContextPhonology::Allophone &a) const {
    u32 h = ch(a.context_);
    return ((h << 7) | (h >> 25)) ^ u32(a.phoneme_);
}



// ===========================================================================
#ifdef OBSOLETE

/**
 * \warning PhonemeGraphConverter does not respect Phoneme::isContextDependent().
 */
class ContextPhonology::PhonemeGraphConverter {
    const ContextPhonology &phonology_;
    const PhonemeGraph &in_;

    typedef std::map<SemiContext, int> VertexContextMap;
    typedef std::set<int> EdgeContextSet;

    std::vector<VertexContextMap> histories_;
    std::vector<VertexContextMap> futures_;
    std::vector<EdgeContextSet> pastFutures_;
    std::vector<EdgeContextSet> futureHistories_;

public:
    PhonemeGraphConverter(const ContextPhonology &phonology, const PhonemeGraph &in) :
	phonology_(phonology),
	in_(in),
	histories_      (in.nNodeIds()),
	futures_        (in.nNodeIds()),
	pastFutures_    (in.nEdgeIds()),
	futureHistories_(in.nEdgeIds())
	{}

    int traverseForward(const PhonemeGraph::Node *v, SemiContext c) {
	VertexContextMap &cs(histories_[v->id()]);
	if (cs.find(c) == cs.end()) {
	    int n = cs.size();
	    cs[c] = n;
	    for (PhonemeGraph::const_OutgoingEdgeIterator e = v->outgoingEdges(); e.valid(); ++e) {
		futureHistories_[e->id()].insert(
		    traverseForward(
			e->target(),
			(e->phoneme()) ? phonology_.extendHistory(c, e->phoneme()->id()) : c));
	    }
	}
	return cs[c];
    }

    int traverseBackward(const PhonemeGraph::Node *v, SemiContext c) {
	VertexContextMap &cs(futures_[v->id()]);
	if (cs.find(c) == cs.end()) {
	    int n = cs.size();
	    cs[c] = n;
	    for (PhonemeGraph::const_IncomingEdgeIterator e = v->incomingEdges(); e.valid(); ++e) {
		pastFutures_[e->id()].insert(
		    traverseBackward(
			e->source(),
			(e->phoneme()) ? phonology_.extendFuture(c, e->phoneme()->id()) : c));
	    }
	}
	return cs[c];
    }

    struct VertexSpawnMap {
	int nHistories, nFutures;
	std::vector<AllophoneGraph::Node*> spawn;

	void build(
	    const PhonemeGraphNodeInfo &proto, AllophoneGraph &out,
	    const VertexContextMap &histories, const VertexContextMap &futures)
	{
	    nHistories = histories.size();
	    nFutures = futures.size();
	    spawn.resize(nHistories * nFutures);

	    for (unsigned int i = 0 ; i < spawn.size() ; ++i)
		spawn[i] = out.addNode(new AllophoneGraph::Node(proto));
	}

	void addVertexContexts(
	    AllophoneGraph &out,
	    const VertexContextMap &histories, const VertexContextMap &futures)
	{
	    VertexContextMap::const_iterator h, f;
	    for (h = histories.begin() ; h != histories.end() ; ++h) {
		for (f = futures.begin() ; f != futures.end() ; ++f) {
		    vertex(h->second, f->second)->setContext(
			Context(h->first, f->first));
		}
	    }
	}

	AllophoneGraph::Node *vertex(int h, int f) const {
	    require(0 <= h && h < nHistories);
	    require(0 <= f && f < nFutures);
	    return spawn[h * nFutures + f];
	}
    };

    void convert(AllophoneGraph &out) {
	PhonemeGraph::const_NodeIterator v(in_.nodes());
	PhonemeGraph::const_EdgeIterator e(in_.edges());

	// determine contexts
	traverseForward(in_.initial(), SemiContext());
	traverseBackward(in_.final(), SemiContext());

	// create vertices
	std::vector<VertexSpawnMap> spawn(in_.nNodeIds());
	for (v.begin() ; v.valid() ; ++v)
	    spawn[v->id()].build(*v, out, histories_[v->id()], futures_[v->id()]);\
	for (v.begin() ; v.valid() ; ++v)
	    spawn[v->id()].addVertexContexts(out, histories_[v->id()], futures_[v->id()]);

	// create edges
	for (e.begin() ; e.valid() ; ++e) {
	    EdgeContextSet &pfs(pastFutures_[e->id()]);
	    EdgeContextSet &fhs(futureHistories_[e->id()]);
	    EdgeContextSet::const_iterator pf, fh;
	    VertexSpawnMap &sourceSpawn(spawn[e->sourceId()]);
	    VertexSpawnMap &targetSpawn(spawn[e->targetId()]);
	    for (pf = pfs.begin() ; pf != pfs.end() ; ++pf) {
		for (fh = fhs.begin() ; fh != fhs.end() ; ++fh) {
		    for (int h = 0 ; h < sourceSpawn.nHistories ; ++h) {
			for (int f = 0 ; f < targetSpawn.nFutures ; ++f) {
			    AllophoneGraph::Node *s, *t;
			    s = sourceSpawn.vertex(h, *pf);
			    t = targetSpawn.vertex(*fh, f);
			    if ((((e->phoneme()) ? phonology_.extendHistory(s->context().history, e->phoneme()->id()) : s->context().history) == t->context().history) &&
				(((e->phoneme()) ? phonology_.extendFuture (t->context().future,  e->phoneme()->id()) : t->context().future) == s->context().future )) {
				out.addEdge(new AllophoneGraph::Edge(*e), s, t);
			    }
			}
		    }
		}
	    }
	}

	// mark or create initial node
	{
	    VertexSpawnMap &s(spawn[in_.initial()->id()]);
	    if (s.spawn.size() == 1) {
		out.setInitial(s.vertex(0, 0));
	    } else {
		out.setInitial(out.addNode(new AllophoneGraph::Node(*in_.initial())));
		for (unsigned int i = 0 ; i < s.spawn.size() ; ++i)
		    out.addEdge(new AllophoneGraph::Edge(PhonemeGraphEdgeInfo(0, 0)),
				out.initial(), s.spawn[i]);
	    }
	}

	// mark or create final node
	{
	    VertexSpawnMap &s(spawn[in_.final()->id()]);
	    if (s.spawn.size() == 1) {
		out.addFinal(s.vertex(0, 0));
	    } else {
		out.addFinal(out.addNode(new AllophoneGraph::Node(*in_.final())));
		for (unsigned int i = 0 ; i < s.spawn.size() ; ++i)
		    out.addEdge(new AllophoneGraph::Edge(PhonemeGraphEdgeInfo(0, 0)),
				s.spawn[i], out.final());
	    }
	}
    }
}; // class ContextPhonology::PhonemeGraphConverter

void ContextPhonology::convert(
    const PhonemeGraph &in,
	AllophoneGraph &out) const
{
    out.clear();

    PhonemeGraphConverter converter(*this, in);
    converter.convert(out);
}
#endif // OBSOLETE

// ===========================================================================

std::string ContextPhonology::PhonemeInContext::formatPhoneme(
    Core::Ref<const PhonemeInventory> pi, Phoneme::Id p) const
{
    if (pi->isValidPhonemeId(p))
	return pi->phoneme(p)->symbol();
    else if (p == Phoneme::term)
	return "#";
    else
	return Core::form("%d?", p);
}

/** Produce a string representing a phoneme in context.
 * Currently the output format has been made compatible with TDC
 * software for training phonetic decision trees.  We might want to
 * change this in the future? */

std::string ContextPhonology::PhonemeInContext::format(
    Core::Ref<const PhonemeInventory> pi) const
{
    std::string result;
    result += formatPhoneme(pi, phoneme_);
    result += "{";
    if (context_.history.size() == 0) {
	result += "#";
    } else {
	for (unsigned int i = context_.history.size(); i;) {
	    result += formatPhoneme(pi, context_.history[--i]);
	    if (i) result += "-";
	}
    }
    result += "+";
    if (context_.future.size() == 0) {
	result += "#";
    } else {
	for (unsigned int i = 0; i < context_.future.size();) {
	    if (i) result += "-";
	    result += formatPhoneme(pi, context_.future[i++]);
	}
    }
    result += "}";
    return result;
}

// ===========================================================================

Phoneme::Id ContextPhonology::PhonemeInContext::phoneme(s16 pos) const {
    if (pos == 0)
	return phoneme_;
    else if (pos > 0) {
	if (u16(pos - 1) < context_.future.length())
	    return context_.future[pos - 1];
	else
	    return Phoneme::term;
    } else { verify(pos < 0);
	if (u16(-1 - pos) < context_.history.length())
	    return context_.history[-1 - pos];
	else
	    return Phoneme::term;
    }
}

void ContextPhonology::PhonemeInContext::setPhoneme(s16 pos, Phoneme::Id ph) {
    if (pos == 0)
	phoneme_ = ph;
    else if (pos > 0) {
	if (ph != Phoneme::term) {
	    if (u16(pos - 1) >= context_.future.length())
		context_.future.resize(pos, Phoneme::term);
	    context_.future[pos - 1] = ph;
	} else {
	    if (u16(pos - 1) < context_.future.length())
		context_.future.resize(pos - 1);
	}
    } else { verify(pos < 0);
	if (ph != Phoneme::term) {
	    if (u16(-1 - pos) >= context_.history.length())
		context_.history.resize(-pos , Phoneme::term);
	    context_.history[-1 - pos] = ph;
	} else {
	    if (u16(-1 - pos) >= context_.history.length())
		context_.history.resize(-1 - pos);
	}
    }
}

// ===========================================================================
#ifdef OBSOLETE
ContextPhonology::PhonemeInContext
ContextPhonology::AllophoneGraphEdge::phonemeInContext() const {
    require(phoneme());
    return PhonemeInContext(
	phoneme()->id(),
	source()->context().history,
	target()->context().future);
}

void ContextPhonology::AllophoneGraph::Drawer::drawEdge(
    const Edge &e, std::ostream &os) const
{
    os << "[label=\"";
    if (e.phoneme()) {
	os << e.phonemeInContext().format(phonemeInventory_);
    }
    if (e.pronunciation()) {
	os << "\\n"
	   << e.pronunciation()->lemmas().first->preferredOrthographicForm()
	   << "\\n/ "
	   << e.pronunciation()->format(phonemeInventory_)
	   << "/";
    }
    os << "\"]";
}

void ContextPhonology::AllophoneGraph::Drawer::drawNode(
    const Node &n, std::ostream &os) const
{
    os << "[label=\"";

    if (n.flags() & Node::wordBoundary)
	os << "WB\\n";

    std::string left, right;
    const Context &context(n.context());
    for (unsigned int i = context_.history.size(); i;) {
	left.append(phonemeInventory_->phoneme(context.history[--i])->symbol());
	left += utf8::blank;
    }
    for (unsigned int i = 0; i < context.future.size();) {
	right += utf8::blank;
	right.append(phonemeInventory_->phoneme(context.future[i++])->symbol());
    }
    os << left << " / " << right;
    os << "\"]";
}
#endif // OBSOLETE
