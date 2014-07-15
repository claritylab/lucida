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
#include <Flf/FlfCore/Ftl.hh>
#include <Flf/FlfCore/LatticeInternal.hh>
#include <Fsa/tAccessible.cc>
template class Ftl::CoaccessibleDfsState<Flf::Lattice>;


#include <Fsa/tAlphabet.hh>
Flf::ConstLatticeRef FtlWrapper::mapInput(Flf::ConstLatticeRef l, const Fsa::AlphabetMapping &m)
{ return Ftl::mapInput<Flf::Lattice>(l, m); }
Flf::ConstLatticeRef FtlWrapper::mapOutput(Flf::ConstLatticeRef l, const Fsa::AlphabetMapping &m)
{ return Ftl::mapOutput<Flf::Lattice>(l, m); }
Flf::ConstLatticeRef FtlWrapper::mapInputOutput(Flf::ConstLatticeRef l, const Fsa::AlphabetMapping &m)
{ return Ftl::mapInputOutput<Flf::Lattice>(l, m); }


#include <Fsa/tBasic.hh>
Flf::ConstLatticeRef FtlWrapper::changeSemiring(Flf::ConstLatticeRef l, Flf::ConstSemiringRef semiring)
{ return Ftl::changeSemiring<Flf::Lattice>(l, semiring); }
Flf::ConstLatticeRef FtlWrapper::normalize(Flf::ConstLatticeRef l)
{ return Ftl::normalize<Flf::Lattice>(l); }
Flf::ConstLatticeRef FtlWrapper::partial(Flf::ConstLatticeRef l, Fsa::StateId initial)
{ return Ftl::partial<Flf::Lattice>(l, initial); }
Flf::ConstLatticeRef FtlWrapper::trim(Flf::ConstLatticeRef l, bool progress)
{ return Ftl::trim<Flf::Lattice>(l, progress); }


#include <Fsa/tBest.hh>
Flf::ConstLatticeRef FtlWrapper::best(Flf::ConstLatticeRef l)
{ return Ftl::best<Flf::Lattice>(l); }
Flf::ScoresRef FtlWrapper::bestscore(Flf::ConstLatticeRef l)
{ return Ftl::bestscore<Flf::Lattice>(l); }
Flf::ConstLatticeRef FtlWrapper::firstbest(Flf::ConstLatticeRef l)
{ return Ftl::firstbest<Flf::Lattice>(l); }
Flf::ConstLatticeRef FtlWrapper::nbest(Flf::ConstLatticeRef l, size_t n, bool bestSequences)
{ return Ftl::nbest<Flf::Lattice>(l, n, bestSequences); }


#include <Fsa/tCache.hh>
Flf::ConstLatticeRef FtlWrapper::cache(Flf::ConstLatticeRef l, u32 maxAge)
{ return Ftl::cache<Flf::Lattice>(l, maxAge); }


#include <Fsa/tCompose.hh>
Flf::ConstLatticeRef FtlWrapper::composeMatching(Flf::ConstLatticeRef l, Flf::ConstLatticeRef r)
{ return Ftl::composeMatching<Flf::Lattice>(l, r); }
Flf::ConstLatticeRef FtlWrapper::composeSequencing(Flf::ConstLatticeRef l, Flf::ConstLatticeRef r)
{ return Ftl::composeSequencing<Flf::Lattice>(l, r); }
Flf::ConstLatticeRef FtlWrapper::difference(Flf::ConstLatticeRef l, Flf::ConstLatticeRef r)
{ return Ftl::difference<Flf::Lattice>(l, r); }
Fsa::ConstMappingRef FtlWrapper::mapToLeft(Flf::ConstLatticeRef l)
{ return Ftl::mapToLeft<Flf::Lattice>(l); }
Fsa::ConstMappingRef FtlWrapper::mapToRight(Flf::ConstLatticeRef l)
{ return Ftl::mapToRight<Flf::Lattice>(l); }


#include <Fsa/tDeterminize.hh>
Flf::ConstLatticeRef FtlWrapper::determinize(Flf::ConstLatticeRef l, bool disambiguate)
{ return Ftl::determinize<Flf::Lattice>(l, disambiguate); }


#include <Fsa/tDraw.hh>
bool FtlWrapper::drawDot(Flf::ConstLatticeRef l, std::ostream &o, Fsa::Hint hint, bool progress)
{ return Ftl::drawDot<Flf::Lattice>(l, o, hint, progress); }
bool FtlWrapper::drawDot(Flf::ConstLatticeRef l, const std::string &file, Fsa::Hint hint, bool progress)
{ return Ftl::drawDot<Flf::Lattice>(l, file, hint, progress); }


#include <Fsa/tInfo.hh>
void FtlWrapper::cheapInfo(Flf::ConstLatticeRef l, Core::XmlWriter &o)
{ Ftl::cheapInfo<Flf::Lattice>(l, o); }
Fsa::AutomatonCounts FtlWrapper::count(Flf::ConstLatticeRef l, bool progress)
{ return Ftl::count<Flf::Lattice>(l, progress); }
size_t FtlWrapper::countInput(Flf::ConstLatticeRef l, Fsa::LabelId label, bool progress)
{ return Ftl::countInput<Flf::Lattice>(l, progress); }
size_t FtlWrapper::countOutput(Flf::ConstLatticeRef l, Fsa::LabelId label, bool progress)
{ return Ftl::countOutput<Flf::Lattice>(l, progress); }
void FtlWrapper::info(Flf::ConstLatticeRef l, Core::XmlWriter &o, bool progress)
{ Ftl::info<Flf::Lattice>(l, o, progress); }
bool FtlWrapper::isEmpty(Flf::ConstLatticeRef l)
{ return Ftl::isEmpty<Flf::Lattice>(l); }
void FtlWrapper::memoryInfo(Flf::ConstLatticeRef l, Core::XmlWriter &o)
{ Ftl::memoryInfo<Flf::Lattice>(l, o); }


#include <Fsa/tLinear.hh>
bool isLinear(Flf::ConstLatticeRef l)
{ return Ftl::isLinear<Flf::Lattice>(l); }


#include <Fsa/tMinimize.hh>
Flf::ConstLatticeRef FtlWrapper::minimize(Flf::ConstLatticeRef l)
{ return Ftl::minimize<Flf::Lattice>(l); }


#include <Fsa/tProject.hh>
Flf::ConstLatticeRef FtlWrapper::projectInput(Flf::ConstLatticeRef l)
{ return Ftl::projectInput<Flf::Lattice>(l); }
Flf::ConstLatticeRef FtlWrapper::projectOutput(Flf::ConstLatticeRef l)
{ return Ftl::projectOutput<Flf::Lattice>(l); }
Flf::ConstLatticeRef FtlWrapper::invert(Flf::ConstLatticeRef l)
{ return Ftl::invert<Flf::Lattice>(l); }


#include <Fsa/tProperties.hh>
Fsa::Property getProperties(Flf::ConstLatticeRef l, Fsa::Property properties)
{ return Ftl::getProperties<Flf::Lattice>(l, properties); }


#include <Fsa/tRemoveEpsilons.hh>
Flf::ConstLatticeRef FtlWrapper::removeEpsilons(Flf::ConstLatticeRef l)
{ return Ftl::removeEpsilons<Flf::Lattice>(l); }


#include <Fsa/tSort.hh>
bool FtlWrapper::isAcyclic(Flf::ConstLatticeRef l)
{ return Ftl::isAcyclic<Flf::Lattice>(l); }
Flf::ConstLatticeRef FtlWrapper::sort(Flf::ConstLatticeRef l, Fsa::SortType type)
{ return Ftl::sort<Flf::Lattice>(l, type); }


#include <Fsa/tStaticAlgorithm.hh>
void FtlWrapper::trimInPlace(Flf::StaticLatticeRef l)
{ Ftl::trimInPlace<Flf::Lattice>(l); }
