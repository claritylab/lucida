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
#ifndef _FLF_CORE_BOUNDARIES_HH
#define _FLF_CORE_BOUNDARIES_HH

#include <Bliss/Phoneme.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Vector.hh>
#include <Fsa/Mapping.hh>

#include "Types.hh"

/*
  A successor of Boundaries must have no reference to any lattice;
  only weak references are allowed.
*/

namespace Flf {

    /**
     * State Boundary
     *
     **/
    class Boundary {
    public:
	struct Transit {
	    static const Bliss::Phoneme::Id InvalidId;
	    /*
	      final   <=> right-across-context
	      initial <=> left-across-context
	    */
	    Bliss::Phoneme::Id final, initial;
	    /*
	      cross-word or within-word boundary
	    */
	    u8 boundary;
	    Transit(
		Bliss::Phoneme::Id final = Bliss::Phoneme::term,
		Bliss::Phoneme::Id initial = Bliss::Phoneme::term,
		u8 boundary = AcrossWordBoundary) :
		final(final),
		initial(initial),
		boundary(boundary) {}

	    bool operator== (const Transit &t) const
		{ return (final == t.final) && (initial == t.initial) && (boundary == t.boundary); }
	    bool operator!= (const Transit &t) const
		{ return (final != t.final) || (initial != t.initial) || (boundary != t.boundary); }
	};
	static const Transit InvalidTransit;

    private:
	Speech::TimeframeIndex time_;
	Transit transit_;
    public:
	Boundary() : time_(Speech::InvalidTimeframeIndex) {}
	Boundary(Speech::TimeframeIndex time) :
	    time_(time), transit_() {}
	Boundary(Speech::TimeframeIndex time, const Transit &transit) :
	    time_(time), transit_(transit) {}

	void setTime(Speech::TimeframeIndex time) { time_ = time; }
	Speech::TimeframeIndex time() const { return time_; }
	void setTransit(const Transit &transit) { transit_ = transit; }
	Transit transit() const { return transit_; }
	bool operator== (const Boundary &b) const
	    { return (time_ == b.time_) && (transit_ == b.transit_); }
	bool operator!= (const Boundary &b) const
	    { return (time_ != b.time_) && (transit_ != b.transit_); }
	bool valid() const { return time_ != Speech::InvalidTimeframeIndex; }

	struct Hash {
	    size_t operator() (const Boundary &b) const {
		return size_t(b.time())
		    ^ ((size_t(b.transit().final) << 25) | (size_t(b.transit().final) >> 7))
		    ^ ((size_t(b.transit().initial) << 18) | (size_t(b.transit().initial) >> 14))
		    ^ ((size_t(b.transit().boundary) << 11) | (size_t(b.transit().boundary) >> 11));
	    }
	};
	struct Equal {
	    bool operator() (const Boundary &b1, const Boundary &b2) const {
		return b1 == b2;
	    }
	};
    };
    const Boundary InvalidBoundary(Speech::InvalidTimeframeIndex, Boundary::Transit());


    /**
     * State Boundaries;
     * collection of state boundary.
     *
     **/
    class Boundaries : public Core::ReferenceCounted {
    public:
#ifdef MEM_DBG
	static u32 nBoundaries;
#endif
	Boundaries();
	virtual ~Boundaries();

	/**
	 * false, if the lattice provides no boundaries
	 **/
	virtual bool valid() const = 0;
	/**
	 * false, if that specific boundary does not exist or is invalid, e.g. not set
	 * !valid() => !valid(sid) for all state id
	 **/
	virtual bool valid(Fsa::StateId sid) const = 0;
	/**
	 * return a specific boundary; for every sid a boundary is returned
	 * !valid(sid) => !get(sid).valid()
	 **/
	virtual const Boundary& get(Fsa::StateId sid) const = 0;

	/**
	 * undefined, if !valid(sid)
	 **/
	Speech::TimeframeIndex time(Fsa::StateId sid) const
	    { return get(sid).time(); }
	/**
	 * undefined, if !valid(sid)
	 **/
	Boundary::Transit transit(Fsa::StateId sid) const
	    { return get(sid).transit(); }

	void dumpBoundary(Fsa::StateId sid, std::ostream &os) const;
    };
    typedef Core::Ref<const Boundaries> ConstBoundariesRef;
    typedef Core::Vector<ConstBoundariesRef> ConstBoundariesRefList;

    extern ConstBoundariesRef InvalidBoundaries;


    class MappedBoundaries : public Boundaries {
    private:
	ConstBoundariesRef boundaries_;
	Fsa::ConstMappingRef map_;

    public:
	MappedBoundaries(ConstBoundariesRef boundaries, Fsa::ConstMappingRef map) :
	    boundaries_(boundaries), map_(map) {}
	virtual ~MappedBoundaries() {}

	virtual bool valid() const
	    { return boundaries_->valid(); }

	virtual bool valid(Fsa::StateId sid) const
	    { return boundaries_->valid(map_->map(sid)); }

	virtual const Boundary& get(Fsa::StateId sid) const
	    { return boundaries_->get(map_->map(sid)); }
    };


    class StaticBoundaries :
	public Core::Vector<Boundary>,
	public Boundaries
    {
	typedef Core::Vector<Boundary> Precursor;
    public:
	typedef Precursor::iterator iterator;
	typedef Precursor::const_iterator const_iterator;
    public:
	StaticBoundaries() {}
	virtual ~StaticBoundaries() {}

	bool operator==(const StaticBoundaries &boundaries) const {
	    return (size() != boundaries.size()) ? false :
		std::equal(begin(), end(), boundaries.begin());
	}
	virtual bool valid() const
	    { return true; }
	virtual bool valid(Fsa::StateId sid) const
	    { return get(sid).valid(); }
	virtual const Boundary& get(Fsa::StateId sid) const
	    { return (sid < size()) ? operator[](sid) : InvalidBoundary; }

	/**
	 * set and return true, if precondition !valid(sid)
	 **/
	bool set(Fsa::StateId sid, const Boundary &boundary);
	/**
	 * postcondition: !valid(sid)
	 **/
	void del(Fsa::StateId sid);
    };
    typedef Core::Ref<StaticBoundaries> StaticBoundariesRef;

} // namespace Flf

#endif // _FLF_CORE_BOUNDARIES_HH
