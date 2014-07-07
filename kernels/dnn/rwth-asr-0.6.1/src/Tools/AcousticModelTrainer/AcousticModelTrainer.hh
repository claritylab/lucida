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
#ifndef _TOOLS_ACOUSTIC_MODEL_TRAINER_ACOUSTIC_MODEL_TRAINER_HH
#define _TOOLS_ACOUSTIC_MODEL_TRAINER_ACOUSTIC_MODEL_TRAINER_HH

#include <Modules.hh>
#include <Signal/ScatterEstimator.hh>
#include <Speech/AlignedFeatureProcessor.hh>
#include <Speech/LabeledFeatureProcessor.hh>
#if 1
namespace Speech { class LatticeSetProcessor : public Core::ReferenceCounted {}; }
#endif

/**
 * Actions:
 *
 * - dry: drives an aligner (given in *.aligner.type). Useful for generating and saving an alignment.
 * - create-model-acceptors: activates the model-acceptor-cache object for each segment of the corpus
 * - show-mixture-set: dumps mixture set in XML format
 * - combine-mixture-sets: combines mixture sets given in old-mixture-set-template and old-mixture-set-ids
 * - map-mixture-set: maps mixture set given mapping file
 * - accumulate-mixture-set-text-dependent: mixtures set estimator accumulates features according to a given alignment
 * - accumulate-mixture-set-ext-independent: mixtures set estimator accumulates features for set of acoustic conditions
 * - accumulate-tdc-sum: decision tree sum file estimator accumulates features according to a given alignment
 * - accumulate-discriminatively: mixtures set estimator discriminatively accumulates features according to a given (weighted) alignment
 * - score-features: acoustic scores of a given alignment accumulated for set of acoustic conditions
 * - estimate-scatter-matrices-text-dependent: estimates between and within class scatter matrices
 *                                             according to a given alignment
 * - estimate-scatter-matrices-text-independent: estimates between and within class scatter matrices
 *                                               according to set of acoustic conditions
 * - estimate-lda-text-dependent: creates projector matrix by LDA according to a given alignment
 * - estimate-lda-text-independent: creates projector matrix by LDA according to set of acoustic conditions
 * - estimate-lda: creates projector matrix by LDA based reading scatter matrices form files.
 *
 * - accumulate-weight-histogram: generates a histogram over all state weights in the alignment
 *   The target histogram file is set using the parameter weight-histogram-accumulator.histogram-file
 *
 * Corpus description: configured in the selection "corpus"
 * Aligner: configured in the selection "aligner" @see Speech::Application
 * Trainers: E.g.: accumulater, scorer etc are configured in the selection "trainer"
 */
class AcousticModelTrainer : public Core::Application
{
public:
    enum Action {
	actionNotGiven,
	actionDryRun,
	actionCreateModelAcceptors,
	actionShowMixtureSet,
	actionConvertMixtureSet,
	actionClusterMixtureSet,
	actionCombineMixtureSets,
	actionMapAdaptation,
	actionCombineMixtureSetEstimators,
	actionMapMixtureSet,
	actionAccumulateMixtureSetTextDependent,
	actionAccumulateMixtureSetTextIndependent,
	actionAccumulateTdcSumFile,
	actionAccumulatorConvert,
	actionCombineTdcSumFile,
	actionAccumulateCartExamples,
	actionMergeCartExamples,
	actionEstimateCart,
	actionAccumulateDiscriminatively,
	actionConvertMixtureSetToMixtureSetEstimator,
	actionConvertMixtureSetToLogLinearMixtureSet,
	actionScoreFeatures,
	actionEstimateScatterMatricesTextDependent,
	actionEstimateScatterMatricesTextIndependent,
	actionEstimateLdaTextDependent,
	actionEstimateLdaTextIndependent,
	actionCombineScatterMatrixAccumulators,
	actionEstimateScatterMatricesFromScatterMatrixAccumulator,
	actionEstimateLdaFromScatterMatrixAccumulator,
	actionEstimateLda,
	actionEstimateAffineFeatureTransform,
	actionEstimateAffineFeatureTransform2,
	actionCalculateAffineFeatureTransform,
	actionCalculateAffineFeatureTransform2,
	actionCombineAffineFeatureTransformEstimators,
	actionScoreAffineFeatureTransform,
	actionScoreAffineFeatureTransform2,
	actionTestAffineFeatureTransform2,
	actionEstimateModelTransform,
	actionCalculateModelTransform,
	actionAccumulateMaximumEntropy,
	actionAccumulateMaximumEntropyFast,
	actionMaximumEntropyShuffledOnline,
	actionMaximumEntropyShuffledOnlineLbfgs,
	actionAccumulateMaximumEntropyBuffered,
	actionConvertLambdas,
	actionAccumulateCombinedMaximumEntropy,
	actionAccumulateCombinedExactMaximumEntropy,
	actionCombineMaximumEntropy,
	actionEstimateMaximumEntropy,
	actionCenterMaximumEntropy,
	actionTieMaximumEntropy,
	actionAccumulateJoergfile,
	actionAccumulateNearestNeighborTree,
	actionAccumulateWeightHistogram
    };

    static const Core::Choice choiceAction;
    static const Core::ParameterChoice paramAction;

    static const Core::ParameterBool paramMixtureSetFilenamesToCombineUseComma;
    static const Core::ParameterStringVector paramMixtureSetFilenamesToCombine;
    static const Core::ParameterStringVector paramMixtureSetFilenamesToCombineComma;
    static const Core::ParameterString paramMappingFilenameToMap;
    static const Core::ParameterBool paramForceExampleRegeneration;
 private:
    void visitCorpus(Speech::CorpusProcessor&);
    /** Passes ones over the whole corpus contolling the CorpusProcessor object.
     *  Note: CorpusProcessor object is destroyed in the end.
     */
    void visitCorpus(Speech::CorpusProcessor*);
    void visitCorpus(Speech::AlignedFeatureProcessor&);
    void visitCorpus(Speech::LabeledFeatureProcessor&);
    void visitCorpus(Core::Ref<Speech::LatticeSetProcessor>);

    void dryRun();
    void createModelAcceptors();

    void showMixtureSet();
    void convertMixtureSet();
    void clusterMixtureSet();
    void combineMixtureSets();
    void mapAdaptation();
    void combineMixtureSetEstimators();
    void mapMixtureSet();
    void accumulateMixtureSetTextDependent();
    void accumulateMixtureSetTextIndependent();

    void accumulateTdcSumFile();
    void combineTdcSumFile();

    void accumulateCartExamples();
    void mergeCartExamples();
    void estimateCart();

    void accumulateDiscriminatively();
    void convertMixtureSetToMixtureSetEstimator();
    void convertMixtureSetToLogLinearMixtureSet();

    void scoreFeatures();

    void estimateScatterMatricesTextDependent();
    void estimateScatterMatricesTextIndependent();
    void estimateLdaTextDependent();
    void estimateLdaTextIndependent();
    void combineScatterMatrixAccumulators();
    void estimateScatterMatricesFromScatterMatrixAccumulator();
    void estimateLdaFromScatterMatrixAccumulator();
    void estimateLda(Signal::ScatterMatricesEstimator &estimator);
    void estimateLda();

    void estimateAffineFeatureTransform();
    void calculateAffineFeatureTransform();
    void combineAffineFeatureTransformEstimators();
    void scoreAffineFeatureTransform();

    /*! @todo give these functions a more descriptive name! */
    void estimateAffineFeatureTransform2();
    void calculateAffineFeatureTransform2();
    void scoreAffineFeatureTransform2();
    void estimateModelTransform();
    void calculateModelTransform();

    void accumulateMaximumEntropy();
    void accumulateMaximumEntropyFast();
    void accumulateMaximumEntropyBuffered();
    void convertLambdas();
    void maximumEntropyShuffledOnline();
    void maximumEntropyShuffledOnlineLbfgs();
    void accumulateCombinedMaximumEntropy();
    void accumulateCombinedExactMaximumEntropy();
    void combineMaximumEntropy();
    void estimateMaximumEntropy();
    void accumulatorConvert();
    void centerMaximumEntropy();
    void tieMaximumEntropy();
    void accumulateJoergfile();
    void accumulateNearestNeighborTree();

    void accumulateWeightHistogram();

public:
    AcousticModelTrainer();
    virtual std::string getUsage() const { return "corpus driven acoustic model trainer"; }

    virtual int main(const std::vector<std::string> &arguments);
};

#endif //_TOOLS_ACOUSTIC_MODEL_TRAINER_ACOUSTIC_MODEL_TRAINER_HH
