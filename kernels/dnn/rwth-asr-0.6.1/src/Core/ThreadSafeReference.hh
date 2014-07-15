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
#ifndef _CORE_THREAD_SAFE_REFERENCE_
#define _CORE_THREAD_SAFE_REFERENCE_

#include "Assertions.hh"
#include "Types.hh"
#include "Thread.hh"

namespace Core {

    /** Base class for reference-counted objects used in a multithreaded environment.
     *
     * To enable reference counting for a class it need to inhert this
     * class publically.
     *
     * Remark: ThreadSafeReferenceCounted is by intention not derived from ReferenceCounted
     *   to forbit access of thread safe reference count by a not thread safe Core::Ref object.
     */
    class ThreadSafeReferenceCounted {
	template <class T> friend class TsRef;
    private:
	mutable u32 referenceCount_;
	mutable Core::Mutex mutex_;
    private:
	static inline ThreadSafeReferenceCounted *sentinel() {
	    static ThreadSafeReferenceCounted sentinel_(1);
	    return &sentinel_;
	}
	static bool isSentinel(const ThreadSafeReferenceCounted *object) {
	    return object == sentinel();
	}
	static bool isNotSentinel(const ThreadSafeReferenceCounted *object) {
	    return object != sentinel();
	}
    protected:
	virtual void free() const { verify_(isNotSentinel(this)); delete this; }

	void lock() const { mutex_.lock(); }
	void release() const { mutex_.release(); }
	void increment() const  { ++referenceCount_; }
	bool decrement() const { return ((--referenceCount_) == 0); }
    protected:
	explicit ThreadSafeReferenceCounted(u32 rc) : referenceCount_(rc) {}
    public:
	ThreadSafeReferenceCounted() : referenceCount_(0) {}
	ThreadSafeReferenceCounted(const ThreadSafeReferenceCounted&) : referenceCount_(0) {}
	ThreadSafeReferenceCounted &operator= (const ThreadSafeReferenceCounted&) { return *this; }
	/** Constructor for static objects. */
	virtual ~ThreadSafeReferenceCounted() {} // calling mutex_.lock() might be useful for debugging

	u32 refCount() const { return referenceCount_; }
    };

    /**
     * Class template for smart pointers using intrusive
     * reference-counting in a multithreaded environment.
     *
     * For more important comments @see Core::Ref
     */
    template <class T /* -> ThreadSafeReferenceCounted */>
    class TsRef {
    protected:
	typedef T Object;
	Object *object_;
	static Object* sentinel() { return static_cast<Object*>(Object::sentinel()); }
    protected:
	inline void reference() {
	    object_->lock();
	    object_->increment();
	    object_->release();
	}
	inline void unreference() {
	    object_->lock();
	    if (object_->decrement()) {
		verify_(Object::isNotSentinel(object_));
		object_->release();
		object_->free();
		(object_ = sentinel())->increment();
	    } else
		object_->release();
	}

	/** Takes over the value of @param o.
	 *  -Handles self assignment, i.e. if @param o == object_.
	 *  -@see the function 'tsRef' about how to create a TsRef object
	 *   conveniently from a pointer.
	 */
	void set(Object *o) {
	    if (object_ != o) {
		unreference();
		object_ = o;
		reference();
	    }
	}
    public:
	TsRef() : object_(sentinel()) { reference(); }
	explicit TsRef(Object *o) : object_(o) { require_(object_); reference(); }
	/** Copy Constructor for the Type Ref<T>
	 *  @warning: Implementaion of this constructor is necesseary although the template
	 *  copy constructor implements exactly the same. If this constructor is not implemented,
	 *  the compiler creates a default copy constructor instead of using the template one.
	 */
	TsRef(const TsRef &r) : object_(r.object_) { reference(); }
	/** Template Copy Constuctor
	 *  Using templates solves the problem of casting ...
	 *  -to const pointer
	 *  -to precursor class pointer.
	 *  For all classes S which are not derived from T, this constructor will not compile.
	 */
	template<class S> TsRef(const TsRef<S> &r) : object_(r._get()) { reference(); }
	~TsRef() { unreference(); }

	TsRef &operator=(const TsRef &rhs) {
	    set(rhs.object_);
	    return *this;
	}
	template<class S> TsRef &operator=(const TsRef<S> &rhs) {
	    set(rhs._get());
	    return *this;
	}

	/** Reset to void.
	 *  Corresponds to assigning zero to normal pointer.
	 */
	void reset() { set(sentinel()); }

	/** Test for identity */
	bool operator==(const TsRef &r) const {
	    return object_ == r.object_;
	}
	template<class S> bool operator==(const TsRef<S> &r) const {
	    return object_ == r._get();
	}
	/** Test for non-identity */
	bool operator!=(const TsRef &r) const {
	    return object_ != r.object_;
	}
	template<class S> bool operator!=(const TsRef<S> &r) const {
	    return object_ != r._get();
	}

	/** Test for non-voidness. */
	operator bool() const {
	    return Object::isNotSentinel(object_);
	}
	/** Test for voidness. */
	bool operator!() const {
	    return Object::isSentinel(object_);
	}

	Object &operator* () const {
	    require_(Object::isNotSentinel(object_));
	    return *object_;
	}
	Object *operator-> () const {
	    require_(Object::isNotSentinel(object_));
	    return object_;
	}

	/** Explicit conversion to normal pointer.
	 * @warning This defeats the reference counting mechanism.
	 * Use with caution!
	 */
	Object* get() const {
	    return Object::isNotSentinel(object_) ? object_ : 0;
	}
	/** Copies the value of the pointer.
	 * @warning Do not use this function. It is only used in the
	 * template copy contructor and the template operator= functions.
	 */
	Object* _get() const { return object_; }
    };

    /** Convenience constructors for TsRef smart pointers. */
    template <class T>
    inline TsRef<T> tsRef(T *o) { return TsRef<T>(o); }

} // namespace Core

#endif // _CORE_THREAD_SAFE_REFERENCE_
