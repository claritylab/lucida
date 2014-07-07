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
#include "NnTrainer.hh"

#include <typeinfo>

#include <Audio/Module.hh>
#include <Flow/Module.hh>
#include <Math/Module.hh>
#include <Signal/Module.hh>
#include <Speech/Module.hh>
#include <Mm/Module.hh>


#ifdef MODULE_NN
#include <Math/CudaMatrix.hh>
#include <Math/CudaWrapper.hh>
#include <Nn/BufferedAlignedFeatureProcessor.hh>
#include <Nn/BatchEstimator.hh>
#include <Nn/FeatureScorer.hh>
#include <Nn/Module.hh>
#include <Nn/NeuralNetwork.hh>
#include <Nn/NeuralNetworkTrainer.hh>
#include <Nn/Prior.hh>
#endif

#include <Speech/AligningFeatureExtractor.hh>

APPLICATION(NnTrainer)

const Core::Choice NnTrainer::choiceAction(
    "notGiven", actionNotGiven,
    "unsupervised-training", actionUnsupervisedTraining,
    "supervised-training", actionSupervisedTraining,
    "batch-estimation", actionBatchEstimation,
    "init-network", actionInitNetwork,
    "combine-statistics", actionCombineStatistics,
    "get-log-prior-from-mixture-set", actionGetLogPriorFromMixtureSet,
    "estimate-mean-and-standard-deviation", actionEstimateMeanAndStandardDeviation,
    "show-statistics", actionShowStatistics,
    Core::Choice::endMark());

const Core::ParameterChoice NnTrainer::paramAction(
    "action", &choiceAction, "operation to perform", actionNotGiven);

const Core::ParameterBool NnTrainer::paramSinglePrecision(
    "single-precision", "use single or double precision", true);

const Core::ParameterString NnTrainer::paramPriorFile("new-prior-file", "prior filename", "");

const Core::ParameterInt NnTrainer::paramSeed(
	"seed", "seed for random number generator", 0);

const Core::ParameterStringVector NnTrainer::paramStatisticsFiles(
	"statistics-files", "filenames to read statistics from", ",");

const Core::ParameterString NnTrainer::paramStatisticsFile(
	"statistics-file", "filename to read/write statistics from/to", "");

const Core::ParameterString NnTrainer::paramFilenameInit(
	"parameters-init", "Name of the file to save the initialization parameters to", "");


// dummy parameter
const Core::ParameterString paramActionName("action", "", "n/a");

NnTrainer::NnTrainer() {
    setTitle("neural-network-trainer");
    INIT_MODULE(Flow);
    INIT_MODULE(Audio);
    INIT_MODULE(Math);
    INIT_MODULE(Mm);
    INIT_MODULE(Signal);
    INIT_MODULE(Speech);
#ifdef MODULE_NN
    INIT_MODULE(Nn);
#endif
}

int NnTrainer::main(const std::vector<std::string> &arguments) {

    srand((u32)paramSeed(this->config));
    log("use ") << paramSeed(config) << " as seed for random number generator";

    if (paramSinglePrecision(config))
	log() << "using single precision";
    else
	log() << "using double precision";

    log() << "action: " <<  paramActionName(config);

    // select the action to be performed
    switch ( (Action) paramAction(config) ) {
    case actionUnsupervisedTraining:
	if (paramSinglePrecision(config))
	    neuralNetworkTrainingUnsupervised<f32>();
	else
	    neuralNetworkTrainingUnsupervised<f64>();
	break;
    case actionSupervisedTraining:
	if (paramSinglePrecision(config))
	    neuralNetworkTrainingSupervised<f32>();
	else
	    neuralNetworkTrainingSupervised<f64>();
	break;
    case actionBatchEstimation:
	if (paramSinglePrecision(config))
	    neuralNetworkBatchEstimation<f32>();
	else
	    neuralNetworkBatchEstimation<f64>();
	break;
    case actionInitNetwork:
	if (paramSinglePrecision(config))
	    neuralNetworkInit<f32>();
	else
	    neuralNetworkInit<f64>();
	break;
    case actionGetLogPriorFromMixtureSet:
	getLogPriorFromMixtureSet();
	break;
    case actionCombineStatistics:
	if (paramSinglePrecision(config))
	    combineStatistics<f32>();
	else
	    combineStatistics<f64>();
	break;
    case actionEstimateMeanAndStandardDeviation:
	if (paramSinglePrecision(config))
	    estimateMeanAndStandardDeviation<f32>();
	else
	    estimateMeanAndStandardDeviation<f64>();
	break;
    case actionShowStatistics:
	if (paramSinglePrecision(config))
	    showStatistics<f32>();
	else
	    showStatistics<f64>();
	break;
    default:
	criticalError("Action not given.");
	break;
    };
    // clear GPU resources
    Math::Cuda::deviceReset(Math::CudaDataStructure::hasGpu());
    return 0;
}

template<typename T>
void NnTrainer::neuralNetworkTrainingUnsupervised() {
    Nn::BufferedFeatureExtractor<T>* trainer = new Nn::BufferedFeatureExtractor<T>(config);
    Speech::CorpusProcessor* processor;
    processor = dynamic_cast<Speech::CorpusProcessor* > (trainer);
    visitCorpus(*processor);
    delete processor;
}

template<typename T>
void NnTrainer::neuralNetworkTrainingSupervised() {
    Nn::BufferedAlignedFeatureProcessor<T>* trainer = new Nn::BufferedAlignedFeatureProcessor<T>(config);
    Speech::AlignedFeatureProcessor* processor;
    processor = dynamic_cast<Speech::AlignedFeatureProcessor* > (trainer);
    visitCorpus(*processor);
}

template<typename T>
void NnTrainer::neuralNetworkBatchEstimation() {
    Nn::BatchEstimator<T> batchEstimator(config);
    batchEstimator.initialize();
    batchEstimator.estimate();
    batchEstimator.finalize();
}

template<typename T>
void NnTrainer::neuralNetworkInit() {
    Nn::NeuralNetwork<T> network(config);
    network.initializeNetwork(1);
    // remove prior if softmax output and parameter is set
    Nn::LinearAndSoftmaxLayer<T> *topLayer = dynamic_cast<Nn::LinearAndSoftmaxLayer<T>* >(&network.getTopLayer());
    if (topLayer){
	Nn::Prior<f32> prior(config);
	if (prior.fileName() != ""){
	    prior.read();
	    network.finishComputation();
	    topLayer->removeLogPriorFromBias(prior);
	}
    }
    network.saveParameters(paramFilenameInit(config));
}


template<typename T>
void NnTrainer::combineStatistics(){
    Nn::NeuralNetwork<T> network(config);
    network.initializeNetwork(1);

    bool singlePrecisionInput;
    u32 statisticsType = 0;
    std::vector<std::string> filenames(paramStatisticsFiles(config));
    if (filenames.size() == 0)
	this->error("no files given for combination");
    if (!Nn::Statistics<T>::getTypeFromFile(filenames.at(0), statisticsType, singlePrecisionInput))
	this->error("could not read header from file: ") << filenames.at(0);
    this->log("statistics type: ") << statisticsType;

    if (singlePrecisionInput || (typeid(T) == typeid(double))){
	Nn::Statistics<T> stats(network.nLayers(), statisticsType);
    stats.initialize(network);
	if (!stats.combine(paramStatisticsFiles(config)))
	    error("failed to combine statistics");
    stats.write(paramStatisticsFile(config));
	this->log("number-of-observations: ") << stats.nObservations();
	this->log("total-objective-function: ") << stats.objectiveFunction();
	this->log("total-frame-classification-error: ") << stats.classificationError();
    }
    else {
	// T == f32
	Nn::Statistics<T> stats(network.nLayers(), statisticsType);
	stats.initialize(network);

	Nn::Statistics<f64> doublePrecisionStats(network.nLayers(), statisticsType);
	doublePrecisionStats.copyStructure(stats);
	if (!doublePrecisionStats.combine(paramStatisticsFiles(config)))
	    error("failed to combine statistics");

	doublePrecisionStats.initComputation();
	stats.initComputation(false);
	stats.reset();
	stats.add(doublePrecisionStats);
	stats.finishComputation();
	stats.write(paramStatisticsFile(config));
    }
}




template<typename T>
void NnTrainer::estimateMeanAndStandardDeviation(){
    Nn::MeanAndVarianceTrainer<T> trainer(config);
    Nn::Statistics<T> statistics(0, Nn::Statistics<T>::MEAN_AND_VARIANCE);
    if (paramStatisticsFiles(config).size() > 0){
	if (!statistics.combine(paramStatisticsFiles(config))){
	    error("failed to combine statistics");
	}
	if (paramStatisticsFile(config) != ""){
	    statistics.write(paramStatisticsFile(config));
	}
    }
    else {
	statistics.read(paramStatisticsFile(config));
    }
    statistics.initComputation();
    trainer.writeMeanAndStandardDeviation(statistics);
}

template<typename T>
void NnTrainer::showStatistics(){
    Nn::NeuralNetwork<T> network(config);
    network.initializeNetwork(1);
    u32 statisticsType = 0;
    bool singlePrecision;
    std::string filename(paramStatisticsFile(config));
    bool readOk = Nn::Statistics<T>::getTypeFromFile(filename, statisticsType, singlePrecision);
    log("statistics type: ") << statisticsType;
    if (!readOk)
	this->error("could not read header from file: ") << filename;
    if (singlePrecision){
	Nn::Statistics<f32> stats(network.nLayers(), statisticsType);
	stats.initialize(network);
	stats.read(filename);
	stats.write(std::cout);
    }
    else{
	Nn::Statistics<f64> stats(network.nLayers(), statisticsType);
	stats.initialize(network);
	stats.read(filename);
	stats.write(std::cout);
    }
}

void NnTrainer::getLogPriorFromMixtureSet() {
    Nn::Prior<f32> prior(config);
    Core::Ref<Mm::MixtureSet> mixtureSet = Mm::Module::instance().readMixtureSet(select("mixture-set"));
    Nn::ClassLabelWrapper labelWrapper(config, mixtureSet->nMixtures());

    prior.setFromMixtureSet(mixtureSet, labelWrapper);
    prior.write();
}

void NnTrainer::visitCorpus(Speech::CorpusProcessor &corpusProcessor) {
	Speech::CorpusVisitor corpusVisitor(select("corpus"));
	corpusProcessor.signOn(corpusVisitor);

	Bliss::CorpusDescription corpusDescription(select("corpus"));
	corpusDescription.accept(&corpusVisitor);
}

void NnTrainer::visitCorpus(Speech::CorpusProcessor *corpusProcessor) {
	visitCorpus(*corpusProcessor);
	delete corpusProcessor;
}

void NnTrainer::visitCorpus(Speech::AlignedFeatureProcessor &alignedFeatureProcessor) {
    visitCorpus(Speech::Module::instance().createAligningFeatureExtractor(
		    select("aligning-feature-extractor"), alignedFeatureProcessor));
}
