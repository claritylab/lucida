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
#ifndef _CORE_IO_REF_HH
#define _CORE_IO_REF_HH

#include "ReferenceCounting.hh"
#include "BinaryStream.hh"
#include "XmlStream.hh"
#include <map>
#include <string>

namespace Core {

    /**
     * IoRef extends Ref by allowing reading and writing to binary streams.
     *
     * WARNING: This does NOT implement object serialisation (i.e. an object
     * pointed to through two references that are both saved will be saved
     * twice, and later restored as two separate copies.), the only goal of this
     * class is to allow polymorphic containers to be saved and restored.
     *
     * A class participating in polymorpic binary stream operations must, in
     * adition to the standard 'read' and 'write' methods, suport the virtual
     * member function 'typeName', that provides an identifying 'std::string', that
     * is unique among the different classes loaded through that particular
     * 'IoRef' type. The member function 'typeName' must not access the state of
     * the object in any way. (Think of it as a "static virtual".)
     *
     * All classes participating in polymorpic IO must be registered beforhand,
     * using the 'registerClass' static member function. In addition to
     * registring the IO tag, this also establishes the way to create "empty"
     * instances of the class; the constructor of the registered class is then
     * called using the arguments suplied to 'registerClass'.
     *
     * NOTE: At present only constructors with one or two arguments are
     * suported. If needed it is easy to extend.
     */
    template <class Base>
    class IoRef: public Ref<Base> {
    public:
	typedef Ref<Base> Precursor;
	typedef Base* (*CreatorFunction)();
    private:
	static std::map<std::string, CreatorFunction>& creators_()
	{
	    static std::map<std::string, CreatorFunction> creators;
	    return creators;
	}

	template <class Derived>
	static Base* creator0_() { return new Derived(); }
	template <class Derived, class T>
	static Base* creator1_() { return new Derived(arg1_<Derived, T>()); }
	template <class Derived, class T1, class T2>
	static Base* creator2_() { return new Derived(arg1_<Derived, T1>(), arg2_<Derived, T2>()); }

	template <typename Derived, typename T> static T& arg1_() {static T arg; return arg;}
	template <typename Derived, typename T> static T& arg2_() {static T arg; return arg;}
    public:
	IoRef(): Precursor() {}
	IoRef(const Ref<Base>& r): Precursor(r) {}
	template<class S> IoRef(const Ref<S> &r) : Precursor(r) {}
	explicit IoRef(Base* p): Precursor(p) {}

	IoRef& operator=(const Ref<Base>& r) { Precursor::operator=(r); return *this; }

	template <class Derived> static void registerClass()
	{
	    creators_()[((Derived*) 0)->Derived::typeName()]=creator0_<Derived>;
	}
	template <class Derived, class T> static void registerClass(const T& arg)
	{
	    arg1_<Derived, T>()=arg;
	    creators_()[((Derived*) 0)->Derived::typeName()]=creator1_<Derived, T>;
	}
	template <class Derived, class T1, class T2> static void registerClass(const T1& arg1, const T2& arg2)
	{
	    arg1_<Derived, T1>()=arg1;
	    arg2_<Derived, T2>()=arg2;
	    creators_()[((Derived*) 0)->Derived::typeName()]=creator2_<Derived, T1, T2>;
	}

	void write(BinaryOutputStream &os) { os << (*this)->typeName(); (*this)->write(os); }
	void read(BinaryInputStream &is)
	{
	    std::string tag;
	    is >> tag;
	    if (!(*this) || (*this)->typeName()!=tag) {
		CreatorFunction creator= creators_()[tag];
		hope(creator);
		(*this)= Core::ref(creator());
	    }
	    (*this)->read(is);
	}
	void dump(XmlOutputStream& xos) { (*this)->dump(xos); }
    };

}

#endif // _CORE_IO_REF_HH
