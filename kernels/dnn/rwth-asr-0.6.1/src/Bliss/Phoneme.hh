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
// $Id: Phoneme.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _BLISS_PHONEME_HH
#define _BLISS_PHONEME_HH

#include <map>
#include <vector>
#include <string>
#include <Core/Assertions.hh>
#include <Core/BinaryStream.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>
#include "Symbol.hh"


namespace Bliss {


    struct u16_char_traits {

	typedef u16 		char_type;
	typedef int 	        int_type;
	typedef std::streampos 	pos_type;
	typedef std::streamoff 	off_type;

	static void
	assign(u16& __c1, const u16& __c2)
	    { __c1 = __c2; }

	static bool
	eq(const u16& __c1, const u16& __c2)
	    { return __c1 == __c2; }

	static bool
	lt(const u16& __c1, const u16& __c2)
	    { return __c1 < __c2; }

	static int
	compare(const u16* __s1, const u16* __s2, size_t __n) {
	    u16* first = const_cast<u16*>(__s1);
	    u16* second = const_cast<u16*>(__s2);

	    for (unsigned int i=1; i <= __n; i++) {
		if ( *first < *second )
		    return -1;
		if ( *first > *second)
		    return 1;
		first++;
		second++;
	    }
	    return 0;
	}

//  	static size_t
//  	length(const u16* __s)
//  	    { return strlen(__s); }

	static const u16*
	find(const u16* __s, size_t __n, const u16& __a) {
	    u16* ptr = const_cast<u16*>(__s);

	    while (ptr < (__s + __n) ) {
		if (*ptr == __a) return ptr;
	    }
	    return NULL;
	}

	static u16*
	move(u16* __s1, const u16* __s2, size_t __n)
	    { return static_cast<u16*>(memmove(__s1, __s2, 2 * __n)); }

	static u16*
	copy(u16* __s1, const u16* __s2, size_t __n)
	    {  return static_cast<u16*>(memcpy(__s1, __s2, 2 * __n)); }

	static u16*
	assign(u16* __s, size_t __n, u16 __a) {
	    u16* ptr = __s;
	    for (unsigned int i=1; i <= __n; i++) {
		*ptr = __a;
		ptr++;
	    }
	    return __s;
	}

	static u16
	to_char_type(const int& __c)
	    { return static_cast<u16>(__c); }

	// To keep both the byte 0xff and the eof symbol 0xffffffff
	// from ending up as 0xffffffff.
	static int
	to_int_type(const u16& __c)
	    { return static_cast<int>(__c); }


	static bool
	eq_int_type(const int& __c1, const int& __c2)
	    { return __c1 == __c2; }

	static int
	eof() { return static_cast<int>(EOF); }

	static int
	not_eof(const int& __c)
	    { return (__c == eof()) ? 0 : __c; }

    };

    /** Phonemic symbol */
    class Phoneme : public Token {
    public:
	typedef u16 Id ;
	typedef Bliss::u16_char_traits Id_char_traits;
	static const Id term = 0;
    private:
	bool isContextDependent_;
    public:
	void setContextDependent(bool cd) {
	    isContextDependent_ = cd;
	}
	bool isContextDependent() const {
	    return isContextDependent_;
	}
    private:
	friend class PhonemeInventory;
	Phoneme();
    };

    class PhonemeAlphabet;

    /** Inventory of phonemic symbols */
    class PhonemeInventory : public Core::ReferenceCounted {
    private:
	friend class PhonemeAlphabet;
	TokenInventory phonemes_;
	struct Internal; Internal *internal_;

    public:
	/** Size of the phoneme inventory. */
	u32 nPhonemes() const {
	    return phonemes_.size() - 1;
	}

	/**
	 * Get phoneme by symbol.
	 * @param symbol a string containing a phoneme symbol.
	 * @return the phoneme which is assigned to the symbol
	 * @c symbol, or 0 if none is found.
	 */
	const Phoneme *phoneme(const std::string &symbol) const {
	    return static_cast<Phoneme*>(phonemes_[symbol]);
	}

	bool isValidPhonemeId(Phoneme::Id id) const {
	    return 0 < id && id <= nPhonemes();
	}

	/**
	 * Get phoneme by ID.
	 * @param id the requested phoneme id.  @c id must be a valid
	 * phoneme Id.  This can be tested with isValidPhonemeId().
	 * @return the phoneme with id number @c id.
	 */
	const Phoneme *phoneme(Phoneme::Id id) const {
	    require(isValidPhonemeId(id));
	    return static_cast<Phoneme*>(phonemes_[id]);
	}

	typedef const Phoneme *const *PhonemeIterator;
	std::pair<PhonemeIterator, PhonemeIterator> phonemes() const {
	    return std::make_pair(
		PhonemeIterator(phonemes_.begin() + 1),
		PhonemeIterator(phonemes_.end()));
	}

	PhonemeInventory();
#ifdef DEPRECATED
	PhonemeInventory(const PhonemeInventory &);
#endif
	~PhonemeInventory();

	/** Create a new phoneme. */
	Phoneme *newPhoneme();

	/** Assign a symbol to a phoneme. */
	void assignSymbol(Phoneme*, const std::string&);

	void writeBinary(Core::BinaryOutputStream&) const;
	void writeXml(Core::XmlWriter&) const;

	/** Returns all phones which match the given phoneme selection.
	    The selection may contain wildcards at boundaries to match multiple phonemes. */
	std::set<Bliss::Phoneme::Id> parseSelection(std::string selection) const;

	Core::Ref<const PhonemeAlphabet> phonemeAlphabet() const;
    };
    typedef Core::Ref<const PhonemeInventory> PhonemeInventoryRef;

    template <typename T>
    class PhonemeMap {
    public:
	typedef T Value;
    private:
	std::vector<T> store_;
    public:
	PhonemeMap(Core::Ref<const PhonemeInventory> pi) :
	    store_(pi->nPhonemes()) {}

	void fill(const Value &v) {
	    std::fill(store_.begin(), store_.end(), v);
	}

	Value &operator[](const Phoneme *p) {
	    require(p);
	    verify(p->id() <= Phoneme::Id(store_.size()));
	    return store_[p->id() - 1];
	}

	const Value &operator[](const Phoneme *p) const {
	    require(p);
	    verify(p->id() <= Phoneme::Id(store_.size()));
	    return store_[p->id() - 1];
	}

	Value &operator[](Phoneme::Id id) {
	    require(0 < id && id <= Phoneme::Id(store_.size()));
	    return store_[id - 1];
	}

	const Value &operator[](Phoneme::Id id) const {
	    require(0 < id && id <= Phoneme::Id(store_.size()));
	    return store_[id - 1];
	}
    };

} // namespace Bliss

#endif // _BLISS_PHONEME_HH
