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
#include <Core/Application.hh>

class FeatureStatistics :
    public Core::Application
{
public:
    enum Action {
	actionDry,
	actionFindMaximum,
	actionApplyScatterMatrixThreshold,
	actionEstimateHistograms,
	actionEstimateSequenceSelectionRatio,
	actionEstimateMean,
	actionEstimateCovariance,
	actionEstimateMeanAndDiagonalCovariance,
	actionAccumulateMeanAndDiagonalCovariance,
	actionEstimatePca,
	actionEstimateCovarianceAndPca,
	actionCalculateCovarianceDiagonalNormalization,
	actionNumberOfActiveElements,
	actionNotGiven
    };

    static const Core::Choice choiceAction;
    static const Core::ParameterChoice paramAction;
private:
    void dryRun();
    void findMaximum();
    void calculateNumberOfActiveElements();
    void applyScatterMatrixThreshold();
    void estimateHistograms();
    void estimateSequenceSelectionRatio();
    void estimateMean();
    void estimateCovariance();
    void estimateMeanAndDiagonalCovariance();
    void accumulateMeanAndDiagonalCovariance();
    void estimatePca();
    void estimateCovarianceAndPca();
    void calculateCovarianceDiagonalNormalization();

    void visitCorpus(Speech::CorpusProcessor &corpusProcessor);
public:
    FeatureStatistics();
    ~FeatureStatistics() {}

    virtual std::string getUsage() const { return "Creates statistics over the extracted features"; }
    virtual int main(const std::vector<std::string> &arguments);
};
