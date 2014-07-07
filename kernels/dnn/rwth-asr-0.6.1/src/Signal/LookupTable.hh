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
#ifndef _SIGNAL_LOOKUP_TABLE_HH
#define _SIGNAL_LOOKUP_TABLE_HH


#include <deque>
#include <algorithm>
#include <numeric>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>
#include <Core/BinaryStream.hh>

namespace Signal {

    //forward declarations for gcc4
    template<class Value, class Index> class LookupTable;
    template<class Value, class Index> inline LookupTable<Value,Index> operator*(const Value scalar, const LookupTable<Value,Index> &lt);
    template<class Value, class Index> inline Core::XmlWriter& operator<<(Core::XmlWriter& o, const LookupTable<Value, Index> &lt);

    /**
     * LookupTable: array of Values indexed by arbitrary numbers of type Index
     *
     * Maps the Index interval to the [0..(max_index - min_index) * bucketSize] integer inteval.
     * Integer interval is automatically extended, if a new Index does not fit into it.
     *
     * Values can be reached by
     *   Value& operator[](Index i) (do not extend the integer interval)
     *   iterator (do not extend the integer interval)
     *   iterator insert(Index i, Value init) (extends the integer interval)
     *
     * getInverse creates a LookupTable of the inverse function.
     *
     */

    template<class Value, class Index> class LookupTable {
    public:
	typedef Value ValueType;
	typedef Index IndexType;

	typedef typename std::deque<ValueType>::iterator Iterator;
	typedef typename std::deque<ValueType>::const_iterator ConstIterator;
    private:
	typedef s32 BucketIndex;
    private:
	Index bucketSize_; // interval size of indices, which are assigned to the same bucket
	BucketIndex offset_;
	std::deque<Value> f_;
	bool grow_;
    private:
	BucketIndex bucket(const Index index) const {
	    verify_(bucketSize() > 0); return (BucketIndex)round(index / bucketSize()) + offset_;
	}
	Index index(const BucketIndex b) const {
	    verify_(bucketSize() > 0); return (Index)(b - offset_) * bucketSize();
	}
    public:
	/**
	 *  Creates a lookup table with variable amount of buckets.
	 */
	LookupTable(const Index bucketSize = 0) : bucketSize_(bucketSize), offset_(0), grow_(true) { }
	/**
	 *  Creates a lookup table with constant amount of buckets.
	 *  Index values smaller than min or larger than max will be set back to min resp. max.
	 */
	LookupTable(const Index bucketSize, const Index min, const Index max);

	void clear() { f_.clear(); offset_ = 0; }

	Iterator insert(const Index index, const Value init = 0);
	ConstIterator find(const Index index, const Value defaultValue = 0) const {
	    BucketIndex b = bucket(index);
	    if (0 <= b && b < (BucketIndex)size()) return f_[b]; else return defaultValue;
	}
	Value& operator[](const Index index) {
	    return *insert(index);
	}
	const Value& operator[](const Index index) const {
	    BucketIndex b = bucket(index); ensure_(0 <= b && b < (BucketIndex)size()); return f_[b];
	}

	Iterator begin() { return f_.begin(); }
	ConstIterator begin() const { return f_.begin(); }
	Iterator end() { return f_.end(); }
	ConstIterator end() const { return f_.end(); }

	Index bucketSize() const { return bucketSize_; }
	size_t size() const { return f_.size(); }
	bool empty() const { return f_.empty(); }

	Value sum() const { return std::accumulate(begin(), end(), (Value)0); }
	Value surface() const { return sum() * bucketSize(); }
	void normalizeSurface();

	/**
	 *  Returns how monotonous the function is.
	 *  @return is:
	 *    0: not monotonous
	 *    1: monotonous with constant parts
	 *    2: strictly monotonous
	 @  @param equalityTolerance controls how large jitter is accepted in constant part.
	 */
	int isMonotonous(Value equalityTolerance = 1) const;

	void getInverse(LookupTable<Index, Value> &inverse) const;
	Value proposeBucketSizeForInverse() const;

	LookupTable<Value,Index>& operator+=(const LookupTable<Value,Index> &toAdd);
	LookupTable<Value,Index>& operator*=(const Value scalar);
	friend LookupTable<Value,Index> operator*<>(const Value scalar, const LookupTable<Value,Index> &lt);

	bool read(Core::BinaryInputStream &i);
	bool write(Core::BinaryOutputStream &o) const;
	friend Core::XmlWriter& operator<< <>(Core::XmlWriter& o, const LookupTable<Value, Index> &lt);
    };

    template<class Value, class Index>
    LookupTable<Value, Index>::LookupTable(const Index bucketSize, const Index min, const Index max) :
	bucketSize_(bucketSize), offset_(0), grow_(true)
    {
	require(bucketSize > 0);
	insert(min); insert(max);
	grow_ = false;
    }

    template<class Value, class Index>
    typename LookupTable<Value, Index>::Iterator
    LookupTable<Value, Index>::insert(const Index index, const Value init)
    {
	BucketIndex b = bucket(index);
	if (!f_.empty()) {
	    if (b < 0) {
		if (grow_) {
		    f_.insert(f_.begin(), -b, init);
		    offset_ -= b;
		}
		b = 0;
	    }
	    else if (b >= (BucketIndex)size()) {
		if (grow_)
		    f_.insert(f_.end(), b - (BucketIndex)size() + 1, init);
		b = size() - 1;
	    }
	} else {
    verify_(grow_);
	    f_.insert(f_.end(), 1, init);
	    offset_ -= b;
	    b = 0;
	}
	return f_.begin() + b;
    }

    template<class Value, class Index>
    void LookupTable<Value, Index>::normalizeSurface()
    {
	Value s = surface();
	require(s != 0);
	std::transform(begin(), end(), begin(), std::bind2nd(std::divides<Value>(), s));
    }

    template<class Value, class Index>
    int LookupTable<Value, Index>::isMonotonous(Value equalityTolerance) const
    {
	require(!empty());
	if (size() < 2) return 1;

	ConstIterator previous = begin();
	ConstIterator current = previous + 1;
	for(;Core::isAlmostEqual(*current, *previous, equalityTolerance) && current != end();
	    ++ previous, ++ current);
	if (current == end())
	    return 1;

	bool increasing = (*current > *previous);
	int result = 2;
	for (;current != end(); ++ previous, ++ current) {
	    if (Core::isAlmostEqual(*current, *previous, equalityTolerance)) {
		result = 1;
	    } else if ((increasing && *current < *previous) ||
		       (!increasing && *current > *previous)) {
		return (result = 0);
	    }
	}
	return result;
    }

    template<class Value, class Index>
    void LookupTable<Value, Index>::getInverse(LookupTable<Index, Value> &inverse) const
    {
	verify(isMonotonous());

	require(inverse.empty());
	if (inverse.bucketSize() == 0)
	    inverse = LookupTable<Index, Value>(proposeBucketSizeForInverse());

	// Note: index is used here rather than iterator since iterators become invalid after each call to insert.
	BucketIndex previousIndex = 0;
	for(BucketIndex b = 0; b < (BucketIndex)size(); ++ b) {
	    Iterator current, previous;
	    *(current = inverse.insert(f_[b])) = index(b);
	    previous = inverse.begin() + previousIndex;

	    // fill skipped buckets
	    if (previous < current)
		std::fill(previous + 1, current, *current);
	    else if (current < previous)
		std::fill(current + 1, previous, *current);

	    previousIndex = current - inverse.begin();
	}
    }

    template<class Value, class Index>
    Value LookupTable<Value, Index>::proposeBucketSizeForInverse() const
    {
	static Value enlargementFactor = 2;
	Value result = (*std::max_element(begin(), end()) - *std::min_element(begin(), end())) /
	    ((Value)size() * enlargementFactor);
	verify(result > 0);
	return result;
    }

    template<class Value, class Index>
    LookupTable<Value,Index>& LookupTable<Value, Index>::operator+=(const LookupTable<Value, Index> &toAdd)
    {
	if (bucketSize() == 0) *this = LookupTable<Value, Index>(toAdd.bucketSize());
	for(BucketIndex b = 0; b < (BucketIndex)toAdd.size(); ++ b) {
	    *insert(toAdd.index(b), 0) += toAdd.f_[b];
	}
	return *this;
    }

    template<class Value, class Index>
    LookupTable<Value, Index>&
    LookupTable<Value, Index>::operator*=(const Value scalar)
    {
	std::transform(begin(), end(), begin(),
		       std::bind2nd(std::multiplies<Value>(), scalar));
	return *this;
    }

    template<class Value, class Index>
    LookupTable<Value, Index> operator*(const Value scalar, const LookupTable<Value, Index> &lt)
    {
	LookupTable<Value, Index> result = lt;
	result *= scalar;
	return result;
    }

    template<class Value, class Index>
    bool LookupTable<Value, Index>::read(Core::BinaryInputStream &i)
    {
	i >> bucketSize_;
	i >> offset_;
	i >> grow_;
	u32 s; i >> s; f_.resize(s);
	for(Iterator it = begin(); it != end(); ++ it)
	    i >> (*it);
	return i.good();
    }

    template<class Value, class Index>
    bool LookupTable<Value, Index>::write(Core::BinaryOutputStream &o) const
    {
	o << bucketSize_;
	o << offset_;
	o << grow_;
	o << (u32)f_.size();
	std::copy(f_.begin(), f_.end(), Core::BinaryOutputStream::Iterator<Value>(o));
	return o.good();
    }

    template<class Value, class Index>
    Core::XmlWriter& operator<<(Core::XmlWriter& o, const LookupTable<Value, Index> &lt)
    {
	o << Core::XmlOpen("lookup-table")
	    + Core::XmlAttribute("bucket-size", lt.bucketSize())
	    + Core::XmlAttribute("offset", lt.offset_);
	for(size_t b = 0; b < lt.f_.size(); ++ b) o << lt.index(b) << " " << lt.f_[b] << "\n";
	o << Core::XmlClose("lookup-table");
	return o;
    }

    template<class Value, class Index>
    Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream& o, const LookupTable<Value, Index> &lt)
    {
	lt.write(o); return o;
    }

    template<class Value, class Index>
    Core::BinaryInputStream& operator>>(Core::BinaryInputStream& i, LookupTable<Value, Index> &lt)
    {
	lt.read(i); return i;
    }

} // namespace Speech


#endif // _SIGNAL_LOOKUP_TABLE_HH
