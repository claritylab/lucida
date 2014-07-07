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
#ifndef _SIGNAL_LIKELIHOOD_FUNCTION_HH
#define _SIGNAL_LIKELIHOOD_FUNCTION_HH

#include <Flow/Node.hh>
#include <Mm/FeatureScorer.hh>

namespace Signal {

    /** Base class for likelihood functions implementing
     *  the class dependent density of feature vector sequence: p(x_1^t | k).
     */
    class LikelihoodFunction : public virtual Core::Component {
    public:
	typedef f32 Score;
	typedef f32 Data;
	typedef f32 Weight;
	typedef std::vector<Score> ScoreVector;
    private:
	Weight sumOfWeights_;
    public:
	LikelihoodFunction(const Core::Configuration &c) : Component(c), sumOfWeights_(0) {}
	virtual ~LikelihoodFunction() {}

	virtual bool setClasses(const std::vector<std::string> &classLabels) = 0;
	virtual bool setDimension(size_t dimension) { return true; }

	virtual void reset() { sumOfWeights_ = 0; }

	/** Accumulates the scores for @param featureVector weighted by @param featureScoreWeight.
	 * The scores for @param featureVector are written in @param scores (if present)
	 */
	virtual void feed(const std::vector<Data> &featureVector, Weight featureScoreWeight, ScoreVector *scores = 0) {
	    sumOfWeights_ += featureScoreWeight;
	}
	/** Returns the current score for the class @param classIndex. */
	virtual Score operator[](u32 classIndex) = 0;
	/** Returns the sum of feature score weights. */
	Weight sumOfWeights() const { return sumOfWeights_; }
    };


    /** IndependentSequenceLikelihood implements the class dependent density of feature vector sequence:
     *    sum_{t} ( - log p(x_t | k) )
     *
     *  Assumptions:
     *    (i) feature vectors are independent
     *    (ii) classes are independent
     *
     *  Density p(x_t | k) is dependent on the FeatureScorer object.
     */
    class IndependentSequenceLikelihood : public LikelihoodFunction {
	typedef LikelihoodFunction Precursor;
    private:
	std::vector<Score> scores_;

	/** Object providing -log( p(x_t | k) ) scores
	 */
	Core::Ref<Mm::FeatureScorer> logLikelihoodFunctions_;
    private:
	/** Copying of Mm::FeatureScorer not supported. */
	IndependentSequenceLikelihood(const IndependentSequenceLikelihood &v) :
	    Component(v.getConfiguration()), Precursor(v.getConfiguration()) {}
	IndependentSequenceLikelihood& operator=(const IndependentSequenceLikelihood&) { return *this; }
    public:
	IndependentSequenceLikelihood(const Core::Configuration &c);
	virtual ~IndependentSequenceLikelihood();

	virtual bool setClasses(const std::vector<std::string> &classLabels);
	virtual bool setDimension(size_t dimension);

	virtual void reset();

	/** Accumulates the scores for @param featureVector. */
	virtual void feed(const std::vector<Data> &featureVector, Weight featureScoreWeight = 1, ScoreVector *scores = 0);
	/** Returns the current score for the class @param classIndex. */
	virtual Score operator[](u32 classIndex) { return scores_[classIndex]; }
    };

} // namespace Signal

#endif // _SIGNAL_LIKELIHOOD_FUNCTION_HH
