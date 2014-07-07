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
#ifndef _FLOW_ABSTRACT_NODE_HH
#define _FLOW_ABSTRACT_NODE_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/Hash.hh>
#include <Core/StringExpression.hh>

#include "Data.hh"
#include "Link.hh"
#include "Types.hh"


namespace Flow {

	/** Abstract flow network node.
	 *
	 * - stores the unresolved attributes of <pre><node></pre>-tags provided in
	 *   the Flow-file for the reasons of further serialization
	 * - serialization-related members are:
	 *   - unresolvedParameters_ : map<string,string> - stores the attributes
	 *   - addUnresolvedAttribute(key, value) - adds a new attribute
	 *   - unresolvedAttributes() - returns the stored attributes
	 */
	class AbstractNode : public virtual Core::Component//, public Core::Thread
	{
	friend class Network;

	public:
		static const Core::ParameterBool paramThreaded;
		static const Core::ParameterBool paramIgnoreUnknownParameters;
		typedef std::map<std::string, std::string> UnresolvedAttributes;

	private:
		typedef Core::StringHashMap<Core::StringExpression> Parameters;

		bool threaded_;
		bool ignoreUnknownParameters_;
		Parameters parameters_;
		UnresolvedAttributes dumpParameters_;

		/** setNetworkParameter is called by the Network
		 * if a network parameter gets a new value.
		 *
		 * It sets the corresponding parameters and calls
		 * setParameter for all the entirely resolved parameters.
		 *
		 * @param key is the key of the parameter
		 * @param networkParameterName is the key of the network parameter
		 * @param networkParameterValue is the value of the network parameter
		 * @return is false if key does not contain this variable
		 */
		bool setNetworkParameter(const std::string &parameterName,
			const std::string &networkParameterName,
			const std::string &networkParameterValue);

		/**
		 * Calls setParameter; return true if parameter is succesfully set or
		 * unknown parameters are allowed
		 **/
		bool checkAndSetParameter(const std::string &name, const std::string &value);

	protected:
		/** Ask for an input port.
		 * Implement this function to provide a mapping form names to port IDs.
		 * Consider using a Core::Choice.
		 * Remeber to create ports using addInput().
		 * @return a valid port ID or IllegalPortId if there is to such port.
		 */
		virtual PortId getInput(const std::string &) = 0;
		virtual PortId connectInputPort(PortId, Link *) = 0;
		virtual void disconnectInputLink(Link *) = 0;
		/** Ask for an output port.
		 * Implement this function to provide a mapping form names to port IDs.
		 * Consider using a Core::Choice.
		 * Remeber to create ports using addOutput().
		 * @return a valid port ID or IllegalPortId if there is to such port.
		 */
		virtual PortId getOutput(const std::string &) = 0;
		virtual PortId connectOutputPort(PortId, Link *) = 0;
		virtual void disconnectOutputLink(Link *) = 0;

		virtual PortId nInputs() const = 0;
		virtual PortId inputConnected(PortId in) const = 0;

		virtual PortId nOutputs() const = 0;
		virtual u32 nOutputLinks(PortId port) const = 0;

		/** Fetch data packet from the "from" port of link @param l.
		 * @param d data pointer to which retrieved packet is attached
		 * @return true on success, or false if the out-of-data
		 */
		template<class T> bool getData(Link *l, DataPtr<T> &d) {
			if (l != 0) {
				if (l->isDataAvailable())
					return l->getData(d);
				else {
					if (l->getFromNode()->work(l->getFromPort()))
						return l->getData(d);
					else {
						error("Node '%s' could not generate any output.",
							l->getFromNode()->name().c_str());
					}
				}
			}
			d.reset();
			return false;
		}
	public:
		AbstractNode(const Core::Configuration &c);
		virtual ~AbstractNode() {}

		// threads
		virtual void Run();

		// external configuration
		void setThreaded(bool threaded = true) { threaded_ = threaded; }
		void setThreaded(const std::string &threaded);
		bool isThreaded() const { return threaded_; }

		/** addParameter adds a parameter to the node
		 * Constant parameters are simply delegated to setParameter,
		 * parameters dependent on network parameters are stored
		 * in parameters_. The setParameter is called, if the configuration
		 * can resolve the parameter.
		 */
		virtual bool addParameter(const std::string &name, const Core::StringExpression &value);
		/** Set node specific parameter.
		 * Implement this template function to handle parameters specific
		 * to your Node.
		 * @param name name of parameter
		 * @param value new value for the parameter
		 * @return true if successful, false if parameter @c name is not
		 * understood. */
		virtual bool setParameter(const std::string &name, const std::string &value) { return false; }
		/** Erases attributes of all output links recursively over all successor nodes.
		 *  Signals that node needs the get reconfigured.
		 */
		virtual void eraseOutputAttributes() {};
		/** Performs static (independent of input data) configuration of node.
		 *  Configure is called if one of the output attributes is not available.
		 *  Override this function to evaluate all input attributes and pass them on
		 *  to successor nodes.
		 *  Note: we assume no dead inputs here.
		 */
		virtual bool configure() = 0;
		/**
		 * Process some data.
		 * Implement this function to do whatever your node is supposed
		 * to.  You are required to produce at least one packet of data on
		 * the specified output @c out.  You may send more packets, also
		 * to other outputs.
		 * @return true on success, false on end-of-file or error.
		 * The idiom is: @code return putData(out, d); @endcode
		 */
		virtual bool work(PortId out) = 0;

		/**
		 * Functions for saving and getting unresolved parameters.
		 */
		void addUnresolvedParameter(const std::string &key, const std::string &value);
		const UnresolvedAttributes& unresolvedAttributes() const;

		bool operator < (const AbstractNode &n) const { return name() < n.name(); }
		friend std::ostream& operator << (std::ostream &o, const AbstractNode &n);
		friend std::ostream& operator << (std::ostream &o, const AbstractNode *n) { return o << *n; }
	};

} // namespace Flow

#endif // _FLOW_ABSTRACT_NODE_HH
