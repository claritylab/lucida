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
#ifndef _FSA_SEMIRING64_HH
#define _FSA_SEMIRING64_HH

#include <Core/Choice.hh>
#include <Core/Types.hh>

#include "tSemiring.hh"
#include "Semiring.hh"

namespace Fsa {
    class Weight64 {
	friend class Ftl::Semiring<Weight64>;
    private:
    union Value {
	s64 s;
	u64 u;
	f64 f;
    };
	Value value_;

    public:
	Weight64() {}
	explicit Weight64(s32 value) { value_.s = value; }
	explicit Weight64(u32 value) { value_.u = value; }
	explicit Weight64(f32 value) { value_.f = value; }
	explicit Weight64(double value) { value_.f = value; }
	// TODO: better implementation
	explicit Weight64(Weight &w) { value_.f = (f32)w; }

	operator int() { return value_.s; }
	operator int() const { return value_.s; }
	operator long() { return value_.s; }
	operator long() const { return value_.s; }
	operator u32() { return value_.u; }
	operator u32() const { return value_.u; }
	operator u64() { return value_.u; }
	operator u64() const { return value_.u; }
	operator float() { return value_.f; }
	operator float() const { return value_.f; }
	const Weight64& operator= (const Weight64 &w) { value_ = w.value_; return *this; }
	operator double() { return value_.f; }
	operator double() const { return value_.f; }
	// TODO: better implementation
	operator Weight() const {
		if (value_.f == Core::Type<f64>::max) // Match case zero in LogSemiring
			return Weight(Core::Type<f32>::max);
		if (value_.f == Core::Type<f64>::min) // Match case invalid in LogSemiring
			return Weight(Core::Type<f32>::min);
		return Weight(value_.f);
	}

	bool operator== (const Weight64 &w) const {
		return memcmp(&value_, &w.value_, sizeof(Value)) == 0;
	}

	bool operator!= (const Weight64 &w) const {
		return memcmp(&value_, &w.value_, sizeof(Value)) != 0;
	}

	bool operator< (const Weight64 &w) const {
		return memcmp(&value_, &w.value_, sizeof(Value)) < 0;
	}
    };

    typedef Ftl::Accumulator<Weight64> Accumulator64;
    typedef Ftl::Semiring<Weight64> Semiring64;
    typedef Semiring64::Ref Semiring64Ref;
    typedef Semiring64::ConstRef ConstSemiring64Ref;

    extern ConstSemiring64Ref LogSemiring64;
} // namespace Fsa

#endif // _FSA_SEMIRING64_HH
