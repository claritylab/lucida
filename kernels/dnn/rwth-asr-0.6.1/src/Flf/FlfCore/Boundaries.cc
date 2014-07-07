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
#include "Boundaries.hh"

namespace Flf {

    const Bliss::Phoneme::Id Boundary::Transit::InvalidId =
	Core::Type<Bliss::Phoneme::Id>::max;

    const Boundary::Transit Boundary::InvalidTransit =
	Boundary::Transit(Boundary::Transit::InvalidId, Boundary::Transit::InvalidId);


#ifdef MEM_DBG
    Boundaries::nBoundaries = 0;

    Boundaries::Boundaries() {
	++nBoundaries;
    }

    Boundaries::~Boundaries() {}
	--nBoundaries;
    }
#else
    Boundaries::Boundaries() {}

    Boundaries::~Boundaries() {}
#endif

    void Boundaries::dumpBoundary(Fsa::StateId sid, std::ostream &os) const {
	const Boundary &b = get(sid);
	if (b.valid()) {
	    os << "t=" << b.time();
	    if (b.transit().initial != Bliss::Phoneme::term) os << ",cw";
	} else os << "t=n/a";
    }

    class InvalidBoundaries_ : public Boundaries {
    public:
	InvalidBoundaries_() {}

	virtual bool valid() const
	    { return false; }
	virtual bool valid(Fsa::StateId sid) const
	    { return false; }
	virtual const Boundary& get(Fsa::StateId sid) const
	    { return InvalidBoundary; }
    };

    ConstBoundariesRef InvalidBoundaries = ConstBoundariesRef(new InvalidBoundaries_);

    bool StaticBoundaries::set(Fsa::StateId sid, const Boundary &boundary) {
	verify_((boundary.time() == Speech::InvalidTimeframeIndex)
	       || (boundary.time() < 4000000000));
	grow(sid);
	if ((*this)[sid].valid()) return false;
	(*this)[sid] = boundary;
	return true;
    }
    void StaticBoundaries::del(Fsa::StateId sid) {
	if (sid < size()) (*this)[sid] = InvalidBoundary;
    }

} // namespace Flf
