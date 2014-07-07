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
#ifndef _FSA_SEMIRING_HH
#define _FSA_SEMIRING_HH

#include <Core/Choice.hh>
#include <Core/Types.hh>

#include "tSemiring.hh"

namespace Fsa {
    class Weight {
	friend class Ftl::Semiring<Weight>;
    private:
	union Value {
	    s32 s;
	    u32 u;
	    f32 f;
	};
	Value value_;

    public:
	Weight() {}
	explicit Weight(s32 value) { value_.s = value; }
	explicit Weight(u32 value) { value_.u = value; }
	explicit Weight(f32 value) { value_.f = value; }
	explicit Weight(double value) { value_.f = value; }

	operator int() { return value_.s; }
	operator int() const { return value_.s; }
	operator u32() { return value_.u; }
	operator u32() const { return value_.u; }
	operator float() { return value_.f; }
	operator float() const { return value_.f; }
	const Weight& operator= (const Weight &w) { value_ = w.value_; return *this; }

	bool operator== (const Weight &w) const {
	    return memcmp(&value_, &w.value_, sizeof(Value)) == 0;
	}

	bool operator!= (const Weight &w) const {
	    return memcmp(&value_, &w.value_, sizeof(Value)) != 0;
	}

	bool operator< (const Weight &w) const {
	    return memcmp(&value_, &w.value_, sizeof(Value)) < 0;
	}
    };

    typedef Ftl::Accumulator<Weight> Accumulator;
    typedef Ftl::Semiring<Weight> Semiring;
    typedef Semiring::Ref SemiringRef;
    typedef Semiring::ConstRef ConstSemiringRef;

    extern ConstSemiringRef UnknownSemiring;
    extern ConstSemiringRef LogSemiring;
    extern ConstSemiringRef TropicalSemiring;
    extern ConstSemiringRef TropicalIntegerSemiring;
    extern ConstSemiringRef CountSemiring;
    extern ConstSemiringRef ProbabilitySemiring;

    extern Core::Choice SemiringTypeChoice;

    extern ConstSemiringRef getSemiring(SemiringType type);
    extern SemiringType getSemiringType(ConstSemiringRef semiring);
} // namespace Fsa

#endif // _FSA_SEMIRING_HH
