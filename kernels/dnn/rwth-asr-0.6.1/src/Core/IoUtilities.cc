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
#include "IoUtilities.hh"

namespace Core {
    /*****************************************************************************/
    void TimeStampWriter::write(std::ostream &out) const {
	struct tm * tm_t;
	tm_t = ::localtime(&t);
#ifdef _C_STYLE_TIMESTAMP
	char *s = ctime(&t);
	for (;*s != '\n'; ++s) out << *s;
#else
	out << tm_t->tm_mday          << '/'
	    << (tm_t->tm_mon + 1)     << '/'
	    << (tm_t->tm_year + 1900) << ' ';
	out << tm_t->tm_hour                                   << ':';
	if (tm_t->tm_min < 10) out << '0'; out << tm_t->tm_min << ':';
	if (tm_t->tm_sec < 10) out << '0'; out << tm_t->tm_sec;
#endif // _C_STYLE_TIMESTAMP
    }
    /*****************************************************************************/

    bool writeLargeBlock(int fd, const char *buf, u64 count)
    {
	ssize_t r = 0;
	do {
	    r = ::write(fd, buf, std::min(count, static_cast<u64>(Core::Type<ssize_t>::max)));
	    if (r > 0) {
		count -= r;
		buf += r;
	    }
	} while (r > 0 && count > 0);
	return count == 0;
    }


} // namespace Core
