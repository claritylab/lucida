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
#include "Statistics.hh"
#include <Core/Component.hh>
#include <Core/CompressedStream.hh>
#include <Core/BinaryStream.hh>
#include <Math/Module.hh>
#include <Math/Matrix.hh>
#include "Module.hh"

using namespace Nn;

template<typename T>
const u32 Statistics<T>::version_ = 2;

template<typename T>
Statistics<T>::Statistics(u32 nLayers, u32 statisticTypes) :
    statisticTypes_(statisticTypes),
    hasClassCounts_(statisticTypes & CLASS_COUNTS),
    hasMeanAndVariance_(statisticTypes & MEAN_AND_VARIANCE),
    hasBaseStatistics_(statisticTypes & BASE_STATISTICS),
    hasGradient_(statisticTypes & GRADIENT),
    hasNnOutput_(statisticTypes & NN_OUTPUT),
    objectiveFunction_(0),
    nClassificationErrors_(0),
    nObservations_(0),
    totalWeight_(0),
    gradientWeights_(nLayers),
    gradientBias_(nLayers),
    isComputing_(false),
    isFinalized_(false)
{
    logProperties();
    setPrecision();
}

template<typename T>
Statistics<T>::Statistics(const Statistics<T>& statistics, bool onlyStructure) :
    statisticTypes_(statistics.statisticTypes_),
    hasClassCounts_(statistics.hasClassCounts_),
    hasMeanAndVariance_(statistics.hasMeanAndVariance_),
    hasBaseStatistics_(statistics.hasBaseStatistics_),
    hasGradient_(statistics.hasGradient_),
    hasNnOutput_(statistics.hasNnOutput_),
    layerIndexToTrainableLayerIndex_(statistics.layerIndexToTrainableLayerIndex_),
    objectiveFunction_(0),
    nClassificationErrors_(0),
    nObservations_(0),
    totalWeight_(0),
    gradientWeights_(statistics.gradientWeights_.size()),
    gradientBias_(statistics.gradientBias_.size()),
    isComputing_(false),
    isFinalized_(false)
{
    setPrecision();
    // allocate mean and variance statistics
    if (hasMeanAndVariance_){
	sum_.resize(statistics.sum_.size());
	sumOfSquares_.resize(statistics.sumOfSquares_.size());
    }
    // allocate gradient
    if (hasGradient_){
	for (u32 layer = 0; layer < gradientWeights_.size(); layer++) {
	    gradientWeights_[layer].resize(statistics.gradientWeights_[layer].size());
	    for (u32 stream = 0; stream < gradientWeights_[layer].size(); stream++) {
		gradientWeights_[layer][stream].resize(
			statistics.gradientWeights_[layer][stream].nRows(),
			statistics.gradientWeights_[layer][stream].nColumns());
	    }
	}
	for (u32 layer = 0; layer < gradientBias_.size(); layer++) {
	    gradientBias_[layer].resize(statistics.gradientBias_[layer].nRows());
	}
    }
    if (hasNnOutput_){
	nnOutput_.resize(statistics.nnOutput_.size());
    }
    if (onlyStructure) {
	reset();
    }
    else {
	if (hasBaseStatistics_){
	    objectiveFunction_ = statistics.objectiveFunction_;
	    nClassificationErrors_ = statistics.nClassificationErrors_;
	    nObservations_ = statistics.nObservations_;
	    totalWeight_ = statistics.totalWeight_;
	}
	if (hasMeanAndVariance_){
	    initComputation(false);
	    sum_.copy(statistics.sum_);
	    sumOfSquares_.copy(statistics.sumOfSquares_);
	}
	if (hasGradient_) {
	    initComputation(false);
	    for (u32 layer = 0; layer < gradientWeights_.size(); layer++) {
		for (u32 stream = 0; stream < gradientWeights_[layer].size(); stream++) {
		    gradientWeights_[layer][stream].copy(statistics.gradientWeights_[layer][stream]);
		}
	    }
	    for (u32 layer = 0; layer < gradientBias_.size(); layer++) {
		gradientBias_[layer].copy(statistics.gradientBias_[layer]);
	    }
	}
	if (hasNnOutput_){
	    nnOutput_.copy(statistics.nnOutput_);
	}
    }
    logProperties();
}

template<typename T>
void Statistics<T>::reset() {
    if (hasClassCounts_) {
	classCount_.clear();
    }
    if (hasBaseStatistics_) {
	objectiveFunction_ = 0;
	nClassificationErrors_ = 0;
	nObservations_ = 0;
	totalWeight_ = 0;
    }
    if (hasMeanAndVariance_){
	sum_.setToZero();
	sumOfSquares_.setToZero();
    }
    if (hasGradient_) {
	for (u32 layer = 0; layer < gradientWeights_.size(); layer++) {
	    for (u32 stream = 0; stream < gradientWeights_[layer].size(); stream++) {
		gradientWeights_[layer][stream].setToZero();
	    }
	}
	for (u32 layer = 0; layer < gradientBias_.size(); layer++) {
	    gradientBias_[layer].setToZero();
	}
    }
    if (hasNnOutput_)
	nnOutput_.setToZero();
    isFinalized_ = false;
}

template<typename T>
void Statistics<T>::finalize(bool normalizeByNOfObservations) {
    if (!isFinalized_){
	T normalizationWeight = normalizeByNOfObservations ? nObservations_ : 1.0;
	objectiveFunction_ *= 1.0 / normalizationWeight;
	if (hasGradient_) {
	    for (u32 layer = 0; layer < gradientWeights_.size(); layer++) {
		for (u32 stream = 0; stream < gradientWeights_[layer].size(); stream++) {
		    gradientWeights_[layer][stream].scale(1.0 / normalizationWeight);
		}
	    }
	    for (u32 layer = 0; layer < gradientBias_.size(); layer++) {
		gradientBias_[layer].scale(1.0 / normalizationWeight);
	    }
	}
	if (hasMeanAndVariance_){
	    sum_.scale(1.0 / totalWeight_);
	    NnVector tmp(sum_.size());
	    tmp.initComputation(false);
	    tmp.copy(sum_);
	    tmp.elementwiseMultiplication(sum_);
	    sumOfSquares_.scale(1.0 / totalWeight_);
	    sumOfSquares_.add(tmp, (T)-1.0);
	}
	if (hasNnOutput_)
	    nnOutput_.scale(1.0 / normalizationWeight);
	isFinalized_ = true;
    }
}

template<typename T>
void Statistics<T>::incClassCount(u32 label, u32 increment) {
    if (classCount_.find(label) == classCount_.end()) {
	classCount_[label] = 0;
    }
    classCount_[label] += increment;
}

template<typename T>
u32 Statistics<T>::classCount(u32 label) const {
    std::map<u32,u32>::const_iterator it = classCount_.find(label);
    if (it == classCount_.end())
	return 0;
    else
	return it->second;
}

template<typename T>
T Statistics<T>::gradientL1Norm() const {
    T result = 0;
    for (typename std::vector<std::vector<NnMatrix> >::const_iterator lit = gradientWeights_.begin(); lit != gradientWeights_.end(); ++lit){
	for (typename std::vector<NnMatrix >::const_iterator sit = lit->begin(); sit != lit->end(); ++sit){
	    result += sit->l1norm();
	}
    }
    for (typename std::vector<NnVector >::const_iterator it = gradientBias_.begin(); it != gradientBias_.end(); ++it)
	result += it->l1norm();
    return result;
}

template<typename T>
void Statistics<T>::logProperties() const {
    Core::Application::us()->log("creating statistics object");
    if (hasClassCounts_)
	Core::Application::us()->log("statistics object has class counts");
    if (hasMeanAndVariance_)
	Core::Application::us()->log("statistics object has mean and variance statistics");
    if (hasBaseStatistics_)
	Core::Application::us()->log("statistics object has base statistics");
    if (hasGradient_)
	Core::Application::us()->log("statistics object has gradient");
    if (hasNnOutput_)
	Core::Application::us()->log("statistics object has neural network output");

}


template<typename T>
void Statistics<T>::initComputation(bool sync) {
    if (!isComputing_) {
	if (hasMeanAndVariance_){
	    sum_.initComputation(sync);
	    sumOfSquares_.initComputation(sync);
	}
	if (hasGradient_) {
	    for (u32 i = 0; i < gradientWeights_.size(); i++) {
		for (u32 s = 0; s < gradientWeights_[i].size(); s++) {
		    if (!gradientWeights_[i][s].isComputing()) {
			gradientWeights_[i][s].initComputation(sync);
		    }
		}
		if (!gradientBias_[i].isComputing())
		    gradientBias_[i].initComputation(sync);
	    }
	}
	if (hasNnOutput_)
	    nnOutput_.initComputation(sync);
    }
    isComputing_ = true;
}

template<typename T>
void Statistics<T>::finishComputation(bool sync) {
    if (isComputing_) {
	if (hasMeanAndVariance_){
	    sum_.finishComputation(sync);
	    sumOfSquares_.finishComputation(sync);
	}
	if (hasGradient_) {
	    for (u32 i = 0; i < gradientWeights_.size(); i++) {
		for (u32 s = 0; s < gradientWeights_[i].size(); s++) {
		    if (gradientWeights_[i][s].isComputing())
			gradientWeights_[i][s].finishComputation(sync);
		}
		if (gradientBias_[i].isComputing())
		    gradientBias_[i].finishComputation(sync);
	    }
	}
	if (hasNnOutput_)
	    nnOutput_.finishComputation(sync);
    }
    isComputing_ = false;
}

template<typename T>
bool Statistics<T>::read(const std::string &filename) {
    bool result = Module::instance().formats().read(filename, *this);
    if (result) {
	Core::Application::us()->log("statistics read from \"") << filename << "\"";
    } else {
	Core::Application::us()->error("failed to read statistics from \"") << filename << "\"";
    }
    return result;
}

template<typename T>
bool Statistics<T>::read(Core::BinaryInputStream &is){
    require(!isComputing_);
    const std::string magic = readHeader(is);
    if (!isValid(magic)) {
	Core::Application::us()->criticalError("invalid magic");
	return false;
    }
    is >> nObservations_;
    is >> totalWeight_;
    // read base statistics
    if (hasBaseStatistics_) {
	is >> nClassificationErrors_;
	is >> objectiveFunction_;
    }
    // read class counts
    if (hasClassCounts_) {
	u32 size;
	is >> size;
	for (u32 i = 0; i < size; i++) {
	    u32 c,n;
	    is >> c;
	    is >> n;
	    classCount_[c] = n;
	}
    }
    if (hasMeanAndVariance_){
	Math::Vector<T> vector;
	vector.read(is);
	sum_.resize(vector.size(), 0, true);
	sum_.copy(vector);
	vector.read(is);
	sumOfSquares_.resize(vector.size(), 0, true);
	sumOfSquares_.copy(vector);
    }
    // read gradient
    if (hasGradient_) {
	// TODO remove dependency on old matrices
	// required for IO
	Math::Matrix<T> matrix;
	Math::Vector<T> vector;

	u32 size;
	is >> size;
	require_eq(size,gradientWeights_.size());
	for (u32 l = 0; l < gradientWeights_.size(); l++) {
	    is >> size;
	    require_eq(size,gradientWeights_[l].size());
	    for (u32 s = 0; s < gradientWeights_[l].size(); s++){
		matrix.read(is);
		gradientWeights_[l][s].copy(matrix);
	    }
	    vector.read(is);
	    gradientBias_[l].copy(vector);
	}
    }
    return true;
}

template<typename T>
bool Statistics<T>::write(const std::string &filename) const
{
    bool result = Module::instance().formats().write(filename, *this);
    if (result) {
	Core::Application::us()->log("statistics written to \"") << filename << "\"";
    } else {
	Core::Application::us()->error("failed to write statistics to \"") << filename << "\"";
    }
    return result;
}



template<typename T>
bool Statistics<T>::write(Core::BinaryOutputStream &os) const {
    require(!isComputing_);
    writeHeader(os);
    os << nObservations_;
    os << totalWeight_;
    // save base statistics
    if (hasBaseStatistics_) {
	os << nClassificationErrors_;
	os << objectiveFunction_;
    }
    // save class counts
    if (hasClassCounts_) {
	os << (u32) classCount_.size();
	for (std::map<u32,u32>::const_iterator it=classCount_.begin(); it!=classCount_.end(); ++it) {
	    os << it->first << it->second;
	}
    }
    // save mean and variance statistics
    if (hasMeanAndVariance_){
	u32 dim = sum_.size();
	Math::Vector<T> vector(dim);
	Math::copy(dim, sum_.begin(), 1, &vector.at(0), 1);
	vector.write(os);
	Math::copy(dim, sumOfSquares_.begin(), 1, &vector.at(0), 1);
	vector.write(os);
    }
    // save gradient
    if (hasGradient_) {
	// TODO remove dependency on old matrices
	// required for IO
	Math::Matrix<T> matrix;
	Math::Vector<T> vector;
	os << (u32) gradientWeights_.size();
	for (u32 l = 0; l < gradientWeights_.size(); l++) {
	    os << (u32) gradientWeights_[l].size();
	    for (u32 s = 0; s < gradientWeights_[l].size(); s++){
		gradientWeights_[l][s].convert(matrix);
		matrix.write(os);
	    }
	    gradientBias_[l].convert(vector);
	    vector.write(os);
	}
    }
    return true;
}

template<typename T>
bool Statistics<T>::write(std::ostream &os) const {
    require(!isComputing_);
    os << "#observations: " << nObservations_ << std::endl;
    os << "total weight: " << totalWeight_ << std::endl;
    // show base statistics
    if (hasBaseStatistics_) {
	os << "#classification errors: " << nClassificationErrors_ << std::endl;
	os << "objective-function: " << objectiveFunction_ << std::endl;
    }
    // show class counts
    if (hasClassCounts_) {
	std::cout << "class counts: " << std::endl;
	for (std::map<u32,u32>::const_iterator it=classCount_.begin(); it!=classCount_.end(); ++it) {
	    os << it->first << ": " << it->second << std::endl;
	}
    }
    // show mean and variance statistics
    if (hasMeanAndVariance_){
	std::cout << "sum features: " << std::endl;
	Math::Vector<T> vector(sum_.size());
	Math::copy(sum_.size(), sum_.begin(), 1, &vector.at(0), 1);
	vector.print(os);
	std::cout << "sum squared features: " << std::endl;
	Math::copy(sum_.size(), sumOfSquares_.begin(), 1, &vector.at(0), 1);
	vector.print(os);
    }
    // show gradient
    if (hasGradient_) {
	// TODO remove dependency on old matrices
	// required for IO
	Math::Matrix<T> matrix;
	Math::Vector<T> vector;
	for (u32 l = 0; l < gradientWeights_.size(); l++) {
	    for (u32 s = 0; s < gradientWeights_[l].size(); s++){
		std::cout << "gradient weight matrix of layer: " << l << " stream: " << s << std::endl;
		gradientWeights_[l][s].convert(matrix);
		matrix.print(os);
	    }
	    std::cout << "gradient bias of layer: " << l << std::endl;
	    gradientBias_[l].convert(vector);
	    vector.print(os);
	}
    }
    // save gradient
    return true;
}

template<typename T>
bool Statistics<T>::write(Core::XmlWriter &o) const
{
    o << Core::XmlOpen("statistics")
    + Core::XmlAttribute("number-of-observations", nObservations_)
    + Core::XmlAttribute("total-weight", totalWeight_)
    + Core::XmlAttribute("number-of-classification-errors", nClassificationErrors_)
    + Core::XmlAttribute("objective-function", objectiveFunction_);

    if (hasClassCounts_) {
	o << Core::XmlOpen("class-counts");
	for (std::map<u32,u32>::const_iterator it=classCount_.begin(); it!=classCount_.end(); ++it) {
	    std::ostringstream ss;
	    ss << it->first;
	    o << Core::XmlOpen("class-count") + Core::XmlAttribute(ss.str(), it->second);
	    o << Core::XmlClose("class-count");
	}
	o << Core::XmlClose("class-counts");
    }
    // save mean and variance statistics
    if (hasMeanAndVariance_){
	Math::Vector<T> vector;
	Math::copy(sum_.size(), sum_.begin(), 1, &vector.at(0), 1);
	o << Core::XmlOpen("sum");
	o << vector;
	o << Core::XmlClose("linear-sum");
	Math::copy(sum_.size(), sumOfSquares_.begin(), 1, &vector.at(0), 1);
	o << Core::XmlOpen("sum-of-squares");
	o << vector;
	o << Core::XmlClose("sum-of-squares");
    }
    if (hasGradient_) {
	o << Core::XmlOpen("gradient");
	for (u32 l = 0; l < gradientWeights_.size(); l++) {
	    // convert gradient to Math::Matrix
	    u32 totalWeightRows = 0;
	    for (u32 s = 0; s < gradientWeights_[l].size(); s++) {
		totalWeightRows += gradientWeights_[l][s].nRows();
	    }
	    Math::Matrix<T> gradient(gradientBias_[l].nRows(), totalWeightRows + 1);
	    for (u32 i = 0; i < gradientBias_[l].nRows(); i++) {
		gradient[i][0] = gradientBias_[l].at(i);
		u32 k = 1;
		for (u32 s = 0; s < gradientWeights_[l].size(); s++) {
		    for (u32 j = 0; j < gradientWeights_[l][s].nRows(); j++) {
			gradient[i][k] = gradientWeights_[l][s].at(j, i);
			k++;
		    }
		}
	    }
	    // write matrix
	    o << gradient;
	}
	o << Core::XmlClose("gradient");
    }
    o << Core::XmlClose("statistics");
    return true;
}



template<typename T>
bool Statistics<T>::combine(const std::vector<std::string> &toCombine)
{
    if (toCombine.size() == 0){
	Core::Application::us()->error("no statistics filenames given for combination");
	return false;
    }
    reset();
    if (!read(toCombine.at(0)))
	return false;
    initComputation();

    Statistics<T> tmp(*this, true);
    for (u32 i = 1; i < toCombine.size(); i++){
	if (!tmp.read(toCombine.at(i))){
	    return false;
	}
	tmp.initComputation();
	add(tmp);
	tmp.finishComputation();
    }
    finishComputation();
    return true;
}

template<typename T>
bool Statistics<T>::getTypeFromFile(const std::string &filename, u32 &statisticsType, bool &singlePrecision) {
    Core::CompressedInputStream cis(filename);
    Core::BinaryInputStream is(cis);
    char magic[17];
    is.read(magic, 16);
    magic[16] = '\0';
    bool result = is.good();
    if (!result){
	is.close();
	cis.close();
	return false;
    }
    is.close();
    cis.close();
    statisticsType = 0;
    if (hasClassCounts(magic))
	statisticsType |= 1;
    if (hasMeanAndVariance(magic))
	statisticsType |= 2;
    if (hasBaseStatistics(magic))
	statisticsType |= 4;
    if (hasGradient(magic))
	statisticsType |= 8;
    singlePrecision = hasSinglePrecision(magic);
    return result;
}


template<typename T>
const std::string Statistics<T>::readHeader(Core::BinaryInputStream &i)
{
    char magic[17];
    i.read(magic, 16);
    magic[16] = '\0';
    if (!isConsistent(magic)) {
	Core::Application::us()->criticalError("inconsistent magic");
	return "INVALID";
    }
    u32 version;
    i >> version;
    if (version != version_) {
	Core::Application::us()->criticalError("version must be \"%d\" and not \"%d\"", version_, version);
	return "INVALID";
    }
    return magic;
}

template<typename T>
bool Statistics<T>::writeHeader(Core::BinaryOutputStream &o) const
{
    o.write(getMagic().c_str(), 16);
    o << version_;
    return true;
}


template<typename T>
const std::string Statistics<T>::getMagic() const
{
    std::string magic("NNSTAT");
    magic += hasClassCounts_ ? 'C' : '#';	//position 6 of 16
    magic += hasMeanAndVariance_? 'M' : '#';	//position 7 of 16
    magic += hasBaseStatistics_ ? 'B' : '#';	//position 8 of 16
    magic += hasGradient_ ? 'G' : '#';		//position 9 of 16
    magic += singlePrecision_ ? 'S' : 'D'; 	//position 10 of 16
    magic += hasNnOutput_ ? 'N' : '#';		//position 11 of 16
    magic += '#';				//position 12 of 16
    magic += '#';				//position 13 of 16
    magic += '#';				//position 14 of 16
    magic += '#';				//position 15 of 16
    return magic;
}

template<typename T>
bool Statistics<T>::isValid(const std::string &magic)
{
    return magic != "INVALID";
}

template<typename T>
bool Statistics<T>::checkConsistency(bool object, bool file, std::string description){
    if (object && !file){
	Core::Application::us()->error("statistics object requires ") << description;
	return false;
    }
    else if (!object && file){
	Core::Application::us()->error("statistics object does not require ") << description;
	return false;
    }
    return true;
}

template<typename T>
bool Statistics<T>::isConsistent(const std::string &magic) const
{
    bool result = true;
    result |= checkConsistency(hasClassCounts_, hasClassCounts(magic), "class counts");
    result |= checkConsistency(hasMeanAndVariance_ , hasMeanAndVariance(magic), "mean and variance");
    result |= checkConsistency(hasBaseStatistics_, hasBaseStatistics(magic), "base statistics");
    result |= checkConsistency(hasGradient_, hasGradient(magic), "gradient");
    if (singlePrecision_ != hasSinglePrecision(magic)) {
	Core::Application::us()->criticalError("statistics: single/double precision mismatch");
	return false;
    }
    return result;
}

template<typename T>
bool Statistics<T>::hasClassCounts(const std::string &magic)
{
    return isValid(magic) and magic.substr(6,1) == "C";
}

template<typename T>
bool Statistics<T>::hasMeanAndVariance(const std::string &magic)
{
    return isValid(magic) and magic.substr(7,1) == "M";
}

template<typename T>
bool Statistics<T>::hasBaseStatistics(const std::string &magic)
{
    return isValid(magic) and magic.substr(8,1) == "B";
}

template<typename T>
bool Statistics<T>::hasGradient(const std::string &magic)
{
    return isValid(magic) and magic.substr(9,1) == "G";
}

template<typename T>
bool Statistics<T>::hasSinglePrecision(const std::string &magic)
{
    return isValid(magic) and magic.substr(10,1) == "S";
}

//=============================================================================
// explicit template instantiation
namespace Nn {
template class Statistics<f32>;
template class Statistics<f64>;
}
