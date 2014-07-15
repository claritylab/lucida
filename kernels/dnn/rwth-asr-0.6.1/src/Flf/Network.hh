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
#ifndef _FLF_NETWORK_HH
#define _FLF_NETWORK_HH

#include <Bliss/CorpusDescription.hh>
#include <Core/Component.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>
#include <Fsa/Automaton.hh>

#include "FlfCore/Lattice.hh"
#include "Segment.hh"


namespace Flf {

    class Node;
    class Network;
    class NetworkCrawler;
    class NetworkParser;


    class NetworkCrawler : public Core::ReferenceCounted {
    private:
	const Network *network_;
	u32 n_;
	std::vector<u32> visited_;
    private:
	bool visit(Node *node);
    public:
	NetworkCrawler(Network *network);
	bool init(Node *node, const std::vector<std::string> &arguments);
	bool sync(Node *node);
	void finalize(Node *node);
	void reset();
	void dump(std::ostream &os) const;
    };
    typedef Core::Ref<NetworkCrawler> NetworkCrawlerRef;

    class Node : public Core::Component, public Core::ReferenceCounted {
	friend class Network;
	friend class NetworkParser;
	friend class NetworkCrawler;
    public:
	typedef u32 Port;
	static const Port InvalidPort;

	struct Link {
	    Node * node;
	    Port from, to;
	    Link() : node(0) {}
	    Link(Node *node, Port from, Port to) : node(node), from(from), to(to) {}
	};
	typedef Core::Vector<Link> LinkList;

    public:
	const std::string name;

    private:
	u32 id_;
	LinkList in_;
	LinkList out_;
    protected:
	/**
	 * init_ is called directly before init(...)
	 * init_ should only be used by intermediate nodes and has to be
	 * passed through (in opposite to init(...), which should not be used
	 * by intermediate nodes at all)
	 * dito for sync_ and finalize_
	 **/
	virtual bool init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments);
	virtual bool sync_(NetworkCrawler &crawler);
	virtual void finalize_(NetworkCrawler &crawler);

    public:
	LinkList & in()  { return in_; }
	LinkList & out() { return out_; }

	bool connected(Port from) const {
	    return (from < in_.size()) && in_[from].node;
	}

	/**
	 * This section defines request handler for some common data types.
	 * Note:
	 * Deliberately, no advanced, powerful, abstract data type handling
	 * was implemented. Keep software simple!
	 **/
	ConstLatticeRef          requestLattice    (Port from = 0);
	Fsa::ConstAutomatonRef   requestFsa        (Port from);
	ConstPosteriorCnRef      requestPosteriorCn(Port from);
	ConstConfusionNetworkRef requestCn         (Port from);
	ConstSegmentRef          requestSegment    (Port from);
	bool                     requestBool       (Port from);
	s32                      requestInt        (Port from);
	f64                      requestFloat      (Port from);
	std::string              requestString     (Port from);
	const void *             requestData       (Port from);

    public:
	Node(const std::string &name, const Core::Configuration &config) :
	    Core::Component(config),
	    name(name) {}
	virtual ~Node() {}

	/**
	 * init, good, pull, and finalize
	 * are making up the used network
	 * protocol. they are called in
	 * the following order.
	 *
	 * init
	 * while good:
	 *     if is_final_node: pull
	 *     sync
	 * finalize
	 **/

	/**
	 * Is called after network is completely build and
	 * before the first lattice is processed by any node.
	 *
	 * <arguments> is the argument of Core::Appliation::main,
	 * i.e. the not-sprint relevant command line arguments.
	**/
	virtual void init(const std::vector<std::string> &arguments) {}

	/**
	 * Is called at every node before pull is called at the final nodes (see below);
	 * all nodes are initailized before sync is called the first time.
	 * The sync request is propagated from the final nodes through all nodes which return
	 * 'false' on blockSync() (see below)
	**/
	virtual void sync() {}

	virtual bool blockSync() const { return false; }

	/**
	 * Is called immideately after sync and has to return true,
	 * iff another data request is accepted.
	 * By default a node with no incoming arc is not good;
	**/
	virtual bool good() { return !in_.empty(); }

	/**
	 * Finalize is called before any node in the network is deallocated.
	**/
	virtual void finalize() {}

	/**
	 * Is called after a sync for each final node.
	**/
	virtual void pull();

	/**
	 * This section defines send handler for some common data types.
	 * Note:
	 * Deliberately, no advanced, powerful, abstract data type handling
	 * was implemented. Keep software simple!
	 **/
	// reference counted
	virtual ConstLatticeRef          sendLattice    (Port to);
	virtual Fsa::ConstAutomatonRef   sendFsa        (Port to);
	virtual ConstPosteriorCnRef      sendPosteriorCn(Port to);
	virtual ConstConfusionNetworkRef sendCn         (Port to);
	virtual ConstSegmentRef          sendSegment    (Port to);
	// primitive data types
	virtual bool                     sendBool       (Port to);
	virtual s32                      sendInt        (Port to);
	virtual f64                      sendFloat      (Port to);
	virtual std::string              sendString     (Port to);
	// arbitrary data
	virtual const void *             sendData       (Port to);

	void dump(std::ostream &o) const;
    };
    typedef Core::Ref<Node> NodeRef;

    /*
      - Nodes, having at least at port 0 lattice as input and one as output
    */
    class FilterNode : public Node {
	typedef Node Precursor;
	friend class Network;
    public:
	static const Core::ParameterBool paramInfo;
	static const Core::ParameterString paramInfoType;
    private:
	bool info_;
	InfoType infoType_;
    protected:
	virtual bool init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments);
	virtual ConstLatticeRef filter(ConstLatticeRef l) = 0;
    public:
	FilterNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~FilterNode() {}
	virtual ConstLatticeRef sendLattice(Port to);
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class Network : public Core::Component {
	friend class NetworkCrawler;
	friend class NetworkParser;
    public:
	typedef std::vector<NodeRef> NodeList;

    protected:
	NodeList nodes_;
	NodeList initialNodes_;
	NodeList finalNodes_;

    public:
	Network(const Core::Configuration &config);

	bool init(NetworkCrawler &crawler, const std::vector<std::string> &arguments);
	void pull();
	bool sync(NetworkCrawler &crawler);
	void finalize(NetworkCrawler &crawler);

	NodeList & nodes() { return nodes_; }

	void dump(std::ostream &o) const;

	static Network * createNetwork(const Core::Configuration &config);
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /*
      Basic Nodes
    */

    /*
      - if it has a connection at port 0, it behaves like a filter node,
	but leaves the lattice untouched. else, it does nothing.
    */
    NodeRef createDummyFilterNode(const std::string &name, const Core::Configuration &config);

    /*
      - incoming lattice(data) is bufferd until next sync
    */
    NodeRef createLatticeBufferNode(const std::string &name, const Core::Configuration &config);

    /*
      - absorbes all incoming lattices/cns/posterior-cns
      - serves always as final node; expects a sequence of sync-pull calls
    */
    NodeRef createSink(const std::string &name, const Core::Configuration &config);

    /*
      - set properties
    */
    NodeRef createPropertiesNode(const std::string &name, const Core::Configuration &config);

    /*
      - build a segment out of incoming a bliss speech segment or basic data types
    */
    NodeRef createSegmentBuilderNode(const std::string &name, const Core::Configuration &config);

    /*
      - concatenate the scores of two topologically equal (down to state numbering) lattices
    */
    NodeRef createAppendScoresNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf

#endif // _FLF_NETWORK_HH
