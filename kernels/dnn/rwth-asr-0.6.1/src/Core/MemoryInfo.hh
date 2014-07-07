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
#ifndef _CORE_MEMORY_INFO_HH
#define _CORE_MEMORY_INFO_HH

#include <Core/Types.hh>
#include <Core/XmlStream.hh>

namespace Core {
/**
 * process memory information
 */
class MemoryInfo
{
public:
    typedef u64 SizeType;

    MemoryInfo();

    /**
     * update memory information
     */
    void update();

    /**
     * total virtual memory size in bytes
     */
    SizeType size() const { return size_; }
    /**
     * resident set size in bytes
     */
    SizeType residentSetSize() const { return rss_; }
    /**
     * shared pages
     */
    SizeType sharedPages() const { return share_; }
    /**
     * code size in bytes
     */
    SizeType codeSize() const { return text_; }
    /**
     * library size in bytes
     */
    SizeType libSize() const { return lib_; }
    /**
     * data/stack size in bytes
     */
    SizeType dataSize() const { return data_; }

    /**
     * peak vmem size (formatted string)
     */
    std::string peak() const;

    void write(XmlWriter &os) const;

private:
    void updateLinux();

    SizeType size_, rss_, share_, text_, lib_, data_;
    u32 pageSize_;
};

inline XmlWriter &operator<<(XmlWriter &os, const MemoryInfo &i) { i.write(os); return os; }

} // namespace Core

#endif // _CORE_MEMORY_INFO_HH
