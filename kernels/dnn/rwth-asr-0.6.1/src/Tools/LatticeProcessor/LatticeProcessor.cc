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
#include "LatticeProcessor.hh"



LatticeProcessor::LatticeProcessor()
    {
	INIT_MODULE(Am);
	INIT_MODULE(Audio);
	INIT_MODULE(Flow);
	INIT_MODULE(Lm);
	INIT_MODULE(Math);
	INIT_MODULE(Mm);
	INIT_MODULE(Signal);
	INIT_MODULE(Speech);
	setTitle("lattice-processor");
	setupProcessors();
    }


const Core::Choice LatticeProcessor::choiceAction(
    "not-given", actionNotGiven,
    "read", actionRead,
    "write", actionWrite,
    "dump-word-boundaries", actionDumpWordBoundaries,
    "generate-numerator", actionGenerateNumerator,
    "info", actionInfo,
    "merge", actionMerge,
    "unite", actionUnite,
    "numerator-from-denominator", actionNumeratorFromDenominator,
    "remove-silences-and-noises", actionRemoveSilencesAndNoises,
    "remove-redundant-silences-and-noises", actionRemoveRedundantSilencesAndNoises,
    "extract-n-best-list", actionExtractNBestList,
    "spoken-and-competing", actionSpokenAndCompeting,
    "prune", actionPrune,
    "time-conditioned", actionTimeConditioned,
    "copy", actionCopy,
    "cache", actionCache,
    "change-semiring", actionChangeSemiring,
    "partial", actionPartial,
    "skip-empty", actionSkipEmpty,
    "word-to-phoneme", actionWordToPhonemeLattice,
    "lemma-pronunciation-to-evaluation-token", actionLemmaPronunciationToEvaluationToken,
    "evaluation-token-to-lemma-pronunciation", actionEvaluationTokenToLemmaPronunciation,
    "lemma-pronunciation-to-syntactic-token", actionLemmaPronunciationToSyntacticToken,
    "multiply", actionMultiply,
    "linear-combination", actionLinearCombination,
    "expm", actionExpm,
    "extend-best-path", actionExtendBestPath,
    "rescore", actionRescore,
    "timeframe-error", actionTimeframeError,
	"determinization", actionDeterminization,
	"remove-epsilon", actionEpsilonRemoval,
    "density", actionDensity,
    "graph-error-rate", actionGer,
    "single-best-legacy", actionSingleBestLegacy,
    "word-posterior-confidence", actionWpConfidence,
    "search-minimum-bayes-risk", actionSearchMinimumBayesRisk,
    "single-best", actionSingleBest,
    "traceback", actionTraceback,
    "evaluate", actionEvaluate,
    "covariance", actionCovariance,
    "segment-single-word", actionSegmentSingleWord,
    "map-orth-to-phonemes", actionMapOrthToPhonemes,
    "map-to-non-coarticulated", actionMapToNonCoarticulation,
    "accumulate-discriminatively", actionAccumulateDiscriminatively,
    "crf", actionCrf,
	"accumulate-nn-statistics", actionAccumulateNnStatistics,
	"optimize-nn", actionOptimizeNn,
    Core::Choice::endMark());

const Core::ParameterStringVector LatticeProcessor::paramActions(
    "actions",
    "actions to perform",
    ",");

const Core::ParameterStringVector LatticeProcessor::paramSelections(
    "selections",
    "configuration selections corresponding to the actions",
    ",");

const Core::Choice LatticeProcessor::choiceCorpusType("bliss",bliss,"plain",plain,Core::Choice::endMark());

const Core::ParameterChoice LatticeProcessor::paramCorpusType(
    "type", &choiceCorpusType, "selection of corpus type (one of: bliss(default), plain)", bliss);

Core::Choice LatticeProcessor::choiceApplication(
    "speech", speech,
    "tagging", tagging,
    Core::Choice::endMark());

Core::ParameterChoice LatticeProcessor::paramApplication(
    "application",
    &choiceApplication,
    "type of application",
    speech);

void LatticeProcessor::setupProcessors()
{
    registerProcessor<Speech::LatticeSetReader>(actionRead,
	    "read lattice from archive\n",
	    "\t# specify which parts of the lattice are read\n"
	    "\t# (keep in mind that '' == 'acoustic')"
	    "\treaders\n"
	    "\tlattice-archive.format          = [fsa]|htk|wolfgang\n"
	    "\tlattice-archive.path            = <file-name>\n"
	    "\tlattice-archive.alphabet        = [lemma-pronunciation]|syntactic-token|evaluation-token|no-lexicon-check\n");

    registerProcessor<Speech::LatticeSetWriter>(actionWrite,
	    "write lattice to archive\n",
	    "\tlattice-archive.format          = [fsa]|htk|wolfgang\n"
	    "\tlattice-archive.path            = <file-name>\n");

    registerProcessor<Speech::WordLatticeMerger>(actionMerge,
	    "merge two lattices, e.g. numerator into denominator for discriminative training\n",
	    "\tfsa-prefix                      = [acoustic]|<prefix>\n"
	    "\t# do merging only if numerator not already in denominator\n"
	    "\tmerge-only-if-spoken-not-in-lattice = [false]|true\n"
	    "\tnumerator-lattice-archive.format = [fsa]|htk|wolfgang\n"
	    "\tnumerator-lattice-archive.path  = <file-name>\n"
	    "\t# requires a language model to determine history of word-conditioned lattices\n");

    registerProcessor<Speech::WordLatticeUnion>(actionUnite,
	    "unite two lattices\n",
	    "\tfsa-prefix                      = [acoustic]|<prefix>\n"
	    "\tnumerator-lattice-archive.format = [fsa]|htk|wolfgang\n"
	    "\tnumerator-lattice-archive.path  = <file-name>\n");

    registerProcessor<Speech::NumeratorFromDenominatorExtractor>(actionNumeratorFromDenominator,
	    "extract numerator (i.e., correct hypotheses according to transcription) from the denominator lattice\n",
	    "\t# does not require any additional/specific parameters\n");

    registerProcessor<Speech::PruningLatticeSetNode>(actionPrune,
	    "prune lattice (only a single score is supported) by forward-backward pruning\n",
	    "\tthreshold                       = 15\n"
	    "\t# specify if threshold is relative to best path in lattice\n"
	    "\tthreshold-is-relative           = [true]|false\n");

    registerProcessor<Speech::CopyNode>(actionCopy);
    registerProcessor<Speech::CopyNode>(actionCache);
    registerProcessor<Speech::LinearCombinationLatticeProcessorNode>(actionLinearCombination,
	    "calculate linear combination(s) of parts in lattice based on the passed scaling factors (e.g. scaled acoustic + lm score)\n",
	    "\toutputs                         = total accuracy\n"
	    "\ttotal.scales                    = 1.0 0.0\n"
	    "\taccuracy.scales                 = 0.0 1.0\n");

    registerProcessor<Speech::InfoLatticeProcessorNode>(actionInfo);
    registerProcessor<Speech::DensityLatticeProcessorNode>(actionDensity,
	    "calculate lattice density\n",
	    "\t# specify type of lattice density\n"
	    "\tshall-evaluate-arcs-per-spoken-word = [true]|false\n"
	    "\tshall-evaluate-arcs-per-timeframe = [true]|false\n");

    registerProcessor<Speech::GerLatticeProcessorNode>(actionGer,
	    "calculate graph error rate\n",
	    "\t# does not require any additional/specific parameters\n");

    registerProcessor<Speech::LatticeSetGenerator>(actionRescore,
	    "annotate a lattice (aka topology) with scores, e.g. acoustic scores or accuracies for discriminative training\n",
	    "\tacoustic-rescorers              = am1,...\n"
	    "\temission-rescorers              = em1,...\n"
	    "\ttdp-rescorers                   = tdp1,...\n"
	    "\tpronunciation-rescorers         = pron1,...\n"
	    "\tlm-rescorers                    = lm1,...\n"
	    "\tcombined-lm-rescorers           = c-lm1,...\n"
	    "\treaders                         = reader1,...\n"
	    "\tdistance-rescorers              = distance1,...\n"
	    "\tpass-extractors                 = pass1,...\n"
	    "\tshare-acoustic-model            = [false]|true\n"
	    "\t[*.segmentwise-feature-extraction]\n"
	    "\t...\n"
	    "\t[*.segmentwise-alignment]\n"
	    "\t...\n"
	    "\t[*.am1]\n"
	    "\t# specify the port to take features from\n"
	    "\tport-name                       = features\n"
	    "\t[*.reader1]\n"
	    "\t# part to read\n"
	    "\tfsa-prefix                      = <prefix>\n"
	    "\tlattice-archive.format          = [fsa]|htk|wolfgang\n"
	    "\tlattice-archive.path            = <file-name>\n"
	    "\t[*.distance1]\n"
	    "\tdistance-type                   = [approximate-word-accuracy]|approximate-phone-accuracy|...\n"
	    "\t# specify where to obtain the correct hypotheses from to generate numerator\n"
	    "\tspoken-source                   = [orthography]|archive\n"
	    "\t[*.approximate-phone-accuracy-lattice-builder]\n"
	    "\t...\n");

    processorFactory_.registerClass(actionAccumulateDiscriminatively, createTrainer);
    addDescription(actionAccumulateDiscriminatively,
	    "all lattice-based discriminative training is done with this node\n",
	    "\t# frames with posterior below this threshold are not accumulated\n"
	    "\tweight-threshold                = 1.19209290e-07F\n"
	    "\t# if the posterior calculation exceeds this threshold, a warning is produced\n"
	    "\tposterior-tolerance             = 100\n"
	    "\tapplication                     = [speech]|tagging\n"
	    "\t# ME stands for Minimum Error training like for example MWE/MPE\n"
	    "\tcriterion                       = [MMI]|MCE|ME|gis-ME|log-ME|context-prior|context-accuracy|weighted-ME|ME-with-i-smoothing|log-ME-with-i-smoothing|weighted-ME-with-i-smoothing|CORRECTIVE|minimum-least-squared-error|plain\n"
	    "\t# name of lattice part for probabilistic model\n"
	    "\tlattice-name                    = total\n"
	    "\tport-name                       = features\n"
	    "\t# stream index where features are taken for accumulation\n"
	    "\taccumulation-stream-index       = 0\n"
	    "\tmodel-type                      = [gaussian-mixture]|maximum-entropy\n");

}


Speech::LatticeSetProcessor* LatticeProcessor::createTrainer(const Core::Configuration &config)
{
    Speech::AbstractSegmentwiseTrainer *trainer = 0;
    ApplicationType appType = static_cast<ApplicationType>(paramApplication(config));
    if (appType == speech)
	trainer = Speech::AbstractAcousticSegmentwiseTrainer::createAbstractAcousticSegmentwiseTrainer(config);
    ensure(trainer);
    return trainer;
}



void LatticeProcessor::setupSharedNeuralNetwork(){
#if 1
    criticalError("Module MODULE_NN_SEQUENCE_TRAINING not available");
#endif
}


void LatticeProcessor::setSharedNeuralNetwork(Core::Ref<Speech::LatticeSetProcessor> &processor){
}

Core::Ref<Speech::LatticeSetProcessor> LatticeProcessor::getProcessor(Action action, const Core::Configuration &config) const {
	Speech::LatticeSetProcessor *obj = processorFactory_.getObject(action, config);
	if (!obj)
	    return Core::Ref<Speech::LatticeSetProcessor>();
	else
	    return Core::Ref<Speech::LatticeSetProcessor>(obj);
}


void LatticeProcessor::addDescription(Action a, const char *desc, const char *options) {
	std::map<Action, ActionDescription>::iterator i;
	bool dummy;
	Core::tie(i, dummy) = descriptions_.insert(std::make_pair(a, ActionDescription()));
	i->second.desc = desc;
	i->second.options = options;
}


std::string LatticeProcessor::getApplicationDescription() const {
	std::string app = "\n"
		"lattice-processor.$arch [OPTION(S)]\n"
		"\n"
		"corpus driven lattice processor\n"
		"\n"
		"options:\n"
		"   --help            print this page (note: --config must be given)\n"
		"   --config=file     the configuration file in the sprint format\n"
		"\n"
		"The lattice processor build a chain of processor nodes described\n"
		"by the list in the 'lattice-processor' configuration part, i.e.,\n"
		"\t[lattice-processor]\n"
		"\t# define node operations\n"
		"\tactions                         = <action1>,<action2>,...\n"
		"\t# define selection names for the configuration\n"
		"\t# (configuration depends on action, see below)\n"
		"\tselections                      = <selection1>,<selection2>,..."
		"\t[*.<selection1>]\n"
		"\t...\n"
		"\n"
		"Simple example:\n"
		"\t[lattice-processor]\n"
		"\tactions                 = read,write\n"
		"\tselections              = lattice-reader,lattice-writer\n"
		"\n"
		"Afterwords it initialises the processor nodes and let them walk\n"
		"throw the corpus described in the corpus configuation part. E.g.:\n"
		"\t[*.corpus]\n"
		"\tfile                            = data/corpus.gz\n"
		"\twarn-about-unexpected-elements  = no\n"
		"\tcapitalize-transcriptions       = no\n"
		"\n"
		"To interprete the corpus it needs a lexicon definition given\n"
		"in the lexicon configuration part. E.g.:\n"
		"\t[*.lexicon]\n"
		"\tfile                            = data/lexicon.gz\n"
		"\n"
		"The output is defined in the general ('*') section of the configuration\n"
		"\t[*]\n"
		"\tstatistics.channel              = nil\n"
		"\tlog.channel                     = output-channel\n"
		"\twarning.channel                 = output-channel\n"
		"\terror.channel                   = output-channel\n"
		"\tconfiguration.channel           = output-channel\n"
		"\tsystem-info.channel             = output-channel\n"
		"\tdot.channel                     = nil\n"
		"\tprogress.channel                = output-channel\n"
		"\toutput-channel.file             = output-channel\n"
		"\toutput-channel.append           = false\n"
		"\toutput-channel.encoding         = UTF-8\n"
		"\toutput-channel.unbuffered       = false\n"
		"\ton-error                        = delayed-exit\n"
		"\t\n"
		"Using this configuration a file output-channel will be created during a\n"
		"run of lattice-processor. These file contain a lots of additional\n"
		"information, warnings and errors.\n"
		"\n"
		"Every processor node can have its own part in the configuration file.\n"
		"E.g.:\n"
		"\t[*.lattice-reader]\n"
		"\treaders                         = total\n"
		"\tlattice-archive.path            = data/input/\n"
		"\tlattice-archive.type            = {fsa|htk}\n"
		"\t\n"
		"\t[*.lattice-writer]\n"
		"\tlattice-archive.path            = data/output/\n"
		"\ttype                            = {fsa|htk}\n"
		"\t\n"
		"\n";

	return app;
}



std::string LatticeProcessor::getParameterDescription() const {
	std::string tmp = "\n"
		"supported actions:\n"
		"\n"
		"\t\"not-given\": do nothing\n"
		"\t[*.selection]\n"
		"\t# nothing to configurate\n"
		"\t\n";
	std::vector<Action> actions = processorFactory_.identifiers();
	for (std::vector<Action>::const_iterator a = actions.begin(); a != actions.end(); ++a) {
	    std::string name = choiceAction[*a];
	    tmp += "\t\"" + name + "\": ";
	    std::map<Action, ActionDescription>::const_iterator desc = descriptions_.find(*a);
	    if (desc != descriptions_.end() && !desc->second.desc.empty()) {
		tmp += desc->second.desc;
		if (!desc->second.options.empty()) {
		    tmp += "\t[*.selection]\n";
		    tmp += desc->second.options;
		}
	    } else {
		tmp += "no description available\n";
	    }
	    tmp += "\n";
	}
	return tmp;
}



/**
 * Do these steps:
 *   - read the configuration and build a lattice processor
 *     node chain from the actions and selections lists
 *   - initialize the chain (starting at the root processor)
 *      -> caution: to simplify interface 'actionAccumulateDiscriminatively'
 *                  there is a factory function 'create...'.
 *         To understand what your setup is doing you have to parse through
 *         the create functions :-( or you have some time and start a table...
 *   - call visitCorpus(rootProcessor)
 */
int LatticeProcessor::main(const std::vector<std::string> &arguments)
{
    // Build up the processor chain based on the actions and selections
    // definition in the configuration.
    // No work is done in this phase, only Constructors are called.
    std::vector<Action> actions;
    std::vector<Selection> selections;
    parseActionsSelections(actions, selections);
    Core::Ref<Speech::LatticeSetProcessor> previousProcessor;
    Core::Ref<Speech::LatticeSetProcessorRoot> rootProcessor;
    // walk throw the actions saved in the config
    {
	const u32 i = 0;
	Core::Ref<Speech::LatticeSetProcessor> processor;

	require(!rootProcessor);
	processor = getProcessor(actions[i], select(selections[i]));
	ensure(processor);
	rootProcessor = Core::Ref<Speech::LatticeSetProcessorRoot>(
			dynamic_cast<Speech::LatticeSetProcessorRoot*>(processor.get()));
	// If this is the first element of the action/selection list
	// and we didn't find the rootProcessor, there has to be an error!
	if (!rootProcessor) {
		criticalError("First node must be a root lattice set processor.");
	}
	previousProcessor = processor;
    }
    for (u32 i = 1; i < actions.size(); ++ i) {
	Core::Ref<Speech::LatticeSetProcessor> processor;
	processor = getProcessor(actions[i], select(selections[i]));
	if (!processor) {
		criticalError("Action '%d' at %d is not known to the lattice processor!", actions[i], i);
	}

	// make sure we have initialized the processor
	verify(processor);

	// check whether the processor might require access to the shared neural network and set it if necessary
	setSharedNeuralNetwork(processor);

	// order processors to a chain
	if (previousProcessor) {
		previousProcessor->setProcessor(processor);
	}
	previousProcessor = processor;
    }

    if (rootProcessor) {
	// prepare and write lexicon
	rootProcessor->initialize();

	// walk through the corpus
	visitCorpus(rootProcessor);

	// finalize training
	finalize(rootProcessor);
    } else {
	criticalError("No root processor defined.");
    }

    // If we reach this point everything has been successful
    return EXIT_SUCCESS;
}

APPLICATION(LatticeProcessor)

/**
 * Parse the actions and selections statements in the configuration
 */
void LatticeProcessor::parseActionsSelections(
    std::vector<Action> &actions, std::vector<Selection> &selections)
{
    require(actions.empty() && selections.empty());
    std::vector<std::string> _actions = paramActions(config);
    selections = paramSelections(config);
    if (_actions.size() != selections.size()) {
	criticalError("Mismatch in number of actions and configuration selections.");
	return;
    }
    for (size_t i = 0; i < _actions.size(); ++ i) {
	actions.push_back(Action(choiceAction[_actions[i]]));
	if (actions.back() == actionNotGiven) {
	    error("action \"") << _actions[i] << "\" not given";
	}
    }
}

/**
 * Walk through the corpus.
 *    - Create a CorpusVisitor class. The corpus visitor is the class coordinating the walk through the corpus.
 *    - Call signOn to the root processor node (which will call signOn of the corpus visitor as well).
 *    - Create a corpus description class.
 *    - Call corpusDescription.accept to start the walk through the corpus
 */
void LatticeProcessor::visitCorpus(Core::Ref<Speech::LatticeSetProcessorRoot> root)
{
    // make sure the root processor is initialized
    require(root);
    // the corpus visitor is the class coordinating the walk through the corpus
    Speech::CorpusVisitor corpusVisitor(select("corpus"));
    // introduce everyone to each other
    root->signOn(corpusVisitor);
    switch (paramCorpusType(select("corpus"))) {
    case bliss:
    {
	Bliss::CorpusDescription corpusDescription(select("corpus"));
	// actually do the work
	corpusDescription.accept(&corpusVisitor);
    }
	break;
    default:
	defect();
    }
}

void LatticeProcessor::finalize(Core::Ref<Speech::LatticeSetProcessorRoot> root)
{
    // make sure the root processor is initialized
    require(root);
    root->logComputationTime();
}
