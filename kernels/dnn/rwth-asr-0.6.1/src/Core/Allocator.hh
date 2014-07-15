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
#ifndef __CORE_ALLOCATOR_HH
#define __CORE_ALLOCATOR_HH

#include <cstdlib>
#include <memory>
#include <Core/Assertions.hh>

#ifdef OS_darwin
#include <sys/cdefs.h>
#ifndef __DARWIN_10_6_AND_LATER
#include <sys/errno.h>
namespace {
// MacOS X's malloc always returns 16 byte aligned memory
int posix_memalign(void **mem, size_t, size_t size) {
    *mem = malloc(size);
    return (*mem == 0 ? ENOMEM : 0);
}
}
#endif
#endif

namespace Core
{

    template< typename T, size_t alignment>
    struct AlignedAlloc
    {
    public:
	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef size_t size_type;
	typedef std::ptrdiff_t difference_type;

	template<typename T2>
	struct rebind
	{
	    typedef AlignedAlloc<T2, alignment> other;
	};

	pointer allocate( size_type n )
	{
	    void *mem = 0;
	    int r = posix_memalign(&mem, alignment, n * sizeof( value_type ));
	    verify_(!r);
	    return (r ? 0 : reinterpret_cast<pointer>(mem));
	}

	inline void deallocate( pointer p, size_type )
	{
	    free( p );
	}

	inline void construct( pointer p, const T& t )
	{
	    new( p ) T( t );
	}

	inline void destroy( pointer p )
	{
	    p->~T();
	}

	size_type max_size() const throw()
	{
	    return size_t(-1) / sizeof(value_type);
	}

    };


    /**
     * Aligned memory allocation for an array.
     */
    template<class T>
    bool allocateAlignedVector(T **ptr, size_t size, T initialValue=static_cast<T>(0), size_t alignment=16) {
	int r = posix_memalign(reinterpret_cast<void**>(ptr), alignment, sizeof(T) * size);
	ensure(ptr);
	ensure(!r);
	std::fill(*ptr, *ptr + size, initialValue);
	return !r;
    }

} // namespace Core

#endif // __CORE_ALLOCATOR_HH
