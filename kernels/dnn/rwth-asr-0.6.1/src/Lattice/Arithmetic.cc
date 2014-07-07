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
#include "Arithmetic.hh"
#include "Basic.hh"
#include <Fsa/Arithmetic.hh>
#include <Fsa/Cache.hh>

namespace Lattice {

    struct Multiplier
    {
	Fsa::Weight value;
	Multiplier(f32 v) : value(v) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::multiply(fsa, value);
	}
    };

    ConstWordLatticeRef multiply(ConstWordLatticeRef l, Fsa::Weight value)
    {
	Multiplier m(value);
	return apply(l, m);
    }

    ConstWordLatticeRef multiply(ConstWordLatticeRef lattice, const std::vector<Fsa::Weight> &values)
    {
	require(lattice->nParts() == values.size());
	if (lattice) {
	    Core::Ref<WordLattice> l(new WordLattice);
	    l->setWordBoundaries(lattice->wordBoundaries());
	    for (u32 i = 0; i < lattice->nParts(); ++ i) {
		l->setFsa(Fsa::multiply(lattice->part(i), values[i]), lattice->name(i));
	    }
	    return l;
	}
	return ConstWordLatticeRef();
    }

    struct Extender
    {
	Fsa::Weight value;
	Extender(f32 v) : value(v) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::extend(fsa, value);
	}
    };

    ConstWordLatticeRef extend(ConstWordLatticeRef l, Fsa::Weight value)
    {
	Extender m(value);
	return apply(l, m);
    }

    struct FinalExtension
    {
	Fsa::Weight value;
	FinalExtension(f32 v) : value(v) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::extendFinal(fsa, value);
	}
    };

    ConstWordLatticeRef extendFinal(ConstWordLatticeRef l, Fsa::Weight value)
    {
	FinalExtension e(value);
	return apply(l, e);
    }

    struct Exponentiator
    {
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::expm(fsa);
	}
    };

    ConstWordLatticeRef expm(ConstWordLatticeRef l)
    {
	Exponentiator e;
	return apply(l, e);
    }

    ConstWordLatticeRef linearCombination(
	ConstWordLatticeRef lattice, const std::vector<Fsa::Weight> &scales)
    {
	if (lattice->nParts() > 0) {
	    Fsa::ConstAutomatonRef total =
		f32(scales[0]) != 1 ? Fsa::multiply(lattice->part(0), scales[0]) : lattice->part(0);
	    for (size_t i = 1; i < lattice->nParts(); ++ i) {
		total =
		    Fsa::extend(
			total,
			f32(scales[i]) != 1 ? Fsa::multiply(lattice->part(i), scales[i]) : lattice->part(i));
	    }
	    WordLattice *result = new WordLattice();
	    result->setWordBoundaries(lattice->wordBoundaries());
	    result->setFsa(Fsa::cache(total), WordLattice::totalFsa);
	    return ConstWordLatticeRef(result);
	}
	return ConstWordLatticeRef();
    }

    ConstWordLatticeRef linearCombination(
	ConstWordLatticeRef lattice,
	const Core::StringHashMap<std::vector<Fsa::Weight> > &outputs)
    {
	Core::Ref<WordLattice> result(new WordLattice);
	result->setWordBoundaries(lattice->wordBoundaries());
	Core::StringHashMap<std::vector<Fsa::Weight> >::const_iterator it = outputs.begin();
	for (; it != outputs.end(); ++ it) {
	    Fsa::ConstAutomatonRef f = linearCombination(
		lattice, it->second)->part(WordLattice::totalFsa);
	    result->setFsa(f, it->first);
	}
	return result;
    }

    ConstWordLatticeRef getParts(
	ConstWordLatticeRef lattice, const std::vector<std::string> &parts)
    {
	WordLattice *result = new WordLattice;
	result->setWordBoundaries(lattice->wordBoundaries());
	for (u32 i = 0; i < parts.size(); ++ i) {
	    if (lattice->hasPart(parts[i])) {
		result->setFsa(lattice->part(parts[i]), parts[i]);
	    }
	}
	return ConstWordLatticeRef(result);
    }

    ConstWordLatticeRef getPart(
	ConstWordLatticeRef lattice, const std::string &part)
    {
	return getParts(lattice, std::vector<std::string>(1, part));
    }

    struct GreaterEqual
    {
	Fsa::Weight threshold;
	GreaterEqual(Fsa::Weight t) : threshold(t) {}
	Fsa::ConstAutomatonRef modify(Fsa::ConstAutomatonRef fsa) {
	    return Fsa::isGreaterEqual(fsa, threshold);
	}
    };

    ConstWordLatticeRef isGreaterEqual(ConstWordLatticeRef l, Fsa::Weight threshold)
    {
	GreaterEqual c(threshold);
	return apply(l, c);
    }

} // namespace Lattice
