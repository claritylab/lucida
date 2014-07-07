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
#ifndef _SIGNAL_FRAME_PREDICTION_HH
#define _SIGNAL_FRAME_PREDICTION_HH

#include <Flow/Synchronization.hh>

namespace Signal {


    /** If true, prediction is only made for target times not found in the input stream.
     *  If false, prediction is made for each target time.
     */
    extern Core::ParameterBool paramPredictOnlyMissing;

    /** If false (default), the output stream has the same number of frames as the target streams,
     *  but only start times are synchronized.
     *  If true, start- and end-times are synchronized, so the frames of target and output-stream have the same length
     */
    extern Core::ParameterBool paramSyncEndTimes;
    /** FramePredictionNode: creates one elements for each elements read from "target" stream
     *  by prediction for target start-times
     *
     *  For more details @see Flow::SynchronizationNode.
     */
    template<class Algorithm>
    class FramePredictionNode : public Flow::SynchronizationNode<Algorithm> {
    private:

    typedef Flow::SynchronizationNode<Algorithm> Precursor;

    public:

    FramePredictionNode(const Core::Configuration &c);

    virtual ~FramePredictionNode() {}


    virtual bool setParameter(const std::string &name, const std::string &value);
    };


    template<class Algorithm>
    FramePredictionNode<Algorithm>::FramePredictionNode(const Core::Configuration &c) :
    Core::Component(c), Precursor(c)
    {
    this->setPredictOnlyMissing(paramPredictOnlyMissing(c));
    this->setSyncEndTimes(paramSyncEndTimes(c));
    }


    template<class Algorithm>
    bool FramePredictionNode<Algorithm>::setParameter(const std::string &name,
			      const std::string &value) {

    if (paramPredictOnlyMissing.match(name))
	this->setPredictOnlyMissing(paramPredictOnlyMissing(value));
    else if (paramSyncEndTimes.match(name))
	this->setSyncEndTimes(paramSyncEndTimes(value));
    else
	return Precursor::setParameter(name, value);
    return true;
    }

} // namespace Signal

#endif // _SIGNAL_FRAME_PREDICTION_HH
