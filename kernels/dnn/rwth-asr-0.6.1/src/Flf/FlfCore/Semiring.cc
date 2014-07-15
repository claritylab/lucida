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
#include <Core/Application.hh>
#include <Core/Parameter.hh>
#include <Core/StringUtilities.hh>
#include <Core/Utility.hh>

#include "Semiring.hh"
#include "Utility.hh"

namespace Flf {
    namespace {
	u32 nScoresRef_ = 0;
    } // namespace
    void incScoresRefCounter() {
	++nScoresRef_;
    }
    void decScoresRefCounter() {
	--nScoresRef_;
    }
    u32 nScoresRef() {
	return nScoresRef_;
    }

    // -------------------------------------------------------------------------
    const Score Semiring::Zero    = Core::Type<Score>::max;
    const Score Semiring::One     = Score(0);
    const Score Semiring::Max     = Core::Type<Score>::max;
    const Score Semiring::Invalid = Core::Type<Score>::min;
    const Score Semiring::DefaultScale   = Score(1);
    const Score Semiring::UndefinedScale = Core::Type<Score>::min;
    const ScoreId Semiring::InvalidId = Core::Type<ScoreId>::max;

    // do NOT change
    const Key Semiring::UndefinedKey = Key("");

    const Semiring::Tolerance Semiring::DefaultTolerance = 1;

    Scores* Semiring::alloc(const ScoresRef &a) const {
	Scores* c = alloc();
	::memcpy(c->begin(), a->begin(), n_ * sizeof(Score));
	return c;
    }
    Scores* Semiring::alloc(const Score &init) const {
	Scores* c = alloc();
	std::fill(c->begin(), c->begin() + n_, init);
	return c;
    }

    Semiring::Semiring(size_t n, const ScoreList &scales, const KeyList &keys) :
	Precursor(),
	n_(n),
	tolerance_(DefaultTolerance),
	scales_(n, DefaultScale),
	keys_(n, UndefinedKey) {
	if (!scales.empty()) setScales(scales);
	if (!keys.empty())   setKeys(keys);
	one_     = ScoresRef(alloc(One));
	zero_    = ScoresRef(alloc(Zero));
	max_     = ScoresRef(alloc(Max));
	invalid_ = ScoresRef(alloc(Invalid));
    }

    Semiring::~Semiring() {}

    bool Semiring::operator==(const Semiring &semiring) const {
	if (type()      != semiring.type())      return false;
	// if (tolerance() != semiring.tolerance()) return false;
	if (scales()    != semiring.scales())    return false;
	if (keys()      != semiring.keys())      return false;
	return true;
    }

    std::string Semiring::name() const {
	std::ostringstream oss;
	oss << "(n=" << size();
	if (tolerance() != DefaultTolerance)
	    oss << ",tol=" << tolerance();
	ScoreId id = 0;
	KeyList::const_iterator itKey = keys_.begin();
	for (ScoreList::const_iterator itScale = scales_.begin(); itScale != scales_.end(); ++itScale, ++itKey, ++id) {
	    oss << ",";
	    if (itKey->empty()) oss << id;
	    else oss << *itKey;
	    if (*itScale != DefaultScale)
		oss << ":" << *itScale;
	}
	oss << ")";
	return oss.str();
    }

    void Semiring::setTolerance(Tolerance tolerance) const {
	tolerance_ = tolerance;
    }

    void Semiring::setScales(const ScoreList &scales) const {
	verify(scales.size() == size());
	ScoreList::iterator it1 = scales_.begin();
	ScoreList::const_iterator it2 = scales.begin();
	for (; it1 != scales_.end(); ++it1, ++it2)
	    if (*it2 != UndefinedScale) *it1 = *it2;
    }

    void Semiring::setScale(ScoreId id, Score scale) const {
	if (scale != UndefinedScale)
	    scales_[id] = scale;
    }

    u32 Semiring::cmpScales(const ScoreList &scales) const {
	verify(scales.size() == size());
	int cmp = 0;
	ScoreList::iterator it1 = scales_.begin();
	ScoreList::const_iterator it2 = scales.begin();
	for (; it1 != scales_.end(); ++it1, ++it2)
	    if (*it2 != UndefinedScale) cmp |= 1;
	    else if (*it2 != *it1) cmp |= 2;
	return cmp;
    }

    void Semiring::setKeys(const KeyList &keys) const {
	verify(keys.size() == size());
	ScoreId id = 0;
	for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it, ++id)
	    if (*it != UndefinedKey)
		if (!setKey(id, *it))
		    Core::Application::us()->error(
			"Failed to set name \"%s\" for dimension %zu, name is already in use.",
			it->c_str(), id);
    }

    bool Semiring::setKey(ScoreId id, const Key &key) const {
	if (key.empty() || !hasId(id)) return false;
	if (keyMap_.insert(std::make_pair(key, id)).second) {
	    keys_[id] = key;
	    return true;
	} else
	    return false;
    }

    bool Semiring::hasKey(const Key &key) const {
	return keyMap_.find(key) != keyMap_.end();
    }

    ScoreId Semiring::id(const Key &key) const {
	KeyMap::const_iterator it = keyMap_.find(key);
	if (it != keyMap_.end()) return it->second;
	return InvalidId;
    }

    bool Semiring::hasId(ScoreId id) const {
	return size_t(id) < size();
    }

    bool Semiring::isDefault(const ScoresRef &a) const {
	return compare(one_, a) == 0;
    }

    size_t Semiring::hash(const ScoresRef &a) const {
	//	    return size_t(project(a)); ???
	return reinterpret_cast<size_t>(a.get());
    }

    ScoresRef Semiring::extend(const ScoresRef &a, const ScoresRef &b) const {
	if (b.get() == one_.get()) return a;
	if (a.get() == one_.get()) return b;
	Scores *c = alloc(b);
	Scores::iterator itc = begin(c), endc = end(c);
	Scores::const_iterator ita = begin(a);
	for(; itc != endc; ++itc, ++ita) *itc += *ita;
	return ScoresRef(c);
    }

    Semiring::ExtenderRef Semiring::createExtender() const {
	defect();
	return Semiring::ExtenderRef();
    }

    Semiring::CollectorRef Semiring::createCollector() const {
	defect();
	return Semiring::CollectorRef();
    }

    ScoresRef Semiring::invert(const ScoresRef &a) const {
	Scores *c = alloc();
	Scores::iterator itc = begin(c), endc = end(c);
	Scores::const_iterator ita = begin(a);
	for(; itc != endc; ++itc, ++ita) *itc = -*ita;
	return ScoresRef(c);
    }

    Score Semiring::project(const ScoresRef &a) const {
	return a->project(scales_);
    }

    int Semiring::compare(const ScoresRef &a, const ScoresRef &b) const {
	if (a.get() == b.get()) return 0;
	Score pa = 0.0, pb = 0.0;
	Scores::const_iterator ita = begin(a), itb = begin(b);
	for (ScoreList::const_iterator its = scales_.begin(), send = scales_.end();
	     its != send; ++its, ++ita, ++itb)
	    { pa += (*its) * (*ita); pb += (*its) * (*itb); }
	return Core::isAlmostEqualUlp(pa, pb, tolerance_) ? 0 : (pa < pb) ? -1 : 1;
    }

    bool Semiring::read(ScoresRef &a, Core::BinaryInputStream &i) const {
	Scores::iterator ita = begin(a), enda = end(a);
	for(; ita != enda; ++ita) i >> *ita;
	return i;
    }

    bool Semiring::write(const ScoresRef &a, Core::BinaryOutputStream &o) const {
	Scores::const_iterator ita = begin(a), enda = end(a);
	for(; ita != enda; ++ita) o << *ita;
	return o;
    }

    std::string Semiring::asString(const ScoresRef &a) const {
	std::string desc;
	Scores::const_iterator ita = begin(a), enda = end(a);
	for(; ita != enda; ++ita) desc += Core::form("%f;", *ita);
	return desc;
    }

    ScoresRef Semiring::fromString(const std::string &s) const {
	Scores *a = alloc();
	Scores::iterator ita = begin(a), enda = end(a);
	std::istringstream iss(s); char c;
	for(; ita != enda; ++ita) if ((!(iss >> *ita >> c)) || (c != ';')) {
	    std::cerr << "could not parse " << s << std::endl;
	    return invalid();
	}
	return ScoresRef(a);
    }

    std::string Semiring::describe(const ScoresRef &a, Fsa::Hint hint) const {
	if (n_ == 0)
	    return "[]";
	std::ostringstream oss;
	if (hint & Fsa::HintShowDetails) {
	    Scores::const_iterator ita = begin(a), enda = end(a);
	    ScoreList::const_iterator its = scales_.begin();
	    oss << "[";
	    if (hint & Fsa::HintAsProbability) {
		if (hint & HintUnscaled) {
		    oss << ::exp(-(*ita));
		    if (*its != DefaultScale) oss << "^" << *its;
		    for(++ita, ++its; ita != enda; ++ita, ++its) {
			oss << " ";
			oss << ::exp(-(*ita));
			if (*its != DefaultScale) oss << "^" << *its;
		    }
		} else {
		    oss << ::exp(-(*its * *ita));
		    for(++ita, ++its; ita != enda; ++ita, ++its) oss << " " << exp(-(*its * *ita));
		}
	    } else {
		if (hint & HintUnscaled) {
		    if (*its != DefaultScale) oss << *its << "*";
		    oss << *ita;
		    for(++ita, ++its; ita != enda; ++ita, ++its) {
			oss << " ";
			if (*its != DefaultScale) oss << *its << "*";
			oss << *ita;
		    }
		} else {
		    oss << (*its * *ita);
		    for(++ita, ++its; ita != enda; ++ita, ++its) oss << " " << (*its * *ita);
		}
	    }
	    oss << "]";
	} else {
	    if (hint & Fsa::HintAsProbability)
		oss << ::exp(-(a->project(scales_)));
	    else
		oss << a->project(scales_);
	}
	return oss.str();
    }

    namespace {
	union f32_u32_union {
	    f32 f; u32 u;
	    inline u32 operator() (f32 x) { f = x; return u; }
	    inline f32 operator() (u32 x) { u = x; return f; }
	} f32_u32_reinterpret_cast;
    } // namespace

    void Semiring::compress(ByteVector &v, const ScoresRef &a) const {
	if (a == one_)
	    v.push_back(1);
	else if (a == zero_)
	    v.push_back(0);
	else if (a == invalid_)
	    v.push_back(2);
	else {
	    v.push_back(3);
	    for (Scores::const_iterator it = begin(a), itEnd = end(a); it != itEnd; ++it) {
		u32 x(f32_u32_reinterpret_cast(f32(*it)));
		v.push_back((x >> 24) & 0xff);
		v.push_back((x >> 16) & 0xff);
		v.push_back((x >> 8) & 0xff);
		v.push_back(x & 0xff);
	    }
	}
    }

    ScoresRef Semiring::uncompress(ByteVector::const_iterator &vIt) const {
	u8 header(*vIt++);
	switch (header) {
	case 0:
	    return zero_;
	case 1:
	    return one_;
	case 2:
	    return invalid_;
	case 3: {
	    ScoresRef a(create());
	    for (Scores::iterator it = begin(a), itEnd = end(a); it != itEnd; ++it) {
		u32 x(0);
		x |= *(vIt++); x <<= 8;
		x |= *(vIt++); x <<= 8;
		x |= *(vIt++); x <<= 8;
		x |= *(vIt++);
		*it = f32_u32_reinterpret_cast(x);
	    }
	    return a; }
	default:
	    defect();
	    return invalid_;
	}
    }

    size_t Semiring::compressedSize() const {
	return 1 + size() * sizeof(u32);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstSemiringRef Semiring::create(Fsa::SemiringType type, size_t n, const ScoreList &scales, const KeyList &keys) {
	switch (type) {
	case Fsa::SemiringTypeLog:
	    return ConstSemiringRef(new LogSemiring(n, scales, keys));
	case Fsa::SemiringTypeTropical:
	    return ConstSemiringRef(new TropicalSemiring(n, scales, keys));
	default:
	    return ConstSemiringRef();
	}
    }

    const Core::ParameterString paramType(
	"type",
	"semiring type",
	"");
    const Core::ParameterInt paramTolerance(
	"tolerance",
	"tolerance used for score comparisons",
	Semiring::DefaultTolerance);
    const Core::ParameterStringVector paramKeys(
	"keys",
	"list of keys");
    const Core::ParameterFloat paramScale(
	"scale",
	"scale",
	Semiring::DefaultScale);
    ConstSemiringRef Semiring::create(const Core::Configuration &config) {
	Fsa::SemiringType _type = getSemiringType(paramType(config));
	if (_type == Fsa::SemiringTypeUnknown)
	    return ConstSemiringRef();
	Tolerance _tolerance(paramTolerance(config));
	KeyList _keys(paramKeys(config));
	ScoreList _scales(_keys.size());
	ScoreList::iterator itScale = _scales.begin();
	for (KeyList::const_iterator itKey = _keys.begin(); itKey != _keys.end(); ++itKey, ++itScale)
	    *itScale = Score(paramScale(Core::Configuration(config, *itKey)));
	ConstSemiringRef _semiring = Semiring::create(_type, _scales.size(), _scales, _keys);
	_semiring->setTolerance(_tolerance);
	return _semiring;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    void Semiring::write(Core::XmlWriter &xml) const {
	xml << Core::XmlOpen("semiring") + Core::XmlAttribute("n", size());
	xml << Core::XmlFull("type", getSemiringTypeName(type()));
	xml << Core::XmlFull("tolerance", tolerance());
	for (u32 i = 0; i < size(); ++i)
	    xml << Core::XmlOpen("dimension")
		<< Core::XmlFull("key", keys()[i])
		<< Core::XmlFull("scale", scales()[i])
		<< Core::XmlClose("dimension");
	xml << Core::XmlClose("semiring");
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class Semiring::XmlElement::Internal : public Core::XmlContext {
    private:
	Core::XmlContext *context_;
	ConstSemiringRef semiring_;

	std::string cdata_;

	Fsa::SemiringType type_;
	Semiring::Tolerance tolerance_;
	Key key_;
	Score scale_;
	KeyList keys_;
	ScoreList scales_;
    private:
	void resetDimension() {
	    key_ = Core::form("dim-%zu", keys_.size());
	    scale_ = Semiring::DefaultScale;
	}

	void reset() {
	    cdata_.clear();
	    type_ = Fsa::SemiringTypeTropical;
	    tolerance_ = Semiring::DefaultTolerance;
	    keys_.clear();
	    scales_.clear();
	    resetDimension();
	}

    public:
	Internal(Core::XmlContext *context) : context_(context) {
	    reset();
	    build();

	    verify((context_) && (context_->parser()));

	}

	virtual Core::XmlSchemaParser* parser() {
	    return (context_) ? context_->parser() : 0;
	}

	ConstSemiringRef semiring() const {
	    return semiring_;
	}

	void build() {
	    semiring_ = Semiring::create(type_, keys_.size(), scales_, keys_);
	    verify(semiring_);
	    semiring_->setTolerance(tolerance_);
	    reset();
	}

	void appendCdata(const char *ch, int len) {
	    cdata_.append(ch, len);
	}

	void type() {
	    std::string typeName;
	    if (!Core::strconv(cdata_, typeName))
		parser()->criticalError("Failed to convert \"%s\"", cdata_.c_str());
	    if ((type_ = getSemiringType(typeName)) == Fsa::SemiringTypeUnknown)
		parser()->criticalError("Semiring type \"%s\" is unknown", cdata_.c_str());
	    cdata_.clear();
	}

	void tolerance() {
	    if (!Core::strconv(cdata_, tolerance_))
		parser()->criticalError("Failed to convert %s", cdata_.c_str());
	    cdata_.clear();
	}

	void key() {
	    if (!Core::strconv(cdata_, key_))
		parser()->criticalError("Failed to convert %s", cdata_.c_str());
	    cdata_.clear();
	}

	void scale() {
	    if (!Core::strconv(cdata_, scale_))
		parser()->criticalError("Failed to convert %s", cdata_.c_str());
	    cdata_.clear();
	}

	void dimension() {
	    keys_.push_back(key_);
	    scales_.push_back(scale_);
	    resetDimension();
	    cdata_.clear();
	}
    };

    Semiring::XmlElement::XmlElement(Core::XmlContext *context) :
	Precursor("semiring", context) {
	internal_ = new Internal(this);
	addChild(new Core::XmlMixedElementRelay(
		     "type", internal_, 0, endHandler(&Internal::type), charactersHandler(&Internal::appendCdata),
		     XML_NO_MORE_CHILDREN));
	addChild(new Core::XmlMixedElementRelay(
		     "tolerance", internal_, 0, endHandler(&Internal::tolerance), charactersHandler(&Internal::appendCdata),
		     XML_NO_MORE_CHILDREN));
	addChild(new Core::XmlMixedElementRelay(
		     "dimension", internal_, 0, endHandler(&Internal::dimension), 0,
		     new Core::XmlMixedElementRelay(
			 "key", internal_, 0, endHandler(&Internal::key), charactersHandler(&Internal::appendCdata),
			 XML_NO_MORE_CHILDREN),
		     new Core::XmlMixedElementRelay(
			 "scale", internal_, 0, endHandler(&Internal::scale), charactersHandler(&Internal::appendCdata),
			 XML_NO_MORE_CHILDREN),
		     XML_NO_MORE_CHILDREN));
    }

    Semiring::XmlElement::~XmlElement() {
	delete internal_;
    }

    void Semiring::XmlElement::end() {
	Precursor::end();
	internal_->build();
    }

    ConstSemiringRef Semiring::XmlElement::semiring() const {
	return internal_->semiring();
    }

    Semiring::XmlElement * Semiring::xmlElement(Core::XmlContext *context) {
	return new XmlElement(context);
	// return new XmlElement(name.c_str(), context);
    }

    bool Semiring::equal(ConstSemiringRef semiring1, ConstSemiringRef semiring2) {
	if (semiring1.get() == semiring2.get())
	    return true;
	return *semiring1 == *semiring2;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	class SemiringXmlParser : public Core::XmlSchemaParser {
	    typedef Core::XmlSchemaParser Precursor;
	private:
	    Semiring::XmlElement *semiringXmlElement_;
	public:
	    SemiringXmlParser(const Core::Configuration &config) :
		Core::XmlSchemaParser(config) {
		semiringXmlElement_ = Semiring::xmlElement(this);
		setRoot(semiringXmlElement_);
	    }
	    ConstSemiringRef parse(std::istream &is) {
		if (Precursor::parseStream(is) != 0)
		    Core::Application::us()->criticalError("Parsing of semiring failed.");
		return semiringXmlElement_->semiring();
	    }
	};
	SemiringXmlParser *semiringXmlParser = 0;
    } // namespace

    ConstSemiringRef Semiring::read(std::istream &is) {
	if (!semiringXmlParser)
	    semiringXmlParser = new SemiringXmlParser(Core::Configuration(Core::Application::us()->getConfiguration(), "semiring-parser"));
	return semiringXmlParser->parse(is);
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    LogSemiring::LogSemiring(size_t n, const ScoreList &scales, const KeyList &keys) :
	Semiring(n, scales, keys) {}

    Fsa::SemiringType LogSemiring::type() const {
	return Fsa::SemiringTypeLog;
    }

    std::string LogSemiring::name() const {
	return "log" + Semiring::name();
    }

    ScoresRef LogSemiring::collect(const ScoresRef &a, const ScoresRef &b) const {
	if (b.get() == zero_.get()) return a;
	if (a.get() == zero_.get()) return b;
	Scores *cPtr = alloc();
	ScoresRef c(cPtr);
	Score x = 0.0, y = 0.0, norm = 0.0;
	Scores::iterator itc = begin(cPtr);
	Scores::const_iterator ita = begin(a), itb = begin(b);
	for(ScoreList::const_iterator its = scales_.begin(), ends = scales_.end();
	    its != ends; ++its, ++itc, ++ita, ++itb) {
	    const Score scale = *its;
	    if (scale != 0.0) {
		if ((*ita == Core::Type<Score>::max) || (*itb == Core::Type<Score>::max))
		    return max();
		const Score sa = scale * *ita, sb = scale * *itb;
		x += sa; y += sb;
		norm += *itc = logAdd(sa, sb);
	    } else
		*itc = 0.0;
	}
	const Score z = logAdd(x, y) / norm;
	itc = begin(cPtr);
	for(ScoreList::const_iterator its = scales_.begin(), ends = scales_.end();
	    its != ends; ++its, ++itc) {
	    const Score scale = *its;
	    if (scale != 0.0)
		*itc *= z / scale;
	}
	// verify(Core::isAlmostEqualUlp(logAdd(project(a), project(b)), project(c), 1000));
	return c;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    TropicalSemiring::TropicalSemiring(size_t n, const ScoreList &scales, const KeyList &keys) :
	Semiring(n, scales, keys) {}

    Fsa::SemiringType TropicalSemiring::type() const {
	return Fsa::SemiringTypeTropical;
    }

    std::string TropicalSemiring::name() const {
	return "tropical" + Semiring::name();
    }

    ScoresRef TropicalSemiring::collect(const ScoresRef &a, const ScoresRef &b) const {
	if (b.get() == zero_.get()) return a;
	if (a.get() == zero_.get()) return b;
	return (compare(a, b) <= 0) ? a : b;
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstSemiringRef cloneSemiring(ConstSemiringRef sr1) {
	ConstSemiringRef sr2 = Semiring::create(sr1->type(), sr1->size(), sr1->scales(), sr1->keys());
	sr2->setTolerance(sr1->tolerance());
	return sr2;
    }

    ConstSemiringRef appendSemiring(ConstSemiringRef sr1, Score scale, const Key &key) {
	ScoreList scales(sr1->scales());
	scales.push_back(scale);
	KeyList keys(sr1->keys());
	keys.push_back(key);
	ConstSemiringRef sr2 = Semiring::create(sr1->type(), sr1->size() + 1, scales, keys);
	sr2->setTolerance(sr1->tolerance());
	return sr2;
    }

    ConstSemiringRef appendSemiring(ConstSemiringRef sr1, ConstSemiringRef sr2) {
	ScoreList scales;
	scales.insert(scales.end(), sr1->scales().begin(), sr1->scales().end());
	scales.insert(scales.end(), sr2->scales().begin(), sr2->scales().end());
	KeyList keys;
	keys.insert(keys.end(), sr1->keys().begin(), sr1->keys().end());
	keys.insert(keys.end(), sr2->keys().begin(), sr2->keys().end());
	if (sr1->type() != sr2->type())
	    std::cerr << "In append semirings: Semiring type mismatch; use semiring type of left argument" << std::endl;
	if (sr1->tolerance() != sr2->tolerance())
	    std::cerr << "In append semirings: Tolerance mismatch; use tolerance of left argument" << std::endl;
	ConstSemiringRef sr3 = Semiring::create(sr1->type(), sr1->size() + sr2->size(), scales, keys);
	sr3->setTolerance(sr1->tolerance());
	return sr3;
    }

    ConstSemiringRef rescaleSemiring(ConstSemiringRef semiring, const ScoreList &scales, const KeyList &keys) {
	ConstSemiringRef _semiring = cloneSemiring(semiring);
	if (!scales.empty())
	    _semiring->setScales(scales);
	if (!keys.empty())
	    _semiring->setKeys(keys);
	return _semiring;
    }

    ConstSemiringRef rescaleSemiring(ConstSemiringRef semiring, ScoreId id, Score scale, const Key &key) {
	ConstSemiringRef _semiring = cloneSemiring(semiring);
	if (scale != Semiring::UndefinedScale)
	    _semiring->setScale(id, scale);
	if (key != Semiring::UndefinedKey)
	    _semiring->setKey(id, key);
	return _semiring;
    }

    ConstSemiringRef toLogSemiring(ConstSemiringRef semiring, Score scale) {
	if ((semiring->type() == Fsa::SemiringTypeLog) && (scale == 1.0))
	    return semiring;
	if (scale == Core::Type<Score>::max)
	    return toTropicalSemiring(semiring);
	ScoreList scales = semiring->scales();
	if (scale == 0.0) {
	    scale = Core::Type<Score>::min;
	    for (ScoreList::const_iterator it = scales.begin(); it != scales.end(); ++it)
		scale = std::max(scale, *it);
	    scale = 1.0 / scale;
	}
	if (scale != 1.0) {
	    for (ScoreList::iterator it = scales.begin(); it != scales.end(); ++it)
		*it *= scale;
	}
	ConstSemiringRef logSemiring = Semiring::create(
	    Fsa::SemiringTypeLog, semiring->size(), scales, semiring->keys());
	logSemiring->setTolerance(semiring->tolerance());
	return logSemiring;
    }

    ConstSemiringRef toTropicalSemiring(ConstSemiringRef semiring) {
	if (semiring->type() == Fsa::SemiringTypeTropical)
	    return semiring;
	ConstSemiringRef tropicalSemiring = Semiring::create(
	    Fsa::SemiringTypeTropical, semiring->size(), semiring->scales(), semiring->keys());
	tropicalSemiring->setTolerance(semiring->tolerance());
	return tropicalSemiring;
    }
    // -------------------------------------------------------------------------


} // namespace Flf
