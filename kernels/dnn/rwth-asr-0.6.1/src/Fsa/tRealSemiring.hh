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
#ifndef _T_FSA_RATIONAL_SEMIRING_HH
#define _T_FSA_RATIONAL_SEMIRING_HH

#include <Core/Types.hh>

#include "tSemiring.hh"

namespace Ftl {
    template<class _Weight, typename _Type>
    class LogSemiring : public Semiring<_Weight> {
	typedef Semiring<_Weight> Precursor;
    public:
	typedef _Type Type;
	typedef Accumulator<_Weight> _Accumulator;
    protected:
	s32 tolerance_;
    public:
	LogSemiring();
	LogSemiring(s32 tolerance);
	virtual std::string name() const;
	_Weight create() const { return _Weight(); }
	_Weight clone(const _Weight &a) const { return _Weight(_Type(a)); }
	virtual _Weight invalid() const;
	virtual _Weight zero() const;
	virtual _Weight one() const;
	virtual _Weight max() const;
	virtual _Weight extend(const _Weight &a, const _Weight &b) const;
	virtual _Accumulator * getExtender(const _Weight &initial) const;
	virtual _Weight collect(const _Weight &a, const _Weight &b) const;
	virtual _Weight invCollect(const _Weight &a, const _Weight &b) const;
	virtual bool hasInvCollect() const { return true; }
	virtual _Accumulator * getCollector(const _Weight &initial) const;
	virtual _Weight invert(const _Weight &a) const;
	virtual int compare(const _Weight &a, const _Weight &b) const;

	virtual size_t hash(const _Weight &a) const;
	virtual bool isDefault(const _Weight &a) const;
	virtual std::string describe(const _Weight&, Fsa::Hint hints = 0) const;

	virtual std::string asString(const _Weight&) const;
	virtual _Weight fromString(const std::string &str) const;
	virtual bool read(_Weight &a, Core::BinaryInputStream &i) const;
	virtual bool write(const _Weight &a, Core::BinaryOutputStream &o) const;

	virtual void compress(Core::Vector<u8> &stream, const _Weight &a) const;
	virtual _Weight uncompress(Core::Vector<u8>::const_iterator &) const;
	virtual size_t compressedSize() const;
    };

    template<class _Weight, typename _Type>
    class TropicalSemiring : public LogSemiring<_Weight, _Type> {
	typedef LogSemiring<_Weight, _Type> Precursor;
    public:
	typedef _Type Type;
	typedef Accumulator<_Weight> _Accumulator;
    public:
	TropicalSemiring();
	TropicalSemiring(s32 tolerance);
	virtual std::string name() const;
	virtual _Weight collect(const _Weight &a, const _Weight &b) const;
	virtual _Weight invCollect(const _Weight &a, const _Weight &b) const
	{ std::cerr << "method \"invCollect\" is not supported" << std::endl; return this->invalid(); }
	virtual bool hasInvCollect() const { return false; }
	virtual _Accumulator * getCollector(const _Weight &initial) const;
    };

    template<class _Weight, typename _Type>
    class ProbabilitySemiring : public LogSemiring<_Weight, _Type> {
	typedef LogSemiring<_Weight, _Type> Precursor;
    public:
	typedef _Type Type;
	typedef Accumulator<_Weight> _Accumulator;
    public:
	ProbabilitySemiring();
	ProbabilitySemiring(s32 tolerance);
	virtual std::string name() const;
	virtual _Weight zero() const;
	virtual _Weight one() const;
	virtual _Weight extend(const _Weight &a, const _Weight &b) const;
	virtual _Accumulator * getExtender(const _Weight &initial) const;
	virtual _Weight collect(const _Weight &a, const _Weight &b) const;
	virtual _Accumulator * getCollector(const _Weight &initial) const;
	virtual _Weight invert(const _Weight &a) const;
	virtual std::string describe(const _Weight&, Fsa::Hint hints = 0) const;
	virtual _Weight fromString(const std::string &str) const;
    };

} // namespace Ftl

#include "tRealSemiring.cc"

#endif // _T_FSA_RATIONAL_SEMIRING_HH
