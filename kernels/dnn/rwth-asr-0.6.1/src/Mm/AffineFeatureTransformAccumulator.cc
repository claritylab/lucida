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
#include "AffineFeatureTransformAccumulator.hh"
#include <Math/Lapack/MatrixTools.hh>
#include <Core/Statistics.hh>

using namespace Mm;


AffineFeatureTransformAccumulator::AffineFeatureTransformAccumulator():
    featureDimension_(0),
    modelDimension_(0)
{ }

AffineFeatureTransformAccumulator::AffineFeatureTransformAccumulator(size_t featureDimension,
	size_t modelDimension, const std::string &key):
    featureDimension_(featureDimension),
    modelDimension_(modelDimension),
    key_(key),
    beta_(0),
    globalVarianceOptimization_(false),
    globalMeanAccumulator_(featureDimension + 1),
    globalCovarianceAccumulator_(featureDimension + 1)
{
    for (u32 i=0; i<modelDimension; i++) {
	gAccumulator_.push_back(Math::Matrix<Sum>(featureDimension + 1));
	kAccumulator_.push_back(Math::Vector<Sum>(featureDimension + 1));
    }
}

void AffineFeatureTransformAccumulator::accumulate(Core::Ref<const Feature::Vector> feature,
	DensityIndex density, Core::Ref<MixtureSet> mixture)
{
    verify(featureDimension_ != 0);

    beta_+= 1;

    FeatureVector extendedFeature(*feature);
    extendedFeature.insert(extendedFeature.begin(), 1.0);

    const GaussDensity *gaussDensity = mixture->density(density);
    const Mean &mean= *(mixture->mean(gaussDensity->meanIndex()));
    const std::vector<VarianceType> &variance=
	    mixture->covariance(gaussDensity->covarianceIndex())->diagonal();

    if (globalVarianceOptimization_ || (mixture->nCovariances() == 1)) {
	if (!globalVarianceOptimization_) {
	    for (u32 i=0; i<modelDimension_; i++)
		gAccumulator_[i].clear();
	    globalModelVariance_= variance;
	    globalVarianceOptimization_= true;
	}
	for (u32 i= 0; i<modelDimension_; i++)
	    for (u32 j= 0; j<featureDimension_+1; j++)
		kAccumulator_[i][j]+= Sum(mean[i])/Sum(variance[i])*Sum(extendedFeature[j]);
    }
    else {
	for (u32 i= 0; i<modelDimension_; i++) {
	    for (u32 j= 0; j<featureDimension_+1; j++) {
		kAccumulator_[i][j]+= Sum(mean[i])/Sum(variance[i])*Sum(extendedFeature[j]);
		for (u32 k= j; k<featureDimension_+1; k++)
		    gAccumulator_[i][j][k]+= Sum(extendedFeature[j])*Sum(extendedFeature[k])/Sum(variance[i]);
	    }
	}
    }

    for (u32 i= 0; i<featureDimension_+1; i++) {
	globalMeanAccumulator_[i]+= Sum(extendedFeature[i]);
	for (u32 j= i; j<featureDimension_+1; j++)
	    globalCovarianceAccumulator_[i][j]+= Sum(extendedFeature[i]) * Sum(extendedFeature[j]);
    }
}

void AffineFeatureTransformAccumulator::accumulate(Core::Ref<const Feature::Vector> feature,
	DensityIndex density, Core::Ref<MixtureSet> mixture, Mm::Weight weight)
{
    verify(featureDimension_ != 0);

    beta_+= weight;

    FeatureVector extendedFeature(*feature);
    extendedFeature.insert(extendedFeature.begin(), 1.0);

    const GaussDensity *gaussDensity = mixture->density(density);
    const Mean &mean= *(mixture->mean(gaussDensity->meanIndex()));
    const std::vector<VarianceType> &variance=
	    mixture->covariance(gaussDensity->covarianceIndex())->diagonal();

    if (globalVarianceOptimization_ || (mixture->nCovariances() == 1)) {
	if (!globalVarianceOptimization_) {
	    for (u32 i=0; i<modelDimension_; i++)
		gAccumulator_[i].clear();
	    globalModelVariance_= variance;
	    globalVarianceOptimization_= true;
	}
	for (u32 i= 0; i<modelDimension_; i++)
	    for (u32 j= 0; j<featureDimension_+1; j++)
		kAccumulator_[i][j]+=
		    Sum(mean[i])/Sum(variance[i])*Sum(extendedFeature[j]) * weight;
    }
    else {
	for (u32 i= 0; i<modelDimension_; i++) {
	    for (u32 j= 0; j<featureDimension_+1; j++) {
		kAccumulator_[i][j]+=
		    Sum(mean[i])/Sum(variance[i])*Sum(extendedFeature[j]) * weight;
		for (u32 k= j; k<featureDimension_+1; k++)
		    gAccumulator_[i][j][k]+=
			Sum(extendedFeature[j])*
			Sum(extendedFeature[k])/Sum(variance[i]) * weight;
	    }
	}
    }

    for (u32 i= 0; i<featureDimension_+1; i++) {
	globalMeanAccumulator_[i]+= Sum(extendedFeature[i]) * weight;
	for (u32 j= i; j<featureDimension_+1; j++)
	    globalCovarianceAccumulator_[i][j]+=
		Sum(extendedFeature[i]) * Sum(extendedFeature[j]) * weight;
    }
}

void AffineFeatureTransformAccumulator::finalize()
{
    if (globalVarianceOptimization_) {
	for (u32 i= 0; i<modelDimension_; i++) {
	    gAccumulator_[i].resize(featureDimension_+1);
	    for (u32 j= 0; j<featureDimension_+1; j++) {
		for (u32 k= j; k<featureDimension_+1; k++) {
		    gAccumulator_[i][j][k]= globalCovarianceAccumulator_[j][k]/Sum(globalModelVariance_[i]);
		}
	    }
	}
    }

    for (u32 i= 0; i<modelDimension_; i++)
	for (u32 j= 0; j<featureDimension_+1; j++)
	    for (u32 k= j+1; k<featureDimension_+1; k++)
		gAccumulator_[i][k][j]= gAccumulator_[i][j][k];

    for (u32 i= 0; i<featureDimension_+1; i++)
	for (u32 j= i+1; j<featureDimension_+1; j++)
	    globalCovarianceAccumulator_[j][i]= globalCovarianceAccumulator_[i][j];

}

void AffineFeatureTransformAccumulator::compact()
{
    for (u32 i=0; i<modelDimension_; i++)
	gAccumulator_[i].clear();
}

Math::Matrix<FeatureType>
AffineFeatureTransformAccumulator::estimate(int iterations, Weight minObsWeight, Criterion criterion)
{
    verify(featureDimension_ != 0);

    bool hldaCriterion;
    if (criterion == naive)
	hldaCriterion = false;
    else if (criterion == mmiPrime) {
	hldaCriterion = false;
    }
    else if (criterion == hlda) {
	hldaCriterion = true;
    }
    else
	defect();

    u32 targetDimension = hldaCriterion ? featureDimension_ : modelDimension_;

    // Set initial value; Identity transform - other posibilities can be better
    Math::Matrix<Sum> initialTransform(targetDimension, featureDimension_+1);
    for (u32 i= 0; i<targetDimension; i++)
	initialTransform[i][i+1]= 1.0;

    return estimate(iterations, minObsWeight, criterion, initialTransform);
}

Math::Matrix<FeatureType>
AffineFeatureTransformAccumulator::estimate(int iterations, Weight minObsWeight,
	Criterion criterion, const Transform &initialTransform)
{
    verify(featureDimension_ != 0);

    bool hldaCriterion;
    if (criterion == naive)
	hldaCriterion = false;
    else if (criterion == mmiPrime) {
	hldaCriterion = false;
    }
    else if (criterion == hlda) {
	hldaCriterion = true;
    }
    else
	defect();

    u32 targetDimension = hldaCriterion ? featureDimension_ : modelDimension_;

    Math::Matrix<Sum> transform(initialTransform);
    if (transform.nColumns()==featureDimension_)
	transform.insertColumn(0, Math::Vector<Sum>(transform.nRows()));
    // Really should use better error handling
    hope(transform.nColumns()==featureDimension_+1);
    hope(transform.nRows()==targetDimension);

    if (criterion != naive & beta_ < minObsWeight)
	return transform;

    if (hldaCriterion) {
	// Augument k and G accumulators with complement dimensions
	kAccumulator_.resize(featureDimension_);
	gAccumulator_.resize(featureDimension_);

	for (u32 i= modelDimension_; i<featureDimension_; i++) {
	    Sum componentVariance= (globalCovarianceAccumulator_[i+1][i+1]
		- globalMeanAccumulator_[i+1] * globalMeanAccumulator_[i+1] / beta_) / beta_;
	    Sum componentMean = (globalMeanAccumulator_[i+1] / beta_);

	    kAccumulator_[i]= globalMeanAccumulator_ * componentMean / componentVariance;
	    gAccumulator_[i]= globalCovarianceAccumulator_ / componentVariance;
	}
    }

    GMatrixAccumulator gInverse(gAccumulator_);

    for (u32 i= 0; i<targetDimension; i++)
	Math::Lapack::invert(gInverse[i]);

    if (criterion == naive) {
	for (u32 i= 0; i<targetDimension; i++)
	    transform.setRow(i, gInverse[i] * kAccumulator_[i]);
	return transform;
    }

    // Compute global covariance matrix
    Math::Matrix<Sum> meanSquare(globalMeanAccumulator_ / beta_, globalMeanAccumulator_ / beta_);
    Math::Matrix<Sum> globalCovariance(globalCovarianceAccumulator_/beta_ - meanSquare);
    globalCovariance.removeRow(0);
    globalCovariance.removeColumn(0);

    // Variables for mmiPrime 'jacobian'
    Math::Matrix<Sum> linearTransform(transform);
    linearTransform.removeColumn(0);
    Math::Matrix<Sum> halfCovarianceTransposed(linearTransform * globalCovariance);
    Math::Matrix<Sum> transformedCovariance(linearTransform * halfCovarianceTransposed.transpose());

    for (int i= 0; i < iterations; i++) {
	for (u32 row= 0; row<targetDimension; row++) {

	    // Compute extended generalised cofactor row vector. Note that scaling is arbitrary
	    // (see eq A.9 in Gales), so transpose of inverse is used instead.
	    Math::Matrix<Sum> linearTransform(transform);
	    linearTransform.removeColumn(0);

	    Math::Matrix<Sum> pseudoInverse(modelDimension_, modelDimension_);
	    if (targetDimension < featureDimension_)
		// The exact motivations for these formulas should be written down somewere...
		// pseudoInverse = linearTransform * globalCovariance * linearTransform.transpose();
		pseudoInverse = transformedCovariance;
	    else
		pseudoInverse = linearTransform;

	    Math::Lapack::invert(pseudoInverse);
	    Math::Vector<Sum> cofactors;

	    if (targetDimension < featureDimension_)
		// pseudoInverse= globalCovariance * linearTransform.transpose() * pseudoInverse;
		cofactors = halfCovarianceTransposed.transpose() * pseudoInverse.column(row);
	    else
		cofactors = pseudoInverse.column(row);

	    cofactors.insert(cofactors.begin(), 0.0);

	    // Compute epsilon1 and epsilon2
	    Sum epsilon1= 0.0, epsilon2= 0.0;
	    for (u32 j= 0; j<featureDimension_+1; j++) {
		for (u32 k= 0; k<featureDimension_+1; k++) {
		    epsilon1+= cofactors[j] * gInverse[row][j][k] * cofactors[k];
		    epsilon2+= cofactors[j] * gInverse[row][j][k] * kAccumulator_[row][k];
		}
	    }

	    //Solve for alpha1 and alpha2
	    Sum alpha1=	(-epsilon2 + std::sqrt(epsilon2*epsilon2 + 4*epsilon1*beta_)) / (2*epsilon1);
	    Sum alpha2=	(-epsilon2 - std::sqrt(epsilon2*epsilon2 + 4*epsilon1*beta_)) / (2*epsilon1);

	    //Test for largest likelihood
	    Sum l1= beta_ * std::log(std::abs(alpha1*epsilon1 + epsilon2)) - 1/2 * alpha1*alpha1*epsilon1;
	    Sum l2= beta_ * std::log(std::abs(alpha2*epsilon1 + epsilon2)) - 1/2 * alpha2*alpha2*epsilon1;
	    Sum alpha= l1>l2 ? alpha1 : alpha2;

	    //Update matrix row using alpha, accumulators and cofactors
	    Math::Vector<Sum> newRow (gInverse[row] * (cofactors*alpha + kAccumulator_[row]));
	    transform.setRow(row, newRow);

	    // Bookkeeping for mmiPrime
	    Math::Vector<Sum> newRowLinear(newRow.begin()+1, newRow.end());
	    linearTransform.setRow(row, newRowLinear);
	    Math::Vector<Sum> newRowHalfCov(globalCovariance * newRowLinear);
	    halfCovarianceTransposed.setRow(row, newRowHalfCov);
	    transformedCovariance.setRow(row, halfCovarianceTransposed * newRowLinear);
	    transformedCovariance.setColumn(row, linearTransform * newRowHalfCov);

	}


    }

    if (hldaCriterion) {
	// Compact k and G
	kAccumulator_.resize(modelDimension_);
	gAccumulator_.resize(modelDimension_);

	// Remove rejected dimensions
	transform.resize(modelDimension_, featureDimension_+1);
    }

    return transform;
}

Sum AffineFeatureTransformAccumulator::score(Transform& transform, Criterion criterion)
{
    verify(featureDimension_ != 0);

    // Compute global covariance matrix
    Math::Matrix<Sum> meanSquare(globalMeanAccumulator_ / beta_, globalMeanAccumulator_ / beta_);
    Math::Matrix<Sum> globalCovariance(globalCovarianceAccumulator_/beta_ - meanSquare);
    globalCovariance.removeRow(0);
    globalCovariance.removeColumn(0);

    Math::Matrix<Sum> linearTransform(transform);
    linearTransform.removeColumn(0);

    Sum jacobian = 0;

    if (criterion == naive)
	jacobian = 0;
    else if (criterion == mmiPrime)
	jacobian = Math::Lapack::logDeterminant(linearTransform * globalCovariance * linearTransform.transpose()) / 2;
    else if (criterion == hlda)
	defect(); //Not implemented yet;
    else
	defect();

    Sum distance = 0;
    for (u32 i= 0; i<modelDimension_; i++) {
	Math::Vector<f32> row(transform.row(i));
	distance += ((gAccumulator_[i] * row) * row) / 2 - kAccumulator_[i]*row;
    }

    return jacobian - distance / beta_;
}

void AffineFeatureTransformAccumulator::combine(
	AbstractAdaptationAccumulator& abstractOther,
	Mm::Weight weight)
{
    AffineFeatureTransformAccumulator* other =
	dynamic_cast<AffineFeatureTransformAccumulator*>(&abstractOther);

    verify(featureDimension_ == other->featureDimension_);
    verify(modelDimension_ == other->modelDimension_);
    verify(globalVarianceOptimization_ == other->globalVarianceOptimization_);
    if (globalVarianceOptimization_)
	verify(globalModelVariance_ == other->globalModelVariance_);

    beta_ += other->beta_ * weight;

    for (u32 i=0; i<modelDimension_; i++) {
	kAccumulator_[i] += other->kAccumulator_[i] * weight;
	if (!globalVarianceOptimization_)
	    gAccumulator_[i] += other->gAccumulator_[i] * weight;
    }

    globalMeanAccumulator_ += other->globalMeanAccumulator_ * weight;
    globalCovarianceAccumulator_ += other->globalCovarianceAccumulator_ * weight;
}

bool AffineFeatureTransformAccumulator::read(Core::BinaryInputStream &is)
{
    is >> featureDimension_ >> modelDimension_ >> beta_ >> gAccumulator_ >> kAccumulator_
	>> globalMeanAccumulator_ >> globalCovarianceAccumulator_ >> globalModelVariance_
	>> globalVarianceOptimization_;
    return is.good();
}

bool AffineFeatureTransformAccumulator::write(Core::BinaryOutputStream &os) const
{
    os << featureDimension_ << modelDimension_ << beta_ << gAccumulator_ << kAccumulator_
	<< globalMeanAccumulator_ << globalCovarianceAccumulator_ << globalModelVariance_
	<< globalVarianceOptimization_;
    return os.good();
}

void AffineFeatureTransformAccumulator::dump(Core::XmlOutputStream& os)
{
    os << Core::XmlOpen("affine-feature-transform-accumulator") + Core::XmlAttribute("key", key_) +
	Core::XmlAttribute("feature-dimension", featureDimension_) +
	Core::XmlAttribute("model-dimension", modelDimension_);

    os << Core::XmlOpen("beta") << beta_ << Core::XmlClose("beta");

    if (!globalVarianceOptimization_) {
	os << Core::XmlOpen("g-accumulator");
	for (u32 i=0; i<modelDimension_; i++)
	    os << gAccumulator_[i];
	os << Core::XmlClose("g-accumulator");
    }

    os << Core::XmlOpen("k-accumulator");
    for (u32 i=0; i<modelDimension_; i++)
	os << kAccumulator_[i];
    os << Core::XmlClose("k-accumulator");

    os << Core::XmlOpen("mean-accumulator");
    os << globalMeanAccumulator_;
    os << Core::XmlClose("mean-accumulator");

    os << Core::XmlOpen("covariance-accumulator");
    os << globalCovarianceAccumulator_;
    os << Core::XmlClose("covariance-accumulator");

    if (globalVarianceOptimization_) {
	os << Core::XmlOpen("global-model-variance");
	os << Math::Vector<VarianceType>(globalModelVariance_);
	os << Core::XmlClose("global-model-variance");
    }

    os << Core::XmlClose("affine-feature-transform-accumulator");
}
