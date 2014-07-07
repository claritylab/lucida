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
#ifndef CORE_FILESTREAM_HH
#define CORE_FILESTREAM_HH

#include <fstream>


namespace Core {


    /**
     * This is just a wrapper class for std::filebuf which grants
     * public access to the file descriptor of the associated file.
     * This hack is needed because the brain-damaged C++ standard says
     * we're not supposed to assume that file descriptors do exist.
     */

    class FileStreamBuffer : public std::filebuf {
    public:
#ifdef __SUNPRO_CC
	int fd() { return std::filebuf::fd(); }
#else
	int fd() { return _M_file.fd(); }
#endif
    };

} // namespace Core

#endif //CORE_FILESTREAM_HH
