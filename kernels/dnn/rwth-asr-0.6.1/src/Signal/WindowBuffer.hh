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
#ifndef _SIGNAL_WINDOW_BUFFER_HH
#define _SIGNAL_WINDOW_BUFFER_HH

#include <deque>
#include <Flow/Data.hh>
#include <Flow/Vector.hh>


namespace Signal {

    /** WindowBuffer is buffer collecting vectors of samples and
	delivering (overlapping) segments of a given length */

    class WindowBuffer {
    public:
	typedef Flow::Time Time;
	typedef f32 Sample;
	enum FlushPolicy { SendRest, PadRest, DiscardRest };
	typedef Flow::Vector<Sample> InputData;
	typedef Flow::Vector<Sample> OutputData;
    protected:
	/** length_ of output vectors */
	u32 length_;
	/** number of elements removed from the beginning of the buffer after a call to get */
	u32 shift_;

	Time bufferStartTime_;
	Time sampleRate_;
	bool flushBeforeGap_;
	std::deque<Sample> buffer_;

	/** number released of outputs */
	u32 nOutputs_;

	/** true if buffer is flushed */
	bool flushed_;
	/** if true, segments are delivered shift-by-shift until the buffer is empty
	 *  if false, segments are delivered until the last segment contains the last sample
	 */
	bool  flushAll_;

	bool needInit_;
    private:
	/** copies @param length element form the beginning of the buffer to @param out and
	 *  sets the beginning and end time of @param out
	 */
	void copy(Flow::Vector<f32> &out, const u32 length);
    protected:
	/** call to force initialization
	 */
	void setNeedInit() { needInit_ = true; }
	/** overload to perform initialization
	 */
	virtual void init();

	/** overload to transform output vectors
	 *
	 *  caution: update the start- and end-time of out if the size is changed
	 */
	virtual void transform(Flow::Vector<Sample> &out) {}
    public:
	WindowBuffer();
	virtual ~WindowBuffer() {}

	/** adds an input vector
	 * @return is false if there is a time gap
	 *   between the end time of the buffer and start time of in
	 */
	bool put(const Flow::Vector<Sample> &in);
	/** returns one vector of the given length_ and
	 * removes shift_ number of elements from the begining of the buffer
	 * @return is false if buffer is smaller then 2 * max(length_, shift_)
	 */
	bool get(Flow::Vector<Sample> &out);
	/** delivers the rest of the buffer_
	 *  @return is false if the buffer_ is empty
	 *
	 *  Caution: an assertion fails if the buffer is larger then 2 * max(length_, shift_)
	 */
	bool flush(Flow::Vector<Sample> &out);

	/** Clears the buffer and resets status variables. */
	virtual void reset();

	/** set the number of samples removed after each call to get */
	void setShift(u32 shift) { shift_ = shift; }
	/** the number of samples removed after each call to get */
	u32 shift() { return shift_; }

	/** sets the length of output vectors */
	void setLength(u32 length) { length_ = length; }
	/** length of output vectors */
	u32 length() const { return length_; }

	void setSampleRate(Time sampleRate) { sampleRate_ = sampleRate; }
	Time sampleRate() const { return sampleRate_; }

	/** number released of outputs */
	u32 nOutputs() { return nOutputs_; }

	/** @return is true if the last output is following */
	bool flushed() { return flushed_; }

	/** if @param flushAll is true, segments are delivered shift-by-shift
	 *    until the buffer is empty
	 *  if @param flushAll false, segments are delivered
	 *    until the last segment contains the last sample
	 */
	void setFlushAll(bool flushAll) { flushAll_ = flushAll; }
	bool flushAll() const { return flushAll_; }

	void setFlushBeforeGap(bool flushBeforeGap) { flushBeforeGap_ = flushBeforeGap; }
	bool shallFlushBeforeGap() const { return flushBeforeGap_; }
    };
}


#endif // _SIGNAL_WINDOW_BUFFER_HH
