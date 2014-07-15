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
#ifndef LATTICEPROCESSOR_HH_
#define LATTICEPROCESSOR_HH_

#include <Modules.hh>
#include <Am/Module.hh>
#include <Audio/Module.hh>
#include <Core/Application.hh>
#include <Core/Factory.hh>
#include <Flow/Module.hh>
#include <Lm/Module.hh>
#include <Signal/Module.hh>
#include <Math/Module.hh>
#include <Mm/Module.hh>
#include <Speech/Module.hh>
#include <Speech/AbstractSegmentwiseTrainer.hh>
#include <Speech/AcousticSegmentwiseTrainer.hh>
#include <Speech/LatticeSetProcessor.hh>
#include <Speech/LatticeSetExtractor.hh>
#include <Speech/PruningLatticeSetNode.hh>
#include <Speech/WordLatticeExtractor.hh>


/**
 * corpus driven lattice processor
 *
 * The lattice processor is an application, creating a lattice processor node chain
 * to parse a corpus lattice by lattice.
 */

class LatticeProcessor :
public Core::Application
{
public:
    /**
     * Actions:
     *
     */
    enum Action {
	actionNotGiven,
	// i/o (archives)
	actionRead,
	actionWrite,
	actionDumpWordBoundaries,
	// structure
	actionGenerateNumerator,
	actionMerge,
	actionUnite,
	actionNumeratorFromDenominator,
	actionRemoveSilencesAndNoises,
	actionRemoveRedundantSilencesAndNoises,
	actionExtractNBestList,
	actionSpokenAndCompeting,
	actionPrune,
	actionTimeConditioned,
	actionLemmaPronunciationToEvaluationToken,
	actionEvaluationTokenToLemmaPronunciation,
	actionLemmaPronunciationToSyntacticToken,
	actionCopy,
	actionCache,
	actionChangeSemiring,
	actionPartial,
	actionSkipEmpty,
	actionWordToPhonemeLattice,
	// (re-)scoring
	actionMultiply,
	actionLinearCombination,
	actionExpm,
	actionExtendBestPath,
	actionRescore,
	actionTimeframeError,
	actionDeterminization,
	actionEpsilonRemoval,
	// evaluation
	actionInfo,
	actionDensity,
	actionGer,
	actionSingleBestLegacy,
	actionWpConfidence,
	actionSearchMinimumBayesRisk,
	actionSingleBest,
	actionTraceback,
	actionEvaluate,
	actionCovariance,
	actionSegmentSingleWord,
	actionMapOrthToPhonemes,
	actionMapToNonCoarticulation,
	// accumulation
	actionAccumulateDiscriminatively,
	// tagging with crf
	actionCrf,
	// NN training
	actionAccumulateNnStatistics,
	actionOptimizeNn
    };

private:
    typedef std::string Selection;
    static const Core::Choice choiceAction;
    static const Core::ParameterStringVector paramActions;
    static const Core::ParameterStringVector paramSelections;
    enum CorpusType {bliss, plain};
    static const Core::Choice choiceCorpusType;
    static const Core::ParameterChoice paramCorpusType;
    enum ApplicationType {
	speech,
	tagging};
    static Core::Choice choiceApplication;
    static Core::ParameterChoice paramApplication;

    Core::ComponentFactory<Speech::LatticeSetProcessor, Action> processorFactory_;

private:
    void parseActionsSelections(std::vector<Action> &, std::vector<Selection> &);

    // walk through the corpus and apply lattice processor chain
    void visitCorpus(Core::Ref<Speech::LatticeSetProcessorRoot>);

    void finalize(Core::Ref<Speech::LatticeSetProcessorRoot>);

    template<class T>
    static Speech::LatticeSetProcessor* createProcessor(const Core::Configuration &c) {
	return new T(c);
    }

    static Speech::LatticeSetProcessor* createTrainer(const Core::Configuration &config);

    template<class T>
    void registerProcessor(Action action, const char *desc = "", const char *options = "") {
	processorFactory_.registerClass(action, createProcessor<T>);
	addDescription(action, desc, options);
    }

    Core::Ref<Speech::LatticeSetProcessor> getProcessor(Action action, const Core::Configuration &config) const ;

    void setupProcessors();

    void setupSharedNeuralNetwork();

    void setSharedNeuralNetwork(Core::Ref<Speech::LatticeSetProcessor> &processor);

private:
    struct ActionDescription {
	std::string desc;
	std::string options;
    };
    std::map<Action, ActionDescription> descriptions_;

    void addDescription(Action a, const char *desc, const char *options) ;

public:
    LatticeProcessor();

    virtual std::string getApplicationDescription() const ;

    virtual std::string getParameterDescription() const;

    virtual int main(const std::vector<std::string> &arguments);
};



#endif /* LATTICEPROCESSOR_HH_ */
