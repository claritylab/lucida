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
#ifndef _MC_COMPONENT_HH
#define _MC_COMPONENT_HH

#include "Types.hh"
#include <Core/Component.hh>
#include <Core/Dependency.hh>
#include <Core/Hash.hh>
#include <Core/ReferenceCounting.hh>

namespace Mc {

    /** Object passed to each Component during configuration update */
    class ScaleUpdate {
    private:
	class ScaleMap : public Core::StringHashMap<Scale>, public Core::ReferenceCounted {};
    public:
	static const Core::ParameterString  paramMapFilename;
    private:
	/** contains name of objects and the corresponding scale */
	Core::Ref<const ScaleMap> map_;
	/** scale of the parent object
	 *  Updated at each scaled node
	 */
	Scale parentScale_;
    public:
	ScaleUpdate();

	/** Reads name to scale map from the xml file given by configuration @param c.
	 *  Each name is extended by selection of the configuration @param c.
	 *  @return is false if no file has been loaded.
	 */
	bool setMap(const Core::Configuration &c);
	void setMap(Core::Ref<const ScaleMap> map) { map_ = map; }
	bool findScale(const std::string name, Scale &scale) const;

	void setParentScale(Scale parentScale) { parentScale_ = parentScale; }
	Scale parentScale() const { return parentScale_; }
    };

    /** Base class scaled model combination components. */
    class Component : public virtual Core::Component {
    public:
	static const Core::ParameterFloat paramScale;
    private:
	/** original scale */
	Scale ownScale_;
	/** parent-scale * own-scale */
	Scale scale_;
    private:
	void setScale(Scale ownScale, Scale parentScale, ScaleUpdate update);
    protected:
	/**
	 *  Override this function
	 *    -to perform updates after scale_ has new value
	 *    -to distibute @param scaleUpdate to child objects.
	 *  @param scaleUpdate contains
	 *    -object-name to scale map
	 *    -scale of this object as parentScale
	 */
	virtual void distributeScaleUpdate(const ScaleUpdate &scaleUpdate) {}
    public:
	Component(const Core::Configuration &c);
	virtual ~Component();

	/** Updates the scale of this and the child objects.
	 *  If @param update object does not contain new scale for this object, old one is kept.
	 *  Passes the modified update to the child objects.
	 */
	void updateScales(ScaleUpdate update);
	/** Loads map file and updates scales */
	void load();

	/** Sets ownScale_ and updates scale_.
	 *  If necessary updates the child objects.
	 */
	void setOwnScale(Scale);
	/** Updates scale_ with parent's scale (@param scale).
	 *  If necessary updates the child objects.
	 */
	void setParentScale(Scale scale);

	/** parent-scale * own-scale */
	Scale scale() const { return scale_; }
	Scale ownScale() const { return ownScale_; }
	Scale parentScale() const { return scale_ / ownScale_; }

	void getDependencies(Core::DependencySet &) const;
    };

} // namespace Mc

#endif // _MC_COMPONENT_HH
