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
#include <Core/TextStream.hh>
#include <Core/StringUtilities.hh>
#include <Core/ObjectCache.hh>
#include <Flf/FlfCore/Lattice.hh>
#include <Flf/FlfCore/Basic.hh>
#include <Flf/FlfCore/Ftl.hh>
#include <Flow/Node.hh>
#include <Flow/DataAdaptor.hh>
#include "ModelCombination.hh"
#include "LatticeArcAccumulator.hh"

namespace Speech {
    /** LemmaPronunciationId_hash_map
     *  */
    void LemmaPronunciationId_hash_map::init(Bliss::LexiconRef lexicon)
    {
	this->lexicon=lexicon;return;
	Bliss::Lexicon::LemmaPronunciationIterator lpi, lpi_end;
	for (Core::tie(lpi, lpi_end) = lexicon->lemmaPronunciations() ; lpi != lpi_end ; ++lpi){
	    const Bliss::LemmaPronunciation* lp = (*lpi);
	    Fsa::LabelId lemmaPronId = lp->id();
	    f32 score = lp->pronunciationScore();
	    insert(std::pair<Fsa::LabelId,f32>(lemmaPronId,score));
	}
    }

    void LemmaPronunciationId_hash_map::read(const std::string &filename)
    {
	Core::TextInputStream stats(filename);
	std::string line,orth,phons;f32 score;
	//std::string::size_type begin,end; //unused

	while(std::getline(stats,line))
	{
	    std::vector<std::string> instance = Core::split(line,"\t");

	    verify(instance.size() == 3);

	    orth = instance[0];
	    phons = instance[1];
	    score = atof(instance[2].c_str());

	    //Bliss::Pronunciation *pron = lexicon->getPronunciation(phons); //unused
	    Bliss::Lexicon::LemmaPronunciationIterator lpi, lpi_end;
	    std::string prefOrth;

	    for (Core::tie(lpi, lpi_end) = lexicon->lemmaPronunciations() ; lpi != lpi_end ; ++lpi){
		const Bliss::LemmaPronunciation* lp = (*lpi);
		prefOrth = std::string(lp->lemma()->preferredOrthographicForm());
		if(prefOrth == orth)
		{
			if(find(lp->id()) == this->end())
			insert(std::pair<Fsa::LabelId,f32>(lp->id(),score));
			else
			    (*this)[lp->id()] += score;

		    break;
		}
	    }
	}
    }

    void LemmaPronunciationId_hash_map::write(const std::string &filename)
    {
	Core::TextOutputStream out(filename);
	LemmaPronunciationId_hash_map::const_iterator i;
	Bliss::Lexicon::LemmaPronunciationIterator lpi, lpi_end;
	std::string orth,phon;f32 score;

	for(i = begin(); i != end(); ++i){
	    for (Core::tie(lpi, lpi_end) = lexicon->lemmaPronunciations() ; lpi != lpi_end ; ++lpi){
		const Bliss::LemmaPronunciation* lp = (*lpi);
		if(lp->id() == i->first)
		{
		    orth = std::string(lp->lemma()->preferredOrthographicForm());
		    phon = lp->pronunciation()->format(lexicon->phonemeInventory());
		    score = i->second;
		    out << orth << '\t' << phon << '\t' << score << std::endl;
		    break;
		}
	    }
	}

	out.close();
    }


    /** LatticeArcAccumulatorNode
     *  */
    Core::ParameterString LatticeArcAccumulatorNode::paramLexiconFilename(
	"lexicon-filename",
	"filename to write lexicon");

    Core::ParameterString LatticeArcAccumulatorNode::paramPronunciationStatsFilenameRead(
	"pronunciation-read",
	"filename to read pronunciation statistics");

    Core::ParameterString LatticeArcAccumulatorNode::paramPronunciationStatsFilenameWrite(
	"pronunciation-write",
	"filename to write pronunciation statistics");

    const Core::ParameterFloat LatticeArcAccumulatorNode::paramMinimumPronWeight(
	"minimal-pronunciation-weight",
	"minimal-pronunciation-weight that will at least remain",
	0.0001);

    const Core::ParameterFloat LatticeArcAccumulatorNode::paramTauWeight(
	"tau",
	"tau parameter in MAP update equation",
	0.0);

    Core::ParameterString LatticeArcAccumulatorNode::paramEncoding(
	"encoding",
	"encoding of updated lexicon",
	"ISO-8859-1");

    LatticeArcAccumulatorNode::LatticeArcAccumulatorNode(const Core::Configuration &c) :
	Core::Component(c),
	Precursor(c),
	tau_(paramTauWeight(config)),
	minimalPronWeight_(paramMinimumPronWeight(config)),
	lexiconFilename_(paramLexiconFilename(c)),
	pronunciationStatsFilenameRead_(paramPronunciationStatsFilenameRead(c)),
	pronunciationStatsFilenameWrite_(paramPronunciationStatsFilenameWrite(c)),
	encoding_(paramEncoding(c)),
	needInit_(true)
    {
	addInputs(2);
	addOutputs(1);

	arcAccumulator_ = new LemmaPronunciationId_hash_map;
    }

    LatticeArcAccumulatorNode::~LatticeArcAccumulatorNode() {

	//write statistics write-filename given
	if(pronunciationStatsFilenameWrite_ != "")
	    arcAccumulator_->write(pronunciationStatsFilenameWrite_);

	//exit if lexicon-filename not given
	if(lexiconFilename_ == "") return;

	Core::hash_map<Fsa::LabelId, LemmaPronunciationId_hash_map > counts;
	Core::hash_map<Fsa::LabelId, LemmaPronunciationId_hash_map > weights;

	// make a copy of the Lexicon, because it is modified
	Core::Ref<const Bliss::Lexicon> lexicon = modelCombination_->lexicon();
	Core::Ref<const Bliss::LemmaPronunciationAlphabet> lpa = lexicon->lemmaPronunciationAlphabet();

	Bliss::Lexicon::LemmaPronunciationIterator lpi, lpi_end;
	for (Core::tie(lpi, lpi_end) = lexicon->lemmaPronunciations() ; lpi != lpi_end ; ++lpi) {
	    const Bliss::LemmaPronunciation* lp = (*lpi);
	    Fsa::LabelId lemmaId = lp->lemma()->id();
	    Fsa::LabelId lemmaPronId = lp->id();

	    weights[lemmaId][lemmaPronId] = lp->pronunciationProbability();
	    counts[lemmaId][lemmaPronId] = 0.0;
	}

	f32 totalCounts = 0.0;
	Core::hash_map<Fsa::LabelId, f32>::const_iterator i;
	for(i = (*arcAccumulator_).begin(); i != (*arcAccumulator_).end(); i++) {
	    const Bliss::LemmaPronunciation* lp = lpa->lemmaPronunciation(i->first);
	    if (lp) {
		Fsa::LabelId lemmaId = lp->lemma()->id();
		Fsa::LabelId lemmaPronId = lp->id();
		verify(i->second >= 0.0);
		counts[lemmaId][lemmaPronId] += i->second;
		totalCounts += i->second;
	    }
	}
	log("total-counts: %f", totalCounts);

	Core::hash_map<Fsa::LabelId, LemmaPronunciationId_hash_map >::const_iterator l;
	for(l = weights.begin(); l != weights.end(); l++) {
	    Fsa::LabelId lemmaId = l->first;
	    updateWeightsMap(counts[lemmaId], weights[lemmaId], tau_);
	}

	for (Core::tie(lpi, lpi_end) = lexicon->lemmaPronunciations(); lpi != lpi_end ; ++lpi) {
	    // a const_cast is not desirable, but the Lexicon stores only
	    // pointers to const LemmaPronunciations.
	    // We hope that the program terminates shortly afterwards.
	    /*! @todo: don't change the lexicon, change only the written file */
	    Bliss::LemmaPronunciation* lp = const_cast<Bliss::LemmaPronunciation*>(*lpi);
	    Fsa::LabelId lemmaId = lp->lemma()->id();
	    Fsa::LabelId lemmaPronId = lp->id();
	    f32 pronWeight = weights[lemmaId][lemmaPronId];
	    if(pronWeight < minimalPronWeight_)
		pronWeight = minimalPronWeight_;
	    lp->setPronunciationProbability(pronWeight);
	}
	Core::TextOutputStream o(lexiconFilename_);
	o.setEncoding(encoding_);
	Core::XmlWriter xo(o);
	o.setIndentation(2);
	xo.generateFormattingHints();
	lexicon->writeXml(xo);
	o.close();
	delete arcAccumulator_;
    }

    bool LatticeArcAccumulatorNode::updateWeightsMap(Core::hash_map<Fsa::LabelId, f32> &counts,
						     Core::hash_map<Fsa::LabelId, f32> &weights,
						     f32 tau) {
	f32 totalCount = 0.0;
	Core::hash_map<Fsa::LabelId, f32>::const_iterator pron_i;
	for(pron_i = counts.begin(); pron_i != counts.end(); pron_i++)
	    totalCount += pron_i->second;

	if(totalCount > 0.0) {
	    f32 interpolationWeight = totalCount / (tau + totalCount);
	    for(pron_i = counts.begin(); pron_i != counts.end(); pron_i++) {
		Fsa::LabelId pronId = pron_i->first;
		weights[pronId] =
		    interpolationWeight * (pron_i->second/totalCount)
		    + (1.0 - interpolationWeight) * weights[pronId];
	    }
	}
	return true;
    }

    bool LatticeArcAccumulatorNode::configure()
    {
	Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
	getInputAttributes(1, *attributes);
	if (!configureDatatype(attributes, Flow::DataAdaptor<ModelCombinationRef>::type())) {
	    return false;
	}

	getInputAttributes(0, *attributes);
	if (!configureDatatype(attributes, Flow::DataAdaptor<Flf::ConstLatticeRef>::type())) {
	    return false;
	}
	return putOutputAttributes(0, attributes);
    }

    bool LatticeArcAccumulatorNode::work(Flow::PortId p)
    {
	if (needInit_) {
	    Flow::DataPtr<Flow::DataAdaptor<ModelCombinationRef> > in;
	    getData(1, in);
	    modelCombination_ = in->data();

	    arcAccumulator_->init(modelCombination_->lexicon());

	    if(pronunciationStatsFilenameRead_ != "")
		{
		    std::vector<std::string> files = Core::split(pronunciationStatsFilenameRead_," ");
		    for(std::vector<std::string>::iterator file = files.begin(); file != files.end(); ++file)
			 arcAccumulator_->read(*file);

	    }
	    needInit_ = false;
	}

	Flow::DataPtr<Flow::DataAdaptor<Flf::ConstLatticeRef> > in;
	while(getData(0, in)) {

	    Lattice::ConstWordLatticeRef lattice = toWordLattice(in->data());
	    //verify(lattice->name(0) == "total");

	    Speech::ArcWeightAccumulator(lattice->part(0), arcAccumulator_);

	    Flow::DataAdaptor<Flf::ConstLatticeRef> *out = new Flow::DataAdaptor<Flf::ConstLatticeRef>();
	    out->data() = in->data();
	    putData(0, out);
	}

	return putData(0, in.get());
    }
}
