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
// $Id: Merger.hh 7257 2009-07-10 12:17:00Z rybach $

#ifndef _FLOW_MERGER_HH
#define _FLOW_MERGER_HH

#include <map>
#include "Collector.hh"
#include "Timestamp.hh"
#include "Vector.hh"

namespace Flow {

    /**
     * Abstract base class for nodes merging several packets into one packet,
     * e.g. VectorConcat, VectorSum
     */

    template <class I, class O>
    class MergerNode :
	public CollectorNode< I, O >
    {
	typedef CollectorNode< I, O > Precursor;
    public:
	typedef typename Precursor::InputData InputData;
	typedef typename Precursor::OutputData OutputData;
	typedef typename Precursor::InputFrame InputFrame;
    public:
	MergerNode(const Core::Configuration &c);
	virtual ~MergerNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();

	/** Merges input frames into one OutputData object.
	 *  Override this function to implement specific merging algorithms.
	 *  Note: Input Frames are passed as reference and not as const reference.
	 *    In this way, InputFrames objects can be changed if necessary for a more
	 *    efficient merging.
	 */
	virtual OutputData *merge(std::vector<InputFrame> &) = 0;

	virtual bool work(Flow::PortId p);
    };

    template <class I, class O>
    MergerNode<I, O>::MergerNode(const Core::Configuration &c) :
	Core::Component(c),
	Precursor(c)
    {
	//init additional params here
    }

    template <class I, class O>
    bool MergerNode<I, O>::setParameter(const std::string &name, const std::string &value)
    {
	return Precursor::setParameter(name, value);
    }

    template <class I, class O>
    bool MergerNode<I, O>::configure()
    {
	if(!Precursor::configure())
	    return false;
	return true;
    }

    template <class I, class O>
    bool MergerNode<I, O>::work(Flow::PortId p)
    {
	if (Precursor::needInit_)
	    Precursor::init();

	Precursor::inputData_.resize(this->nInputs());

	if (this->nInputs() == 0) {
	    this->error("No input connected.");
	    return Precursor::putNullData();
	}

	Time startTime, endTime;
	if (!Precursor::getData(startTime, endTime))
	    return Precursor::putNullData();

	OutputData *out = merge(Precursor::inputData_);
	ensure(out != 0);

	out->setStartTime(startTime);
	out->setEndTime(endTime);
	return this->putData(0, out);
    }

} // namespace Flow

#endif //_FLOW_MERGER_HH
