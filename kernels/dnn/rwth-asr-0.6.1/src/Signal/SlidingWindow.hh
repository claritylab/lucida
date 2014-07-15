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
#ifndef _SIGNAL_SLIDING_WINDOW_HH
#define _SIGNAL_SLIDING_WINDOW_HH

#include <Flow/Node.hh>
#include <deque>
#include <Flow/Vector.hh>

namespace Signal {

    /** SlidingWindow over a sequence of data
     *
     *  Usage:
     *    -call add to update the sliding window;
     *    -call flush to update the sliding window if no more input is available;
     *
     *    -call out to obtain the element at the output point (also called 'right' or 'present');
     *    -call removed to obtain the removed element after the last call to add or flush;
     *     (Remark: neither out nor removed change the status of the sliding window)
     *    -call get to obtain the element at a index relative to the present position;
     *
     *    -use the iterators and operators to reach any arbitrary element in the sliding window.
     */
    template<class T>
    class SlidingWindow : private std::deque<T> {
	typedef typename std::deque<T> Precursor;
    public:
	typedef typename Precursor::const_iterator ConstantIterator;
	typedef typename Precursor::iterator Iterator;
	typedef typename Precursor::const_reverse_iterator ConstantReverseIterator;
	typedef typename Precursor::reverse_iterator ReverseIterator;
    public:
	enum MarginPolicyType { marginPolicyCopy, marginPolicyMean, marginPolicyZero, marginPolicyOne };
	/** Base class for margin policies creating missing elements. */
	class marginPolicy {
	protected:
	    const SlidingWindow &slidingWindow_;
	public:
	    marginPolicy(const SlidingWindow &slidingWindow) : slidingWindow_(slidingWindow) {}
	    virtual ~marginPolicy() {}
	    virtual void get(int relativeIndex, T &out) const = 0;
	};
	/** Creates non-existing elements by copying the first resp. last element in sliding window. */
	struct copyMarginPolicy : public marginPolicy {
	    copyMarginPolicy(const SlidingWindow &slidingWindow) : marginPolicy(slidingWindow) {}
	    virtual ~copyMarginPolicy() {}
	    virtual void get(int relativeIndex, T &out) const {
		this->slidingWindow_.getClosest(relativeIndex, out);
	    }
	};

	/** Creates non-existing elements by average of the present elements in the sliding window.
	 *  (Type of the Data structure T is: Flow::DataPtr<Flow::Vector<f32>)
	 */
	struct meanMarginPolicy : public marginPolicy {
	    meanMarginPolicy(const SlidingWindow &slidingWindow) : marginPolicy(slidingWindow) {}
	    virtual ~meanMarginPolicy() {}

	    template<typename V>
	    Flow::DataPtr<Flow::Vector<V> > setMean( Flow::Vector<V>& vec) const {
		Flow::DataPtr<Flow::Data > data;
		Flow::DataPtr<Flow::Vector<V> > currentData;

		// number of present past and future frames
		size_t validFirst = this->slidingWindow_.pastSize();
		size_t validLast = this->slidingWindow_.futureSize();

		// resize the vector and initialize it (with zeros)
		this->slidingWindow_.getClosest(0, data);
		if ( vec.size() != ((Flow::DataPtr<Flow::Vector<V> >) data)->size() ) {
		    vec.resize( ((Flow::DataPtr<Flow::Vector<V> >) data)->size() );
		}
		std::fill (vec.begin(),vec.end(), (V) 0);

		// sum up all values
		for (int i = - (int) validFirst; i <= (int) validLast;  i++) {
		    this->slidingWindow_.get(i, data);
		    currentData = (Flow::DataPtr<Flow::Vector<V> >) data;

		    std::transform(currentData.get()->begin(), currentData.get()->end(), vec.begin(), vec.begin(),
			    std::plus<V>() );
		} // End for sum up

		// ... normalize the sum to get the mean Values
		std::transform(vec.begin(), vec.end(), vec.begin(),
			std::bind2nd(std::divides<V>(), (validLast + validFirst +1) ));

		return (Flow::DataPtr<Flow::Vector<V> >) (&vec);
	    }

	    virtual void get(int relativeIndex, T &out) const {
		Flow::Vector<f32>* meanVec = new Flow::Vector<f32>();
		out = setMean(
			*
			meanVec
			);
	    }
	};

	/** Creates non-existing elements (all zero)
	 *  (Type of the Data structure T is: Flow::DataPtr<Flow::Vector<f32>)
	 */
	struct zeroMarginPolicy : public marginPolicy {
	    zeroMarginPolicy(const SlidingWindow &slidingWindow) : marginPolicy(slidingWindow) {}
	    virtual ~zeroMarginPolicy() {}

	    template<typename V>
	    Flow::DataPtr<Flow::Vector<V> > setZero( Flow::Vector<V>& vec) const {
		Flow::DataPtr<Flow::Data > data;

		// resize the feature vector if necessary ...
		this->slidingWindow_.getClosest(0, data);
		if ( vec.size() != ((Flow::DataPtr<Flow::Vector<V> >) data)->size() )
		    vec.resize( ((Flow::DataPtr<Flow::Vector<V> >) data)->size() );

		// ... and set all values to zero
		std::fill (vec.begin(),vec.end(), (V) 0);

		return (Flow::DataPtr<Flow::Vector<V> >) (&vec);
	    }

	    virtual void get(int relativeIndex, T &out) const {
		Flow::Vector<f32>* zero = new Flow::Vector<f32>();
		out = setZero(*zero);
	    }
	};

	/** Creates non-existing elements (all one)
	 *  (Type of the Data structure T is: Flow::DataPtr<Flow::Vector<f32>)
	 */
	struct oneMarginPolicy : public marginPolicy {
	    oneMarginPolicy(const SlidingWindow &slidingWindow) : marginPolicy(slidingWindow) {}
	    virtual ~oneMarginPolicy() {}

	    template<typename V>
	    Flow::DataPtr<Flow::Vector<V> > setOne( Flow::Vector<V>& vec) const {
		Flow::DataPtr<Flow::Data > data;

		// resize the feature vector if necessary ...
		this->slidingWindow_.getClosest(0, data);
		if ( vec.size() != ((Flow::DataPtr<Flow::Vector<V> >) data)->size() )
		    vec.resize( ((Flow::DataPtr<Flow::Vector<V> >) data)->size() );

		// ... and set all values to one
		std::fill (vec.begin(),vec.end(), (V) 1);

		return (Flow::DataPtr<Flow::Vector<V> >) (&vec);
	    }

	    virtual void get(int relativeIndex, T &out) const {
		Flow::Vector<f32>* one = new Flow::Vector<f32>();
		out = setOne(*one);
	    }
	};

    public:
	enum MarginConditionType {
	    marginConditionNotEmpty, marginConditionPresentNotEmpty, marginConditionFull
	};
	/** Base class for margin conditions retrieving the state of the sliding window when
	 *  loading or flushing.
	 */
	class marginCondition {
	protected:
	    const SlidingWindow &slidingWindow_;
	public:
	    marginCondition(const SlidingWindow &slidingWindow) : slidingWindow_(slidingWindow) {}
	    virtual ~marginCondition() {}
	    virtual bool isSatisfied() const = 0;
	};
	/** Condition is satisfied if the sliding window contains at least one element. */
	struct notEmptyMarginCondition : public marginCondition {
	    notEmptyMarginCondition(const SlidingWindow &slidingWindow) : marginCondition(slidingWindow) {}
	    virtual ~notEmptyMarginCondition() {}
	    virtual bool isSatisfied() const { return !this->slidingWindow_.empty(); }
	};
	/** Condition is satisfied if the sliding window contains an element at the present position. */
	struct presentNotEmptyMarginCondition : public marginCondition {
	    presentNotEmptyMarginCondition(const SlidingWindow &slidingWindow) :
		marginCondition(slidingWindow) {}
	    virtual ~presentNotEmptyMarginCondition() {}
	    virtual bool isSatisfied() const { return !this->slidingWindow_.presentEmpty(); }
	};
	/** Condition is satisfied if the sliding window is full. */
	struct fullMarginCondition : public marginCondition {
	    fullMarginCondition(const SlidingWindow &slidingWindow) : marginCondition(slidingWindow) {}
	    virtual bool isSatisfied() const { return this->slidingWindow_.full(); }
	};
    private:
	size_t maxFutureSize_;
	size_t maxPastSize_;
	int indexOfPresent_;

	T removed_;
	bool removedValid_;
    private:
	int absoluteIndex(int relativeIndex) const {
	    require_(relativeIndex <= (int)maxFutureSize_ && relativeIndex >= -(int)maxPastSize_);
	    return indexOfPresent_ - relativeIndex;
	}

	/** Removes oldest element if necessary. */
	void removeOldest();
    public:
	SlidingWindow(size_t maxSize = 1, size_t right = 0) { init(maxSize, right); }

	/** initializes the sliding window
	 *    @param maxSize gives the maximum number of elements in the sliding window.
	 *    @param right number of elements in the future.
	 *    @return is false if @param maxSize <= @param right.
	 */
	bool init(size_t maxSize, size_t right);
	void clear();

	/** Adds a new element
	 *  Removes the oldest element if number of past elements exceeded maxPastSize_. (@see init).
	 */
	void add(const T &add);
	/** Simulates adding one empty element.
	 *  Present element will be the element at t+1.
	 *  Removes the oldest element if number of past elements exceeded maxPastSize_ (@see init).
	 */
	void flush();
	/** Simulates adding at least so many (n) empty elements that output (@see out) changes.
	 *  Present element will be the element at t+n.
	 *  Removes the oldest element if number of past elements exceeded maxPastSize_ (@see init).
	 *  Use this function instead of flush if one function call has to produce a valid output.
	 */
	void flushOut();

	/** returns the element at relative index @param relativeIndex.
	 *  @param relativeIndex:
	 *    - > 0 addresses the future elements,
	 *    - == 0 addresses the present element,
	 *    - < 0 addresses the past elements.
	 *  @return is true if sliding window contains an element at position @param relativeIndex.
	 */
	bool get(int relativeIndex, T &out) const;
	/** returns the closest existing element to relative index @param relativeIndex.
	 *  For further explanations @see get.
	 *  Note: assertion is raised if sliding window is empty.
	 */
	void getClosest(int relativeIndex, T &out) const;

	/** returns the present element if contained.
	 *  @return is true if sliding window contains an element at 'present' position, i.e.
	 *  sliding window is filled up so far or is not yet flushed so far.
	 */
	bool out(T &out) const { return get(0, out); }

	/** @return is false if sliding window contains an element at 'present' position. */
	bool presentEmpty() const { return indexOfPresent_ < 0 || indexOfPresent_ >= (int)size(); }

	/** @return is number of future elements in sliding window. */
	size_t futureSize() const { return std::min(std::max(indexOfPresent_, (int)0), (int)size()); }
	size_t maxFutureSize() const { return maxFutureSize_; }

	/** @return is number of past elements in sliding window. */
	size_t pastSize() const { return std::max((int)size() - std::max(indexOfPresent_ + 1, (int)0), 0); }
	size_t maxPastSize() const { return maxPastSize_; }

	/** @param removed is element removed during the last call to add or flushed.
	 *  @return is false if no element has been removed.
	 */
	bool removed(T &removed) const { if (removedValid_) removed = removed_; return removedValid_; }
	/** @return is false if no element has been removed during the last call to add or flushed. */
	bool removedValid() const { return removedValid_; }

	/** maximum number of elements set by the last call to init. */
	size_t maxSize() const { return maxFutureSize_ + maxPastSize_ + 1; }
	/** number of elements */
	size_t size() const { return std::deque<T>::size(); }
	/** @return is true if the sliding-window is empty. */
	bool empty() const { return std::deque<T>::empty(); }
	/** @return is true if the sliding-window is full. */
	bool full() const { return size() == maxSize(); }

	/** @return is the right-th element
	 *  (right = 0 is latest and right = size - 1 is the oldest element).
	 */
	const T& operator[](size_t right) const { return std::deque<T>::operator[](right); }
	T& operator[](size_t right) { return std::deque<T>::operator[](right); }

	/** @return is the latest added element */
	const T& front() const { return std::deque<T>::front(); }
	T& front() { return std::deque<T>::front(); }
	/** @return is the oldest element */
	const T& back() const { return std::deque<T>::back(); }
	T& back() { return std::deque<T>::back(); }

	/** @return is the iterator of the latest added element */
	ConstantIterator begin() const { return std::deque<T>::begin(); }
	Iterator begin() { return std::deque<T>::begin(); }
	/** @return is the iterator of the element after the oldest one */
	ConstantIterator end() const { return std::deque<T>::end(); }
	Iterator end() { return std::deque<T>::end(); }

	/** @return is the reverse iterator of the oldest element */
	ConstantReverseIterator reverseBegin() const { return std::deque<T>::rbegin(); }
	ReverseIterator reverseBegin() { return std::deque<T>::rbegin(); }
	/** @return is the reverse iterator of the element after the latest added one */
	ConstantReverseIterator reverseEnd() const { return std::deque<T>::rend(); }
	ReverseIterator reverseEnd() { return std::deque<T>::rend(); }
    };

    template<class T>
    bool SlidingWindow<T>::init(size_t maxSize, size_t right)
    {
	// special case of collecting a whole segment
	if (maxSize >= (size_t)Core::Type<s32>::max && right >= (size_t)Core::Type<s32>::max)
	    -- right;

	if (maxSize <= right)
	    return false;
	maxFutureSize_ = right;
	maxPastSize_ = maxSize - right - 1;
	clear();
	return true;
    }

    template<class T>
    void SlidingWindow<T>::clear()
    {
	std::deque<T>::clear();
	indexOfPresent_ = maxFutureSize_;
	removedValid_ = false;
    }

    template<class T>
    void SlidingWindow<T>::add(const T &add)
    {
	// add is not possibly until fully flushed or cleared
	verify(indexOfPresent_ == (int)maxFutureSize_);
	Precursor::push_front(add);
	removeOldest();
    }

    template<class T>
    void SlidingWindow<T>::flush()
    {
	if (indexOfPresent_ >= -(int)maxPastSize_) {
	    -- indexOfPresent_;
	    removeOldest();
	}
    }

    template<class T>
    void SlidingWindow<T>::flushOut()
    {
	if ((int)size() < indexOfPresent_)
	    indexOfPresent_ = (int)size();
	flush();
    }

    template<class T>
    void SlidingWindow<T>::removeOldest()
    {
	int s = size() - (indexOfPresent_ + 1);
	// note: assignment in if clause
	if ( (removedValid_ = (s > (int)maxPastSize_)) ) {
	    removed_ = back();
	    this->pop_back();
	}
    }

    template<class T>
    bool SlidingWindow<T>::get(int relativeIndex, T &out) const
    {
	int index = absoluteIndex(relativeIndex);
	if (index >= 0 && index < (int)size()) {
	    out = operator[](index);
	    return true;
	}
	return false;
    }

    template<class T>
    void SlidingWindow<T>::getClosest(int relativeIndex, T &out) const
    {
	require_(!empty());
	int index = std::min(std::max(absoluteIndex(relativeIndex), 0), (int)size() - 1);
	out = operator[](index);
    }

} // namespace Signal

#endif // _SIGNAL_SLIDING_WINDOW_HH
