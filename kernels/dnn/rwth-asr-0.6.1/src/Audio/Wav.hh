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
#ifndef _AUDIO_WAV_HH
#define _AUDIO_WAV_HH

#include "Node.hh"

namespace Audio {

    /** Flow node for reading Microsoft .WAV RIFF audio files */
    class WavInputNode :
	public SourceNode
    {
    private:
	typedef void Handle;
	Handle *sf_;

	virtual bool isFileOpen() const { return sf_ != 0; }
	virtual bool openFile_();
	virtual void closeFile_();
	virtual bool seek(SampleCount newSamplePos);
	template <typename T> u32 readTyped(u32 nSamples, Flow::Timestamp* &d);
	virtual u32 read(u32 nSamples, Flow::Timestamp* &d);
    public:
	static std::string filterName() { return "audio-input-file-wav"; }
	WavInputNode(const Core::Configuration &c);
	virtual ~WavInputNode() { if (isFileOpen()) closeFile_(); }
    };

    /** Flow node for writing Microsoft .WAV RIFF audio files */
    class WavOutputNode :
	public SinkNode
    {
    private:
	typedef void Handle;
	Handle *sf_;

	virtual bool isFileOpen() const { return sf_ != 0; }
	virtual bool openFile_();
	virtual void closeFile_();

	template <typename T> bool writeTyped(const Flow::Data*);
	virtual bool write(const Flow::Data*);
    public:
	static std::string filterName() { return "audio-output-file-wav"; }
	WavOutputNode(const Core::Configuration &c);
    virtual ~WavOutputNode() { if (isFileOpen()) closeFile_(); }
    };

} // namespace Audio

#endif // _AUDIO_WAV_HH
