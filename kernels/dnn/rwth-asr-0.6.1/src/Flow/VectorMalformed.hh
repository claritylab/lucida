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
#ifndef _FLOW_VECTORMALFORMED_HH
#define _FLOW_VECTORMALFORMED_HH

#include <Core/Utility.hh>
#include "Node.hh"
#include "Vector.hh"

namespace Flow {

    // VectorMalformed
    //////////////////

    template<class T, class Policy> class VectorMalformed {
    private:
	Policy policy_;

    public:
	VectorMalformed() {}
	virtual ~VectorMalformed() {}

	void setPolicy(Policy policy) { policy_ = policy; }
	bool apply(std::vector<T> &v) { return policy_.work(Core::isMalformed(v.begin(), v.end()), v); }
    };

    // Policies
    ///////////

    template<class T> class CopyMalformedPolicy {
    private:
	std::vector<T> old_;
    public:
	bool work(bool malformed, std::vector<T>& v) {
	    if (malformed) {
		if (old_.size() != v.size()) return false;
		v = old_;
	    } else old_ = v;
	    return true; }
	static std::string name() { return "copy"; }
    };

    template<class T> class DismissMalformedPolicy {
    public:
	bool work(bool malformed, std::vector<T>& v) { return !malformed; }
	static std::string name() { return "dismiss"; }
    };

    template<class T> class FloorMalformedPolicy {
    public:
	bool work(bool malformed, std::vector<T>& v) {
	    for (u32 i = 0; i < v.size(); i++) {
		if (Core::isMalformed(v[i]))
		    v[i] = v[i] > 0 ? Core::Type<T>::max : Core::Type<T>::min;
	    }
	    return true;
	}

	static std::string name() { return "floor"; }
    };

    template<class T> class KeepMalformedPolicy {
    public:
	bool work(bool malformed, std::vector<T>& v) { return true; }
	static std::string name() { return "keep"; }
    };


    // VectorMalformedNode
    //////////////////////

    template<class T, class Policy>
    class VectorMalformedNode : public SleeveNode, VectorMalformed<T, Policy> {
    public:
	static std::string filterName() {
	    return std::string("generic-vector-") +  Core::Type<T>::name + "-" + Policy::name() + "-malformed";
	}
	VectorMalformedNode(const Core::Configuration &c) : Core::Component(c), SleeveNode(c) {}
	virtual ~VectorMalformedNode() {}

	virtual bool configure() {
	    Core::Ref<const Attributes> a = getInputAttributes(0);
	    if (!configureDatatype(a, Vector<T>::type()))
		return false;
	    return putOutputAttributes(0, a);
	}
	virtual bool work(PortId p) {
	    DataPtr<Vector<T> > in;
	    do {
		if (!getData(0, in)) return putData(0, in.get());
		in.makePrivate();
	    } while (!VectorMalformed<T, Policy>::apply(*in));
	    return putData(0, in.get());
	}
    };

}


#endif // _FLOW_VECTORMALFORMED_HH
