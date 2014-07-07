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
#ifndef _SIGNAL_REPEATING_FRAME_PREDICTION_HH
#define _SIGNAL_REPEATING_FRAME_PREDICTION_HH

#include <Flow/Data.hh>
#include <Flow/Synchronization.hh>
#include "SlidingWindow.hh"

namespace Signal {

    /** RepeatingFramePrediction: predicts input at given target start-times by
     *  copying the previous element.
     */
    class RepeatingFramePrediction {
    public:
    typedef Flow::Time Time;
    typedef Flow::Timestamp Data;
    typedef Flow::DataPtr<Data> DataPointer;
    private:
    /** Contains the two latest element of the input stream. */
    SlidingWindow<DataPointer> slidingWindow_;

    /** If true, prediction is only made for target times not found in the input stream.
     *  If false, prediction is made for each target time.
     */
    bool predictOnlyMissing_;

    /** If false (default), the output stream has the same number of frames as the target streams,
    *   but only start times are synchronized.
    *   If true, start- and end-times are synchronized, so the frames of target and output-stream have the same length
    */
    bool syncEndTimes_;

    private:
    /** Seeks in the input stream until @param time is found.
     *
     *  At the beginning of the input stream, first element  is always added to the slidingWindow_.
     *  After end-of-stream the last elements in the slidingWindow_ are kept until reset() is called.
     *  After seeking start-time of the front element in the slidingWindow_
     *    is greater or equal to @param time and back one is smaller then @param time
     *    (except for the very first element, which can be larger or equal to @param time).
     */
    void seek(Time time);

    /** Copies the front element in the slidingWindow_ to @param out if
     *    -if predictOnlyMissing_ is false and
     *    start-time of front element is equal to @param time.
     *
     *  @return is false if the slidingWindow_ is empty.
     */
    bool copyLatest(Time time, DataPointer &out, Time endTime);

    /** Factual predction: element at @param time is predicted by
     *    the previous element of the input stream.
     *
     *  Caution: start and end-time of the predicted element is set to @param time
     *
     *  @return is false if the slidingWindow_ is empty.
     */
    bool copyPrevious(Time time, DataPointer &out, Time endTime);
    protected:
    /** override nextData to supply input data on demand */
    virtual bool nextData(DataPointer &dataPointer) = 0;
    public:

    static std::string name() { return "signal-repeating-frame-prediction"; }
    RepeatingFramePrediction();
    virtual ~RepeatingFramePrediction() {}

    /** @return is the vector created by prediction at @param time
     *
     *  If @param time is found in the input stream and predictOnlyMissing_ is false:
     *    start-time and end-time are delivered un-changed
     *  Else:
     *     start-time and end-time of a predicted output are both set to @param time.
     *
     *  If @return false call lastError() to get a explanation.
     */
    bool work(const Flow::Timestamp &time, DataPointer &out);

    /** @see predictOnlyMissing_ */
    void setPredictOnlyMissing(bool predictOnlyMissing) { predictOnlyMissing_ = predictOnlyMissing; }

    void setSyncEndTimes(bool snycEndTimes) { syncEndTimes_ = snycEndTimes; }

    std::string lastError() const { defect(); }

    void reset() { slidingWindow_.clear(); }
    };

} // namespace Signal

#endif // _SIGNAL_REPEATING_FRAME_PREDICTION_HH
