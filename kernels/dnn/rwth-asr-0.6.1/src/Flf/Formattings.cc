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
#include <Core/XmlStream.hh>

#include "Formattings.hh"

namespace Flf {

    // -------------------------------------------------------------------------
    const Core::ParameterBool LayeredComponent::paramUse(
	"use",
	"use",
	true);
    const Core::ParameterString LayeredComponent::paramName(
	"name",
	"name",
	"");

    void LayeredComponent::openLayer(const std::string &name) {
	if (useLayer_) {
	    if (name.empty())
		clog() << Core::XmlOpen("layer") + Core::XmlAttribute("name", layerName_);
	    else
		clog() << Core::XmlOpen("layer") + Core::XmlAttribute("name", layerName_ + "/" + name);
	}
    }

    void LayeredComponent::closeLayer() {
	if (useLayer_)
	    clog() << Core::XmlClose("layer");
    }

    LayeredComponent::LayeredComponent(const Core::Configuration &config, const std::string &layerName) :
	Core::Component(config) {
	layerName_ = paramName(select("layer"), layerName);
	useLayer_ = paramUse(select("layer"));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LayerStartNode : public FilterNode {
	friend class Network;
    public:
	static const Core::ParameterString paramName;
    private:
	std::string name_;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    clog() << Core::XmlOpen("layer") + Core::XmlAttribute("name", name_);
	    return l;
	}
    public:
	LayerStartNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {
	    name_ = paramName(config, name);
	}
	virtual ~LayerStartNode() {}
    };
    const Core::ParameterString LayerStartNode::paramName(
	"name",
	"layer name");

    NodeRef createLayerStartNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LayerStartNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class LayerEndNode : public FilterNode {
	friend class Network;
    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    clog() << Core::XmlClose("layer");
	    return l;
	}
    public:
	LayerEndNode(const std::string &name, const Core::Configuration &config) :
	    FilterNode(name, config) {}
	virtual ~LayerEndNode() {}
    };
    NodeRef createLayerEndNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new LayerEndNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
