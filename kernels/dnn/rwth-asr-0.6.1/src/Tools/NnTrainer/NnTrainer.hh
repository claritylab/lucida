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
#ifndef _TOOLS_NN_TRAINER_NN_TRAINER_HH
#define _TOOLS_NN_TRAINER_NN_TRAINER_HH


#include <Modules.hh>
#include <Core/Parameter.hh>
#include <Speech/AlignedFeatureProcessor.hh>
#include <Nn/NeuralNetworkTrainer.hh>

/**	Train the weights of a neural network
 *
 */
class NnTrainer : public Core::Application {
public:
    enum Action {
	actionNotGiven,
	actionUnsupervisedTraining,
	actionSupervisedTraining,
	actionBatchEstimation,
	actionInitNetwork,
	actionCombineStatistics,
	actionGetLogPriorFromMixtureSet,
	actionEstimateMeanAndStandardDeviation,
	actionShowStatistics
    };
    static const Core::Choice choiceAction;
    static const Core::ParameterChoice paramAction;
    static const Core::ParameterBool paramSinglePrecision;
    static const Core::ParameterString paramPriorFile;
    static const Core::ParameterInt paramSeed;
    static const Core::ParameterString paramFilenameInit;
    // statistics IO
    static const Core::ParameterStringVector paramStatisticsFiles;
    static const Core::ParameterString paramStatisticsFile;
 private:
    void visitCorpus(Speech::CorpusProcessor&);
    /** Passes once over the whole corpus controlling the CorpusProcessor object.
     *  Note: CorpusProcessor object is destroyed in the end.
     */
    void visitCorpus(Speech::CorpusProcessor*);
    void visitCorpus(Speech::AlignedFeatureProcessor&);

    // perform training using an unsupervised training algorithm
    template<typename T>
    void neuralNetworkTrainingUnsupervised();

    // perform training using a supervised training algorithm
    template<typename T>
    void neuralNetworkTrainingSupervised();

    // estimate network using statistics and network from file (for batch algorithms)
    template<typename T>
    void neuralNetworkBatchEstimation();

    // initialize network & save it to file
    template<typename T>
    void neuralNetworkInit();

    // calculate log prior from mixture set, convert it to vector and save it
    void getLogPriorFromMixtureSet();

    // combine statistics files
    template<typename T>
    void combineStatistics();

    // combine statistics files
    template<typename T>
    void estimateMeanAndStandardDeviation();

    template<typename T>
    void showStatistics();
public:
    NnTrainer();
    virtual std::string getUsage() const { return "Corpus driven neural network trainer"; }
    virtual int main(const std::vector<std::string> &arguments);
};

#endif //_TOOLS_NN_TRAINER_NN_TRAINER_HH
