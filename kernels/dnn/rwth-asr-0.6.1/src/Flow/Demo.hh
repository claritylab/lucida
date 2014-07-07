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
#ifndef _FLOW_DEMO_HH
#define _FLOW_DEMO_HH

#include <map>

#include <Flow/Node.hh>
#include <Flow/Vector.hh>
#include <Flow/Data.hh>

namespace Flow {

  /**
   * @file
   * Flow Demonstration Node
   *
   * This Node is capabable of handling an arbitrary number of input and output ports.
   * If specified all actions performed within an instance of this Node in a given FLOW network
   * will be documented using different log channels.
   *
   * Additionally, all data passing through an instance of this node can be dumped into files.
   *
   * This node takes several assumptions into account
   *
   * 1. Each input is directed to exactly one output
   * 2. There are as many inputs as outputs
   * 3. There is atleast one input and one output
   *
   * To utilize the full functionallity of a Demo Node you need to specify the
   * dump-attributes and dump-data channels inherited from sleeve node
   */

  class DemoNode : public SleeveNode {

  private:
    // definition of the parameters available to the demonode
    // i.e. commandline options, configfile options and so forth
    static const Core::ParameterInt paramMaxInputs_;

    // neccessary XML channels
    // type is included in base class Flow::Node
    mutable Core::XmlChannel demoChannel_;

    // real private membervariables:
    u32 maxInputs_;

    // guardian members for zero-th ports
    bool isFirstInPort_;
    bool isFirstOutPort_;

  protected:
    // mapping of the input port name to the port number
    std::map<std::string,PortId> inputNameToPort_;
    // mapping of the output port name to the port number
    std::map<std::string,PortId> outputNameToPort_;

  public:

    //typedef according to sprint naming rule for access to super class
    typedef SleeveNode Precursor;

    // constructor and destructor
    DemoNode(const Core::Configuration &c);
    // fullName is inherited from Core::Configurable and returns the runtime name of this node
    // i.e. the configuration path
    virtual ~DemoNode();

    // inherited from SleeveNode and hence inherited from Node
    // these three methods are a must have
    virtual bool setParameter(const std::string &name, const std::string &value);
    virtual bool configure();
    virtual bool work(PortId output);

    // inherited from SleeveNode, but needed to be adjusted for an arbitray
    // number of input and output ports
    PortId getOutput(const std::string &name);
    PortId getInput(const std::string &name);

    // claim class name flow-demo for detection from within a flow network
    static std::string filterName() {return "flow-demo";}

    // print out config information
    // void dumpConfig(const Core::Configuration &c );

  };

}; // end namespache Flow
#endif // _FLOW_DEMO_HH
