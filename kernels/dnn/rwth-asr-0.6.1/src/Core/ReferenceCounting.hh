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
// $Id: ReferenceCounting.hh 6218 2006-11-14 10:32:18Z rybach $

#ifndef _CORE_REFERENCE_COUNTING
#define _CORE_REFERENCE_COUNTING

#include "Assertions.hh"
#include "Types.hh"

namespace Core {

    template <class T> class Ref;
    class WeakRefBase;

    /**
     * Base class for reference-counted objects.
     *
     * To enable reference counting for a class it need to inhert this
     * class publically.
     */

    class ReferenceCounted {
    private:
	template <class T> friend class Ref;
	template <class T> friend class WeakRef;
	friend class ReferenceManager;
	mutable u32 referenceCount_;
	explicit ReferenceCounted(u32 rc) : referenceCount_(rc) {}

	static inline ReferenceCounted *sentinel() {
	    static ReferenceCounted sentinel_(1);
	    return &sentinel_;
	}
	static bool isSentinel(const ReferenceCounted *object) {
	    return object == sentinel();
	}
	static bool isNotSentinel(const ReferenceCounted *object) {
	    return object != sentinel();
	}
    protected:
	virtual ~ReferenceCounted();
    public:
	ReferenceCounted() : referenceCount_(0) {}
	ReferenceCounted(const ReferenceCounted&) : referenceCount_(0) {}
	ReferenceCounted &operator= (const ReferenceCounted&) { return *this; }

	u32 refCount() const { return referenceCount_; }
	void acquireReference() const { ++referenceCount_; }
	bool releaseReference() const { return (!--referenceCount_); }
	void acquireWeakReference(WeakRefBase *) const;
	void releaseWeakReference(WeakRefBase *) const;
	void free() const;
    };

    template <class> class WeakRef;

    /**
     * Class template for smart pointers using intrusive
     * reference-counting.
     *
     * Naming: Concrete smart pointer types should have a "Ref" suffix:
     * e.g. typedef Ref<Foo> FooRef;
     * (Obviously calling this class "Ref" instead of more
     * correcly "Reference" deviates from the naming conventions.
     * However since this class is used extensively a shorter name is
     * mandated.  "Ref" is analogious to the "REF" keyword in
     * Modula-3.)
     *
     * @c T should be a decendent of ReferenceCounted.
     *
     * Note: Virtual derivation from ReferenceCounted is not allowed
     * currently.  Reasons are many but the most imporatant one is
     * that we use a sentinel instead of null to represent a reference
     * is invalid (i.e. void).  Advantage of using a sentinel is that
     * in the most frequently used functions (e.g. copy constructor)
     * the sentinel can be treated as if it was a normal object.  Thus
     * we save a few if-clauses.  Once we use a sentinel, the static
     * sentinel object (delivered by ReferenceCounted::sentinel()) of
     * type ReferenceCounted needs to get casted to the type
     * Object. It is a dirty solution but is the only way to make the
     * usage of the sentinel fast.  In order to be independent of the
     * order of derivation in case of multiple inheritance - i.e. if
     * ReferenceCounted is the first precursor class or not -
     * static_cast is used to cast the sentinel to the type Object.
     * Usage of reinterpret_cast would work only if we could ensure
     * that ReferenceCounted was the very first class in the type
     * Object.  Finally, closing the series of arguments, virtual
     * derivation from ReferenceCounted is not permitted by
     * static_cast.  It does not mean that ReferenceCounting and
     * virtual inheritance does not go along at all: Make your virtual
     * inheritance hiearchy first, and derive only the most specific
     * class from ReferenceCounted.
     */

    template <class T /* -> ReferenceCounted */>
    class Ref {
    private:
	typedef T Object;
	Object *object_;
	static Object* sentinel() { return static_cast<Object*>(Object::sentinel()); }

	/**
	 * Takes over the value of @param o.
	 * - Correctly handles self assignment, i.e. if @param o ==
	 *   object_.
	 * - @see the function ref() for how to create a Ref object
	 *   conveniently from a pointer.
	 */
	void set(Object *o) {
	    const Object *old = object_;
	    ++(object_ = o)->referenceCount_;
	    if (!(--old->referenceCount_)) {
		verify_(Object::isNotSentinel(old));
		old->free();
	    }
	}
    public:
	Ref() : object_(sentinel()) { ++object_->referenceCount_; }

	explicit Ref(Object *o) : object_(o) {
	    require_(o);
	    ++object_->referenceCount_;
	}

	/**
	 * Copy Constructor for the Type Ref<T>
	 *
	 * @warning: Implementaion of this constructor is necesseary
	 * although the template copy constructor implements exactly
	 * the same function.  If this constructor is not defined, the
	 * compiler creates a default copy constructor instead of
	 * using the template one.
	 */
	Ref(const Ref &r) : object_(r.object_) {
	    ++object_->referenceCount_;
	}
	/**
	 * Template Copy Constuctor.
	 *
	 * Using a template allows automatic conversion in following cases:
	 * - from non-const to const reference
	 * - from derived to precursor class reference
	 * For all classes S which are not derived from T, this
	 * constructor will fail to compile.
	 */
	template<class S> Ref(const Ref<S> &r) : object_(r._get()) {
	    ++object_->referenceCount_;
	}
	template<class S> Ref(const WeakRef<S> &r) : object_(r._get()) {
	    ++object_->referenceCount_;
	}

	~Ref() {
	    if (!(--object_->referenceCount_)) {
		verify_(Object::isNotSentinel(object_));
		object_->free();
	    }
	}

	Ref &operator=(const Ref &rhs) {
	    set(rhs.object_);
	    return *this;
	}
	template<class S> Ref &operator=(const Ref<S> &rhs) {
	    set(rhs._get());
	    return *this;
	}
	template<class S> Ref &operator=(const WeakRef<S> &rhs) {
	    set(rhs._get());
	    return *this;
	}

	/**
	 * Reset to void.
	 * Corresponds to assigning zero to normal pointer.
	 */
	void reset() {
	    if (Object::isNotSentinel(object_)) {
		if (!(--object_->referenceCount_)) {
		    object_->free();
		}
		++(object_ = sentinel())->referenceCount_;
	    }
	}

	/** Test for identity */
	bool operator==(const Ref &r) const {
	    return object_ == r.object_;
	}
	template<class S> bool operator==(const Ref<S> &r) const {
	    return object_ == r._get();
	}
	template<class S> bool operator==(const WeakRef<S> &r) const {
	    return object_ == r._get();
	}
	/** Test for non-identity */
	bool operator!=(const Ref &r) const {
	    return object_ != r.object_;
	}
	template<class S> bool operator!=(const Ref<S> &r) const {
	    return object_ != r._get();
	}
	template<class S> bool operator!=(const WeakRef<S> &r) const {
	    return get() != r._get();
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
	/** Value of internal pointer.
	 * @warning Do not use this function. It is only used in the
	 * template copy contructor and the template operator= functions.
	 */
	Object* _get() const { return object_; }
    };

    /** Convenience constructors for Ref smart pointers. */
    template <class T>
    inline Ref<T> ref(T *o) { return Ref<T>(o); }



    /**
     * Base class of the WeakRef template.
     *
     * This is needed to have an unparametrized interface
     * used by the ReferenceManager
     */
    class WeakRefBase
    {
    public:
	virtual ~WeakRefBase() {}
    private:
	virtual void invalidate() = 0;
	friend class ReferenceManager;
    };


    /**
     * Class template for smart pointers provining weak referencing.
     *
     * A weak reference does not prevent the target object from being
     * deleted.  (In so far it is much like a regular pointer.)
     * Instead, the weak reference will be reset when the target
     * object is deleted.  Weak reference are useful to avoid cyclic
     * references which would normally lead to memory leakage.  You
     * always have to test a weak reference before using it.
     * Obviously there are issues with thread safety here!  Also note
     * that weak references are slower that normal references (at
     * least in the current implementation).
     */
    template <class T /* -> ReferenceCounted */>
    class WeakRef : public WeakRefBase {
    private:
	typedef T Object;
	Object *object_;
	static Object* sentinel() { return static_cast<Object*>(Object::sentinel()); }

	void release() {
	    object_->releaseWeakReference(this);
	}
	void acquire() {
	    object_->acquireWeakReference(this);
	}
	void set(Object *o) {
	    release();
	    object_ = o;
	    acquire();
	}
	virtual void invalidate() {
	    object_ = sentinel();
	}
	friend class ReferenceManager;

    public:
	WeakRef() : object_(sentinel()) {}

	explicit WeakRef(Object *o) : object_(o) {
	    require_(o);
	    acquire();
	}

	/**
	 * Copy Constructor for the type WeakRef<T>
	 *
	 * @warning: Implementaion of this constructor is necesseary
	 * although the template copy constructor implements exactly
	 * the same function.  If this constructor is not defined, the
	 * compiler creates a default copy constructor instead of
	 * using the template one.
	 */
	WeakRef(const WeakRef &r) : object_(r.object_) {
	    acquire();
	}

	/**
	 * Template Copy Constuctor.
	 *
	 * Using a template allows automatic conversion in following cases:
	 * - from non-const to const reference
	 * - from derived to precursor class reference
	 * For all classes S which are not derived from T, this
	 * constructor will fail to compile.
	 */
	template<class S> WeakRef(const WeakRef<S> &r) : object_(r._get()) {
	    acquire();
	}
	template<class S> WeakRef(const Ref<S> &r) : object_(r._get()) {
	    acquire();
	}

	~WeakRef() {
	    release();
	}

	WeakRef &operator=(const WeakRef &rhs) {
	    set(rhs.object_);
	    return *this;
	}
	template<class S> WeakRef &operator=(const WeakRef<S> &rhs) {
	    set(rhs._get());
	    return *this;
	}
	template<class S> WeakRef &operator=(const Ref<S> &rhs) {
	    set(rhs._get());
	    return *this;
	}

	/**
	 * Reset to void.
	 * Corresponds to assigning zero to normal pointer.
	 */
	void reset() {
	    release();
	    object_ = sentinel();
	}

	/** Test for identity */
	bool operator==(const WeakRef &r) const {
	    return object_ == r.object_;
	}
	template<class S> bool operator==(const WeakRef<S> &r) const {
	    return object_ == r._get();
	}
	template<class S> bool operator==(const Ref<S> &r) const {
	    return object_ == r._get();
	}
	/** Test for non-identity */
	bool operator!=(const WeakRef &r) const {
	    return object_ != r.object_;
	}
	template<class S> bool operator!=(const WeakRef<S> &r) const {
	    return object_ != r._get();
	}
	template<class S> bool operator!=(const Ref<S> &r) const {
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

	/** Explicit conversion to normal pointer. */
	Object* get() const {
	    return Object::isNotSentinel(object_) ? object_ : 0;
	}

	/** Value of internal pointer.
	 * @warning Do not use!	*/
	Object* _get() const {
	    return object_;
	}
    };

} // Core

#endif // _CORE_REFERENCE_COUNTING
