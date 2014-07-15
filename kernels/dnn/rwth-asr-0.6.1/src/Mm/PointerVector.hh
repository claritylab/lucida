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
#ifndef _MM_POINTER_VECTOR_HH
#define _MM_POINTER_VECTOR_HH

#include <vector>
#include <Core/Assertions.hh>

namespace Mm {

    /** Container of object references in the form of pointers.
     *  Supported functions:
     *    -Controlled adding of new object references.
     *    -Obtaining of constant/not-constant object references.
     *    -Deleting object referenced by elements of the vector.
     */
    template<class T>
    class PointerVector : private std::vector<T*> {
    public:
	typedef std::vector<T*> Parent;
	typedef typename Parent::const_iterator ConstantIterator;
    private:
	/** Copying of pointer tables hard to solve generally because of inheritance and
	 *  it is not (yet) used in this library.
	 */
	PointerVector(PointerVector<T> &v) {}

	/** Copying of pointer tables hard to solve generally because of inheritance and
	 *  it is not (yet) used in this library.
	 */
	PointerVector<T>& operator=(PointerVector<T> &v) { return *this; }

    public:
	PointerVector(size_t size = 0) : Parent(size, 0) {}
	explicit PointerVector(T *p) : Parent(1, p) {}
	~PointerVector() { clear(); }

	/** Adds the @param newObject to the position @param newIndex.
	 *  Assertion is raised if the position @param newIndex is not zero.
	 *  If @param newIndex is larger than the last index, zero elements are inserted.
	 *  @return is index of the object.
	 */
	size_t add(size_t newIndex, T *newObject) {
	    if (newIndex >= this->size()) Parent::resize(newIndex + 1, 0);
	    require(Parent::operator[](newIndex) == 0);
	    Parent::operator[](newIndex) = newObject;
	    return newIndex;
	}

	T* release(size_t index) {
	    require(index < size());
	    T *result = Parent::operator[](index);
	    Parent::operator[](index) = 0;
	    return result;
	}

	/** New object is appended to the vector.
	 *  @return is index of the object.
	 */
	size_t pushBack(T *newObject) { Parent::push_back(newObject); return size() - 1; }
	/** Append with position verification
	 *  New object is appended to the vector and assertion fails
	 *  if @param newIndex is not equal to the position of the object.
	 */
	size_t pushBack(size_t newIndex, T *newObject) {
	    require(newIndex == size()); return pushBack(newObject);
	}

	/** @return type is T* to prevent the pointer value from changing at @param index.
	 */
	T* operator[](size_t index) const { return Parent::operator[](index); }

	ConstantIterator begin() const { return Parent::begin(); }
	ConstantIterator end() const { return Parent::end(); }

	T* front() const { return Parent::front(); }
	T* back() const { return Parent::back(); }

	size_t size() const { return Parent::size(); }

	void clear();
	void resize(size_t _size) {
	    if (_size <= size()) {
		for (size_t i = _size; i < this->size(); ++ i) release(i);
		Parent::resize(_size);
	    } else Parent::resize(_size, 0);
	}
    };

    template<class T>
    void PointerVector<T>::clear() {
	typename Parent::iterator i;
	for(i = Parent::begin(); i != Parent::end(); ++ i)
	    delete *i;
	Parent::clear();
    }

} // namespace Mm

#endif // _MM_POINTER_VECTOR_HH
