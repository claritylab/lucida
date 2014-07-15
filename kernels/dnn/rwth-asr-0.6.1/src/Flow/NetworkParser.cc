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
#include "Filter.hh"
#include "Network.hh"
#include "NetworkParser.hh"
#include "Node.hh"
#include "Registry.hh"
#include <Core/Directory.hh>
#include <cstdlib>

using namespace Core;
using namespace Flow;

/*****************************************************************************/

const Core::ParameterString NodeBuilder::paramNetworkFileExtension(
	"network-file-extension",
	"default network file extension", ".flow");

const Core::ParameterString NodeBuilder::paramNetworkFilePath(
	"network-file-path",
	"path of network files", ".");

/*****************************************************************************/
NetworkParser::NetworkParser(Network &n, const Core::Configuration &c) :
	XmlSchemaParser(c),
	network(n),
	nodeBuilder_(c)
/*****************************************************************************/
{
	XmlElement *parameter_decl =
		collect(new XmlEmptyElementRelay
			("param", this, startHandler(&Self::start_parameter)));
	XmlElement *input_decl =
		collect(new XmlEmptyElementRelay
			("in", this, startHandler(&Self::start_input)));
	XmlElement *output_decl =
		collect(new XmlEmptyElementRelay
			("out", this, startHandler(&Self::start_output)));
	XmlElement *node_decl =
		collect(new XmlEmptyElementRelay
			("node", this, startHandler(&Self::start_node)));
	XmlElement *link_decl =
		collect(new XmlEmptyElementRelay
			("link", this, startHandler(&Self::start_link)));
	XmlElement *networknode_decl =
		collect(new XmlMixedElementRelay
			("network-node", this,
			 startHandler(&Self::start_networknode),
			 endHandler(&Self::end_networknode),
			 0,
			 XML_CHILD(parameter_decl),
			 XML_CHILD(input_decl),
			 XML_CHILD(output_decl),
			 XML_CHILD(node_decl),
			 XML_CHILD(link_decl),
			 XML_NO_MORE_CHILDREN));
	XmlElement *network_decl =
		collect(new XmlMixedElementRelay
			("network", this,
			 startHandler(&Self::start_network),
			 endHandler(&Self::end_network),
			 0,
			 XML_CHILD(parameter_decl),
			 XML_CHILD(input_decl),
			 XML_CHILD(output_decl),
			 XML_CHILD(node_decl),
			 XML_CHILD(link_decl),
			 XML_CHILD(networknode_decl),
			 XML_NO_MORE_CHILDREN));
	setRoot(network_decl);
}

/*****************************************************************************/
bool NetworkParser::buildFromString(const std::string &str)
/*****************************************************************************/
{
	return (parseString(str.c_str()) == 0);
}

/*****************************************************************************/
bool NetworkParser::buildFromFile(const std::string &filename)
/*****************************************************************************/
{
	nodeBuilder_.setCurrentDirectory(Core::directoryName(filename));
	return (parseFile(filename.c_str()) == 0);
}
/*****************************************************************************/
void NetworkParser::start_network(const XmlAttributes atts)
/*****************************************************************************/
{
	const char *name = atts["name"];
	const char *threaded = atts["threaded"];
	if (name)
		network.setTypeName(name);

	if (threaded) network.setThreaded(true);

	nodeBuilder_.addNetworkTemplate("", name ? name : "network");
}

/*****************************************************************************/
void NetworkParser::end_network()
/*****************************************************************************/
{
	nodeBuilder_.currentTemplate().createNetwork(network);
}

/*****************************************************************************/
void NetworkParser::start_networknode(const XmlAttributes atts)
/*****************************************************************************/
{
	nodeBuilder_.addNetworkTemplate(std::string(atts["filter"]), atts["name"]);
}

/*****************************************************************************/
void NetworkParser::end_networknode()
/*****************************************************************************/
{
	nodeBuilder_.finalizeNetworkNode();
}

/*****************************************************************************/
void NetworkParser::start_parameter(const XmlAttributes atts)
/*****************************************************************************/
{
	const char *name = atts["name"];
	if (!name) error("network parameter has no name");
	else nodeBuilder_.currentTemplate().declareParameter(name);
}

/*****************************************************************************/
void NetworkParser::start_input(const XmlAttributes atts)
/*****************************************************************************/
{
	const char *name = atts["name"];
	if (!name) error("network input has no name");
	else nodeBuilder_.currentTemplate().addInput(name);
}

/*****************************************************************************/
void NetworkParser::start_output(const XmlAttributes atts)
/*****************************************************************************/
{
	const char *name = atts["name"];
	if (!name) error("network output has no name");
	else nodeBuilder_.currentTemplate().addOutput(name);
}

/*****************************************************************************/
void NetworkParser::start_node(const XmlAttributes atts)
/*****************************************************************************/
{
	const char *name = atts["name"];
	const char *filter = atts["filter"];
	//const char *threaded = atts["threaded"];

	if (!name) {
		error("network node has no name");
		return;
	}
	if (!filter) {
		error("no filter specified for node \"%s\"", name);
		return;
	}

	// store unresolved attributes for dumping
	NetworkTemplate::NodeAttributes &nodeAttributes = nodeBuilder_.currentTemplate().addNodeAttributes(name, filter);
	for (int i = 0; i < atts.size(); ++i) {
		XmlAttributes::Item a = atts[i];
		nodeAttributes.attributes[std::string(a.key)] = std::string(a.value);
	}
}

/*****************************************************************************/
void NetworkParser::start_link(const XmlAttributes atts)
/*****************************************************************************/
{
	const char *from   = atts["from"];
	const char *to     = atts["to"];
	const char *buffer = atts["buffer"];

	if (!from) {
		error("no 'from' attribute for link");
		return;
	}
	if (!to) {
		error("no 'to' attribute for link");
		return;
	}

	// from
	std::string tmp = config.resolve(from);
	std::string::size_type pos = tmp.find(':');
	std::string from_node = tmp.substr(0, pos);
	std::string from_port;
	if (pos == std::string::npos) from_port = "";
	else from_port = tmp.substr(pos + 1, tmp.size());

	// to
	tmp = config.resolve(to);
	pos = tmp.find(':');
	std::string to_node = tmp.substr(0, pos);
	std::string to_port;
	if (pos == std::string::npos) to_port = "";
	else to_port = tmp.substr(pos + 1, tmp.size());

	// add link to network
	bool status;
	status = nodeBuilder_.currentTemplate().addLink(from_node, from_port, to_node, to_port,
		(buffer ? atoi(buffer) : 0));
	if (!status) error("could not add link to network");
}


/*****************************************************************************
 *  NodeBuilder
 *****************************************************************************/

/*****************************************************************************/
NodeBuilder::NodeBuilder(const Core::Configuration &c) :
	Component(c)
/*****************************************************************************/
{
}

/*****************************************************************************/
NodeBuilder::~NodeBuilder()
/*****************************************************************************/
{
	for(NamedNetworkTemplates::iterator it = networkTemplates_.begin();
		it!=networkTemplates_.end(); ++it) delete it->second;
}

/*****************************************************************************/
Network* NodeBuilder::createNetworkNode(const std::string &name, const std::string &filename)
/*****************************************************************************/
{
	Network *result = 0;
	std::string foundFilename;
	if (findNetworkFile(filename, foundFilename)) {
		result = new Network(select(name), false);
		result->buildFromFile(foundFilename);
		if (result->hasFatalErrors()) {
			delete result; result = 0;
		} else {
			result->setFilename(foundFilename);
		}
	}
	return result;
}

/*****************************************************************************/
Node* NodeBuilder::createFilterNode(const std::string &name, const std::string &filterName)
/*****************************************************************************/
{
	const _Filter *filter = Flow::Registry::instance().getFilter(filterName);
	if (filter) {
	    return filter->newNode(select(name));
	}
	return 0;
}

/*****************************************************************************/
bool NodeBuilder::findNetworkFile(const std::string &pathname, std::string &foundPathname) const
/*****************************************************************************/
{
	std::string pathnameWithExtension = pathname;
	if (Core::filenameExtension(pathnameWithExtension) == "")
		pathnameWithExtension += paramNetworkFileExtension(config);

	std::vector<std::string> paths;
	paths.push_back(currentDirectory_);
	paths.push_back(paramNetworkFilePath(config));
	char *ret;

	for(std::vector<std::string>::const_iterator p = paths.begin(); p != paths.end(); ++ p) {
		std::string result = Core::joinPaths(*p, pathnameWithExtension);
		if (Core::isRegularFile(result)) {
			char realname[PATH_MAX+1];
			ret = realpath(result.c_str(), realname); // absolute path
			foundPathname = std::string(realname);
			return true;
		}
	}
	return false;
}

/*****************************************************************************/
void NodeBuilder::addNetworkTemplate(const std::string &filename, const char *networkname)
/*****************************************************************************/
{
	// only root network may have an empty name
	if(networkTemplates_[""] != 0)
		require(filename != "");

	NetworkTemplate *newTemplate = new NetworkTemplate(networkname, this);
	networkTemplates_[filename] = newTemplate;

	currentTemplate_ = newTemplate;
}

/*****************************************************************************/
AbstractNode* NodeBuilder::createNode(const NetworkTemplate::NodeAttributes &nodeAttributes)
/*****************************************************************************/
{
	using namespace std;
	const string name = nodeAttributes.name;
	string filterName = nodeAttributes.filter;

	filterName = select(name).resolve(filterName);

	AbstractNode* result = 0;

	// node is a filter class
	if ((result = createFilterNode(name, filterName)) != 0) {
		return result;
	}

	// node is a network from a predefined template
	if (networkTemplates_[filterName]) {
		Network *newNetwork = new Network(select(name), false);
		newNetwork->setFilename(filterName);
		networkTemplates_[filterName]->createNetwork(*newNetwork);
		return newNetwork;
	}

	// node is a network from an extern .flow-file
	if ((result = createNetworkNode(name, filterName)) != 0) {
		return result;
	}

	error("Could not find filter \"%s\" (neither as built-in nor as network file).",
		filterName.c_str());
	return result;
}

/*****************************************************************************/
void NodeBuilder::registerNodeInNetwork(AbstractNode *node, Network &network)
/*****************************************************************************/
{
	for (AbstractNode::UnresolvedAttributes::const_iterator attr = node->unresolvedAttributes().begin();
		attr != node->unresolvedAttributes().end(); ++attr) {
		std::string parameterName(attr->first);
		std::string parameterValue(attr->second);

		if ((parameterName != "name") && (parameterName != "filter")) {
			Core::StringExpression parameterValueExpression;
			Core::StringExpressionParser parameterParser(parameterValueExpression);
			parameterParser.accept(parameterValue);

			if (!parameterValueExpression.isConstant()) {
				// set default value from configuration
				parameterValueExpression.setVariables(network.getConfiguration());

				// register as user of network parameter
				bool res = network.addParameterUse(node, parameterName, parameterValueExpression);

				if (!res) {
					error("Network parameter does not exist in \"%s\"=\"%s\"",
					parameterName.c_str(), parameterValue.c_str());
				}
			}

			if (!node->addParameter(parameterName, parameterValueExpression)) {
				error("Failed to add parameter in \"%s\"=\"%s\"",
					parameterName.c_str(), parameterValue.c_str());
			}
		}
	}
}
