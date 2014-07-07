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
#include "CovarianceEstimator.hh"
#include <Math/Vector.hh>
#include <Math/Module.hh>

using namespace Speech;

CovarianceEstimator::CovarianceEstimator(const Core::Configuration &c) :
    Component(c), Extractor(c), Estimator(c), needResize_(true) {
}

void CovarianceEstimator::processFeature(Core::Ref<const Speech::Feature> feature) {
    if (feature->nStreams() and feature->mainStream()->size()) {
	accumulate(*feature->mainStream());
    }
}

void CovarianceEstimator::setFeatureDescription(const Mm::FeatureDescription &description) {
    description.verifyNumberOfStreams(1);
    size_t d;
    description.mainStream().getValue(Mm::FeatureDescription::nameDimension, d);
    if (needResize_) {
	setDimension(d);
	needResize_ = false;
    }
}
