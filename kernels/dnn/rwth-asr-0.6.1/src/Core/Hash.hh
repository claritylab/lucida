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
#ifndef _CORE_HASH_HH
#define _CORE_HASH_HH

#ifdef __SUNPRO_CC
#include <hash_map>
#include <hash_set>
#define __gnu_cxx std
#else

#if ((__GNUC__ > 3) || __INTEL_COMPILER)
#include <functional>
#if ((__GNUC__ >= 4) && (__GNUC_MINOR__ >= 3))
// gcc >= 4.3
#include <backward/hash_map>
#include <backward/hash_set>
#else
// gcc < 4.3
#include <ext/hash_set>
#include <ext/hash_map>
#endif // gcc >= 4.3
#else
// gcc <= 3
#include <ext/stl_hash_fun.h>
#include <ext/hash_set>
#include <ext/hash_map>
#endif

#endif // __SUNPROC_CC
#include <string>
#include <cstring>

namespace Core {

    using __gnu_cxx::hash;
    using __gnu_cxx::hash_set;
    using __gnu_cxx::hash_map;
    using __gnu_cxx::hash_multimap;

    template<class T> struct PointerHash {
	size_t operator() (const T *p) const {
	    return reinterpret_cast<size_t>(p);
	}
    };

    struct StringHash {
	size_t operator() (const char *s) const {
	    size_t result = 0;
	    while (*s) result = 5 * result + size_t(*s++);
	    return result;
	}
	size_t operator() (const std::string &s) const {
	    return (*this)(s.c_str());
	}
    };

    struct StringEquality :
	std::binary_function<const char*, const char*, bool>
    {
	bool operator() (const char *s, const char *t) const {
	    return (s == t) || (std::strcmp(s, t) == 0);
	}
	bool operator() (const std::string &s, const std::string &t) const {
	    return (s == t);
	}
    };

    class StringHashSet :
	public hash_set<std::string, StringHash, StringEquality>
    {};

    template <typename T>
    class StringHashMap :
	public hash_map<std::string, T, StringHash, StringEquality>
    {};

    //typedef hash_map HashMap;
	template <typename T_Key, typename T_Value, typename HashFcn = hash<T_Key>, typename EqualKey = std::equal_to<T_Key> >
	class HashMap :
	public hash_map<T_Key, T_Value, HashFcn, EqualKey>
	{};
}

#endif // _CORE_HASH_HH
