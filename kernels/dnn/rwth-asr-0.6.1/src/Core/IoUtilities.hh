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
#ifndef _CORE_IOUTILITIES_HH_
#define _CORE_IOUTILITIES_HH_

#include <algorithm>
#include <iostream>
#include <time.h>
#include <list>
#include <set>
#include <unistd.h>
#include <vector>
#include <Core/Types.hh>

namespace Core {
    /*****************************************************************************/
    struct OStreamWriter {
	virtual ~OStreamWriter() {}
	virtual void write(std::ostream & out) const = 0;
    };

    /*****************************************************************************/


    /*****************************************************************************/
    /*
      Prints a timestamp.
    */
    struct TimeStampWriter :
	public OStreamWriter {
	time_t t;

	TimeStampWriter() {
	    ::time(&t);
	}
	TimeStampWriter(const time_t & t) :
	    t(t) {}

	void write(std::ostream &out) const;
    };


    inline std::ostream & timestamp(std::ostream &out) {
	Core::TimeStampWriter w;
	w.write(out);
	return out;
    }

    /*****************************************************************************/


    /*****************************************************************************/
    /*
      Prints a list of values.
      Use either ListWriter directly or
      for vector, list, and set you may use the function
      str returning a streamable object.
    */
    template<class InputIterator>
    struct ListWriter :
	public OStreamWriter {

	const InputIterator begin;
	const InputIterator end;
	const std::string delim;
	const std::string openingBracket;
	const std::string closingBracket;

	ListWriter(
	    const InputIterator & begin,
	    const InputIterator & end,
	    const std::string & delim = ", ",
	    const std::string & openingBracket = "",
	    const std::string & closingBracket = ""
	    ) :
	    begin(begin),
	    end(end), delim(delim),
	    openingBracket(openingBracket), closingBracket(closingBracket) {
	}

	void write(std::ostream &out) const {
	    out << openingBracket;
	    if (begin != end) {
		InputIterator it = begin;
		out << *(it++);
		for(; it != end; ++it)
		    out << delim << *it;
	    }
	    out << closingBracket;
	}
    };

    template<typename T>
	ListWriter<typename std::vector<T>::const_iterator> str(const std::vector<T> & v, const std::string & delim = ", ") {
	return ListWriter<typename std::vector<T>::const_iterator>(v.begin(), v.end(), delim, "[", "]");
    }

   template<typename T>
	ListWriter<typename std::set<T>::const_iterator> str(const std::set<T> & v, const std::string & delim = ", ") {
	return ListWriter<typename std::set<T>::const_iterator>(v.begin(), v.end(), delim, "{", "}");
    }

    template<typename T>
	ListWriter<typename std::list<T>::const_iterator> str(const std::list<T> & v, const std::string & delim = ", ") {
	return ListWriter<typename std::list<T>::const_iterator>(v.begin(), v.end(), delim, "[", "]");
    }

    /*****************************************************************************/

    /**
     * Write a larger (>2GB) block of memory to a file.
     * Parameters as in write(2)
     * returns true on success
     */
    bool writeLargeBlock(int fd, const char *buf, u64 count);

} // namespace Core


/*********************************************************************************/
inline std::ostream & operator<<(std::ostream & out, const Core::OStreamWriter & w) {
    w.write(out);
    return out;
}
/*********************************************************************************/

#endif // _CORE_IOUTILITIES_HH_
