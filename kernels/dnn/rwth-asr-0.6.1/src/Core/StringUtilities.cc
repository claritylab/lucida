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
#include "StringUtilities.hh"

#include <Assertions.hh>
#include <stdio.h>
#include <wctype.h>


using namespace Core;

bool Core::isWhitespaceNormalized(const std::string &s, char trailingBlank) {
    std::string::size_type i, b ;

    b = 1;
    for (i = 0 ; i < s.length() ; ++i) {
	if (s[i] == utf8::blank) {
	    if (++b > 1) return false ;
	} else {
	    b = 0 ;
	}
    }
    switch (trailingBlank) {
    case requireTrailingBlank:	return (b == 1);
    case prohibitTrailingBlank:	return (b == 0) || (s.length() == 0);
    case tolerateTrailingBlank:	return true;
    default: require(false);
    }
}

void Core::normalizeWhitespace(std::string &s) {
    std::string::size_type i, j ;

    // remove leading whitespace
    j = s.find_first_not_of(utf8::whitespace) ;
    s.erase(0, j) ;

    // replace consecutive whitespace by single blanks
    for (i = 0 ;; ++ i) {
	i = s.find_first_of(utf8::whitespace, i) ;
	if (i == std::string::npos)
	    break ;
	j = s.find_first_not_of(utf8::whitespace, i) ;
	if (j == std::string::npos)
	    j = s.length() ;
	s.replace(i, j-i, 1, utf8::blank) ;
    }
}

void Core::enforceTrailingBlank(std::string &s) {
    if (s.length() > 0 && s[s.length()-1] != utf8::blank)
	s.append(1, utf8::blank) ;
}

void Core::suppressTrailingBlank(std::string &s) {
    if (s.length() > 0 && s[s.length()-1] == utf8::blank)
	s.erase(s.length()-1);
}

void Core::stripWhitespace(std::string &s) {
    std::string::size_type i ;

    // remove leading whitespace
    i = s.find_first_not_of(utf8::whitespace) ;
    if (i == std::string::npos) i = s.length();
    s.erase(0, i) ;

    // remove trailing whitespace
    i = s.find_last_not_of(utf8::whitespace) ;
    if (i != std::string::npos) s.erase(i + 1) ;

    ensure(s.empty() || s.find_first_of(utf8::whitespace) != 0);
    ensure(s.empty() || s.find_last_of (utf8::whitespace) != s.size() - 1);
}

std::string Core::convertToLowerCase(const std::string &s) {
    std::wstring result(widen(s));
    for (std::wstring::size_type i = 0; i < result.length(); ++i)
	result[i] = towlower(result[i]);
    return narrow(result);
}

std::string Core::convertToUpperCase(const std::string &s) {
    std::wstring result(widen(s));
    for (std::wstring::size_type i = 0; i < result.length(); ++i)
	result[i] = towupper(result[i]);
    return narrow(result);
}

std::string Core::form(const char *format, ...) {
    va_list ap ;
    static size_t buf_size = 0 ;
    static char *buf = 0 ;
    int n ;

    for (;;) {
	va_start(ap, format) ;
	n = vsnprintf(buf, buf_size, format, ap) ;
	va_end(ap) ;

	if (n < 0) {
	    // This implementation of vsnprintf returns -1 if the
	    // output was truncated.  We use the simple heuristic of
	    // doubling the buffer size
	    n = (buf_size > 64) ? 2 * buf_size : 64 ;
	} else {
	    // Newer implementations (>= glibc 2.1) return the number
	    // of characters needed.
	    if (n+1 <= (int)buf_size) break ; // +1 for terminating null byte
	}

	if (buf) delete[] buf ;
	buf = new char[buf_size = n+1] ;
    }

    return std::string(buf, n) ;
}


std::string Core::vform(const char *format, va_list ap) {
    static size_t buf_size = 0 ;
    static char *buf = 0 ;
    int n ;

    va_list ap2;
    for (;;) {
	va_copy(ap2, ap);
	n = vsnprintf(buf, buf_size, format, ap2) ;
	if (n < 0) {
	    // This implementation of vsnprintf returns -1 if the
	    // output was truncated.  We use the simple heuristic of
	    // doubling the buffer size
	    n = (buf_size > 64) ? 2 * buf_size : 64 ;
	} else {
	    // Newer implementations (>= glibc 2.1) return the number
	    // of characters needed.
	    if (n+1 <= (int)buf_size) break ; // +1 for terminating null byte
	}

	delete[] buf;
	buf = new char[buf_size = n+1] ;
    }

    return std::string(buf, n) ;
}


std::vector<std::string> Core::split(
    const std::string &string, const std::string &separator)
{
    std::vector<std::string> result;
    if (string.empty())
	return result;

    std::string::size_type start = 0;
    std::string::size_type end = 0;
    while(end != std::string::npos) {
	end = string.find(separator, start);
	if (end == std::string::npos) {
	    result.push_back(string.substr(start));
	    stripWhitespace(result.back());
	} else {
	    if (end > start) {
		result.push_back(string.substr(start, end - start));
		stripWhitespace(result.back());
	    }
	    start = end + separator.size();
	}
    }
    return result;
}

bool Core::startsWith(const std::string &string, const std::string &search) {
	if (search.size() > string.size()) return false;
	for (std::string::size_type i = 0; i < search.size(); ++i) {
		if (string[i] != search[i]) return false;
	}
	return true;
}
