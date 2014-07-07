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
#ifndef _FSA_HASH_HH
#define _FSA_HASH_HH

#include <Core/Types.hh>
#include <vector>
#include <Core/Vector.hh>

namespace Fsa {

    template<class T> struct IdentityKey { size_t operator() (T t) const { return t;} };

    template<class T, class HashKey = IdentityKey<T>, class HashEqual = std::equal_to<T> >
    class Hash {
    public:
	typedef u32 Cursor;
	static const Cursor InvalidCursor;
    private:
	Core::Vector<Cursor> bins_;
	class Element {
	public:
	    Cursor next_;
	    T data_;
	public:
	    Element(Cursor next, const T &data) : next_(next), data_(data) {}
	};
	HashKey hashKey_;
	HashEqual hashEqual_;
	std::vector<Element> elements_;

    public:
	class const_iterator : public std::vector<Element>::const_iterator {
	public:
	    const_iterator(const typename std::vector<Element>::const_iterator &i) :
		std::vector<Element>::const_iterator(i) {}
	    const T& operator*() const { return std::vector<Element>::const_iterator::operator*().data_; }
	    const T* operator->() const { return &std::vector<Element>::const_iterator::operator*().data_; }
	};

    public:
	Hash(u32 defaultSize = 10) {
	    bins_.resize(defaultSize, InvalidCursor);
	}
	Hash(const HashKey &key, const HashEqual &equal, u32 defaultSize = 10) :
	    hashKey_(key), hashEqual_(equal) {
	    bins_.resize(defaultSize, InvalidCursor);
	}
	void clear() {
	    std::fill(bins_.begin(), bins_.end(), InvalidCursor);
	    elements_.erase(elements_.begin(), elements_.end());
	}
	void resize(u32 size) {
	    std::fill(bins_.begin(), bins_.end(), InvalidCursor);
	    bins_.grow(size, InvalidCursor);
	    size = bins_.size();
	    for (typename std::vector<Element>::iterator i = elements_.begin(); i != elements_.end(); ++i) {
		u32 key = u32(hashKey_((*i).data_)) % size;
		i->next_ = bins_[key];
		bins_[key] = i - elements_.begin();
	    }
	}
	Cursor insertWithoutResize(const T &d) {
	    u32 key = u32(hashKey_(d)) % bins_.size(), i = bins_[key];
	    for (; (i != InvalidCursor) && (!hashEqual_(elements_[i].data_, d)); i = elements_[i].next_);
	    if (i == InvalidCursor) {
		i = elements_.size();
		elements_.push_back(Element(bins_[key], d));
		bins_[key] = i;
	    }
	    return i;
	}
	std::pair<Cursor, bool> insertExisting(const T &d) {
	    u32 key = u32(hashKey_(d)), i = bins_[key % bins_.size()];
	    for (; (i != InvalidCursor) && (!hashEqual_(elements_[i].data_, d)); i = elements_[i].next_);
	    if (i != InvalidCursor) return std::make_pair(i, true);
	    if (elements_.size() > 2 * bins_.size()) resize(2 * bins_.size() - 1);
	    i = elements_.size();
	    key = key % bins_.size();
	    elements_.push_back(Element(bins_[key], d));
	    bins_[key] = i;
	    return std::make_pair(i, false);
	}
	Cursor insert(const T &d) {
	    std::pair<Cursor, bool> tmp = insertExisting(d);
	    return tmp.first;
	}
	Cursor find(const T &d) const {
	    u32 key = u32(hashKey_(d)), i = bins_[key % bins_.size()];
	    for (; (i != InvalidCursor) && (!hashEqual_(elements_[i].data_, d)); i = elements_[i].next_);
	    return i;
	}
	bool has(const T &d) const { return find(d) != InvalidCursor; }
	size_t size() const { return elements_.size(); }
	const T& operator[] (const Cursor p) const { return elements_[p].data_; }
	const_iterator begin() const { return elements_.begin(); }
	const_iterator end() const { return elements_.end(); }
	size_t getMemoryUsed() const { return bins_.getMemoryUsed() + sizeof(Element) * elements_.size(); }
    };

    template<class T, class HashKey, class HashEqual>
    const typename Hash<T, HashKey, HashEqual>::Cursor Hash<T, HashKey, HashEqual>::InvalidCursor =
	Core::Type<typename Hash<T, HashKey, HashEqual>::Cursor>::max;


} // namespace Fsa

#endif // _FSA_HASH_HH
