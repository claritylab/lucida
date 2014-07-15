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
#ifndef _SIGNAL_SLIDING_ALGORITHM_NODE_HH
#define _SIGNAL_SLIDING_ALGORITHM_NODE_HH

#include <Core/Parameter.hh>
#include <Flow/Node.hh>

namespace Signal {


    /** Base class for driving algorithms with buffer of arbitrary length.
     *
     *  @see work method for more details.
     */
    template<class Algorithm>
    class SlidingAlgorithmNode : public Flow::SleeveNode, public Algorithm {
    public:
	typedef typename Algorithm::InputData InputData;
	typedef typename Algorithm::OutputData OutputData;
    private:
	/** flushes the algorithm and puts in @param in
	 *  @return is true if at least one output was sent successfully.
	 */
	bool flushAllAndPut(Flow::PortId p, const InputData &in);
    public:
	SlidingAlgorithmNode(const Core::Configuration &c) : Component(c), SleeveNode(c) {}
	virtual ~SlidingAlgorithmNode() {}

	/** Feeds new data into the Algorithm and reads data out of the Algorithm if existant.
	 *  Steps:
	 *    1) Read data out of Algorithm and if successful send it on in the network, and exit.
	 *    2) If Algorithm cannot deliver an output, read new input from the network and
	 *       feed it into the algorithm and try step 1) again.
	 *
	 *   Error handling:
	 *     1) If EOS arrives from the network, flush the next output of the algorithm.
	 *     2) If Algorithm cannot accept new input, flush all output of the algorithm,
	 *        put them into the network and put the new input into the algorithm again.
	 */
	virtual bool work(Flow::PortId p);
    };

    template<class Algorithm>
    bool SlidingAlgorithmNode<Algorithm>::work(Flow::PortId p)
    {
	Flow::DataPtr<InputData> in;
	Flow::DataPtr<OutputData> out(new OutputData);

	while (!Algorithm::get(*out)) {
	    if (!getData(0, in)) {
		if (in == Flow::Data::eos()) {
		    if (Algorithm::flush(*out))
			return putData(0, out.get());
		}
		return putData(0, in.get());
	    }
	    if (!Algorithm::put(*in) && flushAllAndPut(p, *in))
		return true;
	}
	return putData(0, out.get());
    }


    template<class Algorithm>
    bool SlidingAlgorithmNode<Algorithm>::flushAllAndPut(Flow::PortId p, const InputData &in)
    {
	bool result = true;
	Flow::DataPtr<OutputData> out;

	do {
	    out = Flow::dataPtr(new OutputData());
	    if (!Algorithm::flush(*out))
		break;
	    if (!putData(0, out.get()))
		result = false;
	} while(true);

	if (!Algorithm::put(in))
	    defect();
	return result;
    }

} // namespace Signal

#endif // _SIGNAL_SLIDING_ALGORITHM_NODE_HH
