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
#include <Core/Application.hh>
#include "Network.hh"
#include "Registry.hh"
#include "NetworkParser.hh"
#include "Filter.hh"
#include <iterator>

using namespace Flow;

//initialization of static members:

/*****************************************************************************/
Network::Port::Port(const std::string &name) :
/*****************************************************************************/
	name_(name), link_(0), node_(0), nodePort_(IllegalPortId), linkConnected_(false)
{
}

/*****************************************************************************/
void Network::Port::setLink(Link *link)
/*****************************************************************************/
{
	require(link != 0 && link_ == 0);
	link_ = link;
}

/*****************************************************************************/
void Network::Port::setNode(AbstractNode *node, PortId nodePort)
/*****************************************************************************/
{
	require(node != 0 && node_ == 0);
	require(nodePort != IllegalPortId && nodePort_ == IllegalPortId);

	node_ = node;
	nodePort_ = nodePort;
}

/*****************************************************************************/
const Core::ParameterString Network::paramFilename(
	"file",
	"name of Flow network file to load");
/*****************************************************************************/

const std::string Network::inputRepeaterPrefix = "__input_";

/*****************************************************************************/
Network::Network(const Core::Configuration &c, bool shouldLoad) :
/*****************************************************************************/
	Component(c),
	Precursor(c),
	dumpChannel_(c, "flow-dump-channel"),
	typeName_("network"),
	started_(false),
	lastNonData_(Data::eos())
{
	if (shouldLoad)
		buildFromFile(paramFilename(config));
}

/*****************************************************************************/
Network::~Network()
/*****************************************************************************/
{
	if(dumpChannel_.isOpen()) {
		if(!dump(true, dumpChannel_))
			warning("dump of '%s' failed!", typeName_.c_str());
	}
	//if (started_) stopThread();
	for (std::list<Link*>::const_iterator it = links_.begin(); it != links_.end();
		it++) delete *it;

	for (std::list<AbstractNode*>::const_iterator it = nodes_.begin(); it != nodes_.end();
		it++) delete *it;
}


/*****************************************************************************/
void Network::buildFromString(const std::string &network)
/*****************************************************************************/
{
	NetworkParser parser(*this, config);
	if (!parser.buildFromString(network))
		error("construction of network failed.");
}


/*****************************************************************************/
void Network::buildFromFile(const std::string &filename)
/*****************************************************************************/
{
	log("building Flow network from \"%s\" ...", filename.c_str());
	NetworkParser parser(*this, config);
	if (!parser.buildFromFile(filename))
		error("construction of network failed.");
}


/*****************************************************************************/
bool Network::addNode(AbstractNode *node)
/*****************************************************************************/
{
	if (!node)
		return false;

	if (getNode(node->name()) != 0)
		return false;

	nodes_.push_back(node);
	return true;
}


/*****************************************************************************/
AbstractNode* Network::getNode(const std::string &name)
/*****************************************************************************/
{
	std::list<AbstractNode*>::const_iterator it;

	for (it = nodes_.begin(); it != nodes_.end(); it++)
		if ((*it)->name() == name) return *it;

	return 0;
}

/*****************************************************************************/
bool Network::addLink(const std::string &fromNodeName, const std::string &fromPortName,
	const std::string &toNodeName, const std::string &toPortName,
	u32 buffer)
/*****************************************************************************/
{
	Link *l = new Link();
	bool result = true;
	// connect to network
	if (toNodeName == typeName_) {
		if (fromNodeName == typeName_) {
			if (!connectOutputFromInside(inputRepeaterPrefix + fromPortName, "data", toPortName, l))
				result = false;
		} else {
			if (!connectOutputFromInside(fromNodeName, fromPortName, toPortName, l))
				result = false;
		}
	} else {
		// connect 'from' node
		if (fromNodeName == typeName_) {
			if (!connectInputFromInside(fromPortName, l, buffer))
				result = false;
		} else {
			if (!connectNodeOutput(fromNodeName, fromPortName, l))
				result = false;
		}
		// connect 'to' node
		if (!connectNodeInput(toNodeName, toPortName, l))
			result = false;
	}

	if (!result) {
		delete l;
		return result;
	}

	l->setBuffer(buffer);
	// save names of the connections for the dump
	l->setNodeNames(fromNodeName, fromPortName, toNodeName, toPortName);
	links_.push_back(l);

	return true;
}


/*********************************************************************************/
bool Network::connectInputFromInside(const std::string &portName, Link *l, u32 buffer)
/*********************************************************************************/
{
	PortId portId = getInput(portName);
	if (portId == IllegalPortId) {
		error("could not find input '%s' in network.", portName.c_str());
		return false;
	}
	Port &port = inputs_[portId];

	AbstractNode *repeater = port.node();
	verify(repeater != 0);
	if (repeater->connectOutputPort(port.nodePort(), l) == IllegalPortId) {
		error("could not connect input '%s' in network.", portName.c_str());
		return false;
	}

	// create link for putData
	if (port.link() == 0) {
		Link *portLink = new Link;
		portLink->setBuffer(buffer);
		port.setLink(portLink);
		links_.push_back(portLink);
		connectInputPortLink(portId);
	}
	return true;
}


/*****************************************************************************/
bool Network::connectOutputFromInside(const std::string &fromNodeName,
	const std::string &fromPortName,
	const std::string &toPortName,
	Link *l)
/*****************************************************************************/
{
	PortId toPortId = getOutput(toPortName);
	if (toPortId == IllegalPortId) {
		error("could not find output '%s' in network.", toPortName.c_str());
		return false;
	}

	Port &port = outputs_[toPortId];
	AbstractNode* fromNode = getNode(fromNodeName);
	if (fromNode == 0) {
		error("could not find node '%s' in network.", fromNodeName.c_str());
		return false;
	}

	PortId fromPort = fromNode->getOutput(fromPortName);
	if (fromPort == IllegalPortId) {
		error("could not connect output '%s' at node '%s'.",
			fromPortName.c_str(), fromNodeName.c_str());
		return false;
	}

	if (port.node()) {
		error("Output port '%s' has already been connected.", toPortName.c_str());
		return false;
	}
	port.setNode(fromNode, fromPort);
	port.setLink(l);
	connectOutputPortLink(toPortId);

	return true;
}


/************************************************************************************************/
bool Network::connectNodeInput(const std::string &nodeName, const std::string &portName, Link *l)
/************************************************************************************************/
{
	AbstractNode* toNode = getNode(nodeName);
	if (toNode == 0) {
		error("could not find node '%s' in network.", nodeName.c_str());
		return false;
	}

	PortId toPort = toNode->getInput(portName);
	if ((toPort == IllegalPortId) ||
		(toNode->connectInputPort(toPort, l) == IllegalPortId)) {
		error("could not connect input '%s' at node '%s'.",
			portName.c_str(), nodeName.c_str());
		return false;
	}
	return true;
}

/**************************************************************************************************/
bool Network::connectNodeOutput(const std::string &nodeName, const std::string &portName, Link *l)
/**************************************************************************************************/
{
	AbstractNode* fromNode = getNode(nodeName);
	if (fromNode == 0) {
		error("could not find node '%s' in network.", nodeName.c_str());
		return false;
	}

	PortId fromPort = fromNode->getOutput(portName);
	if ((fromPort == IllegalPortId) ||
		(fromNode->connectOutputPort(fromPort, l) == IllegalPortId)) {
		error("could not connect output '%s' at node '%s'.",
			portName.c_str(), nodeName.c_str());
		return false;
	}
	return true;
}

/*****************************************************************************/
void Network::connectInputPortLink(PortId in)
/*****************************************************************************/
{
	require(validInputPort(in));
	Port &port = inputs_[in];

	verify(!port.linkConnected());
	verify(port.node() != 0);
	verify(port.link() != 0);
	verify(port.nodePort() != IllegalPortId);

	port.node()->connectInputPort(port.nodePort(), port.link());
	port.link()->setFromNode(this);
	// Note: 'in' is actually input port of the network. It will be used in the function work().
	port.link()->setFromPort(in);
	port.link()->configure();
	port.setLinkConnected();
}

/*****************************************************************************/
void Network::disconnectInputPortLink(PortId in)
/*****************************************************************************/
{
	require(validInputPort(in));
	Port &port = inputs_[in];

	verify(port.linkConnected());
	port.node()->disconnectInputLink(port.link());
	port.link()->setFromNode(0);
	port.link()->setFromPort(IllegalPortId);
	port.unsetLinkConnected();
}

/*****************************************************************************/
void Network::connectOutputPortLink(PortId out)
/*****************************************************************************/
{
	require(validOutputPort(out));
	Port &port = outputs_[out];

	verify(!port.linkConnected());
	verify(port.node() != 0);
	verify(port.nodePort() != IllegalPortId);
	verify(port.link() != 0);

	port.node()->connectOutputPort(port.nodePort(), port.link());
	port.link()->setToNode(this);
	port.link()->setToPort(IllegalPortId);
	port.setLinkConnected();
}

/*****************************************************************************/
void Network::disconnectOutputPortLink(PortId out)
/*****************************************************************************/
{
	require(validOutputPort(out));
	Port &port = outputs_[out];

	verify(port.linkConnected());
	port.node()->disconnectOutputLink(port.link());
	port.link()->setToNode(0);
	port.link()->setToPort(IllegalPortId);
	port.unsetLinkConnected();
}

/*****************************************************************************/
void Network::activateOutput(PortId out)
/*****************************************************************************/
{
	require(validOutputPort(out));
	Port &port = outputs_[out];

	if (!port.linkConnected())
		connectOutputPortLink(out);
}

/*****************************************************************************/
bool Network::configureOutputPort(Port &out)
/*****************************************************************************/
{
	ensure(out.linkConnected());
	if (!out.link()->areAttributesAvailable()) {
		AbstractNode *lastNode = out.link()->getFromNode();
		if (!lastNode) {
			error("Output port is not connected.");
			return false;
		}
		if (!lastNode->configure()) {
			error("Configuration of node failed.");
			return false;
		}
		// Nodes report configuration error to the application object, too.
		if (Core::Application::us()->hasFatalErrors())
			return false;
		// If fails, from-node of the input link did not generate ouptut attributes
		// although configuration did not fail.
		ensure(out.link()->areAttributesAvailable());
	}
	return true;
}

/*************************************************************************************/
bool Network::addParameterUse(
	AbstractNode *node, const std::string &name, const Core::StringExpression &value)
/*************************************************************************************/
{
	bool success = false;
	std::list<Network::Parameter>::iterator it;
	for (it = params_.begin(); it != params_.end(); it++) {
		if (value.hasVariable(it->name())) {
			it->addUse(node, name);
			success = true;
		}
	}
	return success;
}

/*****************************************************************************/
bool Network::addInput(const std::string &name)
/*****************************************************************************/
{
	// add repeater node
	const _Filter *f = Flow::Registry::instance().getFilter("generic-repeater");
	if (!f) {
		error("repeater filter not registered.");
		return false;
	}
	AbstractNode *n = f->newNode(select(inputRepeaterPrefix + name));
	if (!n) {
		error("could not create repeater node.");
		return false;
	}
	addNode(n);

	Port port(name);
	port.setNode(n, n->getInput("data"));

	inputs_.push_back(port);

	return true;
}

/*****************************************************************************/
bool Network::addOutput(const std::string &name)
/*****************************************************************************/
{
	outputs_.push_back(Port(name));
	return true;
}

/*****************************************************************************/
PortId Network::getInput(const std::string &name)
/*****************************************************************************/
{
	Network::Port p(name);

	for (u32 i = 0; i < inputs_.size(); i ++) {
		if (inputs_[i] == p)
			return i;
	}

	return IllegalPortId;
}

/*****************************************************************************/
PortId Network::getOutput(const std::string &name)
/*****************************************************************************/
{
	Network::Port p(name);

	for (u32 i = 0; i < outputs_.size(); i++) {
		if (outputs_[i] == p)
			return i;
	}

	return IllegalPortId;
}

/*****************************************************************************/
void Network::outputs(
		std::vector<std::pair<PortId, std::string> > &outputs) const
/*****************************************************************************/
{
	outputs.resize(outputs_.size());
	for(PortId port = 0; port < (PortId)outputs_.size(); ++ port)
		outputs[port] = std::make_pair(port, outputs_[port].name());
}

/*****************************************************************************/
bool Network::putData(PortId in, Data *d)
/*****************************************************************************/
{
	require(validInputPort(in));
	Port &port = inputs_[in];
	verify(port.linkConnected());
	if (Data::isSentinel(d))
		lastNonData_ = d;
	return port.link()->putData(d);
}

/*****************************************************************************/
bool Network::putAttributes(PortId in, Core::Ref<const Attributes> attributes)
/*****************************************************************************/
{
	require(validInputPort(in));
	require(attributes);
	Port &port = inputs_[in];
	verify(port.linkConnected());

	port.link()->setAttributes(attributes);
	port.link()->clear();
	port.link()->configure();

	return true;
}

/*****************************************************************************/
const std::string Network::getAttribute(PortId out, const std::string &name)
/*****************************************************************************/
{
	require(validOutputPort(out));
	Port &port = outputs_[out];
	if (!port.linkConnected()) {
		// see also activateOutput
		error("Output port '%s' is not connected to any node or port.", port.name().c_str());
		return "";
	}
	Link *l = port.link();
	ensure_(l != 0);
	if ((!l->areAttributesAvailable()) && (!configureOutputPort(port)))
		return "";
	ensure(l->areAttributesAvailable());
	return l->attributes()->get(name);
}


/*****************************************************************************/
bool Network::setParameter(const std::string &name, const std::string &value)
/*****************************************************************************/
{
	return setUserDefinedParameter(name, value);
	/*
	if (paramIgnoreUnknownParameters.match(name))
	ignoreUnknownParameters_ = paramIgnoreUnknownParameters(value);
	else
	return setUserDefinedParameter(name, value);
	return true;
	*/
}

/*****************************************************************************/
bool Network::setUserDefinedParameter(const std::string &name, const std::string &value)
/*****************************************************************************/
{
	Network::Parameter *found = 0;
	for (std::list<Network::Parameter>::iterator it = params_.begin();
		it != params_.end(); it++) {
		if ((*it).name() == name) {
			verify(found == 0); // Fails if 'name' found twice.
			found = &(*it);
		}
	}
	if (found != 0) {
		const std::vector<Parameter::Use> &list(found->getUses());
		for (std::vector<Parameter::Use>::const_iterator used = list.begin(); used != list.end(); ++used) {
			if (!used->by->setNetworkParameter(used->as, name, value))
				return false;
		}
		//stopThread();
		return true;
	}
	/*
	else if (ignoreUnknownParameters_)
	return true;
	*/
	return false;
}

/*****************************************************************************/
PortId Network::connectInputPort(PortId in, Link *l)
/*****************************************************************************/
{
	ensure(validInputPort(in));
	Port &port = inputs_[in];
	verify(port.node() != 0);

	/*
	 * Input ports are initialized for external usage, i.e. the input ports are used directly
	 * by external calls to Network::putData. If the network is embedded in an other network
	 * then the port 'in' is deactivated, i.e. the link of the port is disconnected from its to-node,
	 * and the new link 'l' is directly connected to this node.
	 */
	if (port.linkConnected()) {
		verify(l != port.link());
		disconnectInputPortLink(in);
	}
	if (port.node()->connectInputPort(port.nodePort(), l) == IllegalPortId)
		defect();
	return in;
}

/*****************************************************************************/
void Network::disconnectInputLink(Link *l)
/*****************************************************************************/
{
	require(l != 0);
	verify(l->getFromNode() != this); // It would imply that 'l' is a input port link.
	require(l->getToNode() != this); // It would lead to a endless recursion.
	l->getToNode()->disconnectInputLink(l);
}

/*****************************************************************************/
PortId Network::connectOutputPort(PortId out, Link *l)
/*****************************************************************************/
{
	require(validOutputPort(out));

	Port &port = outputs_[out];
	if (port.node() == 0) {
		error("Output port \"%s\" not connected to any source node.", port.name().c_str());
		return IllegalPortId;
	}

	/*
	 * Output ports are initialized for external usage, i.e. the output ports are used directly
	 * by external calls to Network::getData. If the network is embedded in an other network
	 * then the port 'out' is deactivated, i.e. the link of the port is disconnected from its from-node,
	 * and the new link 'l' is directly connected to this node.
	 */
	if (port.linkConnected()) {
		verify(l != port.link());
		disconnectOutputPortLink(out);
	}
	verify(port.nodePort() != IllegalPortId);
	port.node()->connectOutputPort(port.nodePort(), l);
	return out;
}

/*****************************************************************************/
void Network::disconnectOutputLink(Link *l)
/*****************************************************************************/
{
	require(l);
	require(l->getToNode() != 0);
	verify(l->getToNode() != this); // It would imply that 'l' is a output port link.
	verify(l->getFromNode() != this); // It would lead to a endless recursion.
	l->getFromNode()->disconnectOutputLink(l);
}

/*****************************************************************************/
u32 Network::nOutputLinks(PortId out) const
/*****************************************************************************/
{
	require(validOutputPort(out));

	const Port &port = outputs_[out];

	verify(port.node() != 0);
	verify(port.nodePort() != IllegalPortId);

	return port.node()->nOutputLinks(port.nodePort());
}

/*****************************************************************************/
bool Network::configure()
/*****************************************************************************/
{
	// This function is only called if an input port link of the network has no attributes.
	for(std::vector<Port>::iterator port = inputs_.begin(); port != inputs_.end(); ++ port) {
		if (port->linkConnected() && !port->link()->areAttributesAvailable()) {
			warning("Unconfigured input port: '%s'.", port->name().c_str());
			port->link()->setAttributes(Core::ref(new Attributes()));
		}
	}
	return true;
}

/*****************************************************************************/
bool Network::work(PortId in)
/*****************************************************************************/
{
	// This function is only called if an input port link of the network is empty.
	require(validInputPort(in));
	return putData(in, lastNonData_);
}

/*****************************************************************************/
void Network::reset()
/*****************************************************************************/
{
	for(std::vector<Port>::iterator inputPort = inputs_.begin();
		inputPort != inputs_.end(); ++ inputPort) {
		inputPort->node()->eraseOutputAttributes();
		inputPort->link()->clear();
	}
	for(std::vector<Port>::iterator outputPort = outputs_.begin();
		outputPort != outputs_.end(); ++ outputPort) {
		configureOutputPort(*outputPort);
	}
}

/*****************************************************************************/
void Network::go()
/*****************************************************************************/
{
	/*
	for (list<AbstractNode*>::const_iterator n = nodes_.begin(); n != nodes_.end(); n++) {
		(*n)->Start();
	}
	*/
	// gather sinks
	std::list<AbstractNode*> sinks;
	for (std::list<AbstractNode*>::const_iterator n = nodes_.begin(); n != nodes_.end(); n++) {
		if (((*n)->nOutputs() == 0) && (!(*n)->isThreaded()))
			sinks.push_back(*n);
	}

	// loop over all sinks
	while (sinks.size()) {
		for (std::list<AbstractNode*>::iterator n = sinks.begin(); n != sinks.end(); n++) {
			if (!(*n)->work(0)) {
				sinks.erase(n);
				break;
			}
		}
	}
}


/*****************************************************************************/
bool Network::dump(const bool initialCall, Core::XmlChannel& dumpChannel, std::set<std::string> *dumped)
/*****************************************************************************/
{
	// <network> is the root element
	if(initialCall)
		dumpChannel << Core::XmlOpen("network")
			+ Core::XmlAttribute("name", getTypeName() );

	// remember the filenames to avoid multiple inclusions
	if(!dumped) dumped = new std::set<std::string>;

	// dump all nodes in the network which include extern flow-networks
	for (std::list<AbstractNode*>::const_iterator it = nodes_.begin();
		it != nodes_.end(); it++) {
		if(Network* n = dynamic_cast<Network*>(*it)) {
			// dump only if not dumped before
			std::set<std::string>::const_iterator it_dumped = dumped->find(n->filename());
			if(it_dumped == dumped->end()) {
				n->dump(false, dumpChannel, dumped);
				dumped->insert(n->filename());
			}
		}
	}

	// inner networks are stored in <network-node> tags
	if(!initialCall)
		dumpChannel << Core::XmlOpen("network-node")
			+ Core::XmlAttribute("filter", filename())
			+ Core::XmlAttribute("name", getTypeName());

	// dump <param>
	for (std::list<Network::Parameter>::const_iterator it = params_.begin();
		it != params_.end(); it++) {
		dumpChannel << Core::XmlEmpty("param")
			+ Core::XmlAttribute("name", it->name());
	}

	// dump <in>
	for(std::vector<Port>::const_iterator inputPort = inputs_.begin();
		inputPort != inputs_.end(); ++ inputPort) {
		dumpChannel << Core::XmlEmpty("in")
			+ Core::XmlAttribute("name", inputPort->name());
	}

	// dump <out>
	for(std::vector<Port>::const_iterator outputPort = outputs_.begin();
			outputPort != outputs_.end(); ++ outputPort) {
		dumpChannel << Core::XmlEmpty("out")
			+ Core::XmlAttribute("name", outputPort->name());
	}

	// dump <node>
	for (std::list<AbstractNode*>::const_iterator it = nodes_.begin();
		it != nodes_.end(); ++it) {

		// don't dump repeater nodes
		if(0 == ((*it)->name().find(Network::inputRepeaterPrefix))) continue;

		// accumulate parameter before dumping
		Core::XmlEmpty nodeDump("node");
		const UnresolvedAttributes &atts = (*it)->unresolvedAttributes();

		for(UnresolvedAttributes::const_iterator a = atts.begin(); a != atts.end(); ++a) {
			std::string key(a->first);
			std::string value(a->second);
			if(key == "filter") {
				if(Network* n = dynamic_cast<Network*>(*it))
					nodeDump += Core::XmlAttribute(key, n->filename());
				else
					nodeDump += Core::XmlAttribute(key, value);
			} else {
				nodeDump += Core::XmlAttribute(key, value);
			}
		}
		dumpChannel << nodeDump;
	}

	// dump <link>
	for (std::list<Link*>::const_iterator it = links_.begin(); it != links_.end(); ++it) {
		// retrieve saved connection's names from the link object
		std::string from_n, from_p, to_n, to_p;
		(*it)->getNodeNames(&from_n, &from_p, &to_n, &to_p);
		if("" == (from_n + to_n)) continue;
		dumpChannel << Core::XmlEmpty("link")
			+ Core::XmlAttribute("from", from_n+(from_p != "" ? ":"+from_p : "") )
			+ Core::XmlAttribute("to", to_n+(to_p != "" ? ":"+to_p : "") );
	}

	if(initialCall) {
		dumpChannel << Core::XmlClose("network");
		delete dumped;
	}
	else
		dumpChannel << Core::XmlClose("network-node");

	return true;
}


/*****************************************************************************/
std::ostream& Flow::operator << (std::ostream &o, const Network &n)
/*****************************************************************************/
{
	bool dumped = false;

	if (!n.name().empty()) {
		o << "network: '" << n.name() << "'";
		if (n.isThreaded()) o << " threaded ";
		dumped = true;
		o << std::endl;
	}
	if (n.params_.size()) {
		o << "param(s): ";
		std::copy(n.params_.begin(), n.params_.end(),
				std::ostream_iterator<Network::Parameter>(o, "\n"));
		dumped = true;
	}
	if (n.inputs_.size()) {
		o << "input(s): ";
		std::copy(n.inputs_.begin(), n.inputs_.end(),
				std::ostream_iterator<Network::Port>(o, "\n"));
		dumped = true;
	}
	if (n.outputs_.size()) {
		o << "output(s): ";
		std::copy(n.outputs_.begin(), n.outputs_.end(),
				std::ostream_iterator<Network::Port>(o, "\n"));
		dumped = true;
	}
	o << "node(s):" << std::endl;
	std::copy(n.nodes_.begin(), n.nodes_.end(), std::ostream_iterator<AbstractNode*>(o, "\n"));
	o << "link(s):" << std::endl;
	std::copy(n.links_.begin(), n.links_.end(), std::ostream_iterator<Link*>(o, "\n"));

	return o;
}

/*****************************************************************************
 *****************************************************************************
 ** NetworkTemplate
 *****************************************************************************
 *****************************************************************************/

/*****************************************************************************/
NetworkTemplate::NetworkTemplate(const char *typname, NodeBuilder *build) :
	typeName_(typname),
	builder_(build)
/*****************************************************************************/
{
}

/*****************************************************************************/
NetworkTemplate::NodeAttributes& NetworkTemplate::addNodeAttributes(const std::string &name, const std::string &filter)
/*****************************************************************************/
{
	NodeAttributes data;
	data.name = name;
	data.filter = filter;
	savedAttributes_.push_back(data);
	// after creating, the struct NodeAttributes is returned for being filled
	// with unresolved node attributes (see NetworkParser::start_node())
	return savedAttributes_.back();
}

/*****************************************************************************/
bool NetworkTemplate::addLink(const std::string &fromNodeName, const std::string &fromPortName,
	const std::string &toNodeName, const std::string &toPortName,
	u32 buffer)
/*****************************************************************************/
{
	LinkParameter data;
	data.from_n = fromNodeName;
	data.from_p = fromPortName;
	data.to_n = toNodeName;
	data.to_p = toPortName;
	data.buffer = buffer;

	savedLinks_.push_back(data);
	return true;
}

/*****************************************************************************/
void NetworkTemplate::declareParameter(const std::string &name)
/*****************************************************************************/
{
	savedParameters_.push_back(name);
}

/*****************************************************************************/
bool NetworkTemplate::addInput(const std::string &name)
/*****************************************************************************/
{
	savedInputs_.push_back(name);
	return true;
}

/*****************************************************************************/
bool NetworkTemplate::addOutput(const std::string &name)
/*****************************************************************************/
{
	savedOutputs_.push_back(name);
	return true;
}

/*****************************************************************************/
bool NetworkTemplate::createNetwork(Network &network) const
/*****************************************************************************/
{
	using namespace std;
	network.setTypeName(typeName_);
	vector<string>::const_iterator it;

	// param
	for (it=savedParameters_.begin(); it!=savedParameters_.end(); ++it)
		network.declareParameter(*it);

	// inputs & outputs
	for (it=savedInputs_.begin(); it!=savedInputs_.end(); ++it)
		network.addInput(*it);
	for (it=savedOutputs_.begin(); it!=savedOutputs_.end(); ++it)
		network.addOutput(*it);

	// nodes
	for (vector<NodeAttributes>::const_iterator it = savedAttributes_.begin();
		it != savedAttributes_.end(); ++it) {
		// create node and save the unresolved parameters for dumping
		AbstractNode *node = builder_->createNode(*it);
		for(UnresolvedAttributes::const_iterator attribute = it->attributes.begin();
			attribute != it->attributes.end(); ++attribute) {
			node->addUnresolvedParameter(attribute->first, attribute->second);
		}
		builder_->registerNodeInNetwork(node, network);

		// add node to the network
		network.addNode(node);
	}

	// links
	bool status;
	for (vector<LinkParameter>::const_iterator it=savedLinks_.begin();
		it != savedLinks_.end(); it++ ) {
		status = network.addLink(it->from_n, it->from_p, it->to_n, it->to_p, it->buffer);
		if (!status)
			builder_->error("could not add link to network");
	}

	return true;
}
