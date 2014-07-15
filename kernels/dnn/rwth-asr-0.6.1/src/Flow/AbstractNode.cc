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
#include "AbstractNode.hh"

using namespace Flow;


/*****************************************************************************/
const Core::ParameterBool AbstractNode::paramThreaded(
	"threaded", "enable separeted thread for node", false);
const Core::ParameterBool AbstractNode::paramIgnoreUnknownParameters(
	"ignore-unknown-parameters",
	"Controls if error is generated when setting the value of unknown parameters.",
	false);
/*****************************************************************************/


/*****************************************************************************/
AbstractNode::AbstractNode(const Core::Configuration &c) :
	Component(c),
	//Thread(),
	threaded_(false),
	ignoreUnknownParameters_(paramIgnoreUnknownParameters(c))
/*****************************************************************************/
{
	setThreaded(paramThreaded(c));
}

/*****************************************************************************/
void AbstractNode::setThreaded(const std::string &threaded)
/*****************************************************************************/
{
	setThreaded(paramThreaded(threaded));
}


/***************************************************************************************/
bool AbstractNode::checkAndSetParameter(const std::string &name, const std::string &value)
/***************************************************************************************/
{
	if (paramIgnoreUnknownParameters.match(name)) {
		ignoreUnknownParameters_ = paramIgnoreUnknownParameters(value);
		return true;
	} else {
		std::string staticValue; /// It would be better to make sure that checkAndSetParameter is only called when parameters _changed_
		if (config.get(name, staticValue) && staticValue.size())
		{
		    if (value != staticValue)
			warning() << "ignoring flow-parameter '" << name << "=" << value << "', because it was set through configuration: '" << staticValue << "'";
		    return true;
		} else {
		return setParameter(name, value) || ignoreUnknownParameters_;
		}
	}
}


/***************************************************************************************/
bool AbstractNode::addParameter(const std::string &name, const Core::StringExpression &value)
/***************************************************************************************/
{
	std::string v;
	if (value.isConstant()) {
		if (!value.value(v))
			defect();
		return checkAndSetParameter(name, v);
	}
	std::pair<Parameters::iterator, bool> p =
		parameters_.insert(Parameters::value_type(name, value));
	if (p.second) {
		if (p.first->second.value(v))
			return checkAndSetParameter(p.first->first, v);
	} else {
		error("Parameter \"%s\" already exists.", name.c_str());
	}
	return p.second;
}


/*****************************************************************************/
bool AbstractNode::setNetworkParameter(const std::string &parameterName,
	const std::string &networkParameterName,
	const std::string &networkParameterValue)
/*****************************************************************************/
{

	Parameters::iterator p = parameters_.find(parameterName);
	verify(p != parameters_.end());

	if (!p->second.setVariable(networkParameterName, networkParameterValue))
		defect();

	std::string v;
	if (p->second.value(v)) {
		if (checkAndSetParameter(parameterName, v)) {
			// Erase output attributes (recursively) since the node has been changed
			eraseOutputAttributes();
			return true;
		} else {
			error("Failed to set network parameter key=\"%s\" (value=\"%s\") " \
				"of parameter=\"%s\" in node=\"%s\" .",
				networkParameterName.c_str(), networkParameterValue.c_str(),
				parameterName.c_str(), name().c_str());
			return false;
		}
	}
	return true;
}


/*****************************************************************************/
void AbstractNode::Run()
/*****************************************************************************/
{
	// infinite loop until filter has any input data
	while (1) {
		// loop over all connected output ports
		for (PortId i = 0; i < nOutputs(); i++) {

			if (nOutputLinks(i) > 0)
				work(i);
		}
	}
}



/*****************************************************************************/
std::ostream& Flow::operator <<(std::ostream &o, const AbstractNode &n)
/*****************************************************************************/
{
	o << n.name();

	if (n.isThreaded()) {
		o << " ";
		o << "threaded";
	}

	return o;
}


/*****************************************************************************/
void AbstractNode::addUnresolvedParameter(const std::string &key, const std::string &value)
/*****************************************************************************/
{
	dumpParameters_[key] = value;
}

/*****************************************************************************/
const AbstractNode::UnresolvedAttributes& AbstractNode::unresolvedAttributes() const
/*****************************************************************************/
{
	return dumpParameters_;
}
