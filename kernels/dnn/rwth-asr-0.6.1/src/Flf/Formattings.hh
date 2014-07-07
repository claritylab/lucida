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
#ifndef _FLF_FORMATTINGS_HH
#define _FLF_FORMATTINGS_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>

#include "FlfCore/Lattice.hh"
#include "Network.hh"


namespace Flf {

    class LayeredComponent : public virtual Core::Component {
	static const Core::ParameterString paramName;
	static const Core::ParameterBool paramUse;
    private:
	std::string layerName_;
	bool useLayer_;
    protected:
	void openLayer(const std::string &name = "");
	void closeLayer();
    public:
	LayeredComponent(const Core::Configuration &config, const std::string &layerName = "");
	virtual ~LayeredComponent() {}
    };

    /*
      - embeds log data into layer-tags
    */
    NodeRef createLayerStartNode(const std::string &name, const Core::Configuration &config);
    NodeRef createLayerEndNode(const std::string &name, const Core::Configuration &config);

} // namespace Flf
#endif // _FLF_FORMATTINGS_HH
