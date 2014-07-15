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
#ifndef _CART_CLUSTER_HH
#define _CART_CLUSTER_HH

#include <Core/Assertions.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

#include "Properties.hh"
#include "DecisionTree.hh"
#include "Example.hh"

namespace Cart {

// ============================================================================
    /**
       Clustering induced by the application
       of a decision tree on a list of examples.
    */
    class Cluster :
	public Core::ReferenceCounted {
    public:
	const DecisionTree::Node * node;
	ConstExampleRefList * exampleRefs;

	Cluster(
	    const DecisionTree::Node * node,
	    ConstExampleRefList * exampleRefs) :
	    node(node),
	    exampleRefs(exampleRefs) {}
	~Cluster() {
	    delete exampleRefs;
	}

	void write(std::ostream & os) const;
	void writeXml(Core::XmlWriter & xml) const;
    };
    typedef Core::Ref<const Cluster> ClusterRef;
    typedef std::vector<ClusterRef> ClusterRefList;


// ============================================================================


    class ClusterList :
	Core::Component {
    protected:
	typedef Core::Component Precursor;

    public:
	static const Core::ParameterString paramClusterFilename;
	typedef ClusterRefList::const_iterator const_iterator;

    private:
	PropertyMapRef map_;
	ClusterRefList clusterRefs_;

    public:
	ClusterList(
	    const Core::Configuration & config,
	    PropertyMapRef map = PropertyMapRef(new PropertyMap)) :
	    Precursor(config),
	    map_(map) {
	    setMap(map);
	}

	void setMap(PropertyMapRef map) { require(map); map_ = map; }
	bool hasMap() { return !map_->empty(); }
	PropertyMapRef getMap() const { return map_; }
	const PropertyMap & map() const { return *map_; }

	void add(Cluster * cluster) { clusterRefs_.push_back(ClusterRef(cluster)); }
	void add(ClusterRef clusterRef) { clusterRefs_.push_back(clusterRef); }

	size_t size() const { return clusterRefs_.size(); }
	ClusterRef operator[](size_t i) { return  clusterRefs_[i]; }
	const_iterator begin() const { return clusterRefs_.begin(); }
	const_iterator end() const { return clusterRefs_.end(); }

	void write(std::ostream & os) const;
	void writeXml(Core::XmlWriter & xml) const;
	void writeToFile() const;
    };

} // namespace Cart

inline std::ostream & operator<<(std::ostream & out, const Cart::Cluster & c) {
    c.write(out);
    return out;
}

inline std::ostream & operator<<(std::ostream & out, const Cart::ClusterList & c) {
    c.write(out);
    return out;
}

#endif // _CART_CLUSTER_HH
