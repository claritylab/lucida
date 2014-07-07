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
// $Id: Types.cc 9227 2013-11-29 14:47:07Z golik $

#include <Core/Types.hh>

namespace Core {

    template<> const char *Type<u8>::name("u8");
    template<> const u8  Type<u8 >::max(255U);
    template<> const u8  Type<u8 >::min(0U);
    template<> const char *Type<s8>::name("s8");
    template<> const s8  Type<s8 >::max(127);
    template<> const s8  Type<s8 >::min(-128);
    template<> const char *Type<u16>::name("u16");
    template<> const u16 Type<u16>::max(65535U);
    template<> const u16 Type<u16>::min(0U);
    template<> const char *Type<s16>::name("s16");
    template<> const s16 Type<s16>::max(32767);
    template<> const s16 Type<s16>::min(-32768);
    template<> const char *Type<u32>::name("u32");
    template<> const u32 Type<u32>::max(4294967295U);
    template<> const u32 Type<u32>::min(0U);
    template<> const char *Type<s32>::name("s32");
    template<> const s32 Type<s32>::max(2147483647);
    template<> const s32 Type<s32>::min(-2147483647 - 1); // gcc warns about too large int when -2147483648
    template<> const s32 Type<s32>::epsilon(1);
    template<> const s32 Type<s32>::delta(1);
#ifdef OS_darwin
    template<> const char *Type<size_t>::name("size_t");
    template<> const size_t Type<size_t>::max(4294967295U);
    template<> const size_t Type<size_t>::min(0U);
#endif
#if defined(HAS_64BIT)
    template<> const char *Type<u64>::name("u64");
    template<> const u64 Type<u64>::max(18446744073709551615U);
    template<> const u64 Type<u64>::min(0U);
    template<> const char *Type<s64>::name("s64");
    template<> const s64 Type<s64>::max(9223372036854775807LL);
    template<> const s64 Type<s64>::min(-9223372036854775807LL - 1); // gcc warns about too large int when -9223372036854775808LL
#endif
    template<> const char *Type<f32>::name("f32");
    template<> const f32 Type<f32>::max(+3.40282347e+38F);
    template<> const f32 Type<f32>::min(-3.40282347e+38F);
    template<> const f32 Type<f32>::epsilon(1.19209290e-07F);
    template<> const f32 Type<f32>::delta(1.17549435e-38F);
    template<> const char *Type<f64>::name("f64");
    template<> const f64 Type<f64>::max(+1.7976931348623157e+308);
    template<> const f64 Type<f64>::min(-1.7976931348623157e+308);
    template<> const f64 Type<f64>::epsilon(2.2204460492503131e-16);
    template<> const f64 Type<f64>::delta(2.2250738585072014e-308);

    template <size_t size>
    void swapEndianess(void *buf, size_t count) {
	char *b = (char*) buf ;
	for (size_t j = 0 ; j < size / 2 ; ++j)
	    for (size_t i = 0 ; i < count ; ++i)
		std::swap(b[i * size + j], b[i * size + size - j - 1]) ;
    }

    template void swapEndianess<2>(void *buf, size_t count);
    template void swapEndianess<4>(void *buf, size_t count);
    template void swapEndianess<8>(void *buf, size_t count);

} // namespace Core
