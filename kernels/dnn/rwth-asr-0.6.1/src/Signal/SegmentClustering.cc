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
#include "SegmentClustering.hh"

using namespace Signal;


// ------------------------------------------------------------------------
bool DiagCovMonoGaussianModel::accumulate(const std::vector<f32> &vec) {
    Math::Vector<f64> convVec(vec);
    Math::Vector<f64> sqvector(dim());
    sqvector.squareVector(vec);
    mean_     += vec;
    variance_ += sqvector;
    ++nFrames_;
    return(0);
}

bool DiagCovMonoGaussianModel::finalize() {
    f64 inverseNFrames = 1.0F/nFrames_;
    Math::Vector<f64> squareVec(dim());
    mean_     *= inverseNFrames;
    variance_ *= inverseNFrames;
    squareVec.squareVector(mean_);
    variance_ -= squareVec;
    Math::Matrix<f64> invTest = Math::makeDiagonalMatrix(variance_);
    while(!Math::Lapack::invert(invTest, false))
    {
	variance_ += Math::Vector<f64>(invTest.nRows(), 0.1);
	invTest = Math::makeDiagonalMatrix(variance_);
	Core::Application::us()->log() << "variance was singular, adding diagonal 0.1";
    }
    return(0);
}


// ------------------------------------------------------------------------
const f32 FullCovMonoGaussianModel::computeComplexity() {
    f32 complexity;
    complexity = 0.25f * (f32) dim() * ( 3.0f + (f32) dim() );
    return(complexity);
}

bool FullCovMonoGaussianModel::accumulate(const std::vector<f32> &vec) {
    Math::Vector<f64> convVec(vec);
    Math::Matrix<f64> sqvec(dim());
    sqvec.squareVector(convVec);
    mean_     += convVec;
    variance_ += sqvec;
    ++nFrames_;
    return(0);
}

bool FullCovMonoGaussianModel::finalize() {
    f64 inverseNFrames = 1.0F/ nFrames_;
    Math::Matrix<f64> squareVec(dim());
    mean_     = mean_ * inverseNFrames;
    variance_ = variance_ * inverseNFrames;
    squareVec.squareVector(mean_);
    variance_ -= squareVec;
    Math::Matrix<f64> invTest = variance_;
    while(!Math::Lapack::invert(invTest, false))
    {
	variance_ += Math::makeDiagonalMatrix<f64>(invTest.nRows(), 0.1);
	invTest = variance_;
	Core::Application::us()->log() << "variance matrix was singular, adding diagonal 0.1";
    }
    return(0);
}

void FullCovMonoGaussianModel::mergeMeans(const FullCovMonoGaussianModel &x, const FullCovMonoGaussianModel &y) {
    Math::Vector<f64> tmpVector(dim());
    nFrames_  = x.nFrames() + y.nFrames();
    mean_     = x.mean_ * x.nFrames();
    tmpVector = y.mean_ * y.nFrames();
    mean_     += tmpVector;
    mean_     *= (1.0F/ nFrames_);
}

void FullCovMonoGaussianModel::mergeVariance(const FullCovMonoGaussianModel &x,	const FullCovMonoGaussianModel &y) {
    Math::Matrix<f64> tmpMatrix(dim());
    f64 relativeWeight;

    relativeWeight = (f64) x.nFrames() / (f64) nFrames_;
    variance_.squareVector(x.mean_ - mean_);
    variance_ += x.variance_;
    variance_ *= relativeWeight;

    relativeWeight = (f64) y.nFrames()/ (f64) nFrames_;
    tmpMatrix.squareVector(y.mean_- mean_);
    tmpMatrix += y.variance_;
    tmpMatrix *= relativeWeight;

    variance_ += tmpMatrix;
}

void FullCovMonoGaussianModel::computeL() {
    likelihood_ = Math::Lapack::logDeterminant(variance_);
    likelihood_ *= nFrames_;
}

const f32 FullCovMonoGaussianModel::relativeLikelihood(const FullCovMonoGaussianModel &x) const {
    /* p(this|x) */
    f64 likelihood = 0.0F;
    Math::Vector<f64> tmpvec(mean_ - x.mean_);
    Math::Matrix<f64> invmat(x.variance_);
    if(!Math::Lapack::invert(invmat))
	return Core::Type<f32>::max; // failure

    f64 det = Math::Lapack::logDeterminant(x.variance_);
    if(det == Core::Type<f64>::max)
	return Core::Type<f32>::max; // failure

    likelihood -= det;
    likelihood -= invmat.productTrace(variance_);
    likelihood -= invmat.vvt(tmpvec);
    likelihood -= dim()*LN_2PI;
    likelihood *= 0.5F;
    likelihood *= nFrames_;
    return(f32(likelihood));
}

void FullCovMonoGaussianModel::mergeModels(const FullCovMonoGaussianModel &x, const FullCovMonoGaussianModel &y) {
    FullCovMonoGaussianModel::mergeMeans(x,y);
    FullCovMonoGaussianModel::mergeVariance(x,y);
}


// ------------------------------------------------------------------------
void BICFullCovMonoGaussianModel::computeGLR(const BICFullCovMonoGaussianModel &x, const BICFullCovMonoGaussianModel &y) {
    glr_ = 0.5f * (likelihood_ - x.likelihood() - y.likelihood());
}

void BICFullCovMonoGaussianModel::finalize() {
    FullCovMonoGaussianModel::finalize();
    FullCovMonoGaussianModel::computeL();
}

void BICFullCovMonoGaussianModel::mergeModels(const BICFullCovMonoGaussianModel &x, const BICFullCovMonoGaussianModel &y) {
    FullCovMonoGaussianModel::mergeModels(x,y);
    FullCovMonoGaussianModel::computeL();
    BICFullCovMonoGaussianModel::computeGLR(x,y);
}


// ------------------------------------------------------------------------
void KL2FullCovMonoGaussianModel::computeKL2( const KL2FullCovMonoGaussianModel &x, const KL2FullCovMonoGaussianModel &y) {
    kl2_ = Core::Type<f32>::max;
    f32 a = x.relativeLikelihood(x), b = y.relativeLikelihood(y), c = x.relativeLikelihood(y), d = y.relativeLikelihood(x);
    if(a != Core::Type<f32>::max && b != Core::Type<f32>::max && c != Core::Type<f32>::max && d != Core::Type<f32>::max)
	kl2_ = a + b - c - d;
}

void KL2FullCovMonoGaussianModel::mergeModels(const KL2FullCovMonoGaussianModel &x, const KL2FullCovMonoGaussianModel &y) {
    BICFullCovMonoGaussianModel::mergeModels(x,y);
    KL2FullCovMonoGaussianModel::computeKL2(x,y);
}


// ------------------------------------------------------------------------
void CorrFullCovMonoGaussianModel::relativeFeatureNormalize(f32 weight) {
    /*Use a weight alpha (tweaked)*/
    f32 NormFac;

    relativeFeature_ *= weight;
    NormFac = relativeFeature_.logSum();

    relativeFeature_ -= NormFac;
    relativeFeature_.exponent();
}

void CorrFullCovMonoGaussianModel::refresh() {
    if (refresh_){
	relativeFeature_.resize(models_->size(),f32());
	for (u32 i = 0; i < models_->size(); i++)
	    relativeFeature_[i] = relativeLikelihood((*models_)[i]);
	relativeFeatureNormalize(alpha_);
	refresh_ = 0;
    }
}

void CorrFullCovMonoGaussianModel::correlationDistance(CorrFullCovMonoGaussianModel &x, CorrFullCovMonoGaussianModel &y) {
    x.refresh();
    y.refresh();
    corrdist_ = x.relativeFeature() * y.relativeFeature();
    /* Reverse the correlation distance for min/max criteria*/
    corrdist_ = -corrdist_;
}

void CorrFullCovMonoGaussianModel::mergeModels( CorrFullCovMonoGaussianModel &x, CorrFullCovMonoGaussianModel &y) {
    BICFullCovMonoGaussianModel::mergeModels(x,y);
    correlationDistance(x,y);
}

void CorrFullCovMonoGaussianModel::finalize(const std::vector<CorrFullCovMonoGaussianModel> &models) {
    BICFullCovMonoGaussianModel::finalize();
    models_ = &models;
}


// ------------------------------------------------------------------------
// @todo: also define the template functions here
