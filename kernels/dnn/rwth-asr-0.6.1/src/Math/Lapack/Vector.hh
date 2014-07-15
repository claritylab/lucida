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
#ifndef _MATH_MATH_LAPACK_VECTOR_HH
#define _MATH_MATH_LAPACK_VECTOR_HH

#include <Core/Types.hh>
#include <Core/Assertions.hh>
#include <Core/Utility.hh>

namespace Math { namespace Lapack {

    /** Vector is simple wrapper for a 1-D buffer */
    template<class T>
    class Vector {
    public:
	typedef T Type;
	typedef Type* Iterator;
	typedef const Type* ConstIterator;
    private:
	Type* buffer_;
	size_t size_;
    private:
	void deleteBuffer() {
	    delete[] buffer_;
	    buffer_ = 0;
	    size_ = 0;
	}
    public:
	Vector(u32 n = 0) : buffer_(0), size_(0) { resize(n); }
	Vector(const Vector &vector) : buffer_(0), size_(0) { operator=(vector); }
	~Vector() { deleteBuffer(); };

	Vector& operator=(const Vector &vector) {
	    resize(vector.size());
	    std::copy(vector.begin(), vector.end(), buffer_);
	    return *this;
	}

	void resize(u32 n) {
	    if (size_ == n)
		return;
	    deleteBuffer();
	    if (n > 0)
		buffer_ = new Type[size_ = n];
	}
	size_t size() const { return size_; };

	Type& operator[](size_t i) { require_(i < size()); return buffer_[i]; }
	const Type& operator[](size_t i) const { require_(i < size()); return buffer_[i]; }

	Type* buffer() { return buffer_; };

	Iterator begin() { return buffer_; };
	ConstIterator begin() const { return buffer_; };

	Iterator end() { return buffer_ + size_; };
	ConstIterator end() const { return buffer_ + size_; };
    };

    template<class Source, class Target>
    void copy(const Source &source, Target &target) {
	target.resize(source.size());
	std::copy(source.begin(), source.end(), target.begin());
    }

    template<class Source, class Target>
    void copy(const Source &source, Target &target, size_t n) {
	n = std::min(n, source.size());
	target.resize(n);
	std::copy(source.begin(), source.begin() + n, target.begin());
    }

    template<class T>
    T maxAbsoluteElement(const Vector<T> &v) {
	T m; Core::maxAbsoluteElement(v.begin(), v.end(), m); return m;
    }

} } //namespace Math::Lapack

#endif //_MATH_LAPACK_VECTOR_HH
