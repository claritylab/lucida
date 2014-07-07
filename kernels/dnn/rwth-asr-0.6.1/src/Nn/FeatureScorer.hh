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
#ifndef _NN_FEATURESCORER_H_
#define _NN_FEATURESCORER_H_

#include <Mm/AssigningFeatureScorer.hh>
#include "NeuralNetwork.hh"
#include "ClassLabelWrapper.hh"
#include "LinearAndActivationLayer.hh"
#include "Prior.hh"
#include "Types.hh"

namespace Nn {

/*
 *
 * base class for all neural network feature scorers which derive from Mm::CachedAssigningFeatureScorer
 * provides network IO, handling of logPrior, basic getter methods
 *
 *
 */
class BaseFeatureScorer : public Mm::CachedAssigningFeatureScorer {
    typedef Mm::CachedAssigningFeatureScorer Precursor;
protected:
    typedef Types<f32>::NnVector NnVector;
    typedef Types<f32>::NnMatrix NnMatrix;
    typedef Core::Ref<const Mm::AssigningFeatureScorer::AssigningContextScorer> AssigningScorerRef;
protected:
    Prior<f32> prior_;
    ClassLabelWrapper *labelWrapper_;
    u32 nClasses_;
    u32 inputDimension_;
    mutable NeuralNetwork<f32> network_;
public:
    BaseFeatureScorer(const Core::Configuration &c);
    virtual ~BaseFeatureScorer();
    // initialization method
    virtual void init(Core::Ref<const Mm::MixtureSet> mixture);

    virtual Mm::MixtureIndex nMixtures() const { return nClasses_; }
    virtual Mm::ComponentIndex dimension() const { return inputDimension_; }
    virtual Mm::DensityIndex nDensities() const { return nClasses_; }
};

// Neural network feature scorer with on-demand computation of scores

class OnDemandFeatureScorer : public BaseFeatureScorer {

    typedef BaseFeatureScorer Precursor;
protected:
    // by creating a context object, all hidden layers are forwarded
    // the result is shared for all emission scores
    class Context : public Mm::CachedAssigningFeatureScorer::CachedAssigningContextScorer {
	friend class OnDemandFeatureScorer;
	static const Mm::Score invalidScore;
    protected:
	const Mm::FeatureVector featureVector_;
	NnMatrix activationOfLastHiddenLayer_;
    public:
	Context(const Mm::FeatureVector &featureVector,
		const OnDemandFeatureScorer *featureScorer,
		size_t cacheSize,
		bool check = false);
    };
protected:
    u32 topLayerOutputDimension_;
    mutable LinearAndSoftmaxLayer<f32> *outputLayer_;
public:
    OnDemandFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixture);
    virtual ~OnDemandFeatureScorer();
public:
    virtual void init(Core::Ref<const Mm::MixtureSet> mixture);
    virtual Mm::AssigningFeatureScorer::ScoreAndBestDensity calculateScoreAndDensity(const CachedAssigningContextScorer *cs, Mm::MixtureIndex mixtureIndex) const;
    virtual Mm::Score calculateScore(const CachedAssigningContextScorer *cs, Mm::MixtureIndex mixtureIndex, Mm::DensityIndex dnsInMix) const;
    Mm::Score calculateScore(const NnMatrix &activation, Mm::MixtureIndex mixtureIndex) const;

    virtual AssigningScorerRef getAssigningScorer(const Mm::FeatureVector &featureVector) const {
	return AssigningScorerRef(new Context(featureVector, this, nMixtures()));
    }

    virtual bool isPriorRemovedFromBias() const {return outputLayer_->logPriorIsRemovedFromBias();}
protected:
    void forwardHiddenLayers(const Mm::FeatureVector &in, NnMatrix &out) const ;
};

// neural network feature scorer that always evaluates all scores

class FullFeatureScorer : public BaseFeatureScorer {

    typedef BaseFeatureScorer Precursor;
    typedef Core::Ref<const Mm::AssigningFeatureScorer::AssigningContextScorer> AssigningScorerRef;
protected:
    // by creating a context object, all layers are forwarded (including the output layer)
    // the softmax-nonlinearity is discarded
    // the result is shared for all emission scores
    class Context : public Mm::CachedAssigningFeatureScorer::CachedAssigningContextScorer {
	friend class FullFeatureScorer;
	static const Mm::Score invalidScore;
    protected:
	const Mm::FeatureVector featureVector_;
	NnMatrix scores_;
    public:
	Context(const Mm::FeatureVector &featureVector,
		const FullFeatureScorer *featureScorer,
		size_t cacheSize,
		bool check = false);
    };
protected:
    u32 topLayerOutputDimension_;
public:
    FullFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixture);
    virtual ~FullFeatureScorer();
public:
    virtual void init(Core::Ref<const Mm::MixtureSet> mixture);
    virtual Mm::AssigningFeatureScorer::ScoreAndBestDensity calculateScoreAndDensity(const CachedAssigningContextScorer *cs, Mm::MixtureIndex mixtureIndex) const;
    virtual Mm::Score calculateScore(const CachedAssigningContextScorer *cs, Mm::MixtureIndex mixtureIndex, Mm::DensityIndex dnsInMix) const;
    Mm::Score calculateScore(const NnMatrix &activation, Mm::MixtureIndex mixtureIndex) const;

    virtual AssigningScorerRef getAssigningScorer(const Mm::FeatureVector &featureVector) const {
	return AssigningScorerRef(new Context(featureVector, this, nMixtures()));
    }
protected:
    void computeScores(const Mm::FeatureVector &in, NnMatrix &out) const ;
};

// neural network feature scorer that reads scores from feature cache

class PrecomputedFeatureScorer : public BaseFeatureScorer {

    typedef BaseFeatureScorer Precursor;
    typedef Core::Ref<const Mm::AssigningFeatureScorer::AssigningContextScorer> AssigningScorerRef;
protected:
    class Context : public Mm::CachedAssigningFeatureScorer::CachedAssigningContextScorer {
	friend class PrecomputedFeatureScorer;
	static const Mm::Score invalidScore;
    protected:
	const Mm::FeatureVector featureVector_;
    public:
	Context(const Mm::FeatureVector &featureVector,
		const PrecomputedFeatureScorer *featureScorer,
		size_t cacheSize,
		bool check = false);
    };
public:
    PrecomputedFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixture);
    virtual ~PrecomputedFeatureScorer();
public:
    virtual void init(Core::Ref<const Mm::MixtureSet> mixture);
    virtual Mm::AssigningFeatureScorer::ScoreAndBestDensity calculateScoreAndDensity(const CachedAssigningContextScorer *cs, Mm::MixtureIndex mixtureIndex) const;
    virtual Mm::Score calculateScore(const CachedAssigningContextScorer *cs, Mm::MixtureIndex mixtureIndex, Mm::DensityIndex dnsInMix) const;
    Mm::Score calculateScore(const Mm::FeatureVector &featureVector , Mm::MixtureIndex mixtureIndex) const;

    virtual AssigningScorerRef getAssigningScorer(const Mm::FeatureVector &featureVector) const {
	return AssigningScorerRef(new Context(featureVector, this, nMixtures()));
    }
};


}

#endif /* FEATURESCORER_H_ */
