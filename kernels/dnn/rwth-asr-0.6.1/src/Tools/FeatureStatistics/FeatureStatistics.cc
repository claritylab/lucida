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
#include <Am/Module.hh>
#include <Audio/Module.hh>
#include <Core/Application.hh>
#include <Flow/Module.hh>
#include <Lm/Module.hh>
#include <Math/Module.hh>
#include <Mm/Module.hh>
#include <Signal/Module.hh>
#include <Speech/Module.hh>
#include <Flow/Registry.hh>
#ifdef MODULE_NN
#include <Nn/Module.hh>
#endif

#include "FeatureStatistics.hh"
#include <Speech/CovarianceEstimator.hh>

const Core::Choice FeatureStatistics::choiceAction(
	"not-given", actionNotGiven,
	"dry", actionDry,
	"find-maximum", actionFindMaximum,
	"apply-scatter-matrix-threshold", actionApplyScatterMatrixThreshold,
	"estimate-histograms", actionEstimateHistograms,
	"estimate-sequence-selection-ratio", actionEstimateSequenceSelectionRatio,
	"estimate-mean", actionEstimateMean,
	"estimate-covariance", actionEstimateCovariance,
	"estimate-mean-and-diagonal-variance", actionEstimateMeanAndDiagonalCovariance,
	"accumulate-mean-and-diagonal-variance", actionAccumulateMeanAndDiagonalCovariance,
	"estimate-pca", actionEstimatePca,
	"estimate-covariance-and-pca", actionEstimateCovarianceAndPca,
	"calculate-covariance-diagonal-normalization", actionCalculateCovarianceDiagonalNormalization,
	"number-of-active-elements", actionNumberOfActiveElements,
	Core::Choice::endMark());
const Core::ParameterChoice FeatureStatistics::paramAction(
	"action", &choiceAction, "operation to perfom", actionNotGiven);

APPLICATION(FeatureStatistics)

FeatureStatistics::FeatureStatistics()
{
    INIT_MODULE(Am);
    INIT_MODULE(Audio);
    INIT_MODULE(Flow);
    INIT_MODULE(Lm);
    INIT_MODULE(Math);
    INIT_MODULE(Mm);
    INIT_MODULE(Signal);
    INIT_MODULE(Speech);
#ifdef MODULE_NN
    INIT_MODULE(Nn);
#endif
    setTitle("feature-statistics");
}

int FeatureStatistics::main(const std::vector<std::string> &arguments)
{
    switch ((Action)paramAction(config)) {
    case actionDry : dryRun();
    case actionFindMaximum: findMaximum();
    break;
    case actionApplyScatterMatrixThreshold: applyScatterMatrixThreshold();
    break;
    case actionEstimateHistograms: estimateHistograms();
    break;
    case actionEstimateSequenceSelectionRatio: estimateSequenceSelectionRatio();
    break;
    case actionEstimateMean: estimateMean();
    break;
    case actionEstimateCovariance: estimateCovariance();
    break;
    case actionAccumulateMeanAndDiagonalCovariance: accumulateMeanAndDiagonalCovariance();
    break;
    case actionEstimateMeanAndDiagonalCovariance: estimateMeanAndDiagonalCovariance();
    break;
    case actionEstimatePca: estimatePca();
    break;
    case actionEstimateCovarianceAndPca: estimateCovarianceAndPca();
    break;
    case actionCalculateCovarianceDiagonalNormalization: calculateCovarianceDiagonalNormalization();
    break;
    case actionNumberOfActiveElements: calculateNumberOfActiveElements();
    break;
    default:
	criticalError("Action not given.");
    };

    return 0;
}

void FeatureStatistics::dryRun(){
    Speech::FeatureExtractor dummy(select("dummy-feature-extractor"));
    visitCorpus(dummy);
}

void FeatureStatistics::findMaximum()
{
#if 1
    criticalError("Module SPEECH_ADVANCED is not available");
#endif
}

void FeatureStatistics::calculateNumberOfActiveElements()
{
#if 1
    criticalError("Module SPARSE is not available");
#endif
}

void FeatureStatistics::applyScatterMatrixThreshold()
{
	Signal::ScatterThresholding scatterThresholding(select("scatter-matrix-thresholding"));
	scatterThresholding.work();
	scatterThresholding.write();
}

void FeatureStatistics::estimateHistograms()
{
#if 1
    criticalError("Module SIGNAL_ADVANCED is not available");
#endif
}

void FeatureStatistics::estimateSequenceSelectionRatio()
{
#if 1
    criticalError("Module SPEECH_ADVANCED is not available");
#endif
}

void FeatureStatistics::estimateMean()
{
#if 1
    criticalError("Module SIGNAL_ADVANCED is not available");
#endif
}

void FeatureStatistics::estimateCovariance()
{
    Speech::CovarianceEstimator covarianceEstimator(select("covariance-estimator"));
    visitCorpus(covarianceEstimator);
    covarianceEstimator.write();
}

void FeatureStatistics::estimateMeanAndDiagonalCovariance()
{
#if 1
    criticalError("Module MODULE_SPARSE is not available");
#endif

}

void FeatureStatistics::accumulateMeanAndDiagonalCovariance()
{
#if 1
    criticalError("Module MODULE_SPARSE is not available");
#endif
}

void FeatureStatistics::estimatePca()
{
    Signal::PrincipalComponentAnalysis pca(select("pca-estimator"));
    pca.work();
    pca.write();
}

void FeatureStatistics::estimateCovarianceAndPca()
{
    Speech::CovarianceEstimator covarianceEstimator(select("covariance-estimator"));
    visitCorpus(covarianceEstimator);
    Signal::ScatterMatrix covarianceMatrix;
    covarianceEstimator.finalize(covarianceMatrix);
    Signal::PrincipalComponentAnalysis pca(select("pca-estimator"));
    pca.work(covarianceMatrix);
    pca.write();
}

void FeatureStatistics::calculateCovarianceDiagonalNormalization()
{
    Signal::ScatterDiagonalNormalization scatterDiagonalNormalization(
	    select("covariance-diagonal-normalization"));
    scatterDiagonalNormalization.work();
    scatterDiagonalNormalization.write();
}

void FeatureStatistics::visitCorpus(Speech::CorpusProcessor &corpusProcessor)
{
    Speech::CorpusVisitor corpusVisitor(select("coprus-visitor"));
    corpusProcessor.signOn(corpusVisitor);

    Bliss::CorpusDescription corpusDescription(select("corpus"));
    corpusDescription.accept(&corpusVisitor);

    corpusProcessor.respondToDelayedErrors();
}
