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
#include "StateModel.hh"

#include <Core/Hash.hh>
#include <Core/Utility.hh>
#include <Legacy/DecisionTree.hh>

using namespace Am;


std::string EmissionAlphabet::symbol(Fsa::LabelId id) const {
    std::string special = specialSymbol(id);
    if (!special.empty()) return special;
    if (id >= Fsa::LabelId(nMixtures_))
	return std::string("#") + Core::itoa(id - nMixtures_ + 1);
    return Core::itoa(id);
}

void EmissionAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment()
       << nMixtures_ << " emission labels, "
       << nDisambiguators_ << " disambiguation symbols"
       << Core::XmlCloseComment() << "\n";
}

// ===========================================================================
AllophoneStateAlphabet::AllophoneStateAlphabet() :
    contextLength_(0), nStates_(0), nClasses_(0), nDisambiguators_(0)
{}

AllophoneStateAlphabet::AllophoneStateAlphabet(
    Core::Ref<const Bliss::PhonemeInventory> pi, u32 contextLength,  u32 nStates) :
    pi_(pi), contextLength_(contextLength), nStates_(nStates), nDisambiguators_(0)
{
    calcNClasses();
}

void AllophoneStateAlphabet::set(Core::Ref<const Bliss::PhonemeInventory> pi, u32 contextLength,  u32 nStates) {
    pi_ = pi;
    contextLength_ = contextLength;
    nStates_ = nStates;
    calcNClasses();
}

void AllophoneStateAlphabet::calcNClasses() {
    nClasses_ = 4 * nStates_;
    for (u32 c = 0; c < 2 * contextLength_ + 1; ++c) {
	hope(nClasses_ <= Core::Type<u32>::max / (pi_->nPhonemes() + 1));
	nClasses_ *= pi_->nPhonemes() + 1;
    }
}

AllophoneStateAlphabet::Index AllophoneStateAlphabet::index(const AllophoneState &phone) const {
    require(nStates_ && contextLength_);
    require(phone.boundary < 4);
    require(0 <= phone.state && phone.state < (s16)nStates_);
    u32 result = 0;
    for (s32 i = - contextLength_; i <= s32(contextLength_); ++i) {
	result *= pi_->nPhonemes() + 1;
	result += phone.phoneme(i);
    }
    result *= 4;
    result += phone.boundary;
    result *= nStates_;
    result += phone.state;
    ensure(result < nClasses_);
    return result + 1;
}

AllophoneState AllophoneStateAlphabet::allophoneState(AllophoneStateAlphabet::Index in) const {
    require(nStates_ && contextLength_);
    require(in != Fsa::Epsilon);
    require(in < nClasses_);
    AllophoneState result;
    Index code = in - 1;
    result.state    = code % nStates_; code /= nStates_;
    result.boundary = code % 4;        code /= 4;
    for (s32 i = contextLength_; i >= - s32(contextLength_); --i) {
	result.setPhoneme(i, code % (pi_->nPhonemes() + 1));
	code /= pi_->nPhonemes() + 1;
    }
    ensure_(index(result) == in);
    return result;
}

std::string AllophoneStateAlphabet::symbol(Fsa::LabelId id) const {
    std::string special = specialSymbol(id);
    if (special != "") return special;
    std::string tmp;
    if (id > nClasses()) return std::string("#") + Core::itoa(tmp, id - nClasses_ - 1);
    AllophoneState as = allophoneState(id);
    std::string result = as.format(pi_);
    if (as.boundary & Allophone::isInitialPhone) result += "@i";
    if (as.boundary & Allophone::isFinalPhone)   result += "@f";
    result += "." + Core::itoa(tmp, as.state);
    return result;
}

void AllophoneStateAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment()
       << nClasses_ << " allophone state labels, "
       << nDisambiguators_ << " disambiguation symbols"
       << Core::XmlCloseComment() << "\n";
}

// ===========================================================================
EmissionToPhonemeTransducer::EmissionToPhonemeTransducer(
    u32 nMixtures, Core::Ref<const Bliss::PhonemeAlphabet> pa)
{
    setType(Fsa::TypeTransducer);
    setSemiring(Fsa::TropicalSemiring);
    setInputAlphabet(Fsa::ConstAlphabetRef(new EmissionAlphabet(nMixtures)));
    setOutputAlphabet(pa);
}

AllophoneStateToPhonemeTransducer::AllophoneStateToPhonemeTransducer(
    Core::Ref<const Bliss::PhonemeInventory> pi)
{
    setType(Fsa::TypeTransducer);
    setSemiring(Fsa::TropicalSemiring);
    setInputAlphabet(Fsa::ConstAlphabetRef(new AllophoneStateAlphabet()));
    setOutputAlphabet(pi->phonemeAlphabet());
}
