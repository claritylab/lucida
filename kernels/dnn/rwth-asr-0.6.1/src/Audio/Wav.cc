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
#include <Modules.hh>
#include "Wav.hh"
#include <math.h>
#include <cstdio>
#include <Flow/Vector.hh>
#include <Core/Directory.hh>

#if 1
#ifdef MODULE_AUDIO_WAV_SYSTEM
#include <sndfile.h>
#endif
#endif

namespace
{
#ifdef SNDFILE_1
    // version 1.x of libsndfile

    typedef SNDFILE SndFileHandle;
    inline u32 sfGetSampleSize(const SF_INFO &info)
    {
	int subformat = info.format & SF_FORMAT_SUBMASK;
	switch(subformat) {
	case SF_FORMAT_PCM_S8:
	case SF_FORMAT_PCM_U8: return 8; break;
	case SF_FORMAT_PCM_16: return 16; break;
	case SF_FORMAT_PCM_24: return 24; break;
	case SF_FORMAT_PCM_32: return 32; break;
	default: defect(); break;
	}
    }
    inline void sfSetSampleSize(SF_INFO &info, u32 s)
    {
	switch(s) {
	case 8:  info.format |= SF_FORMAT_PCM_U8; break;
	case 16: info.format |= SF_FORMAT_PCM_16; break;
	case 24: info.format |= SF_FORMAT_PCM_24; break;
	case 32: info.format |= SF_FORMAT_PCM_32; break;
	default: defect(); break;
	}
    }
    sf_count_t& sfSamples(SF_INFO &info)
    { return info.frames; }
    inline SndFileHandle* sf_open_read(const char *f, SF_INFO *i)
    { return sf_open(f, SFM_READ, i); }
    inline SndFileHandle* sf_open_write(const char *f, SF_INFO *i)
    { return sf_open(f, SFM_READ, i); }
    inline void sfSetWavFormat(SF_INFO &info)
    { info.format = SF_FORMAT_WAV; }
#else
    // old version of libsndfile

    typedef void SndFileHandle;
    inline u32 sfGetSampleSize(const SF_INFO &info)
    { return info.pcmbitwidth; }
    inline void sfSetSampleSize(SF_INFO &info, u32 s)
    { info.pcmbitwidth = s; }
    u32& sfSamples(SF_INFO &info)
    { return info.samples; }
    inline void sfSetWavFormat(SF_INFO &info)
    { info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM); }
#endif
}

using namespace Audio;

// ===========================================================================
WavInputNode::WavInputNode(const Core::Configuration &c) :
    Core::Component(c), Node(c), SourceNode(c),
    sf_(0)
{}

bool WavInputNode::openFile_() {
    SF_INFO info;
    info.format = 0;
    sf_ = sf_open_read(filename_.c_str(), &info);
    if (sf_ == 0) {
	criticalError("could not open wav file '%s' for reading", filename_.c_str());
	return false;
    }
    setSampleRate(info.samplerate);
    setSampleSize(sfGetSampleSize(info));
    setTrackCount(info.channels);
    setTotalSampleCount(sfSamples(info));
    return true;
}

void WavInputNode::closeFile_() {
    sf_close(static_cast<SndFileHandle*>(sf_));
    sf_ = 0;
}

bool WavInputNode::seek(SampleCount newSamplePos) {
    require(isFileOpen());
    s32 pos = sf_seek(static_cast<SndFileHandle*>(sf_), newSamplePos, SEEK_SET);
    if (pos < 0) {
	error("sf_seek failed");
	return false;
    }
    sampleCount_ = newSamplePos;
    return true;
}

template <typename T>
u32 WavInputNode::readTyped(u32 nSamples, Flow::Timestamp* &d) {
    require(sf_);
    Flow::Vector<T> *v = new Flow::Vector<T>(trackCount_ * nSamples);
    size_t bytesToRead = nSamples * trackCount_ * sizeof(T);
    int bytesRead = sf_read_raw(static_cast<SndFileHandle*>(sf_), &*(v->begin()), bytesToRead);

    if (bytesRead <= 0) {
	if (bytesRead < 0) error("sf_read failed");
	delete v; d = 0; return 0;
    }
    v->resize(bytesRead / sizeof(T));
    d = v;
    return v->size() / trackCount_;
}

u32 WavInputNode::read(u32 nSamples, Flow::Timestamp* &d) {
    require(isFileOpen());
    require(d == 0);
    switch (sampleSize_) {
    case 8:
	return readTyped<s8>(nSamples, d);
    case 16:
	return readTyped<s16>(nSamples, d);
    default:
	error("unsupported sample size: %d bit", sampleSize_);
	return false;
    }
}

// ===========================================================================
WavOutputNode::WavOutputNode(const Core::Configuration &c) :
    Core::Component(c), Node(c), SinkNode(c),
    sf_(0)
{}

bool WavOutputNode::openFile_() {
    SF_INFO info;
    info.samplerate  = int(rint(sampleRate_));
    sfSamples(info) = 0;
    info.channels    = trackCount_;
    sfSetWavFormat(info);
    sfSetSampleSize(info, sampleSize_);
    info.sections    = 1;
    info.seekable    = 1;
    verify(sf_format_check(&info));

    if(!Core::createDirectory(Core::directoryName(filename_)))
	error() << "could not create directory for writing wav file '" << filename_ << "'";

    sf_ = sf_open_write(filename_.c_str(), &info);
    if (sf_ == 0) {
	error("could not open wav file '%s' for writing", filename_.c_str());
	return false;
    }
    return true;
}

void WavOutputNode::closeFile_() {
    sf_close(static_cast<SndFileHandle*>(sf_));
    sf_ = 0;
}

template <typename T>
bool WavOutputNode::writeTyped(const Flow::Data *_in) {
    const Flow::Vector<T> *in(static_cast<const Flow::Vector<T>*>(_in));
    size_t bytesToWrite = sizeof(T) * in->size();
    size_t bytesWritten = sf_write_raw(static_cast<SndFileHandle*>(sf_), (void*) &*(in->begin()), bytesToWrite);
    return (bytesWritten == bytesToWrite);
}


bool WavOutputNode::write(const Flow::Data *in) {
    /*! \todo Should look at datatype not at sample size. */
    switch (sampleSize_) {
    case 8:
	return writeTyped<s8>(in);
    case 16:
	return writeTyped<s16>(in);
    default:
	error("unsupported sample size: %d bit", sampleSize_);
	return false;
    }
}
