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
#ifndef _FLF_CORE_WEIGHT_HH
#define _FLF_CORE_WEIGHT_HH

#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>

#include "Types.hh"


namespace Flf {

    typedef f32 Score;
    typedef Core::Vector<Score> ScoreList;
    typedef size_t ScoreId;
    typedef Core::Vector<ScoreId> ScoreIdList;

    typedef f64 Probability;
    typedef Core::Vector<Probability> ProbabilityList;

    /**
     * fixed dimension scores + reference counting
     *
     * tradeoff between security/comfort, speed, and memory requirements
    **/
    class Semiring;
    class ScoresRef;

    class Scores {
	friend class Semiring;
	friend class ScoresRef;
    public:
	typedef Score * iterator;
	typedef const Score * const_iterator;
    protected:
	Scores() : nRef_(0) {}
	Scores(const Scores &a) : nRef_(0) {}
    public:

#ifdef MEM_DBG
	static u32 nScores;
	void * operator new(size_t size, size_t n);
	void operator delete(void *ptr);
#else
	void * operator new(size_t size, size_t n) {
	    return ::malloc(size + n * sizeof(Score));
	}
	void operator delete(void *ptr) {
	    ::free(ptr);
	}
#endif

	iterator begin() { return reinterpret_cast<iterator>(this+1); }
	const_iterator begin() const { return reinterpret_cast<const_iterator>(this+1); }
	// end = begin() + n

	Score& operator[] (ScoreId i) { return begin()[i]; }
	Score  operator[] (ScoreId i) const { return begin()[i]; }
	Score get(ScoreId i) const { return begin()[i]; }
	void set(ScoreId i, const Score &s) const { const_cast<Scores*>(this)->begin()[i] = s; }
	void add(ScoreId i, const Score &s) const { const_cast<Scores*>(this)->begin()[i] += s; }
	void multiply(ScoreId i, const Score &s) const { const_cast<Scores*>(this)->begin()[i] *= s; }

	// linear combination
	// Remark:
	// 1) 0.0 * inf := 0.0, i.e. a scale of 0.0 masks the weight
	// 2) s * Zero := Zero, s * One = One, i.e. Zero and One are defined independently from the scale
	Score project(const ScoreList &scales) const;

	/*
	  ReferenceCounting
	*/
    private:
	mutable u32 nRef_;
	Scores(u32 nRef) : nRef_(nRef) {}
	static inline Scores *sentinel() {
	    static Scores sentinel_(u32(1));
	    return &sentinel_;
	}
	static inline bool isSentinel(const Scores *obj) {
	    return obj == sentinel();
	}
	static inline bool isNotSentinel(const Scores *obj) {
	    return obj != sentinel();
	}
    public:
	u32 refCount() const { return nRef_; }
	void acquireReference() const { ++nRef_; }
	bool releaseReference() const { return (!--nRef_); }
    };

    class ScoresRef {
    private:
	Scores *obj_;
    private:
	inline void set(Scores *obj) {
	    obj->acquireReference();
	    if (obj_->releaseReference()) {
		verify_(Scores::isNotSentinel(obj_));
		delete obj_;
	    }
	    obj_ = obj;
	}
    public:
	ScoresRef() : obj_(Scores::sentinel()) {
	    obj_->acquireReference();
	}
	explicit ScoresRef(Scores *obj) : obj_(obj) {
	    require_(obj);
	    obj_->acquireReference();
	}
	ScoresRef(const ScoresRef &ref) : obj_(ref.obj_) {
	    obj_->acquireReference();
	}
	~ScoresRef() {
	    if (obj_->releaseReference()) {
		verify_(obj_ != Scores::sentinel());
		delete obj_;
		//		::free(obj_);
	    }
	}
	operator bool() const {
	    return Scores::isNotSentinel(obj_);
	}
	ScoresRef & operator=(const ScoresRef &ref) {
	    set(ref.obj_);
	    return *this;
	}
	bool operator==(const ScoresRef &ref) const {
	    return obj_ == ref.obj_;
	}
	bool operator!=(const ScoresRef &ref) const {
	    return obj_ != ref.obj_;
	}
	bool operator!() const {
	    return Scores::isSentinel(obj_);
	}
	Scores & operator* () const {
	    require_(Scores::isNotSentinel(obj_));
	    return *obj_;
	}
	Scores * operator-> () const {
	    require_(Scores::isNotSentinel(obj_));
	    return obj_;
	}
	Scores * get() const {
	    return Scores::isNotSentinel(obj_) ? obj_ : 0;
	}
	void reset() {
	    set(Scores::sentinel());
	}
    };

} // namespace Flf

#endif // _FLF_CORE_WEIGHT_HH
