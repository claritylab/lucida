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
#include <Core/StringUtilities.hh>
#include "RealSemiring.hh"
#include "Semiring.hh"
#include "Utility.hh"
#include <Core/Vector.hh>

namespace Fsa {
    class UnknownSemiring_ : public Ftl::SemiringAdapter<Weight> {
    public:
	std::string name() const { return "unknown"; }
	Weight invalid() const { return Weight(); }
    };
    ConstSemiringRef UnknownSemiring = ConstSemiringRef(new UnknownSemiring_());


    class PredefinedLogSemiring_ : public LogSemiring_ {
    public:
	PredefinedLogSemiring_() : LogSemiring_(1) {}
	std::string name() const { return "log"; }
    };
    ConstSemiringRef LogSemiring = ConstSemiringRef(new PredefinedLogSemiring_());


    class PredefinedTropicalSemiring_ : public TropicalSemiring_ {
    public:
	PredefinedTropicalSemiring_() : TropicalSemiring_(1) {}
	std::string name() const { return "tropical"; }
    };
    ConstSemiringRef TropicalSemiring = ConstSemiringRef(new PredefinedTropicalSemiring_());

}
namespace Ftl {
    using namespace Fsa;
    typedef Ftl::LogSemiring<Weight, s32> LogIntegerSemiring_;

    template<>
    Fsa::Weight LogIntegerSemiring_::collect(const Fsa::Weight &a, const Fsa::Weight &b) const {
	return invalid();
    }
    template<>
    Semiring<Weight>::Accumulator * LogIntegerSemiring_::getCollector(const Fsa::Weight &initial) const {
	return 0;
    }
    template<>
    int LogIntegerSemiring_::compare(const Fsa::Weight &a, const Fsa::Weight &b) const {
	return (s32(a) < s32(b) ? -1 : s32(a) > s32(b) ? 1 : 0);
    }
    template<>
    Fsa::Weight LogIntegerSemiring_::fromString(const std::string &str) const {
	double tmp;
	if (!Core::strconv(str, tmp))
	    std::cerr << "'" << str << "' is not a valid " << name() << " semiring value." << std::endl;
	return Weight(s32(10000 * tmp));
    }
    template<>
    std::string LogIntegerSemiring_::asString(const Fsa::Weight &a) const {
	double tmp = double(s32(a)) / 10000.0;
	return Core::form("%f", tmp);
    }

    /*
      virtual Core::XmlElement * fromXml(XmlParser &, void (XmlParser::*)(Weight *)) const
      { std::cerr << "method \"fromXml\" is not supported" << std::endl; return 0; }
    */

    template<>
    std::string LogIntegerSemiring_::describe(const Fsa::Weight &a, Fsa::Hint hints) const
    { return asString(a); }
}
namespace Fsa
{

    class TropicalIntegerSemiring_ : public Ftl::TropicalSemiring<Weight, s32> {
	typedef Ftl::TropicalSemiring<Weight, s32> Precursor;
     public:
	TropicalIntegerSemiring_() : Precursor(0) {}
	std::string name() const { return "tropical-integer"; }
    };
    ConstSemiringRef TropicalIntegerSemiring = ConstSemiringRef(new TropicalIntegerSemiring_());

    class PredefinedProbabilitySemiring_ : public ProbabilitySemiring_ {
    public:
	PredefinedProbabilitySemiring_() : ProbabilitySemiring_(1) {}
	std::string name() const { return "probability"; }
    };
    ConstSemiringRef ProbabilitySemiring = ConstSemiringRef(new PredefinedProbabilitySemiring_());

    class CountSemiring_ : public Semiring {
    private:
	s32 infinity() const { return Core::Type<s32>::max; }
    public:
	virtual std::string name() const { return "count"; }
	virtual Weight create() const { return Weight(); }
	virtual Weight clone(const Weight &a) const { return Weight(s32(a)); }
	virtual Weight invalid() const { return Weight(Core::Type<s32>::min); }
	virtual Weight zero() const { return Weight(0); }
	virtual Weight one() const { return Weight(1); }
	virtual Weight max() const { return Weight(infinity()); }
	virtual Weight extend(const Weight &a, const Weight &b) const {
	    if (s32(a) == infinity()) return Weight(infinity());
	    if (s32(b) == infinity()) return Weight(infinity());
	    if (infinity() / std::max(s32(a), s32(b)) < std::min(s32(a), s32(b))) return Weight(infinity());
	    return Weight(s32(a) * s32(b));
	}
	virtual Weight collect(const Weight &a, const Weight &b) const {
	    if (s32(a) == infinity()) return Weight(infinity());
	    if (s32(b) == infinity()) return Weight(infinity());
	    if (infinity() - s32(a) < s32(b)) return Weight(infinity());
	    return Weight(s32(a) + s32(b));
	}
	virtual Weight invert(const Weight &a) const { return Weight(-s32(a)); }
	virtual int compare(const Weight &a, const Weight &b) const { return (s32(a) < s32(b) ? -1 : s32(a) > s32(b) ? 1 : 0); }
	virtual size_t hash(const Weight &a) const { return size_t(s32(a)); }
	virtual Weight fromString(const std::string &str) const {
	    s32 tmp;
	    if (!Core::strconv(str, tmp))
		std::cerr << "'" << str << "' is not a valid " << name() << " semiring value." << std::endl;
	    return Weight(tmp);
	}
	virtual std::string asString(const Weight &a) const {
	    if (s32(a) == infinity()) return "inf";
	    return Core::form("%d", s32(a));
	}
	virtual bool read(Weight &a, Core::BinaryInputStream &i) const {
	    s32 tmp; i >> tmp; a = Weight(tmp);
	    return i;
	}
	virtual bool write(const Weight &a, Core::BinaryOutputStream &o) const {
	    if (s32(a) == infinity()) o << "inf";
	    return o << s32(a);
	}
	void compress(Core::Vector<u8> &stream, const Weight &a) const {
	    appendBytes(stream, u32(a), sizeof(u32));
	}
	Weight uncompress(Core::Vector<u8>::const_iterator &pos) const {
	    return Weight(getBytesAndIncrement(pos, sizeof(u32)));
	}
	size_t compressedSize() const {
	    return sizeof(u32);
	}
	virtual std::string describe(const Weight &a, Fsa::Hint hints = 0) const { return asString(a); }
    };
    ConstSemiringRef CountSemiring = ConstSemiringRef(new CountSemiring_());



    Core::Choice SemiringTypeChoice
    ("log", SemiringTypeLog,
     "tropical", SemiringTypeTropical,
     "tropical-integer", SemiringTypeTropicalInteger,
     "count", SemiringTypeCount,
     "probability", SemiringTypeProbability,
     Core::Choice::endMark());

    ConstSemiringRef getSemiring(SemiringType type) {
	switch (type) {
	case SemiringTypeUnknown:
	    return UnknownSemiring;
	case SemiringTypeLog:
	    return LogSemiring;
	case SemiringTypeTropical:
	    return TropicalSemiring;
	case SemiringTypeTropicalInteger:
	    return TropicalIntegerSemiring;
	case SemiringTypeCount:
	    return CountSemiring;
	case SemiringTypeProbability:
	    return ProbabilitySemiring;
	}
	return ConstSemiringRef();
    }

    SemiringType getSemiringType(ConstSemiringRef semiring) {
	return SemiringType(SemiringTypeChoice[semiring->name()]);
    }

} // namespace Fsa
