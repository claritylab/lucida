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
#include <Fsa/Static.hh>

#include "ClassicStateTying.hh"
#include "ClassicAcousticModel.hh"
#include "Module.hh"
#include <Core/CompressedStream.hh>




using namespace Am;


// ===========================================================================
std::string EmissionAlphabet::symbol(Fsa::LabelId id) const {
    std::string special = specialSymbol(id);
    if (!special.empty()) return special;
    if (id < 0)
	return "INVALID_LABEL_ID";
    if (Mm::MixtureIndex(id) < nMixtures_)
	return Core::itoa(id);
    return std::string("#") + Core::itoa(id - nMixtures_);
}

Fsa::LabelId EmissionAlphabet::index(const std::string &sym) const {
    Fsa::LabelId special = specialIndex(sym);
    if (special != Fsa::InvalidLabelId) return special;
    if (sym.size() && sym[0] == '#') {
	int di = atoi(sym.c_str() + 1);
	if (di < 0) return Fsa::InvalidLabelId;
	return disambiguator(di);
    }
    int id = atoi(sym.c_str());
    if (id < 0 || Mm::MixtureIndex(id) >= nMixtures_) return Fsa::InvalidLabelId;
    return id;
}

void EmissionAlphabet::writeXml(Core::XmlWriter &os) const {
    os << Core::XmlOpenComment()
       << nMixtures_ << " emission labels, "
       << nDisambiguators_ << " disambiguation symbols"
       << Core::XmlCloseComment() << "\n";
}
// ===========================================================================


// ============================================================================
const Core::ParameterString ClassicStateTying::paramFilename(
    "file",
    "external source defining the state tying");

Core::Ref<const ClassicStateTying> ClassicStateTying::createClassicStateTyingRef(
    const Core::Configuration &config, ClassicStateModelRef stateModelRef) {

    ClassicStateTying *result = Am::Module::instance().getStateTying(
	static_cast<ClassicAcousticModel::StateTyingType>(ClassicAcousticModel::paramType(config)),
	config, stateModelRef);
    if (!result || result->hasFatalErrors()) {
	delete result;
	return ClassicStateTyingRef();
    }
    return ClassicStateTyingRef(result);
}

Fsa::ConstAutomatonRef ClassicStateTying::createMixtureToAllophoneStateTransducer(
    s32 nDisambiguators) const {
    ConstEmissionAlphabetRef emissionAlphabet = ConstEmissionAlphabetRef(
	new EmissionAlphabet(nClasses()));

    Fsa::StaticAutomaton *f = new Fsa::StaticAutomaton(Fsa::TypeTransducer);
    f->setSemiring(Fsa::TropicalSemiring);
    f->setInputAlphabet(emissionAlphabet);
    f->setOutputAlphabet(alphabetRef_);
    f->setProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear, Fsa::PropertyNone);
    const Fsa::Weight One(f->semiring()->one());

    Fsa::State *initial, *final, *root;
    if (nDisambiguators > 0) {
	initial = final = f->newState();
	root = f->newState();
	for (Fsa::LabelId d = 0; d < Fsa::LabelId(nDisambiguators); ++d) {
	    initial->newArc(root->id(), One,
			    emissionAlphabet->disambiguator(d),
			    alphabetRef_->disambiguator(d));
	    root->newArc(final->id(), One,
			 emissionAlphabet->disambiguator(d),
			 alphabetRef_->disambiguator(d));
	}
    } else
	initial = final = root = f->newState();
    f->setInitialStateId(initial->id());
    final->setFinal(One);

    for (std::pair<AllophoneStateIterator, AllophoneStateIterator> it =
	     alphabetRef_->allophoneStates(); it.first != it.second; ++it.first)
	root->newArc(root->id(), One, classify(it.first.allophoneState()), it.first.id());
    return Fsa::ConstAutomatonRef(f);
}

void ClassicStateTying::dumpStateTying(Core::Channel &dump) const {
    for (std::pair<AllophoneStateIterator, AllophoneStateIterator> it = alphabetRef_->allophoneStates();
	 it.first != it.second; ++it.first) {
	Mm::MixtureIndex mix = classify(it.first.allophoneState());
	dump << Core::form("%s %d\n",
			   alphabetRef_->symbol(it.first.id()).c_str(),
			   mix);
    }
}
// ============================================================================


// ============================================================================
MonophoneStateTying::MonophoneStateTying(
    const Core::Configuration & config,
    ClassicStateModelRef stateModel) :
    Core::Component(config),
    ClassicStateTying(config, stateModel) {
    nPhonemes_ = Mm::MixtureIndex(stateModel->phonology().getPhonemeInventory()->nPhonemes());
    nClasses_ = 0;
    verify(Bliss::Phoneme::term == 0);
    for (Bliss::Phoneme::Id id = 1; id <= nPhonemes_; ++id) {
	verify(stateModel->phonology().getPhonemeInventory()->isValidPhonemeId(id));
	for (int state = 0; state < stateModel->hmmTopologySet().get(id)->nPhoneStates(); ++state, ++nClasses_) {
	    size_t i = id + state * nPhonemes_;
	    if (classIds_.size() < i + 1)
		classIds_.resize(i + 1);
	    classIds_[i] = nClasses_;
	}
    }
    /*
      if (classifyDumpChannel_.isOpen())
      dumpStateTying(classifyDumpChannel_);
    */
}
// ============================================================================


// ============================================================================
bool LutStateTying::loadLut(const std::string & filename) {
    /*
      nClasses_ and lut_ are set
    */
    verify(!filename.empty());
    Core::CompressedInputStream cis(filename);
    if (!cis.isOpen()) {
	criticalError("cannot open lookup table '%s'", filename.c_str());
    }
    std::string line;
    Mm::MixtureIndex topMixtureId = 0;
    Core::ProgressIndicator *progress = new Core::ProgressIndicator("state tying lookup from file");
    progress->start();
    u32 nLine = 0;
    while (cis) {
	++nLine;
	std::getline(cis, line);
	Core::stripWhitespace(line);
	if (!line.empty() && (*line.c_str() != '#')) {
	    Mm::MixtureIndex mixtureId;
	    std::string allophoneStateString;
	    std::vector<std::string> fields = Core::split(line, " ");
	    if(fields.size() != 2) {
		criticalError("failure while reading line %d from file \"%s\"", nLine, filename.c_str());
		return false;
	    }
	    allophoneStateString = fields[0],
	    Core::strconv(fields[1], mixtureId);
	    AllophoneStateIndex allophoneStateId = alphabetRef_->index(allophoneStateString);

	    // TODO: check if already set
	    lut_[allophoneStateId] = mixtureId;
	    if(mixtureId > topMixtureId)
		topMixtureId = mixtureId;

	    progress->notify();
	}
    }
    nClasses_ = topMixtureId + 1;
    progress->finish();
    delete progress;
    /*
      if (classifyDumpChannel_.isOpen())
	  dumpStateTying(classifyDumpChannel_);
    */
    return true;
}
