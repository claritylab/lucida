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
#include "ScatterEstimator.hh"
#include <Math/Module.hh>

using namespace Signal;

//----------------------------------------------------------------------------
const std::string Signal::totalScatterType("total");
const std::string Signal::betweenClassScatterType("between-class");
const std::string Signal::withinClassScatterType("within-class");

//----------------------------------------------------------------------------
const Core::ParameterInt ScatterMatrixEstimator::paramOutputPrecision(
    "output-precision", "Number of decimal digits in text output formats", 20);
const Core::ParameterBool ScatterMatrixEstimator::paramShallNormalize(
    "shall-normalize", "If yes, scatter matrices are divided by number of observations", true);

ScatterMatrixEstimator::ScatterMatrixEstimator(const Core::Configuration &configuration) :
    Precursor(configuration),
    featureDimension_(0),
    needInit_(true)
{}

ScatterMatrixEstimator::~ScatterMatrixEstimator()
{}

void ScatterMatrixEstimator::initialize()
{
    vectorSquareSum_.resize(featureDimension_, featureDimension_);
    vectorSquareSum_.fill(Sum(0));
    needInit_ = false;
}

void ScatterMatrixEstimator::accumulate(const Math::Vector<Data> &x, f32 weight)
{
    require_(!needInit_);
    require_(x.size() == featureDimension_);

    // vectorSquareSum_ += x * x^T but only for the lower triangle
    for (size_t i = 0; i < featureDimension_; ++i) {
	Math::Vector<Sum> &row(vectorSquareSum_[i]);
	Data x_i = x[i];
	for (size_t j = 0; j <= i; ++j)
	    row[j] += x_i * x[j] * weight;
    }
}

bool ScatterMatrixEstimator::accumulate(const ScatterMatrixEstimator &estimator)
{
    require(estimator.featureDimension_ == featureDimension_);
    vectorSquareSum_ += estimator.vectorSquareSum_;
    return true;
}

bool ScatterMatrixEstimator::finalize()
{
    if (needInit_) {
	error("No observation has been seen.");
	return false;
    }
    finalizeVectorSquareSum();
    return true;
}

void ScatterMatrixEstimator::finalizeVectorSquareSum()
{
    for (size_t i = 0; i < featureDimension_; ++ i) {
	for (size_t j = i + 1; j < featureDimension_; ++ j)
	    vectorSquareSum_[i][j] = vectorSquareSum_[j][i];
    }
}

bool ScatterMatrixEstimator::read(Core::BinaryInputStream &bis)
{
    u32 dimension;
    if (!(bis >> dimension)) return false;
    setDimension(dimension);
    initialize();
    // only for the lower triangle
    for (size_t i = 0; i < featureDimension_; ++i) {
	Math::Vector<Sum> &row(vectorSquareSum_[i]);
	for (size_t j = 0; j <= i; ++j)
	    if (!(bis >> row[j])) return false;
    }
    return true;
}

bool ScatterMatrixEstimator::write(Core::BinaryOutputStream &bos)
{
    if (!(bos << (u32)featureDimension_)) return false;
    // only for the lower triangle
    for (size_t i = 0; i < featureDimension_; ++i) {
	Math::Vector<Sum> &row(vectorSquareSum_[i]);
	for (size_t j = 0; j <= i; ++j)
	    if (!(bos << row[j])) return false;
    }
    return true;
}

void ScatterMatrixEstimator::setDimension(size_t dimension)
{
    if (featureDimension_ != dimension) {
	featureDimension_ = dimension;
	needInit_ = true;
    }
}

bool ScatterMatrixEstimator::write(
    const std::string &filename, const std::string &scatterType,
    const ScatterMatrix &scatterMatrix)
{
    bool success = true;
    if (Math::Module::instance().formats().write(
	    filename,
	    scatterMatrix,
	    paramOutputPrecision(config))) {
	log("The %s scatter matrix is written to '%s'.",
	    scatterType.c_str(), filename.c_str());
    } else {
	error("Failed to write the %s scatter matrix to '%s'.",
	      scatterType.c_str(), filename.c_str());
	success = false;
    }
    return success;
}

//------------------------------------------------------------------------------------------------
const Core::ParameterString TotalScatterMatrixEstimator::paramFilename(
    "file", "Output filename for covariance matrix");
const Core::ParameterFloat TotalScatterMatrixEstimator::paramElementThresholdMin(
    "element-threshold-min", "Min threshold for every covariance entry", Core::Type<Sum>::min);

TotalScatterMatrixEstimator::TotalScatterMatrixEstimator(const Core::Configuration &configuration) :
    Core::Component(configuration),
    Precursor(configuration)
{}

TotalScatterMatrixEstimator::~TotalScatterMatrixEstimator()
{}

void TotalScatterMatrixEstimator::initialize()
{
    vectorSum_.resize(featureDimension_);
    std::fill(vectorSum_.begin(), vectorSum_.end(), Sum(0));
    count_ = 0;
    Precursor::initialize();
}

void TotalScatterMatrixEstimator::accumulate(const Math::Vector<Data> &x)
{
    if (needInit_) initialize();
    vectorSum_ += x;
    ++ count_;
    Precursor::accumulate(x);
}

bool TotalScatterMatrixEstimator::finalize(ScatterMatrix &covarianceMatrix)
{
    if (!Precursor::finalize()) return false;
    covarianceMatrix = vectorSquareSum_ -
	Math::vectorInnerProduct(vectorSum_, vectorSum_ / count_);
    if (paramShallNormalize(config)) covarianceMatrix /= (Sum)count_;
	// If an entry is lower than threshold, it will be set to threshold.
	Sum threshold = paramElementThresholdMin(config);
	for (unsigned int i =0; i<covarianceMatrix.nRows(); ++i) {
		for (unsigned int j=0; j<covarianceMatrix.nColumns(); ++j) {
			if (covarianceMatrix[i][j] < threshold)
				covarianceMatrix[i][j] = threshold;
		}
	}
    return true;
}

bool TotalScatterMatrixEstimator::write()
{
    bool success = true;
    ScatterMatrix covarianceMatrix;
    if (!finalize(covarianceMatrix))
	success = false;
    if (!Precursor::write(paramFilename(config),
			  totalScatterType, covarianceMatrix))
	success = false;
    return success;
}

//------------------------------------------------------------------------------------------------
const Core::ParameterString ScatterMatricesEstimator::paramBetweenClassScatterFilename(
    "between-class-scatter-matrix-file", "output filename for between class scatter matrix");
const Core::ParameterString ScatterMatricesEstimator::paramWithinClassScatterFilename(
    "within-class-scatter-matrix-file", "output filename for within class scatter matrix");
const Core::ParameterString ScatterMatricesEstimator::paramTotalScatterFilename(
    "total-scatter-matrix-file", "output filename for total scatter matrix");

const Core::ParameterString ScatterMatricesEstimator::paramOldAccumulatorFilename(
    "old-accumulator-file", "input filename for accumulators");
const Core::ParameterString ScatterMatricesEstimator::paramNewAccumulatorFilename(
    "new-accumulator-file", "output filename for accumulators");
const Core::ParameterStringVector ScatterMatricesEstimator::paramAccumulatorFilesToCombine(
    "accumulator-files-to-combine", "combine with current estimator", " ", 1);


ScatterMatricesEstimator::ScatterMatricesEstimator(const Core::Configuration &configuration) :
    Core::Component(configuration),
    Precursor(configuration),
    nClasses_(0)
{}

ScatterMatricesEstimator::~ScatterMatricesEstimator()
{}

void ScatterMatricesEstimator::initialize(bool deepInitialization)
{
    vectorSums_.resize(nClasses_);
    counts_.resize(nClasses_);
    for(ClassIndex classIndex = 0; classIndex < nClasses_; classIndex ++) {
	vectorSums_[classIndex].resize(featureDimension_);
	std::fill(vectorSums_[classIndex].begin(), vectorSums_[classIndex].end(), Sum(0));
	counts_[classIndex] = Count(0);
    }
    if (deepInitialization) {
	Precursor::initialize();
    } else {
	needInit_ = false;
    }
}

void ScatterMatricesEstimator::accumulate(ClassIndex classIndex, const Math::Vector<Data> &x, f32 weight)
{
    require_(classIndex < nClasses_);
    if (needInit_) initialize();
    vectorSums_[classIndex] += x * weight;
    counts_[classIndex] += weight;
    Precursor::accumulate(x, weight);
}

bool ScatterMatricesEstimator::accumulate(const ScatterMatricesEstimator &estimator)
{
    require(estimator.nClasses_ == nClasses_);
    for (ClassIndex classIndex = 0; classIndex < nClasses_; ++ classIndex) {
	vectorSums_[classIndex] += estimator.vectorSums_[classIndex];
	counts_[classIndex] += estimator.counts_[classIndex];
    }
    return Precursor::accumulate(estimator);
}

bool ScatterMatricesEstimator::finalize(ScatterMatrix &betweenClassScatterMatrix,
					ScatterMatrix &withinClassScatterMatrix,
					ScatterMatrix &totalScatterMatrix)
{
    if (!Precursor::finalize()) return false;
    Math::Vector<Sum> totalVectorSum = getTotalVectorSum();
    Count totalCount = getTotalCount();
    if (totalCount == 0) { error("No observation has been seen."); return false; }

    // total-mean-part = sum_all * total_all^T / count_all
    Math::Matrix<Sum> totalMeanPart =
	Math::vectorInnerProduct(totalVectorSum, totalVectorSum) / (Sum)totalCount;

    // class-mean-part = sum_{all class c} sum_c * sum_c^T / count_c
    Math::Matrix<Sum> classMeanPart(featureDimension_, featureDimension_);
    for(ClassIndex classIndex = 0; classIndex < nClasses_; ++ classIndex) {
	if (counts_[classIndex] > 0) {
	    classMeanPart += Math::vectorInnerProduct(vectorSums_[classIndex], vectorSums_[classIndex]) /
		Sum(counts_[classIndex]);
	}
    }

    // Between = class-mean-part - total-mean-part
    betweenClassScatterMatrix = classMeanPart - totalMeanPart;

    // Within = vectorSquareSum - class-mean-part = Total - Between,
    // where Total = vectorSquareSum - total-mean-part
    withinClassScatterMatrix = vectorSquareSum_ - classMeanPart;

    // Total = Within + Between = vectorSquareSum - total-mean-part
    totalScatterMatrix = vectorSquareSum_ - totalMeanPart;

    if (paramShallNormalize(config)) {
	betweenClassScatterMatrix /= (Sum)totalCount;
	withinClassScatterMatrix /= (Sum)totalCount;
	totalScatterMatrix /= (Sum)totalCount;
    }
    return true;
}

Math::Vector<ScatterMatricesEstimator::Sum> ScatterMatricesEstimator::getTotalVectorSum() const
{
    Math::Vector<Sum> result(featureDimension_, Sum(0));
    for(ClassIndex classIndex = 0; classIndex < nClasses_; classIndex ++)
	result += vectorSums_[classIndex];
    return result;
}




bool ScatterMatricesEstimator::writeAccumulators()
{
    const std::string filename = paramNewAccumulatorFilename(config);
    if (!filename.empty()) {
	Core::BinaryOutputStream bos(filename);
	if (!bos) {
	    error("Failed to open \"%s\" for writing", filename.c_str());
	    return false;
	}
	if (!write(bos)) {
	    error("Failed to write the scatter accumulator to '%s'.",
		  filename.c_str());
	    return false;
	}
    }
    return true;
}

bool ScatterMatricesEstimator::writeMatrices()
{
    if (paramBetweenClassScatterFilename(config).empty() &&
	paramWithinClassScatterFilename(config).empty() &&
	paramTotalScatterFilename(config).empty())
	return true;
    ScatterMatrix betweenClassScatterMatrix;
    ScatterMatrix withinClassScatterMatrix;
    ScatterMatrix totalScatterMatrix;
    if (!finalize(betweenClassScatterMatrix, withinClassScatterMatrix, totalScatterMatrix))
	return false;
    bool success = true;
    if (!paramBetweenClassScatterFilename(config).empty())
	if (!Precursor::write(paramBetweenClassScatterFilename(config),
			      betweenClassScatterType, betweenClassScatterMatrix))
	    success = false;
    if (!paramWithinClassScatterFilename(config).empty())
	if (!Precursor::write(paramWithinClassScatterFilename(config),
			      withinClassScatterType, withinClassScatterMatrix))
	    success = false;
    if (!paramTotalScatterFilename(config).empty())
	if (!Precursor::write(paramTotalScatterFilename(config),
			      totalScatterType, totalScatterMatrix))
	    success = false;
    return success;
}

bool ScatterMatricesEstimator::read(Core::BinaryInputStream &bis)
{
    if (!Precursor::read(bis)) return false;
    u32 nClasses;
    if (!(bis >> nClasses)) return false;
    setNumberOfClasses(nClasses);
    initialize(false);
    for (size_t i = 0; i < vectorSums_.size(); ++ i) {
	Math::Vector<Sum> &row(vectorSums_[i]);
	for (size_t j = 0; j < row.size(); ++ j) {
	    if (!(bis >> row[j])) return false;
	}
    }
    for (size_t i = 0; i < counts_.size(); ++ i) {
	if (!(bis >> counts_[i])) return false;
    }
    return true;
}

bool ScatterMatricesEstimator::write(Core::BinaryOutputStream &bos)
{
    if (!Precursor::write(bos)) return false;
    if (!(bos << (u32)nClasses_)) return false;
    for (size_t i = 0; i < vectorSums_.size(); ++ i) {
	Math::Vector<Sum> &row(vectorSums_[i]);
	for (size_t j = 0; j < row.size(); ++ j) {
	    if (!(bos << row[j])) return false;
	}
    }
    for (size_t i = 0; i < counts_.size(); ++ i) {
	if (!(bos << counts_[i])) return false;
    }
    return true;
}

void ScatterMatricesEstimator::setNumberOfClasses(size_t nClasses)
{
    if (nClasses_ != nClasses) {
	nClasses_ = nClasses;
	needInit_ = true;
    }
}

bool ScatterMatricesEstimator::write()
{
    return writeAccumulators() && writeMatrices();
}

bool ScatterMatricesEstimator::load()
{
    return loadAccumulatorFile(paramOldAccumulatorFilename(config));
}

bool ScatterMatricesEstimator::loadAccumulatorFile(const std::string &filename)
{
    Core::BinaryInputStream bis(filename);
    if (!bis) {
	error("Failed to open \"%s\" for reading", filename.c_str());
	return false;
    }
    if (!read(bis)) {
	error("Failed to read the scatter accumulator from '%s'.",
	      filename.c_str());
	return false;
    }
    return true;
}

void ScatterMatricesEstimator::addAccumulatorFiles(const std::vector<std::string> &filenames) {
    if (filenames.empty())
	return;
    loadAccumulatorFile(filenames.front());
    for (std::vector<std::string>::const_iterator itFilename = filenames.begin() + 1; itFilename != filenames.end(); ++itFilename) {
	log("Add \"%s\" ...", itFilename->c_str());
	Signal::ScatterMatricesEstimator other(config);
	other.loadAccumulatorFile(*itFilename);
	accumulate(other);
    }
}
