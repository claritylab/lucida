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
#ifndef _AM_CLASSIC_DECISION_TREE_HH
#define _AM_CLASSIC_DECISION_TREE_HH

#include <Cart/Conditions.hh>
#include <Cart/DecisionTree.hh>
#include <Cart/Example.hh>
#include <Cart/Properties.hh>
#include <Core/ReferenceCounting.hh>
#include <Mm/Types.hh>

#include <Am/ClassicStateModel.hh>


namespace Am {
    /*
      Definition of all possible properties of an AllophoneState
      + Conditions
    */
    class PropertyMap :
	public Cart::PropertyMap {
    protected:
	typedef PropertyMap Self;
	typedef Cart::PropertyMap Precursor;

    public:
	const Index       termIndex;
	const std::string termString;

    private:
	size_t historySize_;
	size_t futureSize_;

    public:
	PropertyMap();

	PropertyMap(ClassicStateModelRef stateModel);

	void set(
	    const StringList & keys,
	    const ListOfIndexedStringList & values);

	size_t historySize() const { return historySize_; }
	size_t futureSize()  const { return futureSize_;  }
    };
    typedef Core::Ref<PropertyMap> PropertyMapRef;



    // ============================================================================


    /*
      diff on two property maps
    */
    class PropertyMapDiff :
	Core::Component {
    private:
	/*
	    1 = different number of states
	    2 = different boundaries
	    4 = different phoneme inventories
	    8 = different number of histories
	   16 = different number of futures
	   32 = different number of conditions
	*/
	const PropertyMap & map1_;
	const PropertyMap & map2_;
	u32 differences_;

    private:
	void logDiff(
	    Core::XmlWriter & xml,
	    PropertyMap::Index l,
	    PropertyMap::Index r);

    public:
	PropertyMapDiff(
	    const Core::Configuration & config,
	    const PropertyMap & map1,
	    const PropertyMap & map2);

	u32 differences() { return differences_; }
	bool hasDifferences() { return (differences_ != 0); }
    };


    // ============================================================================



    /*
      AllophoneState + Conditions
    */
    typedef Cart::Conditions Conditions;

    class Properties :
	public Cart::Properties {
    protected:
	typedef Cart::Properties Precursor;
	typedef Properties Self;

    private:
	const PropertyMap &map_;
	AllophoneState alloState_;
	const Conditions *cond_;

	Index maxHistorySize_;
	Index maxFutureSize_;

    public:
	Properties(Cart::PropertyMapRef map) :
	    Precursor(map),
	    map_(dynamic_cast<const PropertyMap &>(*(map.get()))),
	    alloState_(), cond_(0) {}

	Properties(
	    Cart::PropertyMapRef map,
	    const AllophoneState &alloState
	    ) :
	    Precursor(map),
	    map_(dynamic_cast<const PropertyMap &>(*(map.get()))),
	    alloState_(), cond_(0) {
	    set(alloState);
	}

	void set(const AllophoneState &alloState) {
	    alloState_ = alloState;
	    maxHistorySize_ = Index(alloState_.allophone()->history().size());
	    maxFutureSize_ = Index(alloState_.allophone()->future().size());
	}
	const AllophoneState & allophoneState() const {
	    require(alloState_.allophone()); return alloState_;
	}

	void set(const Conditions & cond) {
	    cond_ = &cond;
	}
	const Conditions & conditions() const {
	    require(cond_); return *cond_;
	}

	Index operator[](const Index keyIndex) const {
	    require(alloState_.allophone());
	    const Allophone &allo(*alloState_.allophone());
	    switch (keyIndex) {
	    case 0:
		return Index(alloState_.state());
	    case 1:
		return Index(allo.boundary);
	    default:
		Index contextIndex = keyIndex - 2;
		if (contextIndex < Index(map_.historySize())) {
		    return (contextIndex < maxHistorySize_) ?
			Index(allo.history()[Index(map_.historySize()) - contextIndex - 1]) :
			map_.termIndex;
		    //			map_.undefinedIndex;
		}
		contextIndex -= Index(map_.historySize());
		if (contextIndex == 0) {
		    return Index(allo.central());
		}
		--contextIndex;
		if (contextIndex < Index(map_.futureSize())) {
		    return (contextIndex < maxFutureSize_) ?
			Index(allo.future()[contextIndex]) :
			map_.termIndex;
		    //			map_.undefinedIndex;
		}
		if (keyIndex < Index(map_.size()))
		    return (cond_) ?
			Index(cond_->hasCondition(map_.key(keyIndex))) :
			Index(false);
		return map_.undefinedIndex;
	    }
	}
    };



    // ============================================================================



    class DecisionTree :
	public Cart::DecisionTree {
    protected:
	typedef DecisionTree Self;
	typedef Cart::DecisionTree Precursor;

    public:
	DecisionTree(
	    const Core::Configuration & config,
	    ClassicStateModelRef stateModel) :
	    Precursor(config, Cart::PropertyMapRef(new PropertyMap(stateModel))) {}

	const PropertyMap & stateModelMap() {
	    return dynamic_cast<const PropertyMap &>(*(map_.get()));
	}

	bool loadFromString(const std::string & str);

	bool loadFromStream(std::istream & i);

	bool loadFromFile(const std::string & filename);
    };

} // namespace Am

#endif // _AM_CLASSIC_DECISION_TREE_HH
