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
#ifndef _CORE_TYPES_HH
#define _CORE_TYPES_HH

#include <stdint.h>
#include <cstring>
#include <string>
#include <vector>
#include <complex>

typedef int8_t      s8;
typedef uint8_t     u8;
typedef int16_t     s16;
typedef uint16_t    u16;
typedef int32_t     s32;
typedef uint32_t    u32;
typedef int64_t     s64;
typedef uint64_t    u64;

typedef float       f32;
typedef double      f64;

namespace Core {

    /** Static information about elementary types. */
    template<class T> struct Type {

	/** Name to be used to represent data type. */
	static const char *name;

	/** Largest representable value of data type. */
	static const T max;

	/**
	 * Smallest representable value of data type.
	 * Note that unlike std::numeric_limits<>::min this is the most negative
	 * value also for floating point types.
	 */
	static const T min;

	/**
	 * The difference between the smallest value greater than one and one.
	 */
	static const T epsilon;

	/**
	 * Smallest representable value greater than zero.
	 * For all integer types this is one.  For floating point
	 * types this is the same as std::numeric_limits<>::min or
	 * FLT_MIN / DBL_MIN.
	 */
	static const T delta;

    };

    /**
     *  Use this class for naming your basic classes.
     *  Creating new names: by specialization.
     *  @see example Matrix.hh
     */
    template <typename T>
    class NameHelper : public std::string {
    public:
	NameHelper() : std::string(Type<T>::name) {}
    };

    template <>
    class NameHelper<std::string> : public std::string {
    public:
	NameHelper() : std::string("string") {}
    };

    template <>
    class NameHelper<bool> : public std::string {
    public:
	NameHelper() : std::string("bool") {}
    };

    template <typename T>
    class NameHelper<std::complex<T> > : public std::string {
    public:
	NameHelper() : std::string(std::string("complex-") + NameHelper<T>()) {}
    };

    template <typename T>
    class NameHelper<std::vector<T> > : public std::string {
    public:
	NameHelper() : std::string(std::string("vector-") + NameHelper<T>()) {}
    };

    /**
     * Change endianess of a block of data.
     * The size argument is given as a template parameter, so the
     * compiler can unroll the loop.
     * @param buf pointer to an array of data
     * @param size size of the element data type in bytes.
     * @param count number of elements
     */
    template <size_t size>
    void swapEndianess(void *buf, size_t count = 1);
    template <>
    inline void swapEndianess<1>(void *buf, size_t count) {}

} // namespace Core

namespace std {

    template<> inline bool equal
    (std::vector<u8>::const_iterator b1, std::vector<u8>::const_iterator e1, std::vector<u8>::const_iterator b2)
    { return (memcmp(&(*b1), &(*b2), sizeof(u8) * (e1 - b1)) == 0); }
    template<> inline bool equal
    (std::vector<s8>::const_iterator b1, std::vector<s8>::const_iterator e1, std::vector<s8>::const_iterator b2)
    { return (memcmp(&(*b1), &(*b2), sizeof(s8) * (e1 - b1)) == 0); }

    template<> inline bool equal
    (std::vector<u16>::const_iterator b1, std::vector<u16>::const_iterator e1, std::vector<u16>::const_iterator b2)
    { return (memcmp(&(*b1), &(*b2), sizeof(u16) * (e1 - b1)) == 0); }
    template<> inline bool equal
    (std::vector<s16>::const_iterator b1, std::vector<s16>::const_iterator e1, std::vector<s16>::const_iterator b2)
    { return (memcmp(&(*b1), &(*b2), sizeof(s16) * (e1 - b1)) == 0); }

    template<> inline bool equal
    (std::vector<u32>::const_iterator b1, std::vector<u32>::const_iterator e1, std::vector<u32>::const_iterator b2)
    { return (memcmp(&(*b1), &(*b2), sizeof(u32) * (e1 - b1)) == 0); }
    template<> inline bool equal
    (std::vector<s32>::const_iterator b1, std::vector<s32>::const_iterator e1, std::vector<s32>::const_iterator b2)
    { return (memcmp(&(*b1), &(*b2), sizeof(s32) * (e1 - b1)) == 0); }

} // namespace std

#endif // _CORE_TYPES_HH
