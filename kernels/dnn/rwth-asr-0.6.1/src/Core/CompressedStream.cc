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
// based on
// gzstream, C++ iostream classes wrapping the zlib compression library.
// Copyright (C) 2001  Deepak Bandyopadhyay, Lutz Kettner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#include <Core/Assertions.hh>
#include <Core/CompressedStream.hh>
#include <iostream>
#include <cstring>
#include <zlib.h>
#include <bits/functexcept.h>

using namespace Core;

// ===========================================================================
// class CompressedStreamBuf

namespace Core {
    class CompressedStreamBuf : public std::streambuf {
    private:
	static const int bufferSize = 47+256;    // size of data buff
	// totals 512 bytes under g++ for CompressedInputStream at the end.

	gzFile           file;               // file handle for compressed file
	char             buffer[bufferSize]; // data buffer
	char             opened;             // open/close state of stream
	int              mode_;              // I/O mode

	int flush_buffer();
    public:
	CompressedStreamBuf() : opened(0) {
	    setp( buffer, buffer + (bufferSize-1));
	    setg( buffer + 4,     // beginning of putback area
		  buffer + 4,     // read position
		  buffer + 4);    // end position
	    // ASSERT: both input & output capabilities will not be used together
	}
	int is_open() { return opened; }
	bool open(const std::string &name, std::ios_base::openmode open_mode);
	bool close();
	virtual ~CompressedStreamBuf();

	virtual int overflow(int c = EOF);
	virtual int underflow();
	virtual int sync();
    };
}

CompressedStreamBuf::~CompressedStreamBuf() {
    close();
}

bool CompressedStreamBuf::open(const std::string &name, std::ios_base::openmode mode) {
    // no append nor read/write mode
    require(!(mode & std::ios::ate));
    require(!(mode & std::ios::app));
    require(!((mode & std::ios::in) && (mode & std::ios::out)));

    if (is_open())
	return false;

    char  fmode[10];
    char* fmodeptr = fmode;
    if (mode & std::ios::in)
	*fmodeptr++ = 'r';
    if (mode & std::ios::out)
	*fmodeptr++ = 'w';
    *fmodeptr++ = 'b';
    *fmodeptr = '\0';

    file = gzopen(name.c_str(), fmode);
    if (file == NULL)
	return false;

    opened = 1;
    mode_ = mode;
    return true;
}

bool CompressedStreamBuf::close() {
    if (is_open()) {
	sync();
	opened = 0;
	if (gzclose(file) == Z_OK)
	    return true;
    }
    return false;
}

int CompressedStreamBuf::underflow() { // used for input buffer only
    if ( gptr() && ( gptr() < egptr()))
	return * reinterpret_cast<unsigned char *>( gptr());

    if ( ! (mode_ & std::ios::in) || ! opened)
	return traits_type::eof();
    // Josuttis' implementation of inbuf
    int n_putback = gptr() - eback();
    if ( n_putback > 4)
	n_putback = 4;
    memcpy( buffer + (4 - n_putback), gptr() - n_putback, n_putback);

    int num = gzread( file, buffer+4, bufferSize-4);
    if (num < 0) { // ERROR
	// There is no standard way of reporting a read error to the calling
	// function. By returning EOF (as suggested by the C++ standard),
	// only the eof flag of the overlying stream object is set instead of
	// the bad bit.
	// gzread only returns -1 if the read file is somehow damaged.
	// Throwing an exception here will most probably terminate the process,
	// which should be better than silently ignoring the corrupted file.
	// A similar behaviour is implemented in std::basic_filebuf::underflow.
	std::__throw_ios_failure("CompressedStreamBuf::underflow gzread failed");
    }
    if (num <= 0) // EOF or ERROR
	return traits_type::eof();

    // reset buffer pointers
    setg( buffer + (4 - n_putback),   // beginning of putback area
	  buffer + 4,                 // read position
	  buffer + 4 + num);          // end of buffer

    // return next character
    return * reinterpret_cast<unsigned char *>( gptr());
}

int CompressedStreamBuf::flush_buffer() {
    // Separate the writing of the buffer from overflow() and
    // sync() operation.
    int w = pptr() - pbase();
    if ( gzwrite( file, pbase(), w) != w)
	return traits_type::eof();
    pbump( -w);
    return w;
}

int CompressedStreamBuf::overflow(int c) { // used for output buffer only
    if ( ! ( mode_ & std::ios::out) || ! opened)
	return traits_type::eof();
    if (c != traits_type::eof()) {
	*pptr() = c;
	pbump(1);
    }
    if ( flush_buffer() == traits_type::eof())
	return traits_type::eof();
    return c;
}

int CompressedStreamBuf::sync() {
    // Changed to use flush_buffer() instead of overflow( EOF)
    // which caused improper behavior with std::endl and flush(),
    // bug reported by Vincent Ricard.
    if ( pptr() && pptr() > pbase()) {
	if ( flush_buffer() == traits_type::eof())
	    return -1;
    }
    return 0;
}

// ===========================================================================
// class CompressedInputStream

CompressedInputStream::CompressedInputStream() : std::istream(0), buf_(0) {}

CompressedInputStream::CompressedInputStream(const std::string &name) :
    std::istream(0), buf_(0)
{
    open(name);
}

void CompressedInputStream::open(const std::string &name) {
    if (buf_)
	close();
    if (name == "-") {
	buf_ = std::cin.rdbuf();
    } else {
	CompressedStreamBuf *buf = new CompressedStreamBuf();
	if (!buf->open(name, std::ios::in))
	{
	    setstate(std::ios::failbit);
	    delete buf;
	    return;
	}
	buf_ = buf;
    }
    rdbuf(buf_);
}

void CompressedInputStream::close() {
    if (buf_) {
	if (buf_ != std::cin.rdbuf()) delete buf_;
	buf_ = 0;
	rdbuf(0);
    }
}

// ===========================================================================
// class CompressedOutputStream

CompressedOutputStream::CompressedOutputStream() : std::ostream(0), buf_(0) {}

CompressedOutputStream::CompressedOutputStream(const std::string &name) :
    std::ostream(0), buf_(0)
{
    open(name);
}

void CompressedOutputStream::open(const std::string &name) {
    if (buf_)
	close();
    if ((name.rfind(".gz") == name.length() - 3) || (name.rfind(".Z") == name.length() - 3)) {
	CompressedStreamBuf *buf = new CompressedStreamBuf();
	if (!buf->open(name, std::ios::out))
	{
	    setstate(std::ios::failbit);
	    delete buf;
	    return;
	}
	buf_ = buf;
    } else if (name == "-") {
	buf_ = std::cout.rdbuf();
    } else {
	std::filebuf *buf = new std::filebuf();
	if (!buf->open(name.c_str(), std::ios::out))
	{
	    setstate(std::ios::failbit);
	    delete buf;
	    return;
	}
	buf_ = buf;
    }
    rdbuf(buf_);
}

void CompressedOutputStream::close() {
    if (buf_) {
	if (buf_ != std::cout.rdbuf()) delete buf_;
	buf_ = 0;
	rdbuf(0);
    }
}

// ===========================================================================

std::string Core::extendCompressedFilename(const std::string &filename, const std::string &extension) {
    std::string::size_type gzPos = filename.rfind(".gz");
    std::string::size_type zPos = filename.rfind(".Z");
    std::string::size_type bz2Pos = filename.rfind(".bz2");
    if (gzPos == filename.length() - 3) return std::string(filename, 0, gzPos) + extension + ".gz";
    else if (zPos == filename.length() - 2) return std::string(filename, 0, zPos) + extension + ".Z";
    else if (bz2Pos == filename.length() - 3) return std::string(filename, 0, bz2Pos) + extension + ".bz2";
    return filename + extension;
}
