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
#ifndef _SIGNAL_DC_DETECTION_HH
#define _SIGNAL_DC_DETECTION_HH

#include <Core/Parameter.hh>
#include <Flow/Vector.hh>
#include "SlidingAlgorithmNode.hh"

namespace Signal {

    // DcDetection
    //////////////

    class DcDetection {
    public:

	typedef f32 Sample;
	typedef Flow::Time Time;

	typedef Flow::Vector<Sample> InputData;
	typedef Flow::Vector<Sample> OutputData;

    private:
	Time sampleRate_;

	Sample maxDcIncrement_;

	/** Minimum length of a DC segment
	 */
	u32 minDcLength_;
	Time minDcLengthInS_;


	/** Size of output segments <= maximalOutputSize_
	 */
	u32 maximalOutputSize_;

	u32 nonDcLength_;
	u32 dcLength_;

	u32 nOutputs_;

	/** length of the last continuous non DC segment */
	u32 nonDcSegmentLength_;
	/** non DC segments shorter than this value are discarded */
	u32 minNonDcSegmentLength_;
	/** non DC segments shorter than this value (given in seconds) are discarded */
	Time minNonDcSegmentLengthInS_;

	u32 totalRejected_, totalAccepted_;

	std::deque<Sample> buffer_;

	Time bufferStartTime_;

	bool needInit_;

    private:

	void init();

	/** @return is true if the absolute difference between the sample v
	 *  and the end of the non DC segment is larger then maxDcIncrement_
	 */
	bool isNonDC(const Sample &v) { return fabs(v - buffer_[nonDcLength_ - 1]) >= maxDcIncrement_; }

	/** @return is true if the number of DC samples at the end of the buffer
	 *  are larger or equal to minDCLength_
	 */
	bool isDcDetected() { return dcLength_ >= minDcLength_; }

	/** updates the dcLength_ and nonDcLength_ up to the next possible decision
	 *  or up the end of the buffer
	 *
	 * @return is true if
	 *   - DC decision could be made
	 *   - buffer consist only of non DC samples and it is larger
	 *     then input block size (maximalOutputSize_)
	 */
	bool nextBlock();

	/** updates the dcLength_ and nonDcLength_ up to the end of the buffer
	 *
	 * @return is false if buffer was empty
	 * Remark: assertion fails if a call to nextBlock could succeed.
	 */
	bool lastBlock();

	/** removes the first nonDcLength_ + dcLength_ samples from the buffer and
	 *  copies first nonDcLength_ samples into out.
	 */
	bool flushBlock(Flow::Vector<Sample> &out);

	/** Copies first nonDcLength_ number of samples into out and sets the start- and end-time
	 *  @return is false if the non DC segment is smaller than minNonDcSegmentLength_.
	*/
	bool copyBlock(Flow::Vector<Sample> &out);

	/** Removes the first nonDcLength_ + dcLength_ samples from the buffer.
	 */
	void eraseBlock();

    public:

	DcDetection();

	virtual ~DcDetection() { /*std::cout << "Samples rejected by dc-detection: " << totalRejected_ << " accepted: " << totalAccepted_ << std::endl;*/ }


	void setMaxDcIncrement(Sample maxDcIncrement);

	void setMinDcLengthInS(Time minDcLengthInS);

	void setMinNonDcSegmentLengthInS(Time minNonDcSegmentLengthInS);

	void setMaximalOutputSize(u32 maximalOutputSize);

	void setSampleRate(Time sampleRate);


	/** @return is false if the is a time gap between start time of in
	 *  and end time of the buffer.
	 */
	bool put(const Flow::Vector<Sample> &in);

	/** delivers a block of samples without DC segments
	 *  @return is false if DC/non DC decision could not be made yet
	 *
	 *  remark: long non DC segments are delivered in blocks of length maximalOutputSize_
	 */
	bool get(Flow::Vector<Sample> &out);

	/** delivers a last block of samples without DC segments
	 *  @return is false if buffer is empty
	 */
	bool flush(Flow::Vector<Sample> &out);

	void reset();
    };


    // DcDetectionNode
    ///////////////////////

    class DcDetectionNode : public SlidingAlgorithmNode<DcDetection> {
    public:

	typedef SlidingAlgorithmNode<DcDetection> Predecessor;

    private:

	static Core::ParameterFloat paramMinDcLength;
	static Core::ParameterFloat paramMaxDcIncrement;
	static Core::ParameterFloat paramMinNonDcSegmentLength;
	static Core::ParameterInt paramMaximalOutputSize;

    public:
	static std::string filterName() { return "signal-dc-detection"; }

	DcDetectionNode(const Core::Configuration &c);
	virtual ~DcDetectionNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);

	virtual bool configure();
    };
}


#endif // _SIGNAL_DC_DETECTION_HH
