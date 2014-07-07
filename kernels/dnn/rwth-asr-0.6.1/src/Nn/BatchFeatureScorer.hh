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
#ifndef _NN_BATCHFEATURESCORER_H_
#define _NN_BATCHFEATURESCORER_H_

#include <Mm/FeatureScorer.hh>
#include <Mm/Types.hh>
#include <Mm/Feature.hh>

#include "ClassLabelWrapper.hh"
#include "LinearAndActivationLayer.hh"
#include "NeuralNetwork.hh"
#include "Prior.hh"
#include "Types.hh"


namespace Nn {

/*
 * Neural network feature scorer, that computes scores in batch mode
 * current limitations:
 *  -> only softmax output layer allowed
 *  -> no lazy evaluation of scores
 *
 */
class BatchFeatureScorer : public Mm::FeatureScorer {
    typedef Mm::FeatureScorer Precursor;
protected:
    typedef Types<f32>::NnVector NnVector;
    typedef Types<f32>::NnMatrix NnMatrix;
    static const Core::ParameterInt paramBufferSize;
protected:
    /**
     * Stores the current feature and the number of buffered features.
     * All computations are done in BatchFeatureScorer.
     * This class is used only because it is required by the
     * FeatureScorer interface.
     */
    class ContextScorer : public FeatureScorer::ContextScorer
    {
    public:
	ContextScorer(const BatchFeatureScorer *parent, u32 currentFeature) :
	    parent_(parent),
	    currentFeature_(currentFeature) {}
	virtual ~ContextScorer() {}
	virtual Mm::EmissionIndex nEmissions() const { return parent_->nMixtures(); }
	virtual Mm::Score score(Mm::EmissionIndex e) const {
	    return parent_->getScore(e, currentFeature_);
	}
    private:
	const BatchFeatureScorer *parent_;
	u32 currentFeature_;
    };

    const u32 bufferSize_;
    Prior<f32> prior_;
    mutable u32 nBufferedFeatures_;
    mutable u32 currentFeature_;		/* pointer to current position in buffer */
    mutable NnMatrix buffer_;
    mutable std::vector<bool> scoreComputed_;	/* indicates for which positions in buffer the score has already been computed */
    u32 nClasses_;
    u32 inputDimension_;

    ClassLabelWrapper *labelWrapper_;
    mutable NeuralNetwork<f32> network_;
public:
    BatchFeatureScorer(const Core::Configuration &c, Core::Ref<const Mm::MixtureSet> mixtureSet);
    virtual ~BatchFeatureScorer();
private:
    virtual void init(Core::Ref<const Mm::MixtureSet> mixtureSet);
    virtual void setFeature(u32 position, const Mm::FeatureVector& f) const;
public:
    virtual Mm::EmissionIndex nMixtures() const { return nClasses_; }
    virtual void getFeatureDescription(Mm::FeatureDescription &description) const {
	description.mainStream().setValue(Mm::FeatureDescription::nameDimension, inputDimension_);
    }

    typedef Core::Ref<const ContextScorer> Scorer;
    /**
     * Return a scorer for the current feature and append the
     * given feature to the buffer.
     * The current feature may not be the same as f
     * because of the feature buffering.
     * Requires bufferFilled() == true.
     */
    virtual FeatureScorer::Scorer getScorer(Core::Ref<const Mm::Feature> f) const {
	return getScorer(*f->mainStream());
    }
    virtual FeatureScorer::Scorer getScorer(const Mm::FeatureVector &f) const;

    virtual Mm::Score getScore(Mm::EmissionIndex e, u32 position) const;

    /**
     * reset should be overloaded/defined in/for
     * featurescorer related to sign language recognition
     * especially the tracking part
     *
     */
    virtual void reset() const;

    /**
     * finalize should be overloaded/defined in classes using
     * embedded flow networks to sent final end of sequence token
     * if necessary
     */
    virtual void finalize() const {};

    /**
     * Return true if the feature scorer buffers features.
     */
    virtual bool isBuffered() const { return true; }

    /**
     * Add a feature to the feature buffer.
     */
    virtual void addFeature(const Mm::FeatureVector &f) const;
    virtual void addFeature(Core::Ref<const Mm::Feature> f) const {
	addFeature(*f->mainStream());
    }

    /**
     * Return a scorer for the current feature without adding a
     * new feature to the buffer.
     * Should be called until bufferEmpty() == true.
     * Requires bufferEmpty() == false.
     * Implementation required if isBuffered() == true
     */
    virtual Mm::FeatureScorer::Scorer flush() const;

    /**
     * Return true if the feature buffer is full.
     */
    virtual bool bufferFilled() const { return nBufferedFeatures_ >= bufferSize_ - 1; }

    /**
     * Return true if the feature buffer is empty.
     */
    virtual bool bufferEmpty() const { return nBufferedFeatures_ == 0; }

    /**
     * Return the number of buffered features required to
     * execute getScorer().
     */
    virtual u32 bufferSize() const { return bufferSize_; }
};

} // namespace

#endif
