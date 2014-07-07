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
#ifndef _CORE_RESOURCE_USAGE_INFO_HH
#define _CORE_RESOURCE_USAGE_INFO_HH

#include <Core/Types.hh>
#include <Core/XmlStream.hh>

namespace Core {
/**
 * process resource usage information
 */
class ResourceUsageInfo
{
public:
    typedef u64 SizeType;
    typedef f64 TimeType;

    ResourceUsageInfo();

    /**
     * update information
     */
    void update();

    /**
     * user time used (in seconds).
     * This is the total amount of time spent executing in user mode.
     */
    TimeType userTime() const { return utime_; }
    /**
     * System time used (in seconds).
     * This is the total amount of time spent executing in kernel mode.
     */
    TimeType systemTime() const { return stime_; }
    /**
     * total time used.
     * user time plus system time.
     */
    TimeType totalTime() const { return utime_+ stime_; }
    /**
     * Maximum resident set size used (in kilobytes).
     */
    SizeType maxResidentSetSize() const { return maxrss_; }
    /**
     * page reclaims.
     * The number of page faults serviced without any I/O activity.
     */
    SizeType pageReclaims() const { return minflt_; }
    /**
     * page faults.
     * The number of page faults serviced that required I/O activity.
     */
    SizeType pageFaults() const { return majflt_; }
    /**
     * block input operations.
     * The number of times the file system had to perform input.
     */
    SizeType blockInputOps() const { return inblock_; }
    /**
     * block output operations.
     * The number of times the file system had to perform output.
     */
    SizeType blockOutputOps() const { return outblock_; }
    /**
     * voluntary context switches.
     * The number of times a context switch resulted due to a process
     * voluntarily giving up the processor before its time slice was completed
     */
    SizeType voluntaryContextSwitches() const { return nvcsw_; }
    /**
     * involuntary context switches.
     * The number of times a context switch resulted due to a higher priority
     * process becoming runnable or because the current process exceeded its
     * time slice.
     */
    SizeType involuntaryContextSwitches() const { return nicsw_; }

    void write(XmlWriter &os) const;

private:
    SizeType maxrss_, minflt_, majflt_;
    SizeType inblock_, outblock_, nvcsw_, nicsw_;
    TimeType utime_, stime_;
};

inline XmlWriter &operator<<(XmlWriter &os, const ResourceUsageInfo &i) { i.write(os); return os; }

} // namespace Core

#endif // _CORE_RESOURCE_USAGE_INFO_HH
