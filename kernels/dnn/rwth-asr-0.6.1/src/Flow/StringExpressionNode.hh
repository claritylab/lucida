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
#ifndef _FLOW_STRING_EXPRESSION_NODE_HH
#define _FLOW_STRING_EXPRESSION_NODE_HH

#include "Node.hh"
#include "DataAdaptor.hh"
#include <Core/StringExpression.hh>

namespace Flow {

    /** String expression resolved by node inputs.
     *
     *  Usage:
     *   template: 'string1$input(inputPortName1)string2$input(inputPortName2)...'
     *   input ports to read data from will be: inputPortName1, inputPortName2
     *   resulting string value: template expression, but $input(inputPortName1),
     *     $input(inputPortName2), ... replaced by input values
     *
     *  Example:
     *    ...
     *    <node name="cepstrum" ... warping-function="linear(a=$input(warping-factor))"/>
     *    <link from="..." to="cepstrum:warping-factor"/>
     *    ...
     *
     *    Value of the expression in warping-function will be resolved for each input at the
     *    port warping-factor.
     *
     *  Synchronization of ports:
     *    Classes derived from StringExpressionNode will call the update(Timestamp ts) function to
     *    update the string-expression input-ports. Input ports are read if the timestamp 'ts' does
     *    not fall into the intervall [start-time..end-time] of the current parameter value. Thus,
     *    parameters valid for a complete segment should have start-time equal segment start-time and
     *    end-time equals segment end-time. It is of course possible to have parameters valid for
     *    shorter intervals than the complete segment.
     */
    class StringExpressionNode : public virtual Node {
	typedef Node Precursor;
    private:
	static const std::string openTag;
	static const std::string closeTag;
    private:
	struct InputPort { PortId portId_; Timestamp timestamp_; };
	Core::StringHashMap<InputPort> inputPorts_;
	PortId nextPortId_;

	std::string template_;
	Core::StringExpression stringExpression_;
	std::string resolvedValue_;
	bool newTemplate_;
	bool newData_;
    private:
	void update(Core::StringHashMap<InputPort>::iterator inputPort, const String &value);
    public:
	static std::string filterName() { defect(); }
	StringExpressionNode(const Core::Configuration &c, PortId firstPortId);
	virtual ~StringExpressionNode() {}

	Flow::PortId getInput(const std::string &name);
	void setTemplate(const std::string&);

	/** Performs configuration and merges its own input attributes into @param result. */
	bool configure(Attributes &result);

	/** Reads the input ports and resolves the string expression.
	 *  Input ports are read till @param timestamp falls in the interval [start-time..end-time]
	 *  of an input object.
	 *  @return is true, if the value of string expression has changed.
	 */
	bool update(const Timestamp &timestamp);
	/**
	 *update without timestamp consideration
	 */
	bool update();
	/** @return is resolved value of string expression. */
	std::string value();
	void reset();
    };
}


#endif // _FLOW_STRING_EXPRESSION_NODE_HH
