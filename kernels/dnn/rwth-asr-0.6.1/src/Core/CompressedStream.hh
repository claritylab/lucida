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
// ============================================================================
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
// ============================================================================
//
// Standard streambuf implementation following Nicolai Josuttis, "The
// Standard C++ Library".
// ============================================================================


#ifndef _CORE_COMPRESSED_STREAM_HH
#define _CORE_COMPRESSED_STREAM_HH

// standard C++ with new header file names and std:: namespace
#include <iostream>
#include <fstream>

namespace Core {

    // ----------------------------------------------------------------------------
    // Internal classes to implement CompressedStream. See below for user classes.
    // ----------------------------------------------------------------------------

    class CompressedStreamBuf;

    // ----------------------------------------------------------------------------
    // User classes. Use CompressedInputStream and CompressedOutputStream analogously to ifstream and
    // ofstream respectively. They read and write files based on the gz*
    // function interface of the zlib. Files are compatible with gzip compression.
    // ----------------------------------------------------------------------------

    class CompressedInputStream : public std::istream {
    private:
	std::streambuf *buf_;
    public:
	CompressedInputStream();
	CompressedInputStream(const std::string &name);
	~CompressedInputStream() { close(); }
	void open(const std::string &name);
	void close();
	bool isOpen() const { return buf_; }
    };

    class CompressedOutputStream : public std::ostream {
    private:
	std::streambuf *buf_;
    public:
	CompressedOutputStream();
	CompressedOutputStream(const std::string &name);
	~CompressedOutputStream() { close(); }
	void open(const std::string &name);
	void close();
	bool isOpen() const { return buf_; }
    };

    std::string extendCompressedFilename(const std::string &filename, const std::string &extension);

} //namespace Core

#endif // _CORE_COMPRESSED_STREAM_HH
