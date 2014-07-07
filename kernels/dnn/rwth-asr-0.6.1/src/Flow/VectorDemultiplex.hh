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
#ifndef _FLOW_VECTOR_DEMULTIPLEX_HH
#define _FLOW_VECTOR_DEMULTIPLEX_HH

#include <vector>

#include <Core/Types.hh>
#include <Core/Parameter.hh>

#include "Data.hh"
#include "Link.hh"
#include "Node.hh"

namespace Flow {

    const Core::ParameterInt parameterTrack(
	"track", "track number 0, 1, 2, ...", 0, 0);

    template<class T>
    class VectorDemultiplexNode : public SleeveNode {
    private:
	u32 track_;
	u32 nTracks_;

	u32 offset_;

	void setTrack(u32 track) { offset_ = track_ = track; }
	void setTrackCount(u32 nTracks) { nTracks_ = nTracks; }

    public:
	static std::string filterName() {
	    return std::string("generic-vector-") + Core::Type<T>::name + "-demultiplex";
	}

	VectorDemultiplexNode(const Core::Configuration &c) :
	    Core::Component(c), SleeveNode(c), nTracks_(1)
	{
	    setTrack(parameterTrack(c));
	}
	virtual ~VectorDemultiplexNode() {}

	virtual bool configure() {
	    Core::Ref<Attributes> a(new Attributes());
	    getInputAttributes(0, *a);
	    if (!configureDatatype(a, Vector<T>::type()))
		return false;
	    setTrackCount(atoi(a->get("track-count").c_str()));
	    a->set("track-count", 1);
	    return putOutputAttributes(0, a);
	}

	virtual bool setParameter(const std::string &name, const std::string &value) {
	    if (parameterTrack.match(name))
		setTrack(parameterTrack(value));
	    else
		return false;
	    return true;
	}

	virtual bool work(Flow::PortId p) {
	    Flow::DataPtr<Flow::Vector<T> > in;

	    if (!getData(0, in)) {
		if (in == Data::eos())
		    offset_ = track_;
		return putData(0, in.get());
	    }

	    Flow::Vector<T> *out = new Flow::Vector<T>;
	    out->resize(in->size() / nTracks_ + ((in->size() % nTracks_) > offset_ ? 1 : 0));

	    u32 n, i = 0;
	    for(n = offset_; n < in->size(); n += nTracks_)
		(*out)[i ++] = (*in)[n];

	    offset_ = n - in->size();
	    out->setTimestamp(*in);
	    return putData(0, out);
	}
    };

}

#endif // _FLOW_VECTOR_DEMULTIPLEX_HH
