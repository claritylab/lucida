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
#include "Component.hh"
#include <Core/MapParser.hh>

using namespace Mc;


const Core::ParameterFloat Component::paramScale(
    "scale", "log-linear scale", 1.0);

const Core::ParameterString ScaleUpdate::paramMapFilename(
    "scales-file", "XML file with model-name scale pairs");

//========================================================================================================
ScaleUpdate::ScaleUpdate() : parentScale_(1)
{}

bool ScaleUpdate::setMap(const Core::Configuration &c)
{
    std::string filename = paramMapFilename(c);
    if (filename.empty())
	return false;

    ScaleMap scaleMap;
    Core::XmlMapDocument<ScaleMap> parser(
	c, scaleMap, "model-combination", "model", "name", "scale");
    parser.parseFile(filename.c_str());

    ScaleMap *extendedMap = new ScaleMap;
    for(ScaleMap::const_iterator scale = scaleMap.begin(); scale != scaleMap.end(); ++ scale) {
	std::string extendedKey = c.getSelection();
	if (!scale->first.empty())
	    extendedKey = c.prepareResourceName(extendedKey, scale->first);
	(*extendedMap)[extendedKey] = scale->second;
    }
    setMap(Core::Ref<const ScaleMap>(extendedMap));
    return true;
}

bool ScaleUpdate::findScale(const std::string name, Scale &scale) const
{
    if (map_) {
	ScaleMap::const_iterator s = map_->find(name);
	if (s != map_->end()) {
	    scale = s->second;
	    return true;
	}
    }
    return false;
}

//========================================================================================================
Component::Component(const Core::Configuration &c) :
    Core::Component(c)
{
    scale_ = ownScale_ = paramScale(c);
}

Component::~Component()
{}

void Component::updateScales(ScaleUpdate update)
{
    Scale newScale = ownScale();
    if (update.findScale(fullName(), newScale))
	log("Scale set to %f.", newScale);
    setScale(newScale, update.parentScale(), update);
}

void Component::load()
{
    ScaleUpdate update;
    if (update.setMap(config)) {
	update.setParentScale(parentScale());
	updateScales(update);
    }
}

void Component::setOwnScale(Scale scale)
{
    if (ownScale() != scale)
	setScale(scale, parentScale(), ScaleUpdate());
}

void Component::setParentScale(Scale scale)
{
    if (parentScale() != scale)
	setScale(ownScale(), scale, ScaleUpdate());
}

void Component::setScale(Scale ownScale, Scale parentScale, ScaleUpdate update)
{
    ownScale_ = ownScale;
    scale_ = ownScale_ * parentScale;

    update.setParentScale(scale());
    distributeScaleUpdate(update);
}

void Component::getDependencies(Core::DependencySet &dependency) const
{
    dependency.add(name() + " scale", ownScale());
}
