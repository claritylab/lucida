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
#include <Core/Debug.hh>
#include <Core/StringUtilities.hh>
#include <Core/Utility.hh>

#include "tRealSemiring.hh"
#include "Utility.hh"
#include <limits>


namespace Ftl {
    namespace {
	template<typename T>
	std::string str(const T &t) {
	    std::ostringstream oss; oss << t;
	    return oss.str();
	}
	template<>
	std::string str<f32>(const f32 &f) {
	    return Core::form("%f", double(f));
	}
	template<>
	std::string str<f64>(const f64 &f) {
	    return Core::form("%f", double(f));
	}


	template<class _Type>
	bool checkValidDifference(const _Type &a, const _Type &b) {
	    bool result = true;
	    if ((a != 0) and (b != 0) &&
		(std::min(Core::abs(a / b), Core::abs(b / a)) <= Core::Type<_Type>::epsilon)) {
		std::cerr << __FILE__ << ": Warning: checkValidDifference<" << Core::Type<_Type>::name << ">("
			  << "(" << a << "," << b << "): " << "(" << a << " - " << b << "): "
			  << "difference suffers from insufficient precision" << std::endl;
		result = false;
	    }
	    const _Type x = a - b;
	    const _Type min = ::log(Core::Type<_Type>::delta);
	    if (x <= min) {
		std::cerr << __FILE__ << ": Warning: checkValidDifference<" << Core::Type<_Type>::name << ">("
				  << "(" << a << "," << b << "): "
			  << "difference is too small: " << x << ". needs to be > " << min << " to be calculated with exp. Here exp(a-b)->0" << std::endl;
		result = false;
	    }
	    const _Type max = ::log(Core::Type<_Type>::max);
	    if (x >= max) {
		std::cerr << __FILE__ << "Warning: checkValidDifference<" << Core::Type<_Type>::name << ">("
			  << "(" << a << "," << b << "): "
			  << "difference is too large: " << x << ". needs to be < " << max << " to be calculated by exp. exp(a-b)->inf" << std::endl;
		result = false;
	    }
	    return result;
	}
    }

    template<class _Weight, typename _Type>
    LogSemiring<_Weight, _Type>::LogSemiring() :
	Precursor(Fsa::PropertySemiringAll),
	tolerance_(0) {}

    template<class _Weight, typename _Type>
    LogSemiring<_Weight, _Type>::LogSemiring(s32 tolerance) :
	Precursor(Fsa::PropertySemiringAll),
	tolerance_(tolerance) {}

    template<class _Weight, typename _Type>
    std::string LogSemiring<_Weight, _Type>::name() const
    { return Core::form("log-%s(tolerance=%d)", Core::Type<_Type>::name, tolerance_); }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::invalid() const
    { return _Weight(Core::Type<_Type>::min); }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::zero() const
    { return _Weight(Core::Type<_Type>::max); }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::one() const
    { return _Weight(_Type(0)); }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::max() const
    { return _Weight(Core::Type<_Type>::max); }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::extend(const _Weight &a, const _Weight &b) const
    { return _Weight(_Type(a) + _Type(b)); }

    namespace {
	template<class _Weight, typename _Value>
	struct Extender : public Accumulator<_Weight> {
	    _Value v;
	    Extender(const _Weight &initial) : v(_Value(initial)) {}
	    virtual ~Extender() {}
	    void feed(const _Weight &a) { v += _Value(a); }
	    _Weight get() const { return _Weight(v); }
	};
    }
    template<class _Weight, typename _Type>
    Accumulator<_Weight> * LogSemiring<_Weight, _Type>::getExtender(const _Weight &initial) const
    { return new Extender<_Weight, _Type>(initial); }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::collect(const _Weight &a, const _Weight &b) const {
	if (_Type(b) == _Type(zero())) {
	    if (_Type(a) == _Type(zero()))
		return zero();
	    else
		return a;
	} else if (_Type(a) == _Type(zero())) {
	    return b;
	} else {
	    DBGCMD(1, checkValidDifference(std::min(_Type(a), _Type(b)), std::max(_Type(a), _Type(b))));
	    return _Weight(std::min(_Type(a), _Type(b)) -
			    ::log1p(::exp(std::min(_Type(a), _Type(b)) - std::max(_Type(a), _Type(b)))));
	}
    }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::invCollect(const _Weight &a, const _Weight &b) const {
	if (_Type(a) == _Type(b)) return zero();
	// a != b
	if (_Type(b) == _Type(zero())) return a;
	// b != 0
	if (_Type(a) == _Type(zero())) return invalid();
	// a != 0
	if (_Type(a) > _Type(b)) return invalid();
	// b > a != 0
	DBGCMD(1, checkValidDifference(_Type(a),_Type(b)));
	return _Weight(_Type(a) - ::log1p(-::exp(_Type(a) - _Type(b))));
    }

    namespace {
	template<class _Weight, typename _Value>
	struct LogCollector : public Accumulator<_Weight> {
	    typedef std::vector<_Value> _ValueList;
	    _Value min;
	    _ValueList values;
	    LogCollector(const _Weight &initial) : min(_Value(initial)) {}
	    virtual ~LogCollector() {}
	    void feed(const _Weight &a) {
		if (_Value(a) < min) { values.push_back(min); min = _Value(a); }
		else values.push_back(_Value(a));
	    }
	    _Weight get() const {
		if (values.empty()) return _Weight(min);
		typename _ValueList::const_iterator it = values.begin();
		_Value sum = ::exp(min - *it);
		for (++it; it != values.end(); ++it) {
		    DBGCMD(1, checkValidDifference(min, *it));
		    sum += ::exp(min - *it);
		}
		return _Weight(min - ::log1p(sum));
	    }
	};
    }
    template<class _Weight, typename _Type>
    Accumulator<_Weight> * LogSemiring<_Weight, _Type>::getCollector(const _Weight &initial) const
    { return new LogCollector<_Weight, _Type>(initial); }


    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::invert(const _Weight &a) const
    { return _Weight(-_Type(a)); }

    template<class _Weight, typename _Type>
    int LogSemiring<_Weight, _Type>::compare(const _Weight &a, const _Weight &b) const {
	if (_Type(a) == _Type(b)) return 0;
	if (Core::isAlmostEqualUlp(_Type(a), _Type(b), tolerance_)) return 0;
	if (_Type(a) < _Type(b)) return -1;
	return 1;
    }

    template<class _Weight, typename _Type>
    size_t LogSemiring<_Weight, _Type>::hash(const _Weight &a) const
    { return size_t(_Type(a)); }

    template<class _Weight, typename _Type>
    bool LogSemiring<_Weight, _Type>::isDefault(const _Weight &a) const
    { return (_Type(a) == _Type(this->defaultWeight())); }

    template<class _Weight, typename _Type>
    std::string LogSemiring<_Weight, _Type>::describe(const _Weight &a, Fsa::Hint hints) const {
	return (hints & Fsa::HintAsProbability) ? str(exp(-_Type(a))) : str(_Type(a));
    }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::fromString(const std::string &s) const {
	_Type tmp;
	if (!Core::strconv(s, tmp))
	    std::cerr << "'" << s << "' is not a valid " << name() << " semiring value." << std::endl;
	return _Weight(tmp);
    }

    template<class _Weight, typename _Type>
    std::string LogSemiring<_Weight, _Type>::asString(const _Weight &a) const {
	return str( _Type((a)));
    }

    template<class _Weight, typename _Type>
    bool LogSemiring<_Weight, _Type>::read(_Weight &a, Core::BinaryInputStream &i) const
    { _Type tmp; i >> tmp; a = _Weight(tmp); return i; }

    template<class _Weight, typename _Type>
    bool LogSemiring<_Weight, _Type>::write(const _Weight &a, Core::BinaryOutputStream &o) const
    { return o << _Type(a); }

    template<class _Weight, typename _Type>
    void LogSemiring<_Weight, _Type>::compress(Core::Vector<u8> &stream, const _Weight &a) const {
	Fsa::appendBytes(stream, u32(a), sizeof(u32));
    }

    template<class _Weight, typename _Type>
    _Weight LogSemiring<_Weight, _Type>::uncompress(Core::Vector<u8>::const_iterator &pos) const {
	return _Weight(Fsa::getBytesAndIncrement(pos, sizeof(u32)));
    }

    template<class _Weight, typename _Type>
    size_t LogSemiring<_Weight, _Type>::compressedSize() const {
	return sizeof(u32);
    }


    template<class _Weight, typename _Type>
    TropicalSemiring<_Weight, _Type>::TropicalSemiring() :
	Precursor() {}

    template<class _Weight, typename _Type>
    TropicalSemiring<_Weight, _Type>::TropicalSemiring(s32 tolerance) :
	Precursor(tolerance) {}

    template<class _Weight, typename _Type>
    std::string TropicalSemiring<_Weight, _Type>::name() const
    { return Core::form("tropical-%s(tolerance=%d)", Core::Type<_Type>::name, Precursor::tolerance_); }

    template<class _Weight, typename _Type>
    _Weight TropicalSemiring<_Weight, _Type>::collect(const _Weight &a, const _Weight &b) const
    { return _Weight(std::min(_Type(a), _Type(b))); }

    namespace {
	template<class _Weight, typename _Value>
	struct TropicalCollector : public Accumulator<_Weight> {
	    _Value v;
	    TropicalCollector(const _Weight &initial) : v(_Value(initial)) {}
	    virtual ~TropicalCollector() {}
	    void feed(const _Weight &a) { v = std::min(v, _Value(a)); }
	    _Weight get() const { return _Weight(v); }
	};
    }
    template<class _Weight, typename _Type>
    Accumulator<_Weight> * TropicalSemiring<_Weight, _Type>::getCollector(const _Weight &initial) const
    { return new TropicalCollector<_Weight, _Type>(initial); }

    template<class _Weight, typename _Type>
    ProbabilitySemiring<_Weight, _Type>::ProbabilitySemiring() :
	Precursor() {}

    template<class _Weight, typename _Type>
    ProbabilitySemiring<_Weight, _Type>::ProbabilitySemiring(s32 tolerance) :
	Precursor(tolerance) {}

    template<class _Weight, typename _Type>
    std::string ProbabilitySemiring<_Weight, _Type>::name() const
    { return Core::form("probability-%s(tolerance=%d)", Core::Type<_Type>::name, Precursor::tolerance_); }

    template<class _Weight, typename _Type>
    _Weight ProbabilitySemiring<_Weight, _Type>::zero() const
    { return _Weight(_Type(0)); }

    template<class _Weight, typename _Type>
    _Weight ProbabilitySemiring<_Weight, _Type>::one() const
    { return _Weight(_Type(1)); }

    template<class _Weight, typename _Type>
    _Weight ProbabilitySemiring<_Weight, _Type>::extend(const _Weight &a, const _Weight &b) const
    { return _Weight(_Type(a) * _Type(b)); }

    namespace {
	template<class _Weight, typename _Value>
	struct ProbabilityExtender : public Accumulator<_Weight> {
	    _Value v;
	    ProbabilityExtender(const _Weight &initial) : v(_Value(initial)) {}
	    virtual ~ProbabilityExtender() {}
	    void feed(const _Weight &a) { v *= _Value(a); }
	    _Weight get() const { return _Weight(v); }
	};
    }
    template<class _Weight, typename _Type>
    Accumulator<_Weight> * ProbabilitySemiring<_Weight, _Type>::getExtender(const _Weight &initial) const
    { return new ProbabilityExtender<_Weight, _Type>(initial); }

    template<class _Weight, typename _Type>
    _Weight ProbabilitySemiring<_Weight, _Type>::collect(const _Weight &a, const _Weight &b) const
    { return _Weight(_Type(a) + _Type(b)); }

    namespace {
	template<class _Weight, typename _Value>
	struct ProbabilityCollector : public Accumulator<_Weight> {
	    _Value v;
	    ProbabilityCollector(const _Weight &initial) : v(_Value(initial)) {}
	    virtual ~ProbabilityCollector() {}
	    void feed(const _Weight &a) { v += _Value(a); }
	    _Weight get() const { return _Weight(v); }
	};
    }
    template<class _Weight, typename _Type>
    Accumulator<_Weight> * ProbabilitySemiring<_Weight, _Type>::getCollector(const _Weight &initial) const
    { return new ProbabilityCollector<_Weight, _Type>(initial); }


    template<class _Weight, typename _Type>
    _Weight ProbabilitySemiring<_Weight, _Type>::invert(const _Weight &a) const {
	if (Core::isAlmostEqualUlp(_Type(a), _Type(0), Precursor::tolerance_)) return Precursor::max();
	return _Weight(1/_Type(a));
    }

    template<class _Weight, typename _Type>
    std::string ProbabilitySemiring<_Weight, _Type>::describe(const _Weight &a, Fsa::Hint hints) const {
	return str(_Type(a));
    }

    template<class _Weight, typename _Type>
    _Weight ProbabilitySemiring<_Weight, _Type>::fromString(const std::string &s) const {
	_Weight tmp = Precursor::fromString(s);
	if (Precursor::compare(tmp, _Weight(0)) < 0)
	    std::cerr << "'" << s << "' is not a valid " << name() << " semiring value." << std::endl;
	return tmp;
    }

} // namespace Ftl
