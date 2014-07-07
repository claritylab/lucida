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
#include <Core/Choice.hh>
#include <Core/XmlStream.hh>

#include "FlfCore/Basic.hh"
#include "Info.hh"
#include "NodeFactory.hh"
#include "Network.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    NetworkCrawler::NetworkCrawler(Network *network) : network_(network) {
	visited_.resize(network->nodes().size());
	reset();
    }
    bool NetworkCrawler::visit(Node *node) {
	verify(node && (node->id_ < visited_.size()));
	if (visited_[node->id_] == 0) {
	    visited_[node->id_] = ++n_;
	    return true;
	} else
	    return false;
    }
    bool NetworkCrawler::init(Node *node, const std::vector<std::string> &arguments) {
	verify(node);
	if (visit(node))
	    return node->init_(*this, arguments);
	else
	    return true;
    }
    bool NetworkCrawler::sync(Node *node) {
	if (visit(node))
	    return node->sync_(*this);
	else
	    return true;
    }
    void NetworkCrawler::finalize(Node *node) {
	if (visit(node)) node->finalize_(*this);
    }
    void NetworkCrawler::reset() {
	n_ = 0;
	std::fill(visited_.begin(), visited_.end(), 0);
    }
    void NetworkCrawler::dump(std::ostream &os) const {
	if (n_ == 0) {
	    os << "No node was visited." << std::endl;
	    return;
	}
	Network *network = const_cast<Network*>(network_);
	std::vector<NodeRef> ordered(n_);
	u32 nid = 0;
	for (std::vector<u32>::const_iterator it = visited_.begin(); it != visited_.end(); ++it, ++nid)
	    if (*it != 0)
		ordered[*it - 1] = network->nodes()[nid];
	u32 order = 1;
	for (std::vector<NodeRef>::const_iterator it = ordered.begin(); it != ordered.end(); ++it, ++order)
	    os << order << ". " << (*it)->name << std::endl;
    }
    // -------------------------------------------------------------------------



    // -------------------------------------------------------------------------
    const Node::Port Node::InvalidPort = Core::Type<Node::Port>::max;

    void Node::dump(std::ostream &o) const {
	o << "node \"" << name << "\"";
	if (in_.empty())  o << ", initial";
	if (out_.empty()) o << ", final";
	o << std::endl;
	if (!in_.empty()) {
	    o << "  incoming links:" << std::endl;
	    for (LinkList::const_iterator it = in_.begin(); it != in_.end(); ++it)
		if (it->node)
		    o << "    " << it->node->name << ":" << it->from << " --> " << it->to << std::endl;
	} else
	    o << "  no incoming links" << std::endl;
	if (!out_.empty()) {
	    o << "  outgoing links:" << std::endl;
	    for (LinkList::const_iterator it = out_.begin(); it != out_.end(); ++it)
		if (it->node)
		    o << "    " << it->from << " --> " << it->node->name << ":" << it->to << std::endl;
	} else
	    o << "  no outgoing links" << std::endl;
    }

    bool Node::init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	bool _good = true;
	for (LinkList::const_iterator it = in_.begin(); it != in_.end(); ++it)
	    if (it->node) _good = _good && crawler.init(it->node, arguments);
	init(arguments);
	_good = _good && good();
	return _good;
    }

    bool Node::sync_(NetworkCrawler &crawler) {
	bool _good = true;
	if (!blockSync())
	{
	for (LinkList::const_iterator it = in_.begin(); it != in_.end(); ++it)
	    if (it->node) _good = _good && crawler.sync(it->node);
	}else{
	    log() << "blocking synchronization";
	}
	sync();
	_good = _good && good();
	return _good;
    }

    void Node::finalize_(NetworkCrawler &crawler) {
	for (LinkList::const_iterator it = in_.begin(); it != in_.end(); ++it)
	    if (it->node) crawler.finalize(it->node);
	finalize();
    }

    ConstLatticeRef Node::requestLattice(Port from) {
	require(connected(from));
	return in_[from].node->sendLattice(in_[from].from);
    }
    Fsa::ConstAutomatonRef Node::requestFsa(Port from) {
	require(connected(from));
	return in_[from].node->sendFsa(in_[from].from);
    }
    ConstPosteriorCnRef Node::requestPosteriorCn(Port from) {
	require(connected(from));
	return in_[from].node->sendPosteriorCn(in_[from].from);
    }
    ConstConfusionNetworkRef Node::requestCn(Port from) {
	require(connected(from));
	return in_[from].node->sendCn(in_[from].from);
    }
    ConstSegmentRef Node::requestSegment(Port from) {
	require(connected(from));
	return in_[from].node->sendSegment(in_[from].from);
    }
    bool Node::requestBool(Port from) {
	require(connected(from));
	return in_[from].node->sendBool(in_[from].from);
    }
    s32 Node::requestInt(Port from) {
	require(connected(from));
	return in_[from].node->sendInt(in_[from].from);
    }
    f64 Node::requestFloat(Port from) {
	require(connected(from));
	return in_[from].node->sendFloat(in_[from].from);
    }
    std::string Node::requestString(Port from) {
	require(connected(from));
	return in_[from].node->sendString(in_[from].from);
    }
    const void * Node::requestData(Port from) {
	require(connected(from));
	return in_[from].node->sendData(in_[from].from);
    }
    ConstLatticeRef Node::sendLattice(Port to = 0) {
	criticalError("Node \"%s\" provides no data of type \"lattice\" at port %d.",
		      name.c_str(), to);
	return ConstLatticeRef();
    }
    Fsa::ConstAutomatonRef Node::sendFsa(Port to) {
	criticalError("Node \"%s\" provides no data of type \"fsa\" at port %d.",
		      name.c_str(), to);
	return Fsa::ConstAutomatonRef();
    }
    ConstPosteriorCnRef Node::sendPosteriorCn(Port to) {
	criticalError("Node \"%s\" provides no data of type \"posterior CN\" at port %d.",
		      name.c_str(), to);
	return ConstPosteriorCnRef();
    }
    ConstConfusionNetworkRef Node::sendCn(Port to) {
	criticalError("Node \"%s\" provides no data of type \"CN\" at port %d.",
		      name.c_str(), to);
	return ConstConfusionNetworkRef();
    }
    ConstSegmentRef Node::sendSegment(Port to) {
	criticalError("Node \"%s\" provides no data of type \"segment\" at port %d.",
		      name.c_str(), to);
	return ConstSegmentRef();
    }
    bool Node::sendBool(Port to) {
	criticalError("Node \"%s\" provides no data of type \"bool\" at port %d.",
		      name.c_str(), to);
	return false;
    }
    s32 Node::sendInt(Port to) {
	criticalError("Node \"%s\" provides no data of type \"int\" at port %d.",
		      name.c_str(), to);
	return 0;
    }
    f64 Node::sendFloat(Port to) {
	criticalError("Node \"%s\" provides no data of type \"float\" at port %d.",
		      name.c_str(), to);
	return 0.0;
    }
    std::string Node::sendString(Port to) {
	criticalError("Node \"%s\" provides no data of type \"string\" at port %d.",
		      name.c_str(), to);
	return "";
    }
    const void * Node::sendData(Port to) {
	criticalError("Node \"%s\" provides no data of type \"const void*\" at port %d.",
		      name.c_str(), to);
	return 0;
    }
    void Node::pull() {
	criticalError("Node \"%s\" is not supposed to be a final node.",
		      name.c_str());
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    const Core::ParameterBool FilterNode::paramInfo(
	"info",
	"info",
	false);
    const Core::ParameterString FilterNode::paramInfoType(
	"info-type",
	"info type",
	"normal");

    bool FilterNode::init_(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	if (!connected(0))
	    criticalError("FilterNode: Need a data source at port 0.");
	info_ = paramInfo(config);
	infoType_ = getInfoType(paramInfoType(config));
	return Precursor::init_(crawler, arguments);
    }

    ConstLatticeRef FilterNode::sendLattice(Port to) {
	 ConstLatticeRef l = filter(requestLattice());
	 if (info_) info(l, log(), infoType_);
	 return l;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
       Parse a config file defining a network and store the
       resulting nodes in topological order (if acyclic).
       Warn for cyclic graphs.
    **/
    class NetworkParser : public NodeFactory {
    public:
	static const Core::ParameterStringVector paramInitialNodes;
	static const Core::ParameterStringVector paramLinks;

    private:
	typedef enum { White, Gray, Black } Color;
	struct ColoredNode {
	    NodeRef node;
	    Color color;
	    ColoredNode(NodeRef node, Color color) : node(node), color(color) {}
	};
	typedef Core::hash_map<std::string, ColoredNode, Core::StringHash> ColoredNodeMap;

    private:
	Network &network_;

    private:
	void parse(const Core::Configuration &config) {
	    network_.nodes_.clear();
	    network_.initialNodes_.clear();
	    network_.finalNodes_.clear();

	    ColoredNodeMap coloredNodes;
	    std::vector<NodeRef> S;
	    std::vector<std::string> initials = paramInitialNodes(config);
	    for (std::vector<std::string>::const_iterator it = initials.begin(); it != initials.end(); ++it) {
		const std::string &name = *it;
		if (coloredNodes.find(name) != coloredNodes.end()) {
		    network_.error("Node \"%s\" is listed twice as initial node.",
				   name.c_str());
		    continue;
		}
		NodeRef node = createNode(name, Core::Configuration(config, name));
		if (!node) {
		    network_.criticalError(
			"Node \"%s\" does either not exist or does not specify a valid type.",
			it->c_str());
		    continue;
		}
		network_.initialNodes_.push_back(node);
		coloredNodes.insert(std::make_pair(name, ColoredNode(node, White)));
		S.push_back(node);
	    }
	    if (network_.initialNodes_.empty())
		network_.criticalError("No initial node specified.");
	    u32 order = 0;
	    bool isCyclic = false;
	    while (!S.empty()) {
		NodeRef node = S.back();
		ColoredNodeMap::iterator srcColoredNode = coloredNodes.find(node->name);
		verify(srcColoredNode != coloredNodes.end());
		if (srcColoredNode->second.color == White) {
		    srcColoredNode->second.color = Gray;
		    std::vector<std::string> links = paramLinks(Core::Configuration(config, node->name));
		    if (links.empty()) {
			network_.finalNodes_.push_back(node);
		    } else {
			for (std::vector<std::string>::const_iterator it = links.begin(); it != links.end(); ++it) {
			    std::string link(*it);
			    std::string name;
			    Node::Port from = 0, to = 0;
			    std::string::size_type j = link.find("->");
			    std::string::size_type k = link.rfind(":");
			    if ((j == std::string::npos) && (k == std::string::npos)) {
				name = link;
			    } else {
				if (j != std::string::npos) {
				    from = ::atoi(link.substr(0, j).c_str());
				    j += 2;
				} else
				    j = 0;
				if (k != std::string::npos) {
				    to = ::atoi(link.substr(k + 1).c_str());
				    name = link.substr(j, k - j);
				} else
				    name = link.substr(j);
			    }
			    NodeRef target;
			    ColoredNodeMap::iterator trgColoredNode = coloredNodes.find(name);
			    if (trgColoredNode == coloredNodes.end()) {
				target = createNode(name, Core::Configuration(config, name));
				if (!target) {
				    network_.criticalError(
					"Node \"%s\" does either not exist or does not specify a type",
					name.c_str());
				    continue;
				}
				coloredNodes.insert(std::make_pair(name, ColoredNode(target, White)));
				S.push_back(target);
			    } else {
				target = trgColoredNode->second.node;
				if (trgColoredNode->second.color == White)
				    S.push_back(target);
				else if (trgColoredNode->second.color == Gray)
				    isCyclic = true;
			    }
			    target->in_.grow(to);
			    if (target->in_[to].node) {
				network_.criticalError(
				    "Node \"%s\" has already an incoming arc at port %d",
				    name.c_str(), to);
				continue;
			    }
			    target->in_[to] = Node::Link(node.get(), from, to);
			    node->out_.push_back(Node::Link(target.get(), from, to));
			}
		    }
		} else {
		    if (srcColoredNode->second.color == Gray) {
			srcColoredNode->second.color = Black;
			srcColoredNode->second.node->id_ = order++;
		    }
		    S.pop_back();
		}
	    }
	    if (isCyclic)
		network_.warning("Network is cyclic.");
	    if (network_.finalNodes_.empty())
		network_.criticalError("No final node specified.");
	    network_.nodes_.resize(order--);
	    for (ColoredNodeMap::iterator itColoredNode = coloredNodes.begin();
		 itColoredNode != coloredNodes.end(); ++itColoredNode) {
		itColoredNode->second.node->id_ = order - itColoredNode->second.node->id_;
		network_.nodes_[itColoredNode->second.node->id_] = itColoredNode->second.node;
	    }
	}

    public:
	NetworkParser(Network &network) :
	    NodeFactory(),
	    network_(network) {
	    parse(network.config);
	}
    };
    const Core::ParameterStringVector NetworkParser::paramInitialNodes(
	"initial-nodes",
	"initial nodes; list of node names",
	"");
    const Core::ParameterStringVector NetworkParser::paramLinks(
	"links",
	"links; list of target nodes ([port->]name[:port])",
	"");


    Network::Network(const Core::Configuration &config) :
	Core::Component(config) {
    }

    Network * Network::createNetwork(const Core::Configuration &config) {
	Network *network = new Network(config);
	NetworkParser parse(*network);
	return network;
    }

    bool Network::init(NetworkCrawler &crawler, const std::vector<std::string> &arguments) {
	bool good = true;
	for (NodeList::iterator it = finalNodes_.begin(); it != finalNodes_.end(); ++it)
	    if (!crawler.init(it->get(), arguments)) good = false;
	return good;
    }

    bool Network::sync(NetworkCrawler &crawler) {
	bool good = true;
	for (NodeList::iterator it = finalNodes_.begin(); it != finalNodes_.end(); ++it)
	    if (!crawler.sync(it->get())) good = false;
	return good;
    }

    void Network::finalize(NetworkCrawler &crawler) {
	for (NodeList::iterator it = finalNodes_.begin(); it != finalNodes_.end(); ++it)
	    crawler.finalize(it->get());
    }

    void Network::pull() {
	for (NodeList::iterator it = finalNodes_.begin(); it != finalNodes_.end(); ++it)
	    (*it)->pull();
    }

    void Network::dump(std::ostream &o) const {
	for (NodeList::const_iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
	    (*it)->dump(o);
	    o << std::endl;
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class DummyFilterNode : public Node {
	friend class Network;
    public:
	DummyFilterNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~DummyFilterNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		warning("DummyFilterNode: No incoming lattice at port 0, node will crash on lattice request.");
	    log("\"%s\" is a dummy.", name.c_str());
	}
	ConstLatticeRef sendLattice(Port to) {
	    require(connected(0));
	    return requestLattice(0);
	}
    };
    NodeRef createDummyFilterNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DummyFilterNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LatticeBufferNode : public Node {
	friend class Network;
    private:
	ConstLatticeRef latticeBuffer_;
	bool isValid_;
	Port from_;
    public:
	LatticeBufferNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~LatticeBufferNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    from_ = InvalidPort;
	    for (Port from = 0; from < in().size(); ++from)
		if (in()[from].node) {
		    if (from_ != InvalidPort) {
			error("LatticeBufferNode: Cannot handle multiple data sources.");
			break;
		    } else
			from_ = from;
		}
	    if (from_ == InvalidPort)
		criticalError("LatticeBufferNode: Require a data source.");
	    isValid_ = false;
	}

	virtual void sync() {
	    latticeBuffer_.reset();
	    isValid_ = false;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    if (!isValid_) {
		latticeBuffer_ = requestLattice(from_);
		isValid_ = true;
	    }
	    return latticeBuffer_;
	}
    };
    NodeRef createLatticeBufferNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LatticeBufferNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class Sink : public Node {
	friend class Network;
    public:
	typedef enum {
	    SinkTypeLattice,
	    SinkTypeCn,
	    SinkTypePosteriorCn
	} SinkType;
	static const Core::Choice choiceSinkType;
	static const Core::ParameterChoice paramSinkType;
	static const Core::ParameterBool paramWarnOnEmpty;
	static const Core::ParameterBool paramErrorOnEmpty;
	// deprecated
	static const Core::ParameterBool paramWarnOnEmptyLattice;
	static const Core::ParameterBool paramErrorOnEmptyLattice;
    private:
	SinkType type_;
	bool warnOnEmpty_;
	bool errorOnEmpty_;
    protected:
	void reportEmpty(Port from) {
	    if (warnOnEmpty_)
		warning("Empty %s from \"%s:%d\"",
			choiceSinkType[type_].c_str(), in()[from].node->name.c_str(), in()[from].from);
	    if (errorOnEmpty_)
		criticalError("Empty %s from \"%s:%d\"",
			      choiceSinkType[type_].c_str(), in()[from].node->name.c_str(), in()[from].from);
	}
    public:
	Sink(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {
	    warnOnEmpty_ = paramWarnOnEmpty(config, paramWarnOnEmptyLattice(config));
	    errorOnEmpty_ = paramErrorOnEmpty(config, paramErrorOnEmptyLattice(config));
	}
	virtual ~Sink() {}

	void init(const std::vector<std::string> &arguments) {
	    if (!out().empty())
		error("Sink: Do not expect outgoing links.");
	    type_ = SinkType(paramSinkType(config));
	    log("%s is a \"%s\"-sink.", name.c_str(), choiceSinkType[type_].c_str());
	}

	void pull() {
	    switch (type_) {
	    case SinkTypeLattice:
		for (Port from = 0; from < in().size(); ++from)
		    if (in()[from].node)
			if (!requestLattice(from)) reportEmpty(from);
		break;
	    case SinkTypeCn:
		for (Port from = 0; from < in().size(); ++from)
		    if (in()[from].node)
			if (!requestCn(from)) reportEmpty(from);
		break;
	    case SinkTypePosteriorCn:
		for (Port from = 0; from < in().size(); ++from)
		    if (in()[from].node)
			if (!requestPosteriorCn(from)) reportEmpty(from);
		break;
	    default:
		defect();
	    }
	}
    };
    const Core::Choice Sink::choiceSinkType(
	"lattice", Sink::SinkTypeLattice,
	"CN", Sink::SinkTypeCn,
	"fCN", Sink::SinkTypePosteriorCn,
	Core::Choice::endMark());
    const Core::ParameterChoice Sink::paramSinkType(
	"sink-type",
	&Sink::choiceSinkType,
	"type of input",
	Sink::SinkTypeLattice);
    const Core::ParameterBool Sink::paramWarnOnEmpty(
	"warn-on-empty",
	"warn on empty lattice/cn/posterior-cn",
	true);
    const Core::ParameterBool Sink::paramErrorOnEmpty(
	"error-on-empty",
	"error on empty lattice/cn/posterior-cn",
	false);
    // deprecated
    const Core::ParameterBool Sink::paramWarnOnEmptyLattice(
	"warn-on-empty-lattice",
	"warn on empty lattice",
	true);
    const Core::ParameterBool Sink::paramErrorOnEmptyLattice(
	"error-on-empty-lattice",
	"error on empty lattice",
	false);

    NodeRef createSink(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new Sink(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class PropertiesNode : public FilterNode {
	friend class Network;
    public:
	static const Core::Choice tertiaryLogicChoice;
	static const Core::ParameterBool paramDumpProperties;
    private:
	Fsa::Property knownProperties_;
	Fsa::Property knownPropertyValues_;
	Fsa::Property unknownProperties_;
	bool dump_;
    private:
	void setProperty(Fsa::Property prop, const std::string &val) {
	    if (val.empty())
		return;
	    switch (tertiaryLogicChoice[val]) {
	    case 0:
		knownProperties_ |= prop;
		break;
	    case 1:
		knownProperties_ |= prop;
		knownPropertyValues_ |= prop;
		break;
	    case 2:
		unknownProperties_ |= prop;
	    default:
		defect();
	    }
	}
	void dumpProperties(ConstLatticeRef l, std::ostream &os) {
	    Fsa::Property _knownProperties = l->knownProperties();
	    Fsa::Property _properties = l->properties();
	    os << "properties of " << l->describe() << ":" << std::endl;
	    for (Fsa::Property prop = 1; prop != 0; prop <<= 1)
		if (_knownProperties & prop)
		    os << "  0x" << std::hex << std::setw(8) << std::setfill(' ') << std::right << prop
		       << ": " << ((_properties & prop) ? "true" : "false") << std::endl;
	}
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    l->setProperties(knownProperties_, knownPropertyValues_);
	    l->unsetProperties(unknownProperties_);
	    if (dump_) dumpProperties(l, log());
	    return l;
	}
    public:
	PropertiesNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config),
	    knownProperties_(0),
	    knownPropertyValues_(0),
	    unknownProperties_(0) {
	    dump_ = paramDumpProperties(config);
	}
	virtual ~PropertiesNode() {}
    };
    const Core::Choice PropertiesNode::tertiaryLogicChoice(
	"0",     0, "1",     1, "2",       2,
	"no",    0, "yes",   1, "dunno",   2,
	"false", 0, "true",  1, "unknown", 2,
	"off",   0, "on",    1,
	Core::Choice::endMark());
	const Core::ParameterBool PropertiesNode::paramDumpProperties(
	    "dump",
	    "dump properties",
	    false);

    NodeRef createPropertiesNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new PropertiesNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class SegmentBuilderNode : public Node {
    private:
	Core::XmlChannel progressChannel_;
	ConstSegmentRef segment_;

    public:
	SegmentBuilderNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config), progressChannel_(config, "progress") {}
	virtual ~SegmentBuilderNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    bool hasInput = false;
	    {
		Core::Component::Message msg(log());
		for (u32 i = 0; i <= 9; ++i)
		    if (connected(i)) {
			hasInput = true;
			switch (i) {
			case 0: msg << "Get bliss speech segment at port 0.\n"; break;
			case 1: msg << "Get audio file name  at port 1.\n"; break;
			case 2: msg << "Get start time at port 2.\n"; break;
			case 3: msg << "Get end time at port 3.\n"; break;
			case 4: msg << "Get track at port 4.\n"; break;
			case 5: msg << "Get orthography at port 5.\n"; break;
			case 6: msg << "Get speaker id at port 6.\n"; break;
			case 7: msg << "Get condition id at port 7.\n"; break;
			case 8: msg << "Get recording id at port 8.\n"; break;
			case 9: msg << "Get segment id at port 9.\n"; break;
			}
		    }
	    }
	    if (!hasInput)
		warning("No input; the same default segment will be returned each time.");
	}

	virtual void sync() {
	    segment_.reset();
	}

	virtual ConstSegmentRef sendSegment(Port to) {
	    if (!segment_) {
		Segment *segment = new Segment;
		if (connected(0))
		    segment->setBlissSpeechSegment(static_cast<const Bliss::SpeechSegment*>(requestData(0)));
		if (connected(1))
		    segment->setAudioFilename(requestString(1));
		if (connected(2))
		    segment->setStartTime(requestFloat(2));
		if (connected(3))
		    segment->setEndTime(requestFloat(3));
		if (connected(4))
		    segment->setTrack(requestInt(4));
		if (connected(5))
		    segment->setOrthography(requestString(5));
		if (connected(6))
		    segment->setSpeakerId(requestString(6));
		if (connected(7))
		    segment->setConditionId(requestString(7));
		if (connected(8))
		    segment->setRecordingId(requestString(8));
		if (connected(9))
		    segment->setSegmentId(requestString(9));
		segment_ = ConstSegmentRef(segment);
		if (progressChannel_.isOpen()) {
		    Core::XmlEmpty xmlSegment("segment");
		    xmlSegment + Core::XmlAttribute("name", segment_->segmentId());
		    if (segment->hasRecordingId())
			xmlSegment + Core::XmlAttribute("recording-id", segment_->recordingId());
		    if (segment->hasAudioFilename())
			xmlSegment + Core::XmlAttribute("audio", segment_->audioFilename());
		    if (segment->hasTrack())
			xmlSegment + Core::XmlAttribute("track", segment_->track());
		    if (segment->hasStartTime() && segment->hasEndTime())
			xmlSegment + Core::XmlAttribute("start", segment_->startTime()) + Core::XmlAttribute("end", segment_->endTime());
		    if (segment->hasSpeakerId())
			xmlSegment + Core::XmlAttribute("speaker-id", segment_->speakerId());
		    if (segment->hasConditionId())
			xmlSegment + Core::XmlAttribute("condition-id", segment_->conditionId());
		    progressChannel_ << xmlSegment;
		}
	    }
	    return segment_;
	}
    };
    NodeRef createSegmentBuilderNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new SegmentBuilderNode(name, config));
    }
    // -------------------------------------------------------------------------

    // -------------------------------------------------------------------------
    class AppendScoresNode : public Node {
    private:
	ConstSemiringRef semiring1_;
	ConstSemiringRef semiring2_;
	ConstSemiringRef semiring_;
	ConstLatticeRef appendL_;

    private:
	ConstSemiringRef buildSemiring(ConstSemiringRef semiring1, ConstSemiringRef semiring2) {
	    if (semiring_
		&& ((semiring1_.get() == semiring1.get()) || (*semiring1_ == *semiring1))
		&& ((semiring2_.get() == semiring2.get()) || (*semiring2_ == *semiring2)))
		return semiring_;
	    semiring1_ = semiring1;
	    semiring2_ = semiring2;
	    verify(semiring1_->type() == semiring2_->type());
	    ScoreList scales(semiring1_->scales());
	    scales.insert(scales.end(), semiring2_->scales().begin(), semiring2_->scales().end());
	    KeyList keys(semiring1_->keys());
	    keys.insert(keys.end(), semiring2_->keys().begin(), semiring2_->keys().end());
	    semiring_ = Semiring::create(semiring1_->type(), semiring1_->size() + semiring2_->size(), scales, keys);
	    return semiring_;
	}

    public:
	AppendScoresNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~AppendScoresNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!(connected(0) && connected(1)))
		criticalError("Incoming lattices at port 0 and port 1 required.");
	}

	virtual void finalize() {}

	virtual ConstLatticeRef sendLattice(Port to) {
	    if (!appendL_) {
	    ConstLatticeRef l1 = requestLattice(0);
	    ConstLatticeRef l2 = requestLattice(1);
	    if (l1 && l2)
		appendL_ = appendScores(l1, l2, buildSemiring(l1->semiring(), l2->semiring()));
	    }
	    return appendL_;
	}

	virtual void sync() {
	    appendL_.reset();
	}
    };
    NodeRef createAppendScoresNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new AppendScoresNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
