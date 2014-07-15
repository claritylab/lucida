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
#include "Node.hh"
#include "Datatype.hh"
#include "Registry.hh"

using namespace Flow;


/*****************************************************************************/
Node::Node(const Core::Configuration &c) :
    Component(c),
    Precursor(c),
    filter_(0),
    datatype_(0),
    attributesChannel_(c, "dump-attributes", Core::Channel::disabled),
    dataChannel_      (c, "dump-data",       Core::Channel::disabled)
/*****************************************************************************/
{}

/*****************************************************************************/
bool Node::configure()
/*****************************************************************************/
{
    Core::Ref<Attributes> a(new Attributes());
    for (PortId i = 0; i < nInputs(); i++) {
	Core::Ref<const Attributes> b = getInputAttributes(i);
	ensure(b);
	if (b) a->merge(*b);
    }
    for (PortId i = 0; i < nOutputs(); i++)
	if (!putOutputAttributes(i, a))
	    return false;
    return true;
}


/*****************************************************************************/
bool Node::configureDatatype(Core::Ref<const Attributes> a, const Datatype *d)
/*****************************************************************************/
{
    if (!a) return false;
    std::string dtn(a->get("datatype"));
    if (!dtn.empty()) {
	datatype_ = Flow::Registry::instance().getDatatype(dtn);
    }
    if (datatype_ != d) {
	error("configure datatype \"%s\" failed (expected \"%s\").",
	      (datatype_ ? datatype_->name().c_str() : "not known"),
	      (d ? d->name().c_str() : "not known"));
	return false;
    }
    return true;
}


/*****************************************************************************/
PortId Node::addInput(PortId in)
/*****************************************************************************/
{
    if (in == IllegalPortId) in = inputs_.size();
    if (size_t(in) >= inputs_.size())
	inputs_.resize(in + 1, 0);
    return in;
}


/*****************************************************************************/
PortId Node::addInputs(u32 nPorts)
/*****************************************************************************/
{
    PortId in = inputs_.size();
    inputs_.resize(in + nPorts, 0);
    return in;
}


/*****************************************************************************/
PortId Node::addOutput(PortId out)
/*****************************************************************************/
{
    if (out == IllegalPortId) out = outputs_.size();
    if (size_t(out) >= outputs_.size()) outputs_.resize(out + 1);
    return out;
}

/*****************************************************************************/
PortId Node::addOutputs(u32 nPorts)
/*****************************************************************************/
{
    PortId out = outputs_.size();
    outputs_.resize(out + nPorts);
    return out;
}

/*****************************************************************************/
PortId Node::connectInputPort(PortId in, Link *l)
/*****************************************************************************/
{
    require(validInputPort(in));
    require(l);
    require(l->getToNode() == 0);
    require(l->getToPort() == IllegalPortId);


    if (inputs_[in])
	return IllegalPortId;

    inputs_[in] = l;

    l->setToNode(this);
    l->setToPort(in);

    return in;
}

/*****************************************************************************/
void Node::disconnectInputLink(Link *l)
/*****************************************************************************/
{
    require(l != 0);

    PortId in = l->getToPort();
    require(validInputPort(in));
    require(inputs_[in] == l);

    inputs_[in] = 0;
    l->setToNode(0);
    l->setToPort(IllegalPortId);
}

/*****************************************************************************/
PortId Node::connectOutputPort(PortId out, Link *l)
/*****************************************************************************/
{
    require(validOutputPort(out));
    require(l);
    require(l->getFromNode() == 0);
    require(l->getFromPort() == IllegalPortId);

    outputs_[out].push_back(l);

    l->setFromNode(this);
    l->setFromPort(out);

    return out;
}

/*****************************************************************************/
void Node::disconnectOutputLink(Link *l)
/*****************************************************************************/
{
    require(l);
    require(validOutputPort(l->getFromPort()));
    require(l->getFromNode() == this);

    std::vector<Link*> &links(outputs_[l->getFromPort()]);
    std::vector<Link*>::iterator i = std::find(links.begin(), links.end(), l);
    ensure(i != links.end());

    links.erase(i);
    l->setFromNode(0);
    l->setFromPort(IllegalPortId);
}

/*****************************************************************************/
bool Node::putData(PortId out, Data *d)
/*****************************************************************************/
{
    require(validOutputPort(out));
    require(d != 0);

    if (dataChannel_.isOpen()) {
	dataChannel_ << Core::XmlOpen("dump-data")
	    + Core::XmlAttribute("node", fullName());
	if (nOutputLinks(out) > 0)
	    d->dump(dataChannel_);
	else
	    dataChannel_ << Core::XmlEmpty("dropped");
	dataChannel_ << Core::XmlClose("dump-data");
    }

    if (nOutputLinks(out) == 0) {
	d->lock();
	if (d->refCount() == 0) d->free();
	else d->release();
	return false;
    }

    for (size_t i = 0; i < nOutputLinks(out); i++) {
	if (!outputs_[out][i]->putData(d)) return false;
    }
    return true;
}

/*****************************************************************************/
Core::Ref<const Attributes> Node::getInputAttributes(PortId in)
/*****************************************************************************/
{
    Core::Ref<const Attributes> result;
    Link *inputLink = inputs_[in];
    if (inputLink != 0) {
	if (inputLink->areAttributesAvailable()) {
	    result = inputLink->attributes();
	} else {
	    AbstractNode *predecessorNode = inputLink->getFromNode();
	    ensure(predecessorNode != 0);
	    if (predecessorNode->configure()) {
		result = inputLink->attributes();
		// If fails, from-node of the input link did not generate output attributes
		// although configuration did not fail.
		ensure(result);
	    } else {
		error() << "Configuration of node '" << predecessorNode->name() << "' failed.";
		result = Core::ref(new Attributes());
	    }
	}
    } else {
	warning("Dead input port: %d.", in);
	result = Core::ref(new Attributes());
    }
    return result;
}

/*****************************************************************************/
void Node::getInputAttributes(PortId in, Attributes &attributes)
/*****************************************************************************/
{
    Core::Ref<const Attributes> result = getInputAttributes(in);
    if (result) attributes = *result;
}

/*****************************************************************************/
bool Node::putOutputAttributes(PortId out, Core::Ref<const Attributes> a)
/*****************************************************************************/
{
    require(validOutputPort(out));
    require(a);

    if (attributesChannel_.isOpen()) {
	attributesChannel_ << Core::XmlOpen("dump-attributes") + Core::XmlAttribute("node", fullName());
	if (nOutputLinks(out) > 0)
	    attributesChannel_ << *a;
	else
	    attributesChannel_ << Core::XmlEmpty("dropped");
	attributesChannel_ << Core::XmlClose("dump-attributes");
    }

    // link attributes to each outgoing link
    for (std::vector<Link*>::iterator it = outputs_[out].begin();
	 it != outputs_[out].end(); it++) {
	(*it)->setAttributes(a);
	(*it)->configure();
	(*it)->clear();
    }
    return true;
}

/*****************************************************************************/
void Node::eraseOutputAttributes()
/*****************************************************************************/
{
    for (std::vector<std::vector<Link*> >::iterator output = outputs_.begin();
	 output != outputs_.end(); ++ output) {
	for (std::vector<Link*>::iterator link = output->begin();
	     link != output->end(); ++ link) {
	    // Recursion must not stop even if the link attribute is erased.
	    // Erased link does not obviously means that all the successor node output links are erased.
	    // E.g. using a cache node.
	    (*link)->eraseAttributes();
	    AbstractNode *successorNode = (*link)->getToNode();
	    if (successorNode != 0)
		successorNode->eraseOutputAttributes();
	}
    }
}

