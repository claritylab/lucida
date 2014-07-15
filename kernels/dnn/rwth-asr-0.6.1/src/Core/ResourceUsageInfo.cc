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
#include <Core/ResourceUsageInfo.hh>
#include <sys/time.h>
#include <sys/resource.h>
#include <Core/Application.hh>

using namespace Core;

ResourceUsageInfo::ResourceUsageInfo()
    : maxrss_(0), minflt_(0), majflt_(0),
      inblock_(0), outblock_(0), nvcsw_(0), nicsw_(0),
      utime_(0), stime_(0)
{
    update();
}

void ResourceUsageInfo::update()
{
    struct rusage usage;
    if (::getrusage(RUSAGE_SELF, &usage)) {
	Core::Application::us()->warning("cannot get rusage");
	return;
    }
    utime_ = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    stime_ = usage.ru_stime.tv_sec + usage.ru_utime.tv_usec / 1e6;
    maxrss_ = usage.ru_maxrss;
    minflt_ = usage.ru_minflt;
    majflt_ = usage.ru_majflt;
    inblock_ = usage.ru_inblock;
    outblock_ = usage.ru_oublock;
    nvcsw_ = usage.ru_nvcsw;
    nicsw_ = usage.ru_nivcsw;
}

void ResourceUsageInfo::write(XmlWriter &os) const
{
    os << XmlOpen("resource-usage-information")
       << XmlFull("user-time", userTime())
       << XmlFull("system-time", systemTime())
       << XmlFull("rss", maxResidentSetSize())
       << XmlFull("block-input", blockInputOps())
       << XmlFull("block-output", blockOutputOps())
       << XmlClose("resource-usage-information");
}
