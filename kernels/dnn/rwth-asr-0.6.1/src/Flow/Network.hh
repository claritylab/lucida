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
#ifndef _FLOW_NETWORK_HH
#define _FLOW_NETWORK_HH

#include <algorithm>
#include <list>
#include <string>
#include <ostream>
#include <vector>
#include <set>

#include <Core/Configuration.hh>

#include "Attributes.hh"
#include "Link.hh"
#include "AbstractNode.hh"

namespace Flow {

	/** Flow network class.
	 *
	 * Network serialization procedure:
	 * - ~Network(): if network's "flow-dump-channel.channel"
	 *   in the config file is open, call dump()
	 * - dump() serializes network's own content and
	 *   that of each sub-network as XML to the channel
	 *
	 * - Basic configuration for dumping a network:
	 *   dump.config:
	 *   <pre>
	 *   ---------------------------------------------------------
	 *   [*.network_to_dump]
	 *   flow-dump-channel.channel    = dump-channel
	 *
	 *   [*.channels]
	 *   dump-channel.file            = my_network.dump.flow
	 *   dump-channel.append          = false
	 *   dump-channel.encoding        = UTF-8
	 *   dump-channel.add-sprint-tags = false
	 *   ---------------------------------------------------------
	 *   </pre>
	 */
	class Network : public AbstractNode {

	typedef AbstractNode Precursor;

	private:
		static const Core::ParameterString paramFilename;

		static const std::string inputRepeaterPrefix;
		Core::XmlChannel dumpChannel_;
		bool dump(const bool initialCall, Core::XmlChannel& dumpChannel, std::set<std::string> *dumpedNetworks = 0);
		std::string filename_;

		class Parameter {
		public:
			struct Use {
				AbstractNode *by;
				std::string as;
				Use(AbstractNode *_by, const std::string &_as) : by(_by), as(_as) {}
			};
		private:
			std::string name_;
			std::vector<Use> uses_;
		public:
			Parameter(const std::string &name) { name_ = name; }
			~Parameter() {}

			inline const std::string& name() const { return name_; }

			inline void addUse(AbstractNode *node, const std::string &name) {
					uses_.push_back(Use(node, name));
			}
			inline const std::vector<Use>& getUses() const { return uses_; }

			inline bool operator < (const Parameter &p) const
			{ return name_ < p.name_; }
			friend inline std::ostream& operator << (std::ostream &o, const Parameter &p) {
				o << p.name_ << " [ ";
				for (std::vector<Use>::const_iterator u = p.uses_.begin() ; u != p.uses_.end() ; ++u)
					o << u->by << ":" << u->as << " ";
				return o << "]";
			}
			friend inline std::ostream& operator << (std::ostream &o, const Parameter *p)
			{ return o << *p; }
		}; // class Parameter

		class Port {
		protected:
			std::string name_;
			/** link which is used when the network's putData or getData is called.
			 *  If the port is used by other abstract nodes the link is not connected.
			 */
			Link *link_;
			AbstractNode *node_;
			PortId nodePort_;
			/** shows if the link is connected, i.e. port is used either by putData or getData.
			*/
			bool linkConnected_;
		public:
			Port(const std::string &name = "");
			~Port() {}

			const std::string& name() const { return name_; }

			/** sets the link which is used when the network is used
			 *  from outside by calling putData or getData.
			 */
			void setLink(Link *l);
			Link* link() { return link_; }
			const Link* link() const{ return link_; }

			/** sets the node and the port to which the link can connect
			*/
			void setNode(AbstractNode *node, PortId nodePort);
			/** @return is the node connected to the port
			*/
			AbstractNode* node() { return node_; }
			const AbstractNode* node() const { return node_; }

			/** @return is the PortId of the node connected to the port
			*/
			PortId nodePort() const { return nodePort_; }

			/** @return is true if link is connected to the inside of the network. */
			bool linkConnected() const { return linkConnected_; }
			void setLinkConnected() { linkConnected_ = true; }
			void unsetLinkConnected() { linkConnected_ = false; }

			int operator == (const Port &p) const
			{ return name_ == p.name_; }

			bool operator < (const Port &p) const
			{ return name_ < p.name_; }

			friend std::ostream& operator << (std::ostream &o, const Port &p)
			{ return o << p.name_; }
			friend std::ostream& operator << (std::ostream &o, const Port *p)
			{ return o << *p; }
		}; // class Port

	private:
		/** Name given in network description (e.g.: <network name="network">)
		*/
		std::string typeName_;

		std::list<Parameter> params_;
		/**
		 *  Controls if error is generated when
		 *  setting the value of unknown parameters.
		 */

		std::vector<Port> inputs_;
		std::vector<Port> outputs_;

		std::list<AbstractNode*> nodes_;
		std::list<Link*> links_;

		bool started_;

		/** Remembers the last non data external input by putEos() or putEos().
		 *  If the input port link is empty the work(...) function is called.
		 *  The work(...) function sends the lastNonData_ to the first node.
		 */
		Data* lastNonData_;

		/** Connects a link to a input port
		 *  1) connects the repeater output to the link @param l
		 *  2) creates a link for the port which gets connected to the input of the repeater
		 *     (@see connectInputPortLink).
		 */
		bool connectInputFromInside(const std::string &portName, Link *l, u32 buffer);
		/** Connects a link to a output port
		 *  1) sets the link of the port @param toPortName to @param l
		 *  2) connects the link to the from node (@see connectOutputPortLink).
		 */
		bool connectOutputFromInside(const std::string &fromNodeName,
			const std::string &fromPortName,
			const std::string &toPortName,
			Link *l);

		/** Connects a link to a node input */
		bool connectNodeInput(const std::string &nodeName, const std::string &portName, Link *l);
		/** Connects a link to a node output */
		bool connectNodeOutput(const std::string &nodeName, const std::string &portName, Link *l);

		/** Connects the link of the input port @param port to its repeater node.
		 *  During initilization, input ports gets connected to the repeater node by a link.
		 *  This link is used to put data and attributes to the network.
		 *  The link is automatically disconnected if the network is embedded and the input port is
		 *  connected to a predecessor node.
		 */
		void connectInputPortLink(PortId);
		/** Disconnects the link of the input port from its repeater node.
		 *  For more details @see connectInputPortLink.
		 */
		void disconnectInputPortLink(PortId);
		/** Connects the link of the output port @param port to its source node.
		 *  During initilization, output ports gets connected to their source node by a link.
		 *  This link is used to get data and attributes from the network.
		 *  The link is automatically disconnected if the network is embedded and the output port is
		 *  connected to a successor node. In this way, we can prevent that data objects are put into
		 *  this link since these will never be taken out.
		 *  To reactivate the port for external usage see avtivateOutput.
		 */
		void connectOutputPortLink(PortId);
		/** Disconnects the link of the output port from its source node.
		 *  For more details @see connectOutputPortLink.
		 */
		void disconnectOutputPortLink(PortId);

		/** Configures the part of the network used by the output port @param out. */
		bool configureOutputPort(Port &out);

		/** Sets the value of parameter @c name in each node which uses it. */
		bool setUserDefinedParameter(const std::string &name, const std::string &value);
	protected:
		virtual PortId nInputs() const { return inputs_.size(); }

		virtual PortId inputConnected(PortId in) const {
			require(validInputPort(in)); return inputs_[in].node()->inputConnected(0);
		}
		bool validInputPort(PortId in) const {
			return 0 <= in && in < nInputs();
		}
		virtual PortId nOutputs() const { return outputs_.size(); }
		virtual u32 nOutputLinks(PortId out) const;

		bool validOutputPort(PortId out) const {
			return 0 <= out && out < nOutputs();
		}
		virtual PortId connectInputPort(PortId, Link *);
		virtual void disconnectInputLink(Link *);
		virtual PortId connectOutputPort(PortId, Link *);
		virtual void disconnectOutputLink(Link *);
	public:
		Network(const Core::Configuration &c, bool loadFromFile=true);
		~Network();

		/**
		 * Construct network from XML description string.
		 * Failure can be tested with hasFatalErrors().
		 */
		void buildFromString(const std::string &network);

		/**
		 * Construct network from XML description file.
		 * Failure can be tested with hasFatalErrors().
		 */
		void buildFromFile(const std::string &filename);

		void setTypeName(const std::string &typeName) { typeName_ = typeName; }
		const std::string& getTypeName() const { return typeName_; }

		bool addNode(AbstractNode *node);
		AbstractNode* getNode(const std::string &name);

		bool addLink(const std::string &fromNodeName, const std::string &fromPortName,
			const std::string &toNodeName, const std::string &toPortName,
			u32 buffer = 0);

		void declareParameter(const std::string &name) {
			params_.push_back(Parameter(name));
		}

		/** Register a node as user of a parameter.
		 * @param node is the node which uses the parameter @c param.
		 * @param name is the name of the parameter of the node
		 * @param value is the parameter of the node which has
		 * at least one variable referencing a network parameter
		 */
		bool addParameterUse(AbstractNode *node,
			const std::string &name,
			const Core::StringExpression &value);

		// inputs/outputs
		bool addInput(const std::string &name);
		virtual PortId getInput(const std::string &name);

		bool addOutput(const std::string &name);
		virtual PortId getOutput(const std::string &name);
		const std::string &outputName(PortId out) const {
			require(validOutputPort(out)); return outputs_[out].name();
		}
		void outputs(std::vector<std::pair<PortId, std::string> > &outputs) const;
		/** Prepares the output port for external usage.
		 *  Ensures that the link of the output port is connect. This link will
		 *  be used by getData and getAttribute.
		 *  Outputs are activated at initialization. If the network is embedded
		 *  into another one, outputs connected to another node get deactivated.
		 *  In this way, we can prevent that data objects are put into the port link since these
		 *  will never be taken out.
		 *  Use this function only if you explicitely would like to read data from an embedded network.
		 */
		void activateOutput(PortId);

		/** Put data on input port of network. */
		virtual bool putData(PortId in, Data *d);
		/** Send end-of-stream on output port. */
		bool putEos(PortId out) { return putData(out, lastNonData_); }
		/** Send out-of-data on output port. */
		bool putOod(PortId out) { return putData(out, lastNonData_); }

		/** Fetch data from output port of network. */
		template<class T> bool getData(PortId out, DataPtr<T> &d) {
			require(validOutputPort(out));
			Port &port = outputs_[out];
			if (!port.linkConnected()) {
				// see also activateOutput
				error("Output port '%s' is not connected to any node or port.", port.name().c_str());
				return false;
			}
			Link *l = port.link();
			ensure_(l != 0);
			if (!started_) {
				if ((!l->areAttributesAvailable()) && (!configureOutputPort(port)))
					return false;
				//startThread();
			}
			return AbstractNode::getData(l, d);
		}

		/** Put attribute set on input node of network */
		bool putAttributes(PortId, Core::Ref<const Attributes>);
		/** Inquire an attribute on an output port of the network. */
		const std::string getAttribute(PortId out, const std::string &name);

		virtual bool setParameter(const std::string &name, const std::string &value);

		/** Creates empy input attibutes for input ports without one an attribute. */
		virtual bool configure();

		/** Repeats last non-data input on input port @param in. */
		virtual bool work(PortId in);

		/** Resets all links and nodes. */
		void reset();

		void go();

		friend std::ostream& operator << (std::ostream &o, const Network &n);

		void setFilename(const std::string &f) { filename_ = f; }
		const std::string& filename() const { return filename_; }
	}; // class Network

/*****************************************************************************/

	class NodeBuilder;
	/**
	 * NetworkTemplate
	 *
	 * - stores details about a single network like inputs,
	 *   parameters and the unresolved node attributes
	 * - createNetwork(): instantiates a network, gets
	 *   node-objects via the "parent" NodeBuilder (builder_),
	 *   which is aware of other templates in the flow-file.
	 */
	class NetworkTemplate
	{
	typedef AbstractNode::UnresolvedAttributes UnresolvedAttributes;
	public:
		struct NodeAttributes {
			std::string name, filter;
			UnresolvedAttributes attributes;
		};
	private:
		const char *name_, *filtername_;
		std::string typeName_;
		struct LinkParameter {
			std::string from_n;
			std::string from_p;
			std::string to_n;
			std::string to_p;
			u32 buffer;
		};

		std::vector<LinkParameter> savedLinks_;
		std::vector<std::string> savedParameters_;
		std::vector<std::string> savedInputs_;
		std::vector<std::string> savedOutputs_;
		std::vector<NodeAttributes> savedAttributes_;
		NodeBuilder *builder_;
	public:
		NetworkTemplate(const char *typname, NodeBuilder *builder);

		// instantiates the template in the provided network
		bool createNetwork(Network &network) const;

		// Methods for accumulating information about a network prototype.
		NodeAttributes& addNodeAttributes(const std::string &name, const std::string &filter);
		bool addLink(const std::string &fromNodeName, const std::string &fromPortName,
			const std::string &toNodeName, const std::string &toPortName,
			u32 buffer = 0);
		void declareParameter(const std::string &name);
		bool addInput(const std::string &name);
		bool addOutput(const std::string &name);

	}; // class NetworkTemplate
} // namespace Flow

#endif // _FLOW_NETWORK_HH
