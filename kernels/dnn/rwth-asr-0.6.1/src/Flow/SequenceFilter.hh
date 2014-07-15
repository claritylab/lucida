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
#ifndef _FLOW_SEQUENCE_FILTER_HH
#define _FLOW_SEQUENCE_FILTER_HH

#include "Node.hh"
#include "Vector.hh"

namespace Flow {

    /**
     *  Filters input sequence according to a selection object.
     *  Input:
     *    default port: filtered stream
     *    selection port: vector-bool, i.e. sequence of bool values.
     *  Output:
     *    default port: selected features
     *
     *  - A new selection is read, if the timestamp of the new data is not contained in the time interval
     *    of the current selection.
     *  - Data is filtered out, if the selection object contains 'false' at the position of the data.
     *    This implies as well, that selection has to have at least as many elements as data will come
     *    in its time interval.
     */
    class SequenceFilterNode : public Flow::Node {
    private:
	u32 featureIndex_;
	Flow::DataPtr<Flow::Vector<bool> > selection_;
    private:
	void updateSelection(const Flow::Timestamp &);
    public:
	SequenceFilterNode(const Core::Configuration &);
	static std::string filterName() { return "generic-sequence-filter"; }

	virtual Flow::PortId getInput(const std::string &name) {
	    return name == "selection" ? 1 : 0; }
	virtual Flow::PortId getOutput(const std::string &name) {
	    return 0; }

	virtual bool configure();
	virtual bool work(Flow::PortId out);
    };

} // namespace Speech

#endif // _FLOW_SEQUENCE_FILTER_HH
