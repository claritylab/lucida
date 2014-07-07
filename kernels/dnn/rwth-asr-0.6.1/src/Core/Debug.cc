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
#include "Debug.hh"

namespace Core {

#if defined(__APPLE__)

#include <cassert>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

// Based on Apple's recommended method as described in
// http://developer.apple.com/qa/qa2004/qa1361.html
bool AmIBeingDebugged()
// Returns true if the current process is being debugged (either
// running under the debugger or has a debugger attached post facto).
{
    // Initialize mib, which tells sysctl what info we want.  In this case,
    // we're looking for information about a specific process ID.
    int mib[] =
    {
	CTL_KERN,
	KERN_PROC,
	KERN_PROC_PID,
	getpid()
    };

    // Caution: struct kinfo_proc is marked __APPLE_API_UNSTABLE.  The source and
    // binary interfaces may change.
    struct kinfo_proc info;
    size_t info_size = sizeof ( info );

    int sysctl_result = sysctl ( mib, sizeof(mib) / sizeof(*mib), &info, &info_size, NULL, 0 );
    if ( sysctl_result != 0 )
	return false;

    // This process is being debugged if the P_TRACED flag is set.
    return ( info.kp_proc.p_flag & P_TRACED ) != 0;
}

#elif defined(OS_linux)

// just assume Linux-like with /proc

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <unistd.h>


bool AmIBeingDebugged() {
    // We can look in /proc/self/status for TracerPid.  We are likely used in crash
    // handling, so we are careful not to use the heap or have side effects.
    int status_fd = open("/proc/self/status", O_RDONLY);
    if (status_fd == -1)
	return false;

    // We assume our line will be in the first 1024 characters and that we can
    // read this much all at once.  In practice this will generally be true.
    // This simplifies and speeds up things considerably.
    char buf[1024];

    ssize_t num_read = read(status_fd, buf, sizeof(buf));
    buf[sizeof(buf)-1] = 0; // safety
    close(status_fd);
    if (num_read <= 0) return false;

    const char* searchStr = "TracerPid:\t";
    const char* f = strstr(buf, searchStr);
    if(f == NULL) return false;

    // Our pid is 0 without a debugger, assume this for any pid starting with 0.
    f += strlen(searchStr);
    return f < &buf[num_read] && *f != '0';
}

#else

// TODO implement
bool AmIBeingDebugged() { return false; }
#warning No AmIBeingDebugged implementation for your platform.

#endif

}
