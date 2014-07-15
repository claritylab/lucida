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
#ifndef _CORE_STRING_UTILITIES_HH
#define _CORE_STRING_UTILITIES_HH

#include <cstdarg>
#include <Core/Choice.hh>
#include <Core/Hash.hh>
#include <Core/Unicode.hh>

namespace Core {

    /**
     * Definition: A string has "normalized whitespace" if and only if it
     * matches one the following regular expressions: (\S+ )* or
     * (\S+ )* \S+ where \S matches any non-whitespace character.
     * I.e. There are no whitespaces other than single blanks; the first
     * character is non-blank, and the last character may be blank.
     */

    /**
     * Normalize whitespace in a string.
     * All consecutive whitespaces in the passed string are replaced by a
     * single blank.  Leading whitespace is removed.
     *
     * Attention: If tailing whitspaces exist, a single whitspace has to remain!
     *
     * @param s the string to be normalized
     */
    void normalizeWhitespace(std::string &s);

    /**
     * Ensure that a string ends with a single blank character.
     */
    void enforceTrailingBlank(std::string &s);

    /**
     * Remove trailing blank character.
     * Removes the last character if it is a blank.
     */
    void suppressTrailingBlank(std::string &s);

    const char requireTrailingBlank  = 'Y';
    const char prohibitTrailingBlank = 'N';
    const char tolerateTrailingBlank = '=';

    /**
     * Test if a string is whitespace-normalized.
     * @return true iff @c s matches (\S+ )*
     * @param trailingBlank specifies if a trailing blank is required.
     * The default is to classify strings ending with a blank as not
     * normalized.  If @c requireTrailingBlank is given, the string is
     * considered normalized only if it ends with a single blank.
     */
    bool isWhitespaceNormalized(
	const std::string &s,
	char trailingBlank = prohibitTrailingBlank);

    /**
     * Remove leading and trailing whitespace.
     */
    void stripWhitespace(std::string&);

    /** Convert UTF-8 string to lower case. */
    std::string convertToLowerCase(const std::string&);
    /** Convert UTF-8 string to upper case. */
    std::string convertToUpperCase(const std::string&);

    /**
     * Formatted output conversion.
     * form() is equivalent to snprintf(3) but returns the result as a string.
     * @param format a printf-like format string
     * @return the formatted string
     */

    std::string form(const char *format, ...)
	__attribute__ ((format (printf, 1, 2)));
    std::string vform(const char *format, va_list args);
    // __attribute__ ((format (printf, 1, 2)));


    /**
     * Split string into tokens.
     * Return a list of the tokens in the string, using @c separator
     * as the delimiter string.  Any Leading or trailing whitespace is
     * stripped ffrom the word returned.  Consecutive delimiters are
     * not grouped together and are deemed to delimit empty strings
     * (for example, split("1,,2", ",") returns ["1", "", "2"]"). The
     * @c separator argument may consist of multiple characters (for
     * example, split("1, 2, 3", ", ") returns ["1", "2", "3"]).
     * Splitting an empty string with a specified separator returns an
     * empty list.
     */
    std::vector<std::string> split(const std::string &string, const std::string &separator);

    /**
     * Like the python startswith method of strings
     *
     * return true is string starts with search
     */
    bool startsWith(const std::string &string, const std::string &search);

    /**
       Convinient functions to parse strings.

       returns true on successful conversion;
       the string argument may contain leading and tailing whitspaces.
    */
    /*
      general string to something conversion;
      currently specialized for:
      - standard datatypes
      - strings
      - vectors of standard datatypes
      - vectors of standard strings
      supports:
      - different representations of bool values
      - +/-inf for numerical types
      - ranges for numerical values
    */
    template<typename T>
    bool strconv(const std::string &, T &);

    /**
       Maps a given string to value of type T
       via the given Choice object;
       returns true on successful mapping.
    */
    template<typename T>
    bool strconv(const std::string &, T &,
		 const Choice &);

    /**
      String to list conversion.
      Used by strconv to convert a string to a list.
      Use instead of strconv if you need to specify
      element and range delimiter and range borders
      and step width.
      Default seperator is space or tab for strings
      and comma for any others; number ranges
      can be specified by placing a - between two numbers.
    */
    template<typename T>
    bool str2vector(const std::string &, std::vector<T> &,
		 const std::string & elemDelim);

    template<typename T>
    bool str2vector(const std::string &, std::vector<T> &,
		 const std::string & elemDelim,
		 const std::string & rangeDelim,
		 const T & min = Core::Type<T>::min, const T & max = Core::Type<T>::max, const T & step = T(1));

    template<typename T>
    bool str2vector(const std::string &, std::vector<T> &,
		 const std::string & elemDelim,
		 const std::string & rangeDelim,
		 const std::string & assignDelim,
		 const T & min = Core::Type<T>::min, const T & max = Core::Type<T>::max, const T & step = T(1));

    /**
     * convert vector to string, elements are separated by elemDelim
     */
    template<typename T>
    std::string vector2str(const std::vector<T> &vec, const std::string &elemDelim = " ");

} // namespace Core


// inline definitions and template specialisations
namespace Core {
    /**
      Generalized conversion.
    */
    /*****************************************************************************/
    template<typename T>
    bool strconv(const std::string &s, T &t) {
	std::istringstream ss(s);
	ss >> t;
	return ss.good();
    }
    /*****************************************************************************/



    /**
      Choice conversion.
    */
    /*****************************************************************************/
    template<typename T>
    bool strconv(const std::string &s, T &t, const Choice &c) {
	std::string::size_type a = s.find_first_not_of(utf8::whitespace);
	std::string::size_type b = s.find_last_not_of(utf8::whitespace);
	Choice::Value val =
	    (a == std::string::npos)?c[""]:c[s.substr(a, b-a+1)];
	t = T(val);
	if (val == Choice::IllegalValue) {
	    return false;
	} else {
	    return true;
	}
    }
    /*****************************************************************************/



    /**
      Primitive data type conversion.
    */
    /*****************************************************************************/
    template<typename Signed>
    bool str2signed(const std::string &s, Signed &i) {
	std::string::size_type a = s.find_first_not_of(utf8::whitespace);
	std::string::size_type b = s.find_last_not_of(utf8::whitespace);
	if (a == std::string::npos)
	    return false;
	else
	    switch (infinityChoice[s.substr(a, b-a+1)]) {
	    case 1:
		i = Core::Type<Signed>::max;
		return true;
	    case 2:
		i = Core::Type<Signed>::min;
		return true;
	    default:
		char *endPtr = 0;
		i = s32(::strtol(s.c_str(), &endPtr, 10));
		for (; ::isspace(*endPtr); ++endPtr);
		return *endPtr == '\0';
	    }
    }

    template<typename Unsigned>
    bool str2unsigned(const std::string &s, Unsigned &u) {
	std::string::size_type a = s.find_first_not_of(utf8::whitespace);
	std::string::size_type b = s.find_last_not_of(utf8::whitespace);
	if (a == std::string::npos)
	    return false;
	else
	    switch (infinityChoice[s.substr(a, b-a+1)]) {
	    case 1:
		u = Core::Type<Unsigned>::max;
		return true;
	    case 2:
		return false;
	    default:
		char *endPtr = 0;
		u = Unsigned(::strtoul(s.c_str(), &endPtr, 10));
		for (; ::isspace(*endPtr); ++endPtr);
		return *endPtr == '\0';
	    }
    }

    template<typename Float>
    bool str2float(const std::string &s, Float &f) {
	std::string::size_type a = s.find_first_not_of(utf8::whitespace);
	std::string::size_type b = s.find_last_not_of(utf8::whitespace);
	if (a == std::string::npos)
	    return false;
	else
	    switch (infinityChoice[s.substr(a, b-a+1)]) {
	    case 1:
		f = Core::Type<Float>::max;
		return true;
	    case 2:
		f = Core::Type<Float>::min;
		return true;
	    default:
		char *endPtr = 0;
		f = Float(::strtod(s.c_str(), &endPtr));
		for (; ::isspace(*endPtr); ++endPtr);
		return *endPtr == '\0';
	    }
    }


    template<>
    inline bool strconv<bool>(const std::string &s, bool &b) {
	return strconv(s, b, boolChoice);
    }

    template<>
    inline bool strconv(const std::string &s,  s8 &i) {
	return str2signed(s, i);
    }
    template<>
    inline bool strconv(const std::string &s, s16 &i) {
	return str2signed(s, i);
    }
    template<>
    inline bool strconv(const std::string &s, s32 &i) {
	return str2signed(s, i);
    }
#if defined(HAS_64BIT)
    template<>
    inline bool strconv(const std::string &s, s64 &i) {
	return str2signed(s, i);
    }
#endif
    template<>
    inline bool strconv(const std::string &s,  u8 &u) {
	return str2unsigned(s, u);
    }
    template<>
    inline bool strconv(const std::string &s, u16 &u) {
	return str2unsigned(s, u);
    }
    template<>
    inline bool strconv(const std::string &s, u32 &u) {
	return str2unsigned(s, u);
    }
#if defined(HAS_64BIT)
    template<>
    inline bool strconv(const std::string &s, u64 &u) {
	return str2unsigned(s, u);
    }
#endif
    template<>
    inline bool strconv(const std::string &s, f32 &f) {
	return str2float(s, f);
    }
    template<>
    inline bool strconv(const std::string &s, f64 &f) {
	return str2float(s, f);
    }
    /*****************************************************************************/



    /**
      String conversion; strips whitspaces.
    */
    /*****************************************************************************/
    template<>
    inline bool strconv(const std::string &s, std::string &ss) {
	std::string::size_type a = s.find_first_not_of(utf8::whitespace);
	std::string::size_type b = s.find_last_not_of(utf8::whitespace);
	ss = (a == std::string::npos)?"":s.substr(a, b - a + 1);
	return true;
    }
    /*****************************************************************************/



    /**
      Vector of primitive datatypes conversion.
    */
    /*****************************************************************************/
    template<typename T>
    bool str2vector(const std::string & s, std::vector<T> & e,
		 const std::string & elemDelim) {
	require(!elemDelim.empty());
	std::string::size_type start = s.find_first_not_of(utf8::whitespace);
	std::string::size_type end;
	std::string slice;
	T t;

	while(start != std::string::npos) {
	    end = s.find(elemDelim, start);
	    if (end == std::string::npos) {
		slice = s.substr(start);
		start = std::string::npos;
	    } else {
		slice = s.substr(start, end - start);
		start = s.find_first_not_of(utf8::whitespace, end + elemDelim.size());
	    }
	    if (!strconv(slice, t))
		return false;
	    e.push_back(t);
	}
	return true;
    }

    template<typename T>
    bool str2vector(const std::string & s, std::vector<T> & e,
		 const std::string & elemDelim,
		 const std::string & rangeDelim,
		 const T & minValue, const T & maxValue, const u32 & minSize, const u32 & maxSize, const T & step) {
	require(!elemDelim.empty());
	require(!rangeDelim.empty());
	std::string::size_type start = s.find_first_not_of(utf8::whitespace);
	std::string::size_type center, end;
	std::string slice;
	T t, rangeStart, rangeEnd;

	while(start != std::string::npos) {
	    end = s.find(elemDelim, start);
	    if (end == std::string::npos) {
		slice = s.substr(start);
		start = std::string::npos;
	    } else {
		slice = s.substr(start, end - start);
		start = s.find_first_not_of(utf8::whitespace, end + elemDelim.size());
	    }
	    center = slice.find(rangeDelim, 1);
	    if (center != std::string::npos) {
		if (!strconv(slice.substr(0, center), rangeStart)
		    || !strconv(slice.substr(std::min(center + rangeDelim.size(), slice.size())), rangeEnd))
		    return false;
		rangeStart = std::max(rangeStart, minValue);
		rangeEnd   = std::min(rangeEnd,   maxValue);
		for (t = rangeStart; t <= rangeEnd; t += step)
		    e.push_back(t);
	    } else {
		if (!strconv(slice, t))
		    return false;
		if((minValue <= t) && (t <= maxValue))
		    e.push_back(t);
		else {
		    std::cerr << "configuration error: ";
		    std::cerr << "the given string contains out of range values." << std::endl;
		    return false;
		}
	    }
	}
	if((e.size() >= minSize) && (e.size() <= maxSize))
	    return true;
	else {
	    std::cerr << "configuration error: ";
	    std::cerr << "the given string has an incorrect number of elements." << std::endl;
	    return false;
	}
    }

    template<typename T>
    bool str2vector(const std::string & s, std::vector<T> & e,
		 const std::string & elemDelim,
		 const std::string & rangeDelim,
		 const std::string & assignDelim,
		 const T & minValue, const T & maxValue, const u32 & minSize, const u32 & maxSize, const T & step) {
	require(!elemDelim.empty());
	require(!rangeDelim.empty());
	require(!assignDelim.empty());
	std::string::size_type start = s.find_first_not_of(utf8::whitespace);
	std::string::size_type center, end, assignPoint;
	std::string slice, wholeSlice;
	T t, rangeStart, rangeEnd, weight;

	while(start != std::string::npos) {
	    end = s.find(elemDelim, start);
	    if (end == std::string::npos) {
		wholeSlice = s.substr(start);
		start = std::string::npos;
	    } else {
		wholeSlice = s.substr(start, end - start);
		start = s.find_first_not_of(utf8::whitespace, end + elemDelim.size());
	    }

	    assignPoint = wholeSlice.find(assignDelim, 1);
	    if (assignPoint != std::string::npos) {
		if( !strconv(wholeSlice.substr(std::min(assignPoint + assignDelim.size(), wholeSlice.size())), weight))
		    return false;
		weight = std::max(weight, minValue);
		weight = std::min(weight, maxValue);
		slice = wholeSlice.substr(0, assignPoint);
		center = slice.find(rangeDelim, 1);
		if (center != std::string::npos) {
		    if (!strconv(slice.substr(0, center), rangeStart)
			|| !strconv(slice.substr(std::min(center + rangeDelim.size(), slice.size())), rangeEnd))
			return false;
		    if ( rangeStart> rangeEnd)
			return false;
		    for (t = rangeStart; t <= rangeEnd; t += step)
			e.push_back(weight);
		} else {
		    e.push_back(weight);
		}
	    } else {
		if (!strconv(wholeSlice, weight))
		    return false;
		weight = std::max(weight, minValue);
		weight = std::min(weight, maxValue);
		e.push_back(weight);
	    }

	}
	if((e.size() >= minSize) && (e.size() <= maxSize))
	    return true;
	else {
	    std::cerr << "configuration error: ";
	    std::cerr << "the given string has an incorrect number of elements." << std::endl;
	    return false;
	}
    }


    template<>
    inline bool strconv(const std::string & s, std::vector<u8 > & v) {
	return str2vector(s, v, ",", ":");
    }
    template<>
    inline bool strconv(const std::string & s, std::vector<u16> & v) {
	return str2vector(s, v, ",", ":");
    }
    template<>
    inline bool strconv(const std::string & s, std::vector<u32> & v) {
	return str2vector(s, v, ",", ":");
    }
#if defined(HAS_64BIT)
    template<>
    inline bool strconv(const std::string & s, std::vector<u64> & v) {
	return str2vector(s, v, ",", ":");
    }
#endif

    template<>
    inline bool strconv(const std::string & s, std::vector<s8 > & v) {
	return str2vector(s, v, ",", ":");
    }
    template<>
    inline bool strconv(const std::string & s, std::vector<s16> & v) {
	return str2vector(s, v, ",", ":");
    }
    template<>
    inline bool strconv(const std::string & s, std::vector<s32> & v) {
	return str2vector(s, v, ",", ":");
    }
#if defined(HAS_64BIT)
    template<>
    inline bool strconv(const std::string & s, std::vector<s64> & v) {
	return str2vector(s, v, ",", ":");
    }
#endif

    template<>
    inline bool strconv(const std::string & s, std::vector<f32> & v) {
	return str2vector(s, v, ",", ":");
    }
    template<>
    inline bool strconv(const std::string & s, std::vector<f64> & v) {
	return str2vector(s, v, ",", ":");
    }
    /*****************************************************************************/



    /**
      Vector of strings conversion; strips whitespaces.
    */
    /*****************************************************************************/
    template<>
    inline bool strconv(const std::string & s, std::vector<std::string> & v) {
	std::string::size_type start = s.find_first_not_of(utf8::whitespace);
	std::string::size_type end;

	while(start != std::string::npos) {
	    v.push_back("");
	    end = s.find_first_of(utf8::whitespace, start + 1);
	    if (end == std::string::npos) {
		v.back() = s.substr(start, s.find_last_not_of(utf8::whitespace) - start + 1);
		start = std::string::npos;
	    } else {
		v.back() = s.substr(start, s.find_last_not_of(utf8::whitespace, end - 1) - start + 1);
		start = s.find_first_not_of(utf8::whitespace, end + 1);
	    }
	}
	return true;
    }


    /*****************************************************************************/

    template<typename T>
    std::string vector2str(const std::vector<T> &vec, const std::string &elemDelim){
	std::stringstream ss;
	for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); it++){
	    ss << *it << elemDelim;
	}
	return ss.str();
    }


} // namespace Core

#endif //  _CORE_STRING_UTILITIES_HH
