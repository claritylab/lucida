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
#ifndef _FLOW_DEMO_CC
#define _FLOW_DEMO_CC

#include <Flow/Demo.hh>

using namespace Flow;

// initialize parameters
const Core::ParameterInt DemoNode::paramMaxInputs_("max-input-nodes","maximum number of input ports",Core::Type<u32>::max);

// =======================================================

DemoNode::DemoNode(const Core::Configuration &c) :
    Core::Component(c), Precursor(c),
    demoChannel_(c,"demo-info"),
    maxInputs_(1),
    isFirstInPort_(true) ,
    isFirstOutPort_(true)
{
	// note that attributesChannel_ and dataChannel_ are inherited from base class Flow::Node
	// and can be set using dump-attributes = <file> and dump-data = <file>
	// i.e. if you just want to dump the attributes and the data seen by the demo node and not of all nodes use
	// demo.dump-data.channel = <file> and demo.dump-attributes.channel = <file>
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("demo-node: " + fullName());
	}

	// the operator (config,defaultvalue) gets the value from the config
	// if the parameter is not present in the config
	// the default value is returned
	if(paramMaxInputs_(c,0)){
		maxInputs_ = paramMaxInputs_(c);
	}

	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("initializing parameter")
					 << Core::XmlFull("maxInputNodes",maxInputs_)
					 << Core::XmlClose("initializing parameter");
	}
}

DemoNode::~DemoNode() {
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlClose("demo-node: " + fullName());
	}
}

bool DemoNode::setParameter(const std::string &name, const std::string &value){
	if(demoChannel_.isOpen()){
		require_(demoChannel_.isOpen());
		demoChannel_ << Core::XmlOpen("setParameter method of " + fullName())
					 << Core::XmlComment("Demo Node setParameters method is called")
					 << Core::XmlOpen("received parameter")
					 << Core::XmlFull("parameter name",name)
					 << Core::XmlFull("parameter value",value)
					 << Core::XmlClose("received parameter");
	}
	// check if the given name matches any parameter string
	if(paramMaxInputs_.match(name)){
		int maxin =paramMaxInputs_(value);
		require_(maxin > 0);
		maxInputs_=(u32)maxin;
		if(demoChannel_.isOpen()){
			demoChannel_ << Core::XmlOpen("parameter maxInputs matched")
						 << Core::XmlFull("maxInputs_",maxInputs_)
						 << Core::XmlClose("parameter maxInputs matched");
		}
	} // end if
	else {
		if(demoChannel_.isOpen()){
			demoChannel_ << Core::XmlEmpty("parameter not matched in Demo Node")
						 << Core::XmlEmpty("parameter forwarded to precursor node")
						 << Core::XmlClose("setParameter method of " + fullName());
		}
		return Precursor::setParameter(name,value);
	} // else if
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlClose("setParameter method of " + fullName());
	}
	return true;
}

/**
 * we make some general assumptions here
 * 1.
 * if attributes arrive at input port x than the intended
 * recepient node is connected using output port x
 * 2.
 * attributes will not be merged or altered but simply forwarded
 */

bool DemoNode::configure() {
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("configure method of " + fullName());
		demoChannel_ << Core::XmlComment("configure method of a Demo Node is called");

		// require that the number of input ports equals the number of output
		// ports
		demoChannel_ << Core::XmlEmpty("checking number of input ports == number of output ports");
		demoChannel_ << (Core::XmlEmpty("")
						 + Core::XmlAttribute("nInput",nInputs())
						 + Core::XmlAttribute("nOutput",nOutputs())
			);
	}
	// various methods like warning, error and so forth are defined in
	// Core:Component and hence inherited in all classes !
	if(nInputs() != nOutputs()){
		error("Number of input and output ports differs !");
	}

	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlComment("number of input ports matches number of output ports");
		demoChannel_ << (Core::XmlEmpty("ports") + Core::XmlAttribute("number of ports",nInputs()));
	}
	bool status = true;
	// configure input port attributes
	// note due to the generic number of possible input ports
	// and the variety of possible attribute types it is not
	// possible to check whehter or not the attribute types
	// are correct; at least not in a reasonable way
	for(PortId port = 0; port < nInputs(); ++port){
		if(demoChannel_.isOpen()){
			demoChannel_ << Core::XmlOpen("fetching attributes")
						 << Core::XmlFull("from input port",port)
						 << Core::XmlClose("fetching attributes");
		}
		Core::Ref<const Attributes> attr = getInputAttributes(port);
		// note the output of attributes into an xml file is done
		// in the putOutputAttributes method inherited from Flow::Node
		if(!putOutputAttributes(port,attr)){
			if(demoChannel_.isOpen()){
				demoChannel_ << (Core::XmlEmpty("forwarding of attributes failed")
								 + Core::XmlAttribute("port",port));
			}
			status = false;
		}
	}
	if(demoChannel_.isOpen()){
		demoChannel_ << (Core::XmlEmpty("forwarding status of attributes")
						 + Core::XmlAttribute("status",status));
		demoChannel_ << Core::XmlClose("configure method of " + fullName());
	}
	return status;
}

bool DemoNode::work(PortId output){
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("work method of " + fullName());
	}
	require(nInputs() > 0);
	require(nInputs() == nOutputs());

	// create necessary smart data pointer
	DataPtr<Data> data;
	if(demoChannel_.isOpen()){
		demoChannel_ << (Core::XmlEmpty("fetching data from input port") +
						 Core::XmlAttribute("port",output));
	}
	// note: because we require that the number of output ports equalls the
	// the number of input notes and that the data of input port x should be
	// send to output port x the getData method is called with output
	// in a "real" flow node you have to use the correct input port id !
	if(!getData(output,data)){
		// put "empty" object into the queue of the flow network
		// the following node won't get data and signal this
		// Flow is a pull network
		if(demoChannel_.isOpen()){
			demoChannel_ << Core::XmlEmpty("no data fetched");
			demoChannel_ << Core::XmlEmpty("provding empty data for successor nodes");
		}
		/* in cases you require an output port to have a specific number of links
		 *
		 *    if(nOutputLinks(output) > 0){
		 *      "do some stuff like providing data"
		 *    } else {
		 *      "do some error handling like: "
		 *    demoChannel_ << (Core::XmlEmpty("no activ links from output port")
		 *                   + Core::XmlAttribute("output port",output))
		 *                << Core::XmlClose("work method of " + fullName());
		 *    return false;
		 *    }
		 */

		// basically you send an END OF SEQUENCE signal
		bool success = putData(output,data.get());
		if(demoChannel_.isOpen()){
			demoChannel_ << (Core::XmlEmpty("data provided")
							 + Core::XmlAttribute("output port",output))
						 << Core::XmlClose("work method of " + fullName());
		}
		return success;
	}

	// forwarding data
	// note: similar to the putAttributes method the data is dumped in the
	// putData method.
	if(demoChannel_.isOpen()){
		demoChannel_ << (Core::XmlEmpty("providing data from input port")
						 + Core::XmlAttribute("port",output));
	}
	if(!putData(output,data.get())){
		if(demoChannel_.isOpen()){
			demoChannel_ << (Core::XmlEmpty("no data provided")
							 + Core::XmlAttribute("output port",output))
						 << Core::XmlClose("work method of " + fullName());
		}
		return false;
	}
	if(demoChannel_.isOpen()){
		demoChannel_ << (Core::XmlEmpty("data provided")
						 + Core::XmlAttribute("output port",output))
					 << Core::XmlClose("work method of " + fullName());
	}
	return true;
}

// generic getInput routine allowing an arbitrary number of input ports
// a similar implementation can be found in Flow::Collector.hh
PortId DemoNode::getInput(const std::string &name){
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("getInput method of " + fullName())
					 << Core::XmlComment("allowing arbitrary number of input nodes");
		// if you want to create a node with a fixed number of ports
		// than you could programm it like
		// if (name == "x") return 0

		// find out if the desired node is already found and connected
		// i.e. name and port id are already known
		demoChannel_ << Core::XmlEmpty("testing if port name="+ name + " already known");
	}
	if(inputNameToPort_.find(name) != inputNameToPort_.end()){
		if(demoChannel_.isOpen()){
			demoChannel_ << (Core::XmlEmpty("port name already taken with port") +
							 Core::XmlAttribute("number",inputNameToPort_[name]))
						 << Core::XmlClose("getInput method of " + fullName());
		}
		return IllegalPortId;
	}
	PortId port = 0;

	// ok new Input source hence add it to the namePort mapping
	// and create new Port

	if(!isFirstInPort_){
		port = addInput();
	}

	inputNameToPort_[name]=port;
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("adding new input port")
					 << (Core::XmlEmpty("") +
						 Core::XmlAttribute("port name",name) +
						 Core::XmlAttribute("port number",port))
					 << Core::XmlClose("adding new input port")
					 << Core::XmlClose("getInput method of " + fullName());
	}
	isFirstInPort_=false;
	return port;
}

// generic getOutput method allowing an arbitrary number of output ports
PortId DemoNode::getOutput(const std::string &name){
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("getOutput method of " + fullName())
					 << Core::XmlComment("allowing arbitrary number of output nodes");
	}
	/* if you want your getOutput method to allow only an arbitrary number of ports
	 * with port names given by postive numbers in the flow network file you could
	 * programm this like: a similiar method can be found in Image::VitruvianManPatches
	 *
	 *  char* endptr;
	 *  int result = strtol(name.c_str(),&endptr,10);
	 *  // check if the string has been a integer number and that is in range 0 to maxInputs_
	 *  demoChannel_ << Core::XmlOpen("checking written port number")
	 *            << Core::XmlEmpty("port number = " + name)
	 *           << Core::XmlClose("checking written port number");
	 *  if(*endptr!='\0' || result < 0 || result >= (int)maxInputs_){
	 *    demoChannel_ << Core::XmlEmpty("port number no number or not in range 0 to max-input-nodes")
	 *                 << Core::XmlEmpty("no new output port added")
	 *                 << Core::XmlClose("getOutput method of " + fullName());
	 *    return IllegalPortId;
	 *  }
	 *  PortId newPort = addOutput(result);
	 *  demoChannel_ << Core::XmlOpen("new output port added")
	 *               << Core::XmlFull("output port",newPort)
	 *               << Core::XmlClose("new output port added")
	 *               << Core::XmlClose("getOutput method of " + fullName());
	 * return newPort;
	 */
	// for consistency we allow arbitrary names as in getInput()
	PortId port = 0;
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlEmpty("testing if port name="+ name + " is already known");
	}
	if(outputNameToPort_.find(name) != outputNameToPort_.end()){
		port = outputNameToPort_[name];
		if(demoChannel_.isOpen()){
			demoChannel_ << (Core::XmlEmpty("port name already taken with port") +
							 Core::XmlAttribute("number",port))
						 <<  (Core::XmlEmpty("adding new link to ") +
							  Core::XmlAttribute("port",port))
						 << Core::XmlClose("getInput method of " + fullName());
		}
		return addOutput(port);
	}

	// ok new Output source hence add it to the namePort mapping
	// and create new Port

	if(!isFirstOutPort_){
		port = addOutput();
	}

	outputNameToPort_[name]=port;
	if(demoChannel_.isOpen()){
		demoChannel_ << Core::XmlOpen("adding new output port")
					 << (Core::XmlEmpty("") +
						 Core::XmlAttribute("port name",name) +
						 Core::XmlAttribute("port number",port))
					 << Core::XmlClose("adding new output port")
					 << Core::XmlClose("getOutput method of " + fullName());
	}
	isFirstOutPort_=false;
	return port;
}
#endif // _FLOW_DEMO_HH
