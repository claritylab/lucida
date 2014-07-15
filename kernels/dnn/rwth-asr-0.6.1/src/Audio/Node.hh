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
#ifndef _AUDIO_NODE_HH
#define _AUDIO_NODE_HH

#include <Core/Parameter.hh>
#include <Core/Types.hh>
#include <Flow/Node.hh>
#include <Flow/Timestamp.hh>

namespace Audio {

    /** Abstract Flow node for audio input/output. */
    class Node :
	public Flow::Node
    {
	typedef Flow::Node Predecessor;
    public:
	typedef u32 SampleCount;
	static const SampleCount invalidSampleCount;
    protected:
	std::string filename_;
	Flow::Time sampleRate_; // [Hz]
	u8 sampleSize_;  // [Bits]
	u8 trackCount_;
	SampleCount sampleCount_; // samples processed so far

	virtual bool openFile_() = 0;
	virtual void closeFile_() = 0;
    public:
	virtual void setSampleRate(Flow::Time _sampleRate) {
	    sampleRate_ = _sampleRate;
	}
	virtual void setSampleSize(u8 _sampleSize) {
	    sampleSize_ = _sampleSize;
	}
	virtual void setTrackCount(u8 _trackCount) {
	    trackCount_ = _trackCount;
	}

	virtual bool isFileOpen() const;
	bool openFile() {
	    require(!isFileOpen());
	    bool result = openFile_();
	    sampleCount_ = 0;
	    ensure(result == isFileOpen());
	    return result;
	}
	void closeFile() {
	    require(isFileOpen());
	    closeFile_();
	    ensure(!isFileOpen());
	}

	virtual void setFilename(const std::string &_filename) {
	    if (isFileOpen()) closeFile();
	    filename_ = _filename;
	}

	static const Core::ParameterString paramFilename;

	Node(const Core::Configuration &c);
	virtual ~Node() { if (isFileOpen()) closeFile(); }
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** Abstract Flow node for audio input. */
    class SourceNode :
	public virtual Node
    {
	typedef Node Predecessor;
    private:
	Flow::Time startTime_, endTime_; // [s]
	u32 blockSize_;
	SampleCount totalSampleCount_;
    protected:
	/** Set total number of sample in audio file.
	 * Call this function from openFile(), if you know the total
	 * length of the audio file.
	 * Note: One samples always means one sample per track! */
	void setTotalSampleCount(SampleCount);

	/** Open audio file and read sample format.
	 * Implement this in concrete input nodes.
	 * @return true on success.  On failure call error() and
	 * return false. */
	virtual bool openFile_() = 0;

	/** Set file position.
	 * Implement this in concrete file input nodes, iff seeking is
	 * possible.  Default implementation disallows seeking.
	 * @param newSamplePos number of next sample to be read,
	 * counted from beginning of stream
	 * @return true on success.  On failure call error() and
	 * return false. */
	virtual bool seek(SampleCount newSamplePos);

	/** Read at most @c nSamples samples from file.
	 * Implement this in concrete input nodes.
	 * @param nSamples number of samples to read
	 * @param d store pointer of new data packet here. If EOF or
	 * an error occurs d must be set to zero.
	 * @return number of samples read, may be less than @c nSamples.
	 * On failure call error() and return 0.
	 * Note: One samples always means one sample per track! */
	virtual u32 read(u32 nSamples, Flow::Timestamp* &d) = 0;

	/** @return is sample position of startTime_. */
	SampleCount getStartSample() const;
	/** @return is sample position of endTime_. */
	SampleCount getEndSample() const;
	void setBlockSize(u32 blockSize) { blockSize_ = blockSize; }
	u32 blockSize() const { return blockSize_; }
    public:
	static const Core::ParameterInt paramBlockSize;
	static const Core::ParameterFloat paramStartTime;
	static const Core::ParameterFloat paramEndTime;

	bool setStartTime(Flow::Time);
	bool setEndTime(Flow::Time);

	SourceNode(const Core::Configuration &c);
	virtual bool setParameter(const std::string &name, const std::string &value);

	virtual Flow::PortId getOutput(const std::string &name) { return 0; }

	/** Post sample format to output port. If necessary open file. */
	virtual bool configure();
	virtual bool work(Flow::PortId out);
    };

    /** Abstract Flow node for audio output.
	Forwards all input on its first output port. */
    class SinkNode :
	public virtual Node
    {
	typedef Node Predecessor;
    protected:
	/** Create audio file and set sample format */
	virtual bool openFile_() = 0;

	virtual bool write(const Flow::Data*) = 0;
    public:
	SinkNode(const Core::Configuration &c);

	virtual Flow::PortId getInput(const std::string &name) { return 0; }
	virtual Flow::PortId getOutput(const std::string &name) { return 0; }

	/** Set sample format from attributes. */
	virtual bool configure();
	virtual bool work(Flow::PortId out);
    };

} // namespace Audio


#endif // _AUDIO_NODE_HH
