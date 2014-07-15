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
// $Id: BinaryStream.cc 9227 2013-11-29 14:47:07Z golik $

#include "BinaryStream.hh"

using namespace Core;


// ===========================================================================
// class BinaryOutputStream

BinaryStreamIos::BinaryStreamIos(Endianess endianess) :
    ios_(0), fstream_(0)
{
    ios_ = fstream_ = new std::fstream;
    setEndianess(endianess);
}

BinaryStreamIos::BinaryStreamIos(std::ios &i, Endianess endianess) :
    ios_(&i),
    fstream_(dynamic_cast<std::fstream*>(ios_))
{
    setEndianess(endianess);
}

BinaryStreamIos::BinaryStreamIos(
    const std::string &fileName,
    std::ios_base::openmode mode,
    Endianess endianess) :
    ios_(0), fstream_(0)
{
    mode |= std::ios::binary;
    fstream_ = new std::fstream(fileName.c_str(), mode);
    ios_ = fstream_;
    setEndianess(endianess);
}

BinaryStreamIos::~BinaryStreamIos() {
    if (fstream_) delete fstream_;
}

void BinaryStreamIos::open(
    const std::string &name,
    std::ios_base::openmode mode)
{
    mode |= std::ios::binary;
    if (fstream_) fstream_->open(name.c_str(), mode);
}

void BinaryStreamIos::close() {
    if (fstream_) fstream_->close();
}

bool BinaryStreamIos::isOpen() const {
    if (fstream_) return fstream_->is_open();
    return false;
}

void BinaryStreamIos::setEndianess(Endianess endianess) {
#if   __BYTE_ORDER == __LITTLE_ENDIAN
    swap_ = (endianess == bigEndian);
#elif __BYTE_ORDER == __BIG_ENDIAN
    swap_ = (endianess == littleEndian);
#else
#error "Machine has unsupported byte order!"
#endif
}

template <typename T>
bool BinaryOutputStream::write(const T *v, std::streamsize n) {
    if (!swap_) return os_->write((char*)v, sizeof(T) * n);
    T* swapBuffer = new T[n];
    memcpy(swapBuffer, v, sizeof(T) * n);
    swapEndianess<sizeof(T)>(swapBuffer, n);
    bool result = os_->write((char*)swapBuffer, sizeof(T) * n);
    delete[] swapBuffer;
    return result;
}

// namespace Core has to be defined explicitely here, otherwise gcc-4 will produce an ICE
namespace Core {
    template <>
    bool BinaryOutputStream::write(const bool *v, std::streamsize n) {
	u8* tmp = new u8[n];
	for (std::streamsize i = 0; i < n; i++) tmp[i] = (v[i]) ? 0xff : 0;
	bool success = os_->write((char*) tmp, n);
	delete[] tmp;
	return success;
    }
} // end namespace Core

template bool BinaryOutputStream::write<char>(const char*, std::streamsize);
template bool BinaryOutputStream::write<s8>(const s8*, std::streamsize);
template bool BinaryOutputStream::write<u8>(const u8*, std::streamsize);
template bool BinaryOutputStream::write<s16>(const s16*, std::streamsize);
template bool BinaryOutputStream::write<u16>(const u16*, std::streamsize);
template bool BinaryOutputStream::write<s32>(const s32*, std::streamsize);
template bool BinaryOutputStream::write<u32>(const u32*, std::streamsize);
template bool BinaryOutputStream::write<s64>(const s64*, std::streamsize);
template bool BinaryOutputStream::write<u64>(const u64*, std::streamsize);
template bool BinaryOutputStream::write<f32>(const f32*, std::streamsize);
template bool BinaryOutputStream::write<f64>(const f64*, std::streamsize);

BinaryOutputStream &BinaryOutputStream::operator<<(u8 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(s8 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(s16 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(u16 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(s32 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(u32 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(s64 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(u64 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(f32 n) { write(&n); return *this; }
BinaryOutputStream &BinaryOutputStream::operator<<(f64 n) { write(&n); return *this; }

#ifdef OS_darwin
template bool BinaryOutputStream::write<size_t>(const size_t*, std::streamsize);
BinaryOutputStream &BinaryOutputStream::operator<<(size_t n) { write(&n); return *this; }
#endif

BinaryOutputStream &BinaryOutputStream::operator<<(const char *s) {
    u32 size = strlen(s);
    write(&size);
    write(s, size);
    return *this;
}

BinaryOutputStream &BinaryOutputStream::operator<<(const std::string &s) {
    u32 size = u32(s.size());
    write(&size);
    write(s.data(), s.size());
    return *this;
}

BinaryOutputStream &BinaryOutputStream::operator<<(bool b) { write(&b); return *this; }

// ===========================================================================
// class BinaryInputStream

template <typename T>
bool BinaryInputStream::read(T *v, std::streamsize n) {
    if (!is_->read((char*)v, sizeof(T) * n)) return false;
    if (swap_) swapEndianess<sizeof(T)>(v, n);
    return true;
}

template <typename T>
std::streamsize BinaryInputStream::readSome(T *v, std::streamsize n) {
    // WARNING: "readsome() is buggy, that is why we are using a work-around, see http://support.microsoft.com/kb/246934/de"
    read(v, n);
    return is_->gcount() / sizeof(T);
}

namespace Core {
template <>
bool BinaryInputStream::read(bool *v, std::streamsize n) {
    u8 *tmp = new u8[n];
    bool success = is_->read((char*) tmp, n);
    for (std::streamsize i = 0; i < n; i++) v[i] = (tmp[i]) ? true : false;
    delete[] tmp;
    return success;
}
}

template bool BinaryInputStream::read<s8>(s8*, std::streamsize);
template bool BinaryInputStream::read<u8>(u8*, std::streamsize);
template bool BinaryInputStream::read<s16>(s16*, std::streamsize);
template bool BinaryInputStream::read<u16>(u16*, std::streamsize);
template bool BinaryInputStream::read<s32>(s32*, std::streamsize);
template bool BinaryInputStream::read<u32>(u32*, std::streamsize);
template bool BinaryInputStream::read<s64>(s64*, std::streamsize);
template bool BinaryInputStream::read<u64>(u64*, std::streamsize);
template bool BinaryInputStream::read<f32>(f32*, std::streamsize);
template bool BinaryInputStream::read<f64>(f64*, std::streamsize);

template std::streamsize BinaryInputStream::readSome<s8>(s8*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<u8>(u8*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<s16>(s16*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<u16>(u16*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<s32>(s32*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<u32>(u32*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<s64>(s64*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<u64>(u64*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<f32>(f32*, std::streamsize);
template std::streamsize BinaryInputStream::readSome<f64>(f64*, std::streamsize);

// Needed in Fsa/tInput.cc (at least in Release mode)
template bool BinaryInputStream::read<char>(char*, std::streamsize);

BinaryInputStream &BinaryInputStream::operator>>(u8 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(s8 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(s16 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(u16 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(s32 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(u32 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(s64 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(u64 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(f32 &n) { read(&n); return *this; }
BinaryInputStream &BinaryInputStream::operator>>(f64 &n) { read(&n); return *this; }

BinaryInputStream &BinaryInputStream::operator>>(std::string &s) {
    s.erase();
    u32 size;
    if (read(&size)) {
	s.resize(size);
	read(&s[0], size);
    }
    return *this;
}

BinaryInputStream &BinaryInputStream::operator>>(bool &b) { read(&b); return *this; }

BinaryInputStreams::BinaryInputStreams() {

}

BinaryInputStreams::BinaryInputStreams(
    const std::vector<std::string> &filenames,
    std::ios_base::openmode mode,
    BinaryStreamIos::Endianess endianess) :
    Precursor(filenames.size())
{
    for (u32 n = 0; n < filenames.size(); ++ n) {
	Precursor::operator[](n) =
	    new BinaryInputStream(filenames[n], mode, endianess);
	if (!(*this)[n]) {
	    (*this)[n].close();
	}
    }
}

BinaryInputStreams::BinaryInputStreams(
	std::vector<std::istream*> *istreams,
	BinaryStreamIos::Endianess endianess) :
	Precursor(istreams->size())
{
	for (u32 n = 0; n < istreams->size(); ++ n) {
		Precursor::operator[](n) =
			new BinaryInputStream(*(istreams->at(n)), endianess);
		if (!(*this)[n]) {
			(*this)[n].close();
		}
	}
}

BinaryInputStreams::~BinaryInputStreams() {
    for (u32 n = 0; n < size(); ++ n) {
	delete Precursor::operator[](n);
    }
}

bool BinaryInputStreams::areOpen() const {
    for (u32 n = 0; n < size(); ++ n) {
	if (!(*this)[n].isOpen()) {
	    return false;
	}
    }
    return (size() > 0);
}
