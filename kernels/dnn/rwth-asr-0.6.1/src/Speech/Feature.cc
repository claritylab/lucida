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
#include "Feature.hh"

using namespace Speech;


Core::Ref<const Mm::Feature::Vector> Feature::convert(Flow::DataPtr<FlowVector> &v)
{
    v.makePrivate();
    Vector *r = new Mm::Feature::Vector;
    std::swap(*r, *v.get());
    delete v.release();
    return Core::ref(r);
}

void Feature::take(Flow::DataPtr<FlowVector> &v)
{
    require_(v);

    clear();
    setTimestamp(*v);
    add(convert(v));
}


void Feature::take(Flow::DataPtr<FlowFeature> &f)
{
    require_(f);

    /*
     * Note: FlowFeature object might reference FlowVectors alone (strong indication
     * for an optimized covertion of them) but the FlowFeature can be referenced more times.
     * Thus, optimized conversion is only possible if FlowFeature is referenced only once.
     */
    f.makePrivate();

    clear();
    setTimestamp(*f);
    for(size_t i = 0; i < f->size(); ++i)
	add(i, convert((*f)[i]));
}


bool Feature::take(Flow::DataPtr<Flow::Timestamp> &t)
{
    if (t) {
	Flow::DataPtr<FlowVector> f(t);
	if (f) {
	    t.reset();
	    take(f);
	    return true;
	} else {
	    Flow::DataPtr<FlowFeature> f(t);
	    if (f) {
		t.reset();
		take(f);
		return true;
	    }
	}
    }
    return false;
}


Mm::FeatureDescription* Feature::getDescription(const Core::Configurable &parent) const
{
    return new Mm::FeatureDescription(parent, *this);
}
