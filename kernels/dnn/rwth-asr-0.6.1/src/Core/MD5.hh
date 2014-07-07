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
#ifndef _CORE_MD5_HH
#define _CORE_MD5_HH

#include <string>
#include <sstream>

#include "BinaryStream.hh"
#include "StringUtilities.hh"

namespace Core {

    class MD5 {
    private:
	// mutable is a messy way of doing the lazy finalisation stuff
	mutable u32 A_, B_, C_, D_;
	mutable u32 nblocks;
	mutable u8 buf[64];
	mutable u32 count;
	mutable bool isFinalized_;

    private:
	void transform(const void *data) const;
	void finalize(void) const;

    public:
	MD5();

	void update(const char *v, u32 n) const;
	void update(const std::string&) const;
	bool updateFromFile(const std::string &filename);

	bool operator== (const MD5 &m) const {
	    for (int i = 0; i < 16; i++) if (buf[i] != m.buf[i]) return false;
	    return true;
	}

	operator std::string() const;
	friend std::ostream& operator<< (std::ostream &o, const MD5 &m);
	friend BinaryOutputStream& operator<<(BinaryOutputStream&, const MD5&);
    };
}

#endif // _CORE_MD5_HH
