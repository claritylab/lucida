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
#ifndef _AUDIO_RAW_HH
#define _AUDIO_RAW_HH

#include <Core/BinaryStream.hh>
#include "Node.hh"

namespace Audio {

    /**
     * Abstract node for audio sources without header information.
     * RawSourceNode provides parameters for setting sample rate and
     * format.
     */
    class RawSourceNode :
	public SourceNode
    {
	typedef SourceNode Predecessor;
    public:
	static const Core::ParameterFloat paramSampleRate;
	static const Core::ParameterInt paramSampleSize;
	static const Core::ParameterInt paramTracks;

	RawSourceNode(const Core::Configuration &c);
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

    /** Flow node for reading raw audio files */
    class RawFileInputNode :
	public RawSourceNode
    {
	typedef RawSourceNode Predecessor;
    protected:
	Core::BinaryInputStream bis_;
	u32 offset_;

	virtual bool isFileOpen() const { return bis_.isOpen(); }
	virtual bool openFile_();
	virtual void closeFile_();
	virtual bool seek(SampleCount newSamplePos);
	template <typename T> u32 readTyped(u32 nSamples, Flow::Timestamp* &d);
	virtual u32 read(u32 nSamples, Flow::Timestamp* &d);
	virtual void readHeader();
	virtual std::streampos position(SampleCount newSamplePos) const;
    public:
	static const Core::ParameterInt paramOffset;
	static std::string filterName() { return "audio-input-file-raw"; }
	RawFileInputNode(const Core::Configuration &c);
	virtual bool setParameter(const std::string &name, const std::string &value);
    };

} // namespace Audio

#endif // _AUDIO_RAW_HH
