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
#ifndef _SPEECH_COVARIANCE_ESTIMATOR_HH
#define _SPEECH_COVARIANCE_ESTIMATOR_HH

#include <Signal/EigenTransform.hh>
#include "DataExtractor.hh"

namespace Speech {

class CovarianceEstimator :
public FeatureExtractor,
public Signal::TotalScatterMatrixEstimator
{
    typedef FeatureExtractor Extractor;
    typedef Signal::TotalScatterMatrixEstimator Estimator;
protected:
    virtual void processFeature(Core::Ref<const Feature> feature);
    virtual void setFeatureDescription(const Mm::FeatureDescription &description);
public:
    CovarianceEstimator(const Core::Configuration &c);
    ~CovarianceEstimator() {}
private:
    bool needResize_;
};

} // namespace Speech

#endif // _SPEECH_COVARIANCE_ESTIMATOR_HH
