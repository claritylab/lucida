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
#include "Utility.hh"

namespace Fsa {

    u32 estimateBytes(u32 x) {
	if (x >= (u32(1) << 24)) return 4;
	else if (x >= (u32(1) << 16)) return 3;
	else if (x >= (u32(1) << 8)) return 2;
	return 1;
    }

    void setBytes(Core::Vector<u8>::iterator i, u32 x, int nBytes) {
	switch (nBytes) {
	case 4: *(i++) = ((x >> 24) & 0xff);
	case 3: *(i++) = ((x >> 16) & 0xff);
	case 2: *(i++) = ((x >> 8) & 0xff);
	case 1: *(i++) = (x & 0xff);
	case 0: break;
	}
    }

    void appendBytes(Core::Vector<u8> &v, u32 x, int nBytes) {
	switch (nBytes) {
	case 4: v.push_back((x >> 24) & 0xff);
	case 3: v.push_back((x >> 16) & 0xff);
	case 2: v.push_back((x >> 8) & 0xff);
	case 1: v.push_back(x & 0xff);
	case 0: break;
	}
    }

    u32 getBytesAndIncrement(Core::Vector<u8>::const_iterator &a, int nBytes) {
	u32 x = 0;
	switch (nBytes) {
	case 4: x |= *(a++); x <<= 8;
	case 3: x |= *(a++); x <<= 8;
	case 2: x |= *(a++); x <<= 8;
	case 1: x |= *(a++);
	case 0: break;
	}
	return x;
    }

    u32 getBytes(Core::Vector<u8>::const_iterator a, int nBytes) {
	Core::Vector<u8>::const_iterator a_ = a;
	return getBytesAndIncrement(a_, nBytes);
    }

    const LabelIdStrings::Id LabelIdStrings::Empty = 0;
    const LabelIdStrings::Id LabelIdStrings::Invalid = Core::Type<LabelIdStrings::Id>::max;

    LabelIdStrings::Id LabelIdStrings::append(Core::Vector<LabelId>::const_iterator i) {
	Id output = start();
	for (; *i != InvalidLabelId; ++i) strings_.push_back(*i);
	return stop(output);
    }

    u32 LabelIdStrings::length(Id s) const {
	u32 l = 0;
	for (const_iterator it = begin(s); *it != InvalidLabelId; ++it, ++l);
	return l;
    }

    void LabelIdStrings::dump(std::ostream &os, Id id, ConstAlphabetRef alphabet) const {
	if (alphabet)
	    for (const_iterator it = begin(id); *it != InvalidLabelId; ++it)
		os << alphabet->symbol(*it) << " ";
	else
	    for (const_iterator it = begin(id); *it != InvalidLabelId; ++it)
		os << *it << " ";
    }

    size_t LabelIdStrings::getMemoryUsed() const {
	return hashedStrings_.getMemoryUsed() + strings_.getMemoryUsed();
    }

    QualifiedFilename splitQualifiedFilename(const std::string &file, const std::string &defaultQualifier) {
	std::string::size_type colonPos = file.find(':');
	if (colonPos == std::string::npos)
	    return QualifiedFilename(defaultQualifier, file);
	else
	    return QualifiedFilename(file.substr(0, colonPos), file.substr(colonPos + 1));
    }
} // namespace Fsa
