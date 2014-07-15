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
#ifndef _T_FSA_SEMIRING_HH
#define _T_FSA_SEMIRING_HH

#include <iostream>
#include <Core/BinaryStream.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/StringUtilities.hh>

#include "Types.hh"

namespace Ftl {

    template<class _Weight>
    struct Accumulator {
	virtual ~Accumulator() {}

	virtual void feed(const _Weight&) = 0;
	virtual _Weight get() const = 0;
    };

    template<class _Weight>
    class Semiring : public Core::ReferenceCounted {
	typedef Semiring<_Weight> Self;
	typedef const Semiring<_Weight> ConstSelf;
    public:
	typedef _Weight Weight;
	typedef Core::Ref<Self> Ref;
	typedef Core::Ref<const Self> ConstRef;
	typedef Ftl::Accumulator<_Weight> Accumulator;
    private:
	Fsa::Property properties_;
    public:
	Semiring() : properties_() {}
	Semiring(Fsa::Property properties) : properties_(properties) {}
	virtual ~Semiring() {}

	Fsa::Property getProperties(Fsa::Property properties = Fsa::PropertyAll) const
	    { return properties_ & properties; }
	bool hasProperty(Fsa::Property property) const
	    { return getProperties(property) & property; }

	virtual std::string name() const = 0;
	virtual _Weight create() const = 0;
	virtual _Weight clone(const _Weight &a) const = 0;
	virtual _Weight invalid() const = 0;
	virtual _Weight zero() const = 0;
	virtual _Weight one() const = 0;
	virtual _Weight max() const = 0;
	virtual _Weight extend(const _Weight &a, const _Weight &b) const = 0;
	virtual _Weight collect(const _Weight &a, const _Weight &b) const = 0;
	virtual _Weight invCollect(const _Weight &a, const _Weight &b) const
		{ std::cerr << "method \"invCollect\" is not supported" << std::endl; return invalid(); }
	virtual bool hasInvCollect() const { return false; }
	virtual _Weight invert(const _Weight &a) const = 0;
	virtual int compare(const _Weight &a, const _Weight &b) const = 0;
	virtual size_t hash(const _Weight &a) const = 0;

	virtual std::string asString(const _Weight&) const = 0;
	virtual _Weight fromString(const std::string &str) const = 0;
	virtual bool read(_Weight &a, Core::BinaryInputStream &i) const = 0;
	virtual bool write(const _Weight &a, Core::BinaryOutputStream &o) const = 0;

	virtual void compress(Core::Vector<u8> &stream, const _Weight &a) const = 0;
	virtual _Weight uncompress(Core::Vector<u8>::const_iterator &) const = 0;
	virtual size_t compressedSize() const = 0;

	virtual _Weight defaultWeight() const
	    { return one(); }
	virtual bool isDefault(const _Weight &a) const
	    { return compare(a, defaultWeight()) == 0; }
	virtual std::string describe(const _Weight &a, Fsa::Hint hints = 0) const
	    { return asString(a); }
	/**
	 * Accumulator class
	 * and default implementations for extend and collect
	 **/
    protected:
	struct AnchoredAccumulator : public Ftl::Accumulator<_Weight> {
	    typedef _Weight (ConstSelf::*Function)(const _Weight &a, const _Weight &b) const;
	    const Self *self;
	    Function f;
	    _Weight w;
	    AnchoredAccumulator(const Self *self, Function f, const _Weight &init) :
		self(self), f(f), w(init) { self->acquireReference(); }
	    virtual ~AnchoredAccumulator() { if (self->releaseReference()) self->free(); }
	    void feed(const _Weight &a) { w = (self->*f)(w, a); }
	    _Weight get() const { return w; }
	};
    public:
	virtual Accumulator * getExtender(const _Weight &initial) const { return new AnchoredAccumulator(this, &Self::extend, initial); }
	Accumulator * getExtender() const { return getExtender(one()); }
	virtual Accumulator * getCollector(const _Weight &initial) const { return new AnchoredAccumulator(this, &Self::collect, initial); }
	Accumulator * getCollector() const { return getCollector(zero()); }
    };


    template<class _Weight>
    class SemiringAdapter : public Semiring<_Weight> {
    protected:
	typedef Semiring<_Weight> Precursor;
    public:
	SemiringAdapter() :
	    Precursor(Fsa::PropertySemiringAll & ~Fsa::PropertySemiringIo) {}
	SemiringAdapter(Fsa::Property properties) :
	    Precursor(properties) {}

	virtual _Weight invalid() const = 0;

	virtual _Weight create() const
	    { std::cerr << "method \"create\" is not supported" << std::endl; return invalid(); }
	virtual _Weight clone(const _Weight &a) const
	    { std::cerr << "method \"clone\" is not supported" << std::endl; return invalid(); }
	virtual std::string name() const
	    { std::cerr << "method \"name\" is not supported" << std::endl; return std::string(); }
	virtual _Weight zero() const
	    { std::cerr << "method \"zero\" is not supported" << std::endl; return invalid(); }
	virtual _Weight one() const
	    { std::cerr << "method \"one\" is not supported" << std::endl; return invalid(); }
	virtual _Weight max() const
	    { std::cerr << "method \"max\" is not supported" << std::endl; return invalid(); }
	virtual _Weight extend(const _Weight &a, const _Weight &b) const
	    { std::cerr << "method \"extend\" is not supported" << std::endl; return invalid(); }
	virtual _Weight collect(const _Weight &a, const _Weight &b) const
	    { std::cerr << "method \"collect\" is not supported" << std::endl; return invalid(); }
	virtual _Weight invert(const _Weight &a) const
	    { std::cerr << "method \"invert\" is not supported" << std::endl; return invalid(); }
	virtual int compare(const _Weight &a, const _Weight &b) const
	    { std::cerr << "method \"compare\" is not supported" << std::endl; return 0; }
	virtual size_t hash(const _Weight &a) const
	    { std::cerr << "method \"hash\" is not supported" << std::endl; return 0; }
	virtual std::string asString(const _Weight&) const
	    { std::cerr << "method \"asString\" is not supported" << std::endl; return std::string(); }
	virtual _Weight fromString(const std::string &str) const
	    { std::cerr << "method \"fromString\" is not supported" << std::endl; return invalid(); }
	virtual bool read(_Weight &a, Core::BinaryInputStream &i) const
	    { std::cerr << "method \"read\" is not supported" << std::endl; return false; }
	virtual bool write(const _Weight &a, Core::BinaryOutputStream &o) const
	    { std::cerr << "method \"write(binary)\" is not supported" << std::endl; return false; }
	virtual void compress(Core::Vector<u8> &stream, const _Weight &a) const
	    { std::cerr << "method \"compress()\" is not supported" << std::endl; }
	virtual _Weight uncompress(Core::Vector<u8>::const_iterator &) const
	    { std::cerr << "method \"uncompress()\" is not supported" << std::endl; return invalid(); }
	virtual size_t compressedSize() const
	    { std::cerr << "method \"compressedSize()\" is not supported" << std::endl; return 0; }
    };

} // namespace Ftl

#endif // _T_FSA_SEMIRING_HH
