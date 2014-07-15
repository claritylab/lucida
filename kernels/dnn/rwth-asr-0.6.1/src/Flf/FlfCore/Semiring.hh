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
#ifndef _FLF_CORE_SEMIRING_HH
#define _FLF_CORE_SEMIRING_HH

#include <Core/Configuration.hh>
#include <Core/Hash.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>
#include <Core/XmlParser.hh>
#include <Core/XmlStream.hh>
#include <Fsa/tSemiring.hh>

#include "Types.hh"
#include "Weight.hh"

namespace Flf {
    typedef std::string Key;
    typedef std::vector<Key> KeyList;

    class Semiring;
    typedef Core::Ref<const Semiring> ConstSemiringRef;
    typedef Core::Vector<ConstSemiringRef> ConstSemiringRefList;

    class Semiring : public Ftl::Semiring<ScoresRef> {
	typedef Semiring Self;
	typedef Ftl::Semiring<ScoresRef> Precursor;
    public:
	typedef Core::Ref<Self> Ref;
	typedef Core::Ref<const Self> ConstRef;
	typedef Core::hash_map<Key, ScoreId, Core::StringHash> KeyMap;

	static const Score Zero;
	static const Score One;
	static const Score Max;
	static const Score Invalid;
	static const Score DefaultScale;
	static const Score UndefinedScale;
	static const ScoreId InvalidId;

	static const Key UndefinedKey;

	typedef s32 Tolerance;
	static const Tolerance DefaultTolerance;

    protected:
	const size_t n_;
	mutable Tolerance tolerance_;
	mutable ScoreList scales_;
	mutable KeyList keys_;
	mutable KeyMap keyMap_;

	ScoresRef one_;
	ScoresRef zero_;
	ScoresRef max_;
	ScoresRef invalid_;
	ScoresRef default_;

    protected:
	Scores* alloc() const { return new (n_) Scores; }
	Scores* alloc(const ScoresRef &a) const;
	Scores* alloc(const Score &init) const;

	/**
	 * convinient wrappers to get score iterators
	 **/
	Scores::iterator begin(Scores *a) const { return a->begin(); }
	Scores::iterator end(Scores *a)   const { return a->begin() + n_; }

    public:
	Semiring(size_t n, const ScoreList &scales = ScoreList(), const KeyList &keys = KeyList());
	~Semiring();

	bool operator==(const Semiring &semiring) const;

	/**
	 * size, type, and name
	 **/
	size_t size() const { return n_; }
	virtual std::string name() const = 0;
	virtual Fsa::SemiringType type() const = 0;

	/**
	 * tolerance
	 **/
	void setTolerance(Tolerance tolerance) const;
	Tolerance tolerance() const { return tolerance_; }

	/**
	 * scales
	 **/
	virtual void setScales(const ScoreList &scales) const;
	ScoreList & scales() const { return scales_; }
	virtual void setScale(ScoreId id, Score scale) const;
	Score scale(ScoreId id) const { return scales_[id]; }
	/*
	 * return value:
	 * 0 scales are equal
	 * 1 undefined values found
	 * 2 scales are not equal, no undefined values
	 * 3 scales are not equal and undefined values found
	 **/
	u32 cmpScales(const ScoreList &scales) const;

	/**
	 * identifier mapping
	 **/
	void setKeys(const KeyList &keys) const;
	KeyList & keys() const { return keys_; }
	bool setKey(ScoreId id, const Key &key) const;
	bool hasKey(const Key &key) const;
	const Key& key(ScoreId id) const { return keys_[id]; }
	bool hasId(ScoreId id) const;
	ScoreId id(const Key &key) const;

	/**
	 * create and clone weights and standard weights
	 **/
	virtual ScoresRef create() const { return ScoresRef(alloc()); }
	virtual ScoresRef clone(const ScoresRef &a) const { return ScoresRef(alloc(a)); }
	virtual ScoresRef one()     const { return one_; }
	virtual ScoresRef zero()    const { return zero_; }
	virtual ScoresRef max()     const { return max_; }
	virtual ScoresRef invalid() const { return invalid_; }
	virtual ScoresRef defaultWeight() const { return one_; }

	virtual bool isDefault(const ScoresRef &a) const;
	virtual size_t hash(const ScoresRef &a) const;

	/**
	 * iterate over score dimensions
	 **/
	Scores::const_iterator begin(const ScoresRef &a) const { return a->begin(); }
	Scores::const_iterator end(const ScoresRef &a)   const { return a->begin() + n_; }
	Scores::iterator begin(ScoresRef &a)             const { return a->begin(); }
	Scores::iterator end(ScoresRef &a)               const { return a->begin() + n_; }

	/**
	 * extend, collect, invert, project, and compare
	 **/
	virtual ScoresRef extend(const ScoresRef &a, const ScoresRef &b) const;
	// virtual ScoresRef collect(const ScoresRef &a, const ScoresRef &b) const = 0;
	virtual ScoresRef invert(const ScoresRef &a) const;
	virtual Score project(const ScoresRef &a) const;
	virtual int compare(const ScoresRef &a, const ScoresRef &b) const;

	class Extender : public Core::ReferenceCounted {
	public:
	    virtual ~Extender() {}
	    virtual void reset() = 0;
	    virtual void feed(ScoresRef) = 0;
	    virtual ScoresRef get() const = 0;
	};
	typedef Core::Ref<Extender> ExtenderRef;
	virtual ExtenderRef createExtender() const;

	class Collector : public Core::ReferenceCounted {
	public:
	    virtual ~Collector() {}
	    virtual void reset() = 0;
	    virtual void feed(ScoresRef) = 0;
	    virtual ScoresRef get() const = 0;
	};
	typedef Core::Ref<Collector> CollectorRef;
	virtual CollectorRef createCollector() const;

	/**
	 * i/o
	 **/
	virtual bool read(ScoresRef &a, Core::BinaryInputStream &i) const;
	virtual bool write(const ScoresRef &a, Core::BinaryOutputStream &o) const;
	virtual std::string asString(const ScoresRef &a) const;
	virtual ScoresRef fromString(const std::string &s) const;
	virtual std::string describe(const ScoresRef &a, Fsa::Hint hint = Fsa::HintNone) const;
	virtual void compress(ByteVector &stream, const ScoresRef &a) const;
	virtual ScoresRef uncompress(ByteVector::const_iterator &) const;
	virtual size_t compressedSize() const;

	/**
	 * compare semirings
	 **/
	static bool equal(ConstSemiringRef semiring1, ConstSemiringRef semiring2);

	/**
	 * create semirings
	 **/
	static ConstSemiringRef create(Fsa::SemiringType, size_t n, const ScoreList &scales = ScoreList(), const KeyList &keys = KeyList());

	/**
	 * config
	 *
	 * type       = tropical*|log
	 * tolerance  = 1
	 * keys       = key1 key2 ...
	 * key1.scale = 1.0
	 * key2.scale = 1.0
	 * ...
	 **/
	static ConstSemiringRef create(const Core::Configuration &config);

	/**
	 * XML
	 *
	 * ...
	 * <semiring>
	 *   <type> tropical* | log </type>
	 *   <tolerance> 1 </tolerance>
	 *   <dimension> <key> dim-0 </key> <scale> 1.0 </scale> </dimension>
	 *   <dimension> <key> dim-1 </key> <scale> 1.0 </scale> </dimension>
	 *   ...
	 * </semiring>
	 * ...
	 **/
	void write(Core::XmlWriter &xml) const;

	class XmlElement : public Core::XmlMixedElement {
	    friend class Semiring;
	    typedef Core::XmlMixedElement Precursor;
	private:
	    struct Internal;
	    Internal *internal_;
	private:
	    virtual void end();
	    virtual void characters(const char*, int) {}
	    XmlElement(Core::XmlContext *context);
	public:
	    virtual ~XmlElement();
	    ConstSemiringRef semiring() const;
	};
	static XmlElement * xmlElement(Core::XmlContext *context);
	static ConstSemiringRef read(std::istream &);
    };


    /**
     * Log semiring
     *
     **/
    class LogSemiring : public Semiring {
    public:
	LogSemiring(size_t n, const ScoreList &scales = ScoreList(), const KeyList &keys = KeyList());
	virtual ~LogSemiring() {}
	virtual Fsa::SemiringType type() const;
	virtual std::string name() const;
	virtual ScoresRef collect(const ScoresRef &a, const ScoresRef &b) const;
    };


    /**
     * Tropical semiring
     *
     **/
    class TropicalSemiring : public Semiring {
    public:
	TropicalSemiring(size_t n, const ScoreList &scales = ScoreList(), const KeyList &keys = KeyList());
	virtual ~TropicalSemiring() {}
	virtual Fsa::SemiringType type() const;
	virtual std::string name() const;
	virtual ScoresRef collect(const ScoresRef &a, const ScoresRef &b) const;
    };


    /**
     * Manipulate semirings; the original semiring is not modified.
     *
     * appendSemiring:
     * Append another semiring or a single dimension.
     *
     * rescaleSemiring:
     * Rescale/rename all dimesnions or a single dimension.
     *
     * toLogSemiring:
     * Return log-aquivalent of semiring.
     * If scale is not set, the dimensional scales are normalized, i.e. scale = 1.0 / (sum of dimensional scales).
     * Thus, a scale of 1.0 leaves the dimensional scales untouched.
     *
    **/
    ConstSemiringRef cloneSemiring(ConstSemiringRef semiring);
    ConstSemiringRef appendSemiring(ConstSemiringRef semiring1, ConstSemiringRef semiring2);
    ConstSemiringRef appendSemiring(ConstSemiringRef semiring, Score scale = Semiring::DefaultScale, const Key &key = Semiring::UndefinedKey);
    ConstSemiringRef rescaleSemiring(ConstSemiringRef semiring, const ScoreList &scales, const KeyList &keys = KeyList());
    ConstSemiringRef rescaleSemiring(ConstSemiringRef semiring, ScoreId id, Score scale, const Key &key = Semiring::UndefinedKey);
    ConstSemiringRef toLogSemiring(ConstSemiringRef semiring, Score scale = 0.0); // scale == 0.0 --> 1/<max-scale>
    ConstSemiringRef toTropicalSemiring(ConstSemiringRef semiring);


} // namespace Flf
#endif // _FLF_CORE_SEMIRING_HH
