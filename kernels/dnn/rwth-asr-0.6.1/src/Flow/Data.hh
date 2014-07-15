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
#ifndef _FLOW_DATA_HH
#define _FLOW_DATA_HH

#include "Types.hh"
#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>
#include <Core/ThreadSafeReference.hh>

namespace Flow {

    class Datatype;
    class Link;
    class AbstractNode;

    /** Base class for Data class in Flow
     *  Features:
     *   -Multithread safe reference counting. I.e. objects can be accessed by DataPtr.
     *   -Polymorfic I/O operation.
     */
    class Data : public Core::ThreadSafeReferenceCounted {
	typedef Data Self;
	typedef Core::ThreadSafeReferenceCounted Precursor;
	template <class T> friend class Core::TsRef;
	template <class T> friend class DataPtr;
	friend class Datatype;
	friend class Link;
	friend class Node;
    private:
	const Datatype *datatype_;
    private:
	Data(u32 rc) : Precursor(rc), datatype_(type()) {}
	static const Datatype *type();

	/** sentinel for void pointer
	 *  Note: Precursor::Sentinel is overriden to create a sentinel of Data
	 *  instead the precursor class. In this way, all the sentinels are Data objects and can be
	 *  handled equally. @see dump method as an example.
	 */
	static inline Data *sentinel() {
	    static Data sentinel_(1);
	    return &sentinel_;
	}
    protected:
	virtual void free() const;

	void lock() const { Precursor::lock(); }
	void release() const { Precursor::release(); }
	void increment() const  { Precursor::increment(); }
	bool decrement() const { return Precursor::decrement(); }
    protected:
	Data(const Datatype *dt) : datatype_(dt) {}

	Core::XmlOpen xmlOpen() const;
	Core::XmlClose xmlClose() const;
    public:
	Data() : datatype_(type()) {}
	Data(const Data& data) : datatype_(data.datatype_) {}
	virtual ~Data() {}

	// copy
	virtual Data* clone() const { return new Data(*this); }

	// datatype
	const Datatype* datatype() const { return datatype_; }

	// equality
	virtual bool operator==(const Data &other) const { defect(); return false; }

	// enable support for ascii output (dump filter)
	virtual Core::XmlWriter& dump(Core::XmlWriter&) const;
	virtual	bool read(Core::BinaryInputStream &) { defect(); return false; }
	virtual bool write(Core::BinaryOutputStream &) const { defect(); return false; }
    public:
	/** End of stream (EOS) sentinel */
	static inline Data *eos() {
	    static Data sentinelEos_(1);
	    return &sentinelEos_;
	}
	/** Out of data (OOD) sentinel
	 *  This sentinel has been introduced to support partial processing of longer
	 *  external input sequence. Nodes supporting OOD handling freeze their
	 *  status and forward the OOD object on recieving an OOD object. Currently,
	 *  only single input single output nodes support OOD. Extending nodes with
	 *  several input and/or output ports requires a larger redesign. Possible
	 *  alternative to the OOD concept is using multithreading.
	 */
	static inline Data *ood() {
	    static Data sentinelOod_(1);
	    return &sentinelOod_;
	}

	static bool isSentinel(const ThreadSafeReferenceCounted *object) {
	    return object == sentinel() || object == eos() || object == ood();
	}
	static bool isNotSentinel(const ThreadSafeReferenceCounted *object) {
	    return object != sentinel() && object != eos() && object != ood();
	}
    };

    /** Class template for Data smart pointers
     *  Extends the features of Core::TsRef by:
     *    -up and down cast using dynamic_cast
     *    -makePrivate() which ensures that the object is referenced only by this DataPtr.
     *
     *  Note: for comments more important, please @see Core::Ref and Core::TsRef.
     *
     *  Remark: Objects referenced by DataPtr can be referenced by Core::TsRef as well.
     *    However, in certain applications multithread safety is not important, thus
     *    usage of the mutex_ is wasted time. In these application, thread safe object
     *    needs to be copied into a object derived from ReferenceCounted.
     *    For an examles @see Speech::Feature.
     */
    template<class T> class DataPtr : public Core::TsRef<T> {
	typedef Core::TsRef<T> Precursor;
	typedef typename Precursor::Object Object;
	friend class Link;
	friend class AbstractNode;
    private:
	void take(Data *p) {
	    this->unreference();
	    Precursor::object_ = convert(p);
	}

	template<class S> static Object* convert(S *o) {
	    if (Object::isSentinel(o))
		return static_cast<Object*>(o);
	    Object* result = dynamic_cast<Object*>(o);
	    return (result == 0) ? Precursor::sentinel() : result;
	}
    public:
	DataPtr() : Precursor() {}
	DataPtr(const DataPtr &p) : Precursor(p) {}
	template <class S> explicit DataPtr(const DataPtr<S> &p) {
	    Precursor::object_ = convert(p._get());
	    Precursor::reference();
	}
	explicit DataPtr(Object &d) {
	    Precursor::object_ = dynamic_cast<T*>(d->clone());
	    ensure_(Precursor::object_ != 0);
	    Precursor::reference();
	}
	explicit DataPtr(Object *d) : Precursor(d) {}
	virtual ~DataPtr() {}

	template<class S> DataPtr &operator=(const DataPtr<S> &rhs) {
	    this->set(convert(rhs._get()));
	    return *this;
	}

	/** Test for identity
	 *  Remark: overriding of the precursor function is necessary because
	 *  in comparisons the compiler uses Precursor::operator bool() instead of Precursor::operator==.
	 */
	bool operator==(const DataPtr &r) const {
	    return Precursor::operator==(r);
	}
	template<class S> bool operator==(const DataPtr<S> &r) const {
	    return Precursor::operator==(r);
	}
	bool operator==(const Data *o) const { return Precursor::object_ == o; }

	Object* get() const { return Precursor::_get(); }

	Object* release() {
	    Object* o = Precursor::object_;
	    Precursor::object_ = this->sentinel();
	    Precursor::reference();
	    if (Object::isSentinel(o)) {
		o->decrement();
		return 0;
	    } else {
		if (!o->decrement()) {
		    // If reference count of 'd' is larger than 0 then
		    // the object may get destroyed by one of the other owners.
		    // Use makePrivate() before calling release.
		    defect();
		}
		return o;
	    }
	}

	void makePrivate() {
	    if (Object::isSentinel(Precursor::object_)) return;
	    Precursor::object_->lock();
	    Object *o = Precursor::object_;
	    if (o->refCount() > 1) {
		Precursor::object_ = dynamic_cast<Object*>(o->clone());
		ensure_(Precursor::object_ != 0);
		Precursor::object_->increment();
		o->decrement();
	    }
	    o->release();
	}
    };

    /** Convenience constructors for DataPtr smart pointers. */
    template <class T>
    inline DataPtr<T> dataPtr(T *o) { return DataPtr<T>(o); }

} // namespace Flow

namespace Core {
    template <typename T>
    class NameHelper<Flow::DataPtr<T> > : public std::string {
    public:
	NameHelper() : std::string(Core::NameHelper<T>() + "-pointer") {}
    };
} // namespace Core

#endif // _FLOW_DATA_HH
