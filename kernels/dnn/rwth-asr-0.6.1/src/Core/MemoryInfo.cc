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
#include <Core/MemoryInfo.hh>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <Core/StringUtilities.hh>
#include <Core/Application.hh>

using namespace Core;

MemoryInfo::MemoryInfo()
    : size_(0), rss_(0), share_(0), text_(0), lib_(0), data_(0), pageSize_(0)
{
    update();
}

void MemoryInfo::update()
{
#ifdef OS_linux
    updateLinux();
#else
    Core::Application::us()->warning("memory information cannot be determined");
#endif
}

void MemoryInfo::updateLinux()
{
    pageSize_ = ::sysconf(_SC_PAGESIZE);
    pid_t pid = getpid();
    std::string statm = Core::form("/proc/%d/statm", pid);
    std::ifstream stat(statm.c_str());
    if (!stat) {
	Core::Application::us()->warning("cannot read %s", statm.c_str());
	return;
    }
    stat >> size_ >> rss_ >> share_ >> text_ >> lib_ >> data_;
    size_ *= pageSize_;
    rss_ *= pageSize_;
    text_ *= pageSize_;
    lib_ *= pageSize_;
    data_ *= pageSize_;
}

std::string MemoryInfo::peak() const
{
#ifndef OS_linux
    return "unknown";
#endif
    pid_t pid = getpid();
    std::string status = Core::form("/proc/%d/status", pid);
    std::ifstream stat(status.c_str());
    if (!stat) {
	Core::Application::us()->warning("cannot read %s", status.c_str());
	return "unknown";
    }
    std::string buffer;
    while (std::getline(stat, buffer).good()) {
	if (buffer.substr(0, 6) == "VmPeak") {
	    buffer = buffer.substr(7);
	    Core::stripWhitespace(buffer);
	    return buffer;
	}
    }
    return "unknown";
}

void MemoryInfo::write(XmlWriter &os) const
{
    os << XmlOpen("memory-information")
       << XmlFull("vmem", size())
       << XmlFull("rss", residentSetSize())
       << XmlFull("data", dataSize())
       << XmlClose("memory-information");
}
