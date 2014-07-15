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
#ifndef _BLISS_PHONOLOGY_HH
#define _BLISS_PHONOLOGY_HH

#include <vector>
#include <Core/ReferenceCounting.hh>
#include "Phoneme.hh"
#include "Lexicon.hh"
#include <string>


namespace Bliss {

    /**
     * Abstract base class for all types of allophones.
     *
     * This class is currently empty and is here for purely structural
     * reasons.
     *
     * We could have used the term "phone" but it's too easily
     * confusable with "phoneme".
     */

    class Allophone {};


    /**
     * Abstract base-class for all phonological models.
     *
     * We use the term "phonology" for any mapping from phonemes
     * (broad, word-discriminating acoustic units) to allophones
     * (narrow, sound-oriented units).
     *
     * Currently @c Phonology is empty and serves purely structural
     * purposes.
     */

    class Phonology {};


    /**
     * Phonology for context dependent phonemes.
     *
     * The @c ContextPhonology allophone is the @c PhonemeInContext.
     * A phoneme in context is defined by its underlying phoneme and
     * its left and right context phonemes.  Such units are commonly
     * referred to as "n-phones" (e.g. triphone, quinphone,...).
     *
     * The conversion from a phoneme graph to a context-dependent
     * phoneme graph involves replication of vertices and edges
     * according to the various contexts in which they occur.  The
     * current implementation proceeds as follows:
     * -# Determine all left contexts (histories) of each vertex.
     * -# Determine all right contexts (futures) of each vertex.
     * -# For each phoneme graph vertex build the cartesian product of
     * left and right contexts and create an allophone graph vertex
     * for each context.
     */

    class ContextPhonology :
	public Phonology,
	public Core::ReferenceCounted
    {
    public:
	class SemiContext :
	    public std::basic_string<Phoneme::Id, Phoneme::Id_char_traits>
	{
	public:
	    typedef std::basic_string<Phoneme::Id, Phoneme::Id_char_traits> Precursor;
	    SemiContext() {};
	    SemiContext(Phoneme::Id p) : Precursor(1, p) {}
	    SemiContext(size_t s, Phoneme::Id p = Phoneme::term) : Precursor(s, p) {}
	    SemiContext(const SemiContext &o) : Precursor(o) {}

	    bool empty() const;

	    struct Hash { u32 operator() (const SemiContext &sc) const; };
	};
	struct Context {
	    SemiContext history, future;
	    Context() {}
	    Context(const SemiContext &h, const SemiContext &f) : history(h), future(f) {}
	    bool operator== (const Context &rhs) const {
		return history == rhs.history
		    && future  == rhs.future;
	    }
	    struct Hash {
		SemiContext::Hash sch;
		u32 operator() (const Context &c) const;
	    };
	};
	class PhonemeInContext;
	typedef PhonemeInContext Allophone;
    private:
	PhonemeInventoryRef pi_;
	s32 maximumHistoryLength_;
	s32 maximumFutureLength_;

	/**
	 * Extend semi-context by one phoneme.
	 * The semi-context is extended by the given phoneme at the
	 * point farest from the present phoneme.
	 * Assertion is raised if maximumLength is exceeded.
	 */
	static void append(SemiContext&, Phoneme::Id, s32 maximumLength);

	/**
	 * Push phoneme into semi-context.
	 * The semi-context is extended by the given phoneme at a
	 * point closest close to present phoneme.  If length exceeds
	 * maximumLength, the phoneme farest from present one is
	 * discarded.
	 */
	static void push(SemiContext&, Phoneme::Id, s32 maximumLength);

    public:
	ContextPhonology(PhonemeInventoryRef,
			 u32 maximumHistoryLength = 1,
			 u32 maximumFutureLength  = 1);

	PhonemeInventoryRef getPhonemeInventory() const { return pi_; }
	s32 maximumHistoryLength() const { return maximumHistoryLength_; }
	s32 maximumFutureLength() const { return maximumFutureLength_; }

	void appendFuture(Allophone&, Phoneme::Id) const;
	void appendFuture(Context&, Phoneme::Id) const;
	void appendHistory(Allophone&, Phoneme::Id) const;
	void appendHistory(Context&, Phoneme::Id) const;
	void pushFuture(Allophone&, Phoneme::Id) const;
	void pushFuture(Context&, Phoneme::Id) const;
	void pushHistory(Allophone&, Phoneme::Id) const;
	void pushHistory(Context&, Phoneme::Id) const;

	Allophone allophone(const Pronunciation&, int i) const;
	Allophone operator() (const Pronunciation &p, int i) const;
    };
    typedef Core::Ref<const ContextPhonology> ContextPhonologyRef;


    class ContextPhonology::PhonemeInContext :
	public Bliss::Allophone
    {
    private:
	friend class ContextPhonology;
	Phoneme::Id phoneme_;
	Context context_;

    public:
	PhonemeInContext() : phoneme_(Phoneme::term) {};
	PhonemeInContext(Phoneme::Id phoneme, const SemiContext &history = SemiContext(), const SemiContext &future = SemiContext()) :
	    phoneme_(phoneme), context_(history, future) {}

	bool operator== (const PhonemeInContext &rhs) const {
	    return phoneme_ == rhs.phoneme_
		&& context_ == rhs.context_;
	}

	bool operator!= (const PhonemeInContext &rhs) const {
	    return !(this->operator==(rhs));
	}

	Phoneme::Id central() const { return phoneme_; }

	Phoneme::Id phoneme(s16 pos = 0) const;
	void setPhoneme(s16 pos = 0, Phoneme::Id = Phoneme::term);

	const Context &context() const {
	    return context_;
	}
	const SemiContext &history() const {
	    return context_.history;
	}
	void setHistory(const SemiContext &history) {
	    context_.history = history;
	}
	const SemiContext &future() const {
	    return context_.future;
	}
	void setFuture(const SemiContext &future) {
	    context_.future = future;
	}

    private:
	std::string formatPhoneme(Core::Ref<const PhonemeInventory>, Phoneme::Id) const;
    public:
	std::string format(Core::Ref<const PhonemeInventory>) const;

	struct Hash {
	    Context::Hash ch;
	    u32 operator() (const PhonemeInContext&) const;
	};
	friend struct PhonemeInContext::Hash;
    };

} // namespace Bliss

#endif // _BLISS_PHONOLOGY_HH
