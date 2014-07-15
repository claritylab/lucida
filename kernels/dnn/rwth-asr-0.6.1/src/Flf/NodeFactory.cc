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
#include "NodeFactory.hh"
#include "NodeRegistration.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    namespace {
	void dumpFormatted(std::ostream &o, const std::string &s, u32 indent = 0) {
	    std::string::size_type i, j;
	    for (i = 0; (j = s.find_first_of("\n", i)) != std::string::npos; i = j + 1)
		o << std::setw(indent) << " " << s.substr(i, j - i) << std::endl;
	    o << std::setw(indent) << " " << s.substr(i, s.size() - 1) << std::endl;
	}
    } // namespace

    void NodeCreator::dump(std::ostream &o) const {
	if (generalDesc.empty() && configDesc.empty() && portDesc.empty()) {
	    o << "No description available for node \"" << name << "\"." << std::endl;
	    return;
	}
	if (!generalDesc.empty()) {
	    o << "Node \"" << name << "\":" << std::endl;
	    dumpFormatted(o, generalDesc, 4);
	}
	if (!configDesc.empty()) {
	    o << "Configuration of node \"" << name << "\":" << std::endl;
	    dumpFormatted(o, configDesc, 4);
	}
	if (!portDesc.empty()) {
	    o << "Port assignment of node \"" << name << "\":" << std::endl;
	    dumpFormatted(o, portDesc, 4);
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
       Class to register node creator and
       creates node from given name and node config.
    **/
    NodeFactory::NodeFactory() {
	registerNodeCreators(this);
    }
    bool NodeFactory::add(const NodeCreator &creator) {
	return creatorMap_.insert(std::make_pair(creator.name, creator)).second;
    }
    NodeRef NodeFactory::createNode(const std::string &name, const Core::Configuration &config) {
	NodeCreatorMap::iterator itCreator = creatorMap_.find(paramNodeType(config));
	if (itCreator != creatorMap_.end())
	    return itCreator->second(name, config);
	else
	    return NodeRef();
    }
    void NodeFactory::dumpNodeList(std::ostream &o) const {
	std::vector<std::string> sortedNodes;
	for (NodeCreatorMap::const_iterator it = creatorMap_.begin(); it != creatorMap_.end(); ++it)
	    sortedNodes.push_back(it->second.name);
	std::sort(sortedNodes.begin(), sortedNodes.end());
	o << "List of registered nodes:" << std::endl;
	for (std::vector<std::string>::const_iterator it = sortedNodes.begin(); it != sortedNodes.end(); ++it)
	    o << "    " << *it << std::endl;
    }
    void NodeFactory::dumpNodeDescription(std::ostream &o, const std::string &name) const {
	NodeCreatorMap::const_iterator it = creatorMap_.find(name);
	if (it == creatorMap_.end())
	    o << "Error: Could not find node \"" << name << "\"" << std::endl;
	else
	    it->second.dump(o);
    }

    const Core::ParameterString NodeFactory::paramNodeType(
	"type",
	"node type",
	"");
    // -------------------------------------------------------------------------

} // namespace
