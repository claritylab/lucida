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
#ifndef _MM_LINEAR_FEATURE_TRANSFORM_ACCUMULATOR_HH
#define _MM_LINEAR_FEATURE_TRANSFORM_ACCUMULATOR_HH

#include "Types.hh"
#include "MixtureSet.hh"
#include "Feature.hh"
#include "AbstractAdaptationAccumulator.hh"
#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>
#include <Math/Matrix.hh>
#include <map>

namespace Mm {

    /**
     * AffineFeatureTransformAccumulator
     *
     * Implementation of C-MLLR or Feature Space MLLR, as described in [Gales 1998]
     * Extended to handle dimension reducing transformations, by using the
     * degenerate MMI criterion [Visweswariah 2004]. The extension is equivalent
     * to ML estimation for non-projecting transformations.
     */
    class AffineFeatureTransformAccumulator: public AbstractAdaptationAccumulator {
    public:
	typedef std::vector< Math::Matrix<Sum> > GMatrixAccumulator;
	typedef std::vector< Math::Vector<Sum> > KVectorAccumulator;
	typedef Math::Matrix<FeatureType> Transform;
	enum Criterion { naive, mmiPrime, hlda };
    private:
	u32 featureDimension_;
	u32 modelDimension_;
	std::string key_;
	Sum beta_;
	GMatrixAccumulator gAccumulator_;
	KVectorAccumulator kAccumulator_;
	std::vector<VarianceType> globalModelVariance_;
	bool globalVarianceOptimization_;

	Math::Vector<Sum> globalMeanAccumulator_;
	Math::Matrix<Sum> globalCovarianceAccumulator_;
    public:
	AffineFeatureTransformAccumulator();
	AffineFeatureTransformAccumulator(size_t featureDimension, size_t modelDimension_,
		const std::string &key);

	void accumulate(Core::Ref<const Feature::Vector>, DensityIndex, Core::Ref<MixtureSet>);
	void accumulate(Core::Ref<const Feature::Vector>, DensityIndex, Core::Ref<MixtureSet>, Mm::Weight);
	void accumulate(Core::Ref<const Feature::Vector> f, DensityIndex di, MixtureIndex mi, Core::Ref<MixtureSet> m)
	{
	    accumulate(f, di, m);
	}
	void accumulate(Core::Ref<const Feature::Vector> f, DensityIndex di, MixtureIndex mi, Core::Ref<MixtureSet> m, Mm::Weight w)
	{
	    accumulate(f, di, m, w);
	}
	void finalize();
	void compact();
	Transform estimate(int iterations, Weight minObsWeight,
		Criterion criterion, const Transform &initialTransform);
	Transform estimate(int iterations, Weight minObsWeight, Criterion criterion);
	Sum score(Transform& transform, Criterion criterion);

	virtual std::string typeName() const { return "affine-feature-transform-accumulator"; }
	u32 featureDimension() { return featureDimension_; }

	void combine(AbstractAdaptationAccumulator&, Mm::Weight);

	bool read(Core::BinaryInputStream &is);
	bool write(Core::BinaryOutputStream &os) const;
	void dump(Core::XmlOutputStream&);
    };

}

#endif
