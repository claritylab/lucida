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
// $Id: Graph.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _CORE_GRAPH_HH
#define _CORE_GRAPH_HH

#include <vector>

#include <Core/Assertions.hh>
#include <Core/Types.hh>

namespace Core {

    /**
     * General purpose class template for directed graphs.
     *
     * Memory complexity:
     *  N * ( sizeof(int) + 4 * sizeof(void*) )
     *  E * ( sizeof(int) + 5 * sizeof(void*) )
     * Time complexity:
     *  addNode       O(1)
     *  addEdge       O(1)
     *  removeNode    O(E/M)
     *  removeEdge    O(E/N)
     */

    template <class N, class E>
    class Graph {
	typedef Graph Self;
    public:
	typedef N Node;
	typedef N* NodePointer;
	typedef const N* const_NodePointer;
	typedef E Edge;
	typedef E* EdgePointer;
	typedef const E* const_EdgePointer;
	typedef u32 NodeId;
	typedef u32 EdgeId;

	static const NodeId invalidNodeId() { return Core::Type<NodeId>::max; }
	static const EdgeId invalidEdgeId() { return Core::Type<EdgeId>::max; }

    private:
	template <class T>
	class AdjacentEdgeIterator {
	public:
	    typedef T &Reference ;
	    typedef T *Pointer ;
	protected:
	    T *edge_;
	    AdjacentEdgeIterator(T *e) : edge_(e) {}
	public:
	    bool valid() const {
		return edge_ != 0;
	    }

	    Reference item() const {
		require(valid());
		return *edge_;
	    }

	    Reference operator*() const {
		return item();
	    }

	    Pointer pointer() const {
		require(valid());
		return edge_;
	    }

	    Pointer operator->() const {
		return pointer();
	    }

	    operator Pointer() const {
		return pointer();
	    }
	};

	template <class T>
	class _OutgoingEdgeIterator :
	    public AdjacentEdgeIterator<T>
	{
	    typedef Graph::AdjacentEdgeIterator<T> Predecessor;
	    typedef _OutgoingEdgeIterator Self;
	public:
	    _OutgoingEdgeIterator() : Predecessor(0) {};
	    _OutgoingEdgeIterator(T *e) : Predecessor(e) {};
	    Self& operator++() {
		edge_ = edge_->linkOutgoing();
		return *this ;
	    }
	};

	template <class T>
	class _IncomingEdgeIterator :
	    public AdjacentEdgeIterator<T>
	{
	    typedef Graph::AdjacentEdgeIterator<T> Predecessor;
	    typedef _IncomingEdgeIterator Self;
	public:
	    _IncomingEdgeIterator() : Predecessor(0) {};
	    _IncomingEdgeIterator(T *e) : Predecessor(e) {};
	    Self& operator++() {
		edge_ = edge_->linkIncoming();
		return *this ;
	    }
	};

    public:
	typedef _OutgoingEdgeIterator<      Edge>       OutgoingEdgeIterator;
	typedef _OutgoingEdgeIterator<const Edge> const_OutgoingEdgeIterator;
	typedef _IncomingEdgeIterator<      Edge>       IncomingEdgeIterator;
	typedef _IncomingEdgeIterator<const Edge> const_IncomingEdgeIterator;

	class NodeBase {
	    friend class Graph;
	private:
	    NodeId id_;
	    EdgePointer outgoing_, incoming_;
	public:
	    NodeId id() const {return id_; }
	    EdgePointer outgoing() const { return outgoing_; }
	    EdgePointer incoming() const { return incoming_; }

	    OutgoingEdgeIterator outgoingEdges() {
		return OutgoingEdgeIterator(outgoing());
	    }

	    const_OutgoingEdgeIterator outgoingEdges() const {
		return const_OutgoingEdgeIterator(outgoing());
	    }

	    IncomingEdgeIterator incomingEdges() {
		return IncomingEdgeIterator(incoming());
	    }

	    const_IncomingEdgeIterator incomingEdges() const{
		return const_IncomingEdgeIterator(incoming());
	    }

	    int nIncomingEdges() const {
		int result = 0;
		for (EdgePointer e = incoming() ; e ; e = e->linkIncoming()) ++result;
		return result;
	    }

	    int nOutgoingEdges() const {
		int result = 0;
		for (EdgePointer e = outgoing() ; e ; e = e->linkOutgoing()) ++result;
		return result;
	    }

	    EdgePointer incomingEdge() {
		require(nIncomingEdges() == 1);
		return incoming();
	    }

	    EdgePointer outgoingEdge() {
		require(nOutgoingEdges() == 1);
		return outgoing();
	    }

	protected:
	    NodeBase() :
		id_(invalidNodeId()), outgoing_(0), incoming_(0)
		{}
	    NodeBase(const NodeId &id) : id_(id) {}
	};

	class EdgeBase {
	    friend class Graph;
	private:
	    EdgeId id_;
	    NodePointer source_, target_;
	    EdgePointer linkOutgoing_, linkIncoming_;
	public:
	    EdgeId id() const  { return id_; }
	    NodePointer source() const { return source_; }
	    NodePointer target() const { return target_; }
	    NodeId sourceId() const {
		if (source_) return source_->id();
		else return invalidNodeId();
	    }
	    NodeId targetId() const {
		if (target_) return target_->id();
		else return invalidNodeId();
	    }
	    EdgePointer linkOutgoing()  const { return linkOutgoing_; }
	    EdgePointer linkIncoming() const  { return linkIncoming_; }
	protected:
	    EdgeBase() :
		id_(invalidEdgeId()),
		source_(0), target_(0),
		linkOutgoing_(0), linkIncoming_(0)
		{}
	};

    private:
	void linkEdgeToTarget(EdgePointer e, NodePointer n) {
	    require(!e->target_);
	    e->target_ = n;
	    e->linkIncoming_ = n->incoming();
	    n->incoming_ = e;
	}

	void linkEdgeToSource(EdgePointer e, NodePointer n) {
	    require(!e->source_);
	    e->source_ = n ;
	    e->linkOutgoing_ = n->outgoing() ;
	    n->outgoing_ = e;
	}

	void unlinkEdgeFromSource(EdgePointer e) {
	    require(e->source_);
	    EdgePointer *ep ;
	    for (ep = &e->source_->outgoing_ ; *ep != e ; ep = &(*ep)->linkOutgoing_)
		verify(*ep) ;
	    *ep = e->linkOutgoing_ ;
	    e->source_ = 0 ;
	}

	void unlinkEdgeFromTarget(EdgePointer e) {
	    require(e->target_);
	    EdgePointer *ep ;
	    for (ep = &e->target_->incoming_ ; *ep != e ; ep = &(*ep)->linkIncoming_)
		verify(*ep) ;
	    *ep = e->linkIncoming_ ;
	    e->target_ = 0 ;
	}

    protected:
	typedef std::vector<NodePointer> NodeList;
	NodeList nodes_;
	typedef std::vector<EdgePointer> EdgeList;
	EdgeList edges_;

    private:
	template <class T, class I>
	class Iterator {
	public:
	    typedef Iterator Self ;
	    typedef T &Reference ;
	    typedef T *Pointer ;
	private:
	    I begin_, current_, end_;
	    friend class Graph;
	public:
	    Iterator(I b, I e) {
		begin_ = b;
		end_ = e;
		begin();
	    }
	    bool valid() const {
		return begin_ <= current_ && current_ < end_;
	    }

	    Reference item() const {
		require(valid());
		return **current_;
	    }

	    Reference operator*() const {
		return item();
	    }

	    Pointer pointer() const {
		require(valid());
		return *current_;
	    }

	    Pointer operator->() const {
		return pointer();
	    }

	    operator Pointer() const {
		return pointer();
	    }

	    void begin() {
		current_ = begin_;
		while (current_ < end_ && *current_ == 0) ++current_;
	    }

	    void end() {
		current_ = end_;
		do --current; while (current_ >= begin_ && *current_ == 0);
	    }

	    Self& operator++() {
		do ++current_; while (current_ < end_ && *current_ == 0);
		return *this;
	    }

	    Self operator++(int) {
		Self tmp = *this;
		do ++current_; while (current_ < end_ && *current_ == 0);
		return tmp;
	    }

	    Self& operator--() {
		do --current_; while (current_ >= begin_ && *current_ == 0);
		return *this;
	    }

	    Self operator--(int) {
		Self tmp = *this;
		do --current_; while (current_ >= begin_ && *current_ == 0);
		return tmp;
	    }
	};

    public:
	Graph() {};
	~Graph() { clear(); }

	typedef Iterator<      Node, typename NodeList::      iterator>       NodeIterator;
	typedef Iterator<const Node, typename NodeList::const_iterator> const_NodeIterator;

	NodeIterator nodes() {
	    return NodeIterator(nodes_.begin(), nodes_.end());
	}

	const_NodeIterator nodes() const {
	    return const_NodeIterator(nodes_.begin(), nodes_.end());
	}

	typedef Iterator<Edge, typename EdgeList::iterator> EdgeIterator_;
	class EdgeIterator : public EdgeIterator_ {
	public:
	    EdgeIterator(typename EdgeList::iterator b, typename EdgeList::iterator e) : EdgeIterator_(b, e) {}
	    NodePointer currentNode() const { return item().source(); }
	};
	typedef Iterator<const Edge, typename EdgeList::const_iterator> const_EdgeIterator_;
	class const_EdgeIterator : public const_EdgeIterator_ {
	public:
	    const_EdgeIterator(typename EdgeList::const_iterator b, typename EdgeList::const_iterator e) : const_EdgeIterator_(b, e) {}
	    NodePointer currentNode() const { return item().source(); }
	};

	EdgeIterator edges() {
	    return EdgeIterator(edges_.begin(), edges_.end());
	}

	const_EdgeIterator edges() const {
	    return const_EdgeIterator(edges_.begin(), edges_.end());
	}

	NodeId nNodeIds() const {
	    return nodes_.size();
	}

	NodePointer node(NodeId n) {
	    return nodes_[n];
	}

	const NodePointer node(NodeId n) const {
	    return nodes_[n];
	}

	EdgeId nEdgeIds() const {
	    return edges_.size();
	}

	EdgePointer edge(EdgeId e) {
	    return edges_[e];
	}

	const EdgePointer edge(EdgeId e) const {
	    return edges_[e];
	}

	NodePointer addNode(NodePointer n) {
	    require(n->id() == invalidNodeId());
	    n->id_ = nodes_.size();
	    nodes_.push_back(n);
	    return n;
	}

	NodePointer newNode() {
	    return addNode(new Node);
	}

	EdgePointer addEdge(EdgePointer e, NodePointer from, NodePointer to) {
	    require(e->id() == invalidEdgeId());
	    e->id_ = edges_.size();
	    edges_.push_back(e);
	    linkEdgeToSource(e, from);
	    linkEdgeToTarget(e, to);
	    return e;
	}

	EdgePointer addEdge(const Edge &e, NodePointer from, NodePointer to) {
	    return addEdge(new Edge(e), from, to);
	}


	void deleteNode(NodePointer n) {
	    EdgePointer e, *ne;
	    for (e = n->incoming() ; e ; e = ne) {
		unlinkEdgeFromSource(e);
		ne = e->linkIncoming();
		delete e;
	    }
	    for (e = n->outgoing() ; e ; e = ne) {
		unlinkEdgeFromTarget(e);
		ne = e->linkOutgoing();
		delete e;
	    }
	    verify(n == nodes_[n->id()]);
	    nodes_[n->id()] = 0;
	    delete n;
	}

	void deleteEdge(EdgePointer e) {
	    unlinkEdgeFromSource(e);
	    unlinkEdgeFromTarget(e);
	    verify(e == edges_[e->id()]);
	    edges_[e->id()] = 0;
	    delete e;
	}

	void clear() {
	    for (typename NodeList::iterator n = nodes_.begin(); n != nodes_.end(); ++n)
		delete *n;
	    nodes_.clear();
	    for (typename EdgeList::iterator e = edges_.begin(); e != edges_.end(); ++e)
		delete *e;
	    edges_.clear();
	};
    };

    template<class N, class E>
    class GraphWithInitialFinals : public Graph<N, E> {
	typedef  Graph<N, E> Precursor;
    private:
	typename Precursor::NodePointer initial_;
	typename Precursor::NodeList finals_;

    public:
	GraphWithInitialFinals() { initial_ = 0; }

	typename Precursor::NodePointer initial() { return initial_; }
	const typename Precursor::NodePointer initial() const { return initial_; }
	void setInitial(typename Precursor::NodePointer n) { initial_ = n; }

	u32 nFinals() const { return finals_.size(); }
	typename Precursor::NodePointer final() {
	    require(nFinals() <= 1);
	    return (nFinals()) ? *(finals_.begin()) : 0;
	}
	const typename Precursor::NodePointer final() const {
	    require(nFinals() <= 1);
	    return (nFinals()) ? *(finals_.begin()) : 0;
	}
	typename Precursor::NodeIterator finals() { return NodeIterator(finals_.begin(), finals_.end()); }
	typename Precursor::const_NodeIterator finals() const { return const_NodeIterator(finals_.begin(), finals_.end()); }
	bool isFinal(typename Precursor::NodePointer n) {}
	bool isFinal(const typename Precursor::NodeId &n) {}
	void addFinal(typename Precursor::NodePointer n) { finals_.push_back(n); }
	void addFinal(const typename Precursor::NodeId &id) { addFinal(nodes[id]); }

	void clear() {
	    Precursor::clear();
	    initial_ = 0;
	    finals_.clear();
	}
    };

    template<class G> class Drawer {
    public:
	virtual void drawGraph(std::ostream &os) const {
	    os << "ranksep = 1.0;" << std::endl
	       << "rankdir = LR;" << std::endl
	       << "size = \"8.5,11\";" << std::endl
	       << "center = 1;" << std::endl
	       << "orientation = Landscape" << std::endl
	       << "node [fontname=\"Helvetica\"]" << std::endl
	       << "edge [fontname=\"Helvetica\"]" << std::endl;
	}
	virtual void drawNode(const typename G::Node&, std::ostream&) const {}
	virtual void drawEdge(const typename G::Edge&, std::ostream&) const {}

	void draw(const  G &graph, std::ostream &os, const std::string &title) {
	    os << "digraph \"" << title.c_str() << "\" {" << std::endl;
	    drawGraph(os);
	    for (typename G::const_NodeIterator n = graph.nodes(); n.valid(); ++n) {
		os << "n" << (*n).id() << " ";
		drawNode(*n, os);
		os << std::endl;
	    }
	    for (typename G::const_EdgeIterator e = graph.edges(); e.valid(); ++e) {
		os << "n"     << e.currentNode()->id() << " -> n" << e->targetId() << " ";
		drawEdge(*e, os);
		os << std::endl;
	    }
	    os << "}" << std::endl;
	}
    };

} // namespace Core

#endif // _CORE_GRAPH_HH
