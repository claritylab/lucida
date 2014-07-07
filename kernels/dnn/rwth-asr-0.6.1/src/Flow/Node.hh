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
#ifndef _FLOW_NODE_HH
#define _FLOW_NODE_HH

#include "Attributes.hh"
#include "Data.hh"
#include "Link.hh"
#include "Types.hh"
#include "AbstractNode.hh"
#include <Core/Component.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>
#include <Core/Types.hh>
#include <Core/StringExpression.hh>
#include <iostream>
#include <string>
#include <unistd.h>


namespace Flow {

    class _Filter;

    /** Abstract flow network node. */

    class Node : public AbstractNode {
    private:
	typedef AbstractNode Precursor;
	friend class _Filter;
    private:
	const _Filter *filter_;

	const Datatype *datatype_;

	std::vector<Link*> inputs_;
	std::vector<std::vector<Link*> > outputs_;

	Core::XmlChannel attributesChannel_, dataChannel_;
    protected:
	/** Connect link to input port.
	 * Do not connect more than one link to an input, will return
	 * InvalidPortId if anyone tries to do so. */
	virtual PortId connectInputPort(PortId, Link *);
	virtual void disconnectInputLink(Link *);
	virtual PortId connectOutputPort(PortId, Link *);
	virtual void disconnectOutputLink(Link *);

	/** Create an input port.
	 * Call this function getInput() to create ports dynamically.
	 * @param in the desired port ID.  If not given the next free ID
	 * will be used.
	 * @see getInput() */
	PortId addInput(PortId in = IllegalPortId);
	/** Create several input ports.
	 * Call this function from the constructor to create static ports.
	 *  @see getInput() */
	PortId addInputs(u32 nPorts);
	virtual PortId nInputs() const { return inputs_.size(); }
	virtual PortId inputConnected(PortId in) const {
	    require(validInputPort(in)); return (inputs_[in] != 0);
	}
	bool validInputPort(PortId in) const {
	    return 0 <= in && in < nInputs();
	}

	/** Add an output port.
	 * Call this function from getOutput() to create ports dynamically.
	 * @see getOutput() */
	PortId addOutput(PortId out = IllegalPortId);
	/** Create several output ports.
	 * Call this function from the constructor to create static ports.
	 * @see getOutput() */
	PortId addOutputs(u32 nPorts);
	virtual PortId nOutputs() const { return outputs_.size(); }
	bool validOutputPort(PortId out) const {
	    return 0 <= out && out < nOutputs();
	}
	/** Number of link connceted to an output port. */
	virtual u32 nOutputLinks(PortId out) const {
	    require(validOutputPort(out));
	    return outputs_[out].size();
	}

	bool configureDatatype(Core::Ref<const Attributes> a, const Datatype *d);

	/** Fetch data packet from input port
	 * @param in port id of input port to get data from
	 * @param d data pointer to which retrieved packet is attached
	 * @return true on success, or false if the out-of-data
	 */
	template<class T> inline bool getData(PortId in, DataPtr<T> &d) {
	    require(validInputPort(in));
	    return AbstractNode::getData(inputs_[in], d);
	}

	/** Send data packet on output port.
	 * @param out port id of output port to send to
	 * @param d data packet to transmit.
	 * putData() receives ownership of @c d.
	 * @pre If a "datatype" attribute has been set during the
	 * configuration phase, the data packet must conform to this.
	 * @return true on success */
	virtual bool putData(PortId out, Data *d);

	/** Send end-of-stream on output port. */
	bool putEos(PortId out) { return putData(out, Data::eos()); }
	/** Send out-of-data on output port. */
	bool putOod(PortId out) { return putData(out, Data::ood()); }

	/** Retrieve attribute set from input port.
	 *  In order to avoid if-clauses in the overloaded Node::configure() function,
	 *  @return is always valid, e.i. reference points to a valid object.
	 *  If the configuration of the predecessor node fails, an empty Attributes object is generated.
	 */
	Core::Ref<const Attributes> getInputAttributes(PortId in);
	void getInputAttributes(PortId, Attributes &);
	/** Put attribute set on output port.
	 *  Additionally, output links are reconfigured and cleared.
	 */
	bool putOutputAttributes(PortId out, Core::Ref<const Attributes> a);
	/** Erases attributes of all output links recursively. */
	void eraseOutputAttributes();
    public:
	Node(const Core::Configuration &c);
	virtual ~Node() {}

	virtual PortId getInput(const std::string &name) {
	    return IllegalPortId;
	}
	virtual PortId getOutput(const std::string &name) {
	    return IllegalPortId;
	}

	/** Performs static (independent of input data) configuration of node.
	 *  Configure is called if one of the output attributes is not available.
	 *  Override this function to evaluate all input attributes and pass them on
	 *  to successor nodes.
	 *  The default implementation simply forwards the
	 *  merged attributes from all input links to all outputs.
	 *  Note:
	 *    -we assume no dead inputs here.
	 *    -This function is the safest point to reset the algorithms used by the node.
	 *     The alternative would be to reset the algorithms when a non-data object has
	 *     been recieved. This solution can lead easily to unintended behaviour, since
	 *     it is not garatinteed at all that every node receives for example a EOS object!
	 */
	virtual bool configure();
	/** Processes some data.
	 *  Implement this function to do whatever your node is supposed
	 *  to.  You are required to produce at least one packet of data on
	 *  the specified output @c out.  You may send more packets, also
	 *  to other outputs.
	 *  @return true on success, false on end-of-file or error.
	 *  The idiom is: @code return putData(out, d); @endcode
	 */
	virtual bool work(PortId out) = 0;
    };

    /** Base class for nodes with a single input. */
    class SinkNode : public Node {
    public:
	SinkNode(const Core::Configuration &c) :
	    Core::Component(c), Node(c)
	{
	    addInput(0);
	}
	virtual PortId getInput(const std::string &name) { return 0; }
	//  virtual void Run();
    };

    /** Base class for nodes with a single output. */
    class SourceNode : public Node {
    public:
	SourceNode(const Core::Configuration &c) :
	    Core::Component(c), Node(c)
	{
	    addOutput(0);
	}
	virtual PortId getOutput(const std::string &name) { return 0; }
    };

    class SleeveNode : public Node {
    public:
	SleeveNode(const Core::Configuration &c) :
	    Core::Component(c), Node(c)
	{
	    addInput(0);
	    addOutput(0);
	}
	virtual PortId getInput(const std::string &name)  { return 0; }
	virtual PortId getOutput(const std::string &name) { return 0; }
    };

}


#endif // _FLOW_NODE_HH
