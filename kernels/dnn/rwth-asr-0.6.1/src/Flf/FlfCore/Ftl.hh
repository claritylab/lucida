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
#ifndef _FLF_FTL_HH
#define _FLF_FTL_HH

#include <Core/XmlStream.hh>
#include <Fsa/hInfo.hh>
#include <Fsa/hSort.hh>
#include <Fsa/Automaton.hh>
#include <Fsa/AlphabetUtility.hh>
#include <Fsa/Mapping.hh>
#include <Fsa/Types.hh>

#include "Lattice.hh"

namespace FtlWrapper {

    Flf::ConstLatticeRef best             (Flf::ConstLatticeRef l);
    Flf::ScoresRef       bestscore        (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef cache            (Flf::ConstLatticeRef l, u32 maxAge = 10000);
    Flf::ConstLatticeRef changeSemiring   (Flf::ConstLatticeRef l, Flf::ConstSemiringRef semiring);
    void                 cheapInfo        (Flf::ConstLatticeRef l, Core::XmlWriter &o);
    Flf::ConstLatticeRef composeMatching  (Flf::ConstLatticeRef l, Flf::ConstLatticeRef r);
    Flf::ConstLatticeRef composeSequencing(Flf::ConstLatticeRef l, Flf::ConstLatticeRef r);
    Fsa::AutomatonCounts count            (Flf::ConstLatticeRef l, bool progress);
    size_t               countInput       (Flf::ConstLatticeRef l, Fsa::LabelId label, bool progress = false);
    size_t               countOutput      (Flf::ConstLatticeRef l, Fsa::LabelId label, bool progress = false);
    Flf::ConstLatticeRef determinize      (Flf::ConstLatticeRef l, bool disambiguate = false);
    Flf::ConstLatticeRef difference       (Flf::ConstLatticeRef l, Flf::ConstLatticeRef r);
    bool                 drawDot          (Flf::ConstLatticeRef l, std::ostream &o,
					   Fsa::Hint hint = Fsa::HintNone, bool progress = false);
    bool                 drawDot          (Flf::ConstLatticeRef l, const std::string &file,
					   Fsa::Hint hint = Fsa::HintNone, bool progress = false);
    Flf::ConstLatticeRef firstbest        (Flf::ConstLatticeRef l);
    Fsa::Property        getProperties    (Flf::ConstLatticeRef l, Fsa::Property properties = Fsa::PropertyAll);
    void                 info             (Flf::ConstLatticeRef l, Core::XmlWriter &o,
					   bool progress = false);
    Flf::ConstLatticeRef invert           (Flf::ConstLatticeRef l);
    bool                 isAcyclic        (Flf::ConstLatticeRef l);
    bool                 isLinear         (Flf::ConstLatticeRef l);
    bool                 isEmpty          (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef mapInput         (Flf::ConstLatticeRef l, const Fsa::AlphabetMapping &m);
    Flf::ConstLatticeRef mapInputOutput   (Flf::ConstLatticeRef l, const Fsa::AlphabetMapping &m);
    Flf::ConstLatticeRef mapOutput        (Flf::ConstLatticeRef l, const Fsa::AlphabetMapping &m);
    Fsa::ConstMappingRef mapToLeft        (Flf::ConstLatticeRef l);
    Fsa::ConstMappingRef mapToRight       (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef minimize         (Flf::ConstLatticeRef l);
    void                 memoryInfo       (Flf::ConstLatticeRef l, Core::XmlWriter &o);
    Flf::ConstLatticeRef nbest            (Flf::ConstLatticeRef l, size_t n = 1, bool bestSequences = true);
    Flf::ConstLatticeRef normalize        (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef partial          (Flf::ConstLatticeRef l, Fsa::StateId initial);
    Flf::ConstLatticeRef projectInput     (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef projectOutput    (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef removeEpsilons   (Flf::ConstLatticeRef l);
    Flf::ConstLatticeRef sort             (Flf::ConstLatticeRef l, Fsa::SortType type);
    Flf::ConstLatticeRef trim             (Flf::ConstLatticeRef l, bool progress = false);
    void                 trimInPlace      (Flf::StaticLatticeRef l);

} // namespace FtlWrapper

#endif // _FLF_FTL_HH
