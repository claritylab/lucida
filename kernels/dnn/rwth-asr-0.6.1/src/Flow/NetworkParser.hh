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
#ifndef _FLOW_NETWORK_PARSER_HH
#define _FLOW_NETWORK_PARSER_HH


#include <Core/Configuration.hh>
#include <Core/XmlParser.hh>

#include "Network.hh"


namespace Flow {

	using Core::XmlAttributes;
	class Node;
/*****************************************************************************/
	/**
	 * NodeBuilder
	 *
	 * - accumulates details about the networks in the Flow-file
	 * - creates nodes (createNode())
	 *   - built-in filter class
	 *   - sub-network defined in <pre><network-node></pre>
	 *     (stored in networkTemplates_)
	 *   - external Flow-file
	 * - networkTemplates_ : map<string, NetworkTemplate*> - stores the templates
	 * - currentTemplate() - returns the pointer to the top-most template
	 */
	class NodeBuilder: public virtual Core::Component {
		typedef std::map<std::string, NetworkTemplate*> NamedNetworkTemplates;

	private:
		NamedNetworkTemplates networkTemplates_;
		NetworkTemplate *currentTemplate_;
		std::string currentDirectory_;

		Network* createNetworkNode(const std::string &name, const std::string &networkName);
		Node* createFilterNode(const std::string &name, const std::string &filterName);

		bool findNetworkFile(const std::string &pathname, std::string &foundPathname) const;

		static const Core::ParameterString paramNetworkFileExtension;
		static const Core::ParameterString paramNetworkFilePath;
	public:
		NodeBuilder(const Core::Configuration &c);
		~NodeBuilder();

		// methods called by NetworkParser while parsing XML (flow)
		void setCurrentDirectory(const std::string &dirname) { currentDirectory_ = dirname; };
		void addNetworkTemplate(const std::string &filename, const char *networkName);
		NetworkTemplate& currentTemplate() { return *currentTemplate_; }
		void finalizeNetworkNode() { currentTemplate_ = networkTemplates_[""]; }

		// methods called by NetworkTemplate while initializing a network from a template

		/**
		 *  Creates a node of type @param filterName with the name @param name.
		 *  @param filterName can contain:
		 *    -name of a built in filter (e.g. signal-window),
		 *    -name of a sub-network file
		 *    -parameter resolved in the configuration file (e.g. in flow-file " ... filter="$(filter-type) ..."
		 *     and in config-file "... name.filter-type = signal-window ...).
		 */
		AbstractNode* createNode(const NetworkTemplate::NodeAttributes &nodeAttributes);
		void registerNodeInNetwork(AbstractNode *node, Network &network);
	}; // class NodeBuilder

/*****************************************************************************/

	/**
	 * NetworkParser
	 *
	 * - SAX-like XML parser
	 * - starts parsing a Flow-file (buildFromFile())
	 * - NodeBuilder instance manages the templates & node creation (nodeBuilder_)
	 * - delegates the details obtained on entering tags to
	 *   the top-most template via NodeBuilder::currentTemplate()
	 * - Instantiates the network provided to ctor on reaching
	 *   <pre></network></pre>
	 *
	 * @see Graphical class overview of serialization-related members
	 * <a href="http://www-i6.informatik.rwth-aachen.de/~sprint/flow.png">here</a>
	 */

	class NetworkParser : public Core::XmlSchemaParser {
		typedef NetworkParser Self;
	private:
		Network &network;
		NodeBuilder nodeBuilder_;

		void start_network(const XmlAttributes atts);
		void end_network();
		void start_networknode(const XmlAttributes atts);
		void end_networknode();
		void start_parameter(const XmlAttributes atts);
		void start_input(const XmlAttributes atts);
		void start_output(const XmlAttributes atts);
		void start_node(const XmlAttributes atts);
		void start_link(const XmlAttributes atts);

	public:
		NetworkParser(Network &network, const Core::Configuration &c);
		bool buildFromString(const std::string &str);
		/**
		 * Construct network from XML description file.
		 *
		 * Subnetworks:
		 * If subnetwork name does not have an extension value paramNetworkFileExtension is appended.
		 * XML description file is checked in directories relative:
		 * 1. to the directory of the parent network
		 * 2. to configuation parameter paramNetworkFilePath
		 */
		bool buildFromFile(const std::string &filename);
	}; // class NetworkParser

} // namespace Flow

#endif // _FLOW_NETWORK_PARSER_HH
