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
#ifndef _FLF_NODE_FACTORY_HH
#define _FLF_NODE_FACTORY_HH

#include <Core/Configuration.hh>
#include <Core/Hash.hh>
#include <Core/Parameter.hh>

#include "Network.hh"


namespace Flf {

    /**
     * Function producing a node.
     **/
    typedef NodeRef (*NodeCreatorFcn)(const std::string &name, const Core::Configuration &config);
    struct NodeCreator {
	const std::string name;
	const std::string generalDesc;
	const std::string configDesc;
	const std::string portDesc;
	NodeCreatorFcn f;
	// old, deprecated
	NodeCreator(const std::string &name, const std::string &desc, NodeCreatorFcn f) :
	    name(name), generalDesc(desc), f(f) { require(f); }
	// new
	NodeCreator(const std::string &name,
		    const std::string &generalDesc,
		    const std::string &configDesc,
		    const std::string &portDesc,
		    NodeCreatorFcn f) :
	    name(name), generalDesc(generalDesc), configDesc(configDesc), portDesc(portDesc), f(f) { require(f); }
	NodeRef operator() (const std::string &name, const Core::Configuration &config) {
	    return f(name, config);
	}
	void dump(std::ostream &o) const;
    };

    /**
     * Class to register node creator and
     * creates node from given name and node config.
    **/
    class NodeFactory {
    public:
	static const Core::ParameterString paramNodeType;
    private:
	typedef Core::hash_map<std::string, NodeCreator, Core::StringHash> NodeCreatorMap;
	NodeCreatorMap creatorMap_;
    public:
	NodeFactory();
	bool add(const NodeCreator &creator);
	NodeRef createNode(const std::string &name, const Core::Configuration &config);
	void dumpNodeList(std::ostream &o) const;
	void dumpNodeDescription(std::ostream &o, const std::string &name) const;
    };

} // namespace
#endif // _FLF_NODE_FACTORY_HH
