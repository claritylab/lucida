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
#include "TextDependentSequenceFiltering.hh"
#include "AlignmentNode.hh"
#include <Am/Module.hh>
#include <Flow/DataAdaptor.hh>

using namespace Speech;


//------------------------------------------------------------------------------
// AlignmentToSequenceSelectionNode::Set

const Core::ParameterStringVector AlignmentToSequenceSelectionNode::Set::paramSymbols(
    "symbols", "set of filtered symbols");
const Core::ParameterBool AlignmentToSequenceSelectionNode::Set::paramShallComplement(
    "complement", "complements the underlying set?", false);

AlignmentToSequenceSelectionNode::Set::Set(const Core::Configuration &c) :
    Core::Component(c),
    shallComplement_(false),
    needInit_(false)
{
    Bliss::LexiconRef lexicon = Bliss::Lexicon::create(select("lexicon"));
    if (!lexicon) criticalError("Failed to initialize the lexicon.");

    Core::Ref<Am::AcousticModel> acousticModel = Am::Module::instance().createAcousticModel(
	c, lexicon, Am::AcousticModel::noEmissions |
	Am::AcousticModel::noStateTying | Am::AcousticModel::noStateTransition);
    if (!acousticModel) criticalError("Failed to create the acoustic model.");

    allophoneStateAlphabet_ = acousticModel->allophoneStateAlphabet();
    phonemeInventory_ = acousticModel->phonemeInventory();

    storeSymbols(paramSymbols(config));
    storeComplementing(paramShallComplement(config));
}

AlignmentToSequenceSelectionNode::Set::~Set()
{}

void AlignmentToSequenceSelectionNode::Set::initialize()
{
    if (needInit_) {
	setSymbols(symbols_);
	if (empty())
	    setComplementing(true);
    }
    needInit_ = false;
}

bool AlignmentToSequenceSelectionNode::Set::setParameter(const std::string &name, const std::string &value)
{
    std::string n(name);
    if (!acceptParameterNamePrefix(n))
	return false;

    if (paramSymbols.match(n)) {
	storeSymbols(paramSymbols(value));
    } else if (paramShallComplement.match(n)) {
	storeComplementing(paramShallComplement(value));
    } else
	return false;
    return true;
}

bool AlignmentToSequenceSelectionNode::Set::acceptParameterNamePrefix(std::string &n) const
{
    std::string prefix = name() + ".";
    if (n.find(prefix) == 0) {
	n = n.substr(prefix.size());
	return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// AlignmentToSequenceSelectionNode::PhonemeSet

void AlignmentToSequenceSelectionNode::PhonemeSet::setSymbols(
    const std::vector<std::string> &symbols)
{
    delete phonemeMap_;
    phonemeMap_ = new Bliss::PhonemeMap<char>(phonemeInventory_);
    phonemeMap_->fill((char)false);
    empty_ = true;
    for(std::vector<std::string>::const_iterator i = symbols.begin(); i != symbols.end(); i++) {
	const Bliss::Phoneme *ph = phonemeInventory_->phoneme(*i);
	if (ph) {
	    (*phonemeMap_)[ph] = (char)true;
	    empty_ = false;
	} else
	    error("Phoneme '%s' not found in the phoneme inventory.", i->c_str());
    }
    if (!symbols.empty() && empty_) {
	std::stringstream ss;
	std::copy(symbols.begin(), symbols.end(), std::ostream_iterator<std::string>(ss, " "));
	warning("The symbols '%s' yielded an empty phoneme set.", ss.str().c_str());
    }
}

//------------------------------------------------------------------------------
// AlignmentToSequenceSelectionNode::StateSet

void AlignmentToSequenceSelectionNode::StateSet::setSymbols(
    const std::vector<std::string> &symbols)
{
    for(std::vector<std::string>::const_iterator i = symbols.begin(); i != symbols.end(); i++) {
	char *endptr;
	State state = strtol(i->c_str(), &endptr, 10);
	if (endptr == 0 || *endptr == '\0')
	    set_.insert(state);
	else
	    error("Could not parse state id '%s'.", i->c_str());
    }
}

//------------------------------------------------------------------------------
// AlignmentToSequenceSelectionNode

AlignmentToSequenceSelectionNode::AlignmentToSequenceSelectionNode(const Core::Configuration &c) :
    Core::Component(c),
    Flow::Node(c)
{
    sets_.push_back(Core::ref(new PhonemeSet(select("phoneme-selection"))));
    sets_.push_back(Core::ref(new StateSet(select("state-selection"))));

    addInputs(1);
    addOutputs(2);
}

bool AlignmentToSequenceSelectionNode::setParameter(const std::string &name, const std::string &value)
{
    for(Sets::iterator set = sets_.begin(); set != sets_.end(); ++ set) {
	if ((*set)->setParameter(name, value))
	    return true;
    }
    return false;
}

bool AlignmentToSequenceSelectionNode::configure()
{
    initialize();

    Core::Ref<Flow::Attributes> attributeSelection(new Flow::Attributes);
    getInputAttributes(0, *attributeSelection);
    Core::Ref<Flow::Attributes> attributeRatio(new Flow::Attributes(*attributeSelection));

    if (!configureDatatype(attributeSelection, Flow::DataAdaptor<Alignment>::type()))
	return false;

    attributeSelection->set("datatype", Flow::Vector<bool>::type()->name());
    attributeRatio->set("datatype", Flow::Float32::type()->name());
    return putOutputAttributes(0, attributeSelection) && putOutputAttributes(1, attributeRatio);
}

void AlignmentToSequenceSelectionNode::initialize()
{
    for(Sets::iterator set = sets_.begin(); set != sets_.end(); ++ set)
	(*set)->initialize();
}

bool AlignmentToSequenceSelectionNode::work(Flow::PortId p)
{
    Flow::DataPtr<Flow::DataAdaptor<Alignment> > in;
    if (getData(0, in)) {
	const Alignment &alignment = in->data();
	Flow::Vector<bool> *selection = new Flow::Vector<bool>(alignment.size());
	selection->setTimestamp(*in);
	Flow::Float32 *ratio = new Flow::Float32();
	ratio->setTimestamp(*in);
	size_t filteredFeatureCount = 0;

	Flow::Vector<bool>::iterator s = selection->begin();
	Alignment::const_iterator a = alignment.begin();
	for(; a != alignment.end(); ++ a, ++ s) {
	    *s = true;
	    for(Sets::const_iterator set = sets_.begin(); set != sets_.end(); ++ set)
		*s = *s && (*set)->contains(a->emission);
	    if (*s) ++ filteredFeatureCount;
	}
	if (alignment.size() > 0) ratio->data() = (f32)filteredFeatureCount / (f32)alignment.size();
	else ratio->data() = Core::Type<f32>::max + 1;

	putData(0, selection);
	putData(1, ratio);
	return true;
    }
    putData(0, in.get());
    putData(1, in.get());
    return true;
}
