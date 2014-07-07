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
#ifndef _NN_NEURAL_NETWORK_STATISTICS_HH
#define _NN_NEURAL_NETWORK_STATISTICS_HH

#include <Core/XmlStream.hh>

#include <vector>
#include <map>
#include <cstring>

#include "NeuralNetwork.hh"
#include "Types.hh"

namespace Nn {

/*
 *  Statistics container for neural network training
 *
 *  Plans: avoid initialization from network when loading from file
 */

template <class T>
class Statistics {
    friend class Statistics<f32>;
    friend class Statistics<f64>;
protected:
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;

public:
    /* defines which statistics are accumulated */
    // set values to 1,2,4,...
    enum StatisticTypes {
	NONE = 0,
	CLASS_COUNTS = 1,
	MEAN_AND_VARIANCE = 2,
	BASE_STATISTICS = 4,
	GRADIENT = 8,
	NN_OUTPUT = 16
    };
protected:
    static const u32 version_;
    // statistic types
    u32 statisticTypes_;
    bool hasClassCounts_;
    bool hasMeanAndVariance_;
    bool hasBaseStatistics_;
    bool hasGradient_;
    bool hasNnOutput_;
    bool singlePrecision_;

    std::vector<s32> layerIndexToTrainableLayerIndex_;
    // base statistics
    T objectiveFunction_;
    u32 nClassificationErrors_;
    // observation count
    u32 nObservations_;
    // sum of accumulation weights
    T totalWeight_;
    // mapping from class label to class count
    std::map<u32,u32> classCount_;
    // feature statistics
    NnVector sum_;
    NnVector sumOfSquares_;
    // gradient of weights for each layer (gradientWeights[layer][stream])
    std::vector< std::vector<NnMatrix> > gradientWeights_;
    // gradient of bias for each layer
    std::vector<NnVector> gradientBias_;
    // NN output
    NnVector nnOutput_;
    // is in computing state (GPU)
    bool isComputing_;
    // is finalized (e.g. normalized by number of observations)
    bool isFinalized_;
public:
    Statistics(u32 nTrainableLayers, u32 statisticTypes);
    Statistics(const Statistics<T>& statistics, bool onlyStructure = false);
    virtual ~Statistics() {}
    /** resets the statistics to 0, but keeps sizes of matrices */
    void reset();
    /** transformation to a minimization problem and normalization */
    void finalize(bool normalizeByNOfObservations = true);
    void incClassificationErrors(u32 increment = 1) { nClassificationErrors_ += increment; }
    void incObservations(u32 increment = 1) { nObservations_ += increment; }
    void addToTotalWeight(T weight) { totalWeight_ += weight; }
    void incClassCount(u32 label, u32 increment = 1);
    void addToObjectiveFunction(T value) { objectiveFunction_ += value; }
    T objectiveFunction() const { return objectiveFunction_; }
    T classificationError() const { return (T)nClassificationErrors_ / nObservations_; }
    u32 nObservations() const { return nObservations_; }
    T totalWeight() const { return totalWeight_; }
    u32 classCount(u32 label) const;

    // feature sum and squared feature sum
    NnVector& featureSum() { return sum_; }
    NnVector& squaredFeatureSum() { return sumOfSquares_; }

    /** return the gradient of a certain layer*/
    //assumption: layer is trainable
    std::vector<NnMatrix>& gradientWeights(u32 layer) { return gradientWeights_.at(layerIndexToTrainableLayerIndex_.at(layer)); }
    NnVector& gradientBias(u32 layer) { return gradientBias_.at(layerIndexToTrainableLayerIndex_.at(layer)); }
    /** computes the l1 norm of the gradient*/
    T gradientL1Norm() const;

    // neural network output
    NnVector& nnOutput() { return nnOutput_; }

    bool hasClassCounts() const { return hasClassCounts_; }
    bool hasBaseStatistics() const { return hasBaseStatistics_; }
    bool hasGradient() const { return hasGradient_; }
    bool hasNnOutput() const { return hasNnOutput_; }

    /** initialize statistics from neural network */
    template<typename S>
    void initialize(const NeuralNetwork<S> &neuralNetwork);

    // synchronization
    void initComputation(bool sync = true);
    void finishComputation(bool sync = true);
    bool isComputing() const { return isComputing_; }

    // this += rhs
    template<typename S>
    void add(const Statistics<S>& rhs);

    // copy structure
    template<typename S>
    void copyStructure(const Statistics<S> &rhs);

    // Statistics IO
public:
    bool read(const std::string &filename);
    bool read(Core::BinaryInputStream &is);
    bool write(const std::string &filename) const;
    bool write(Core::BinaryOutputStream &os) const;
    bool write(std::ostream &os) const;
    bool write(Core::XmlWriter &o) const;
    bool combine(const std::vector<std::string> &toCombine);
    static bool getTypeFromFile(const std::string &filename, u32 &statisticsType, bool &singlePrecision);
private:
    void logProperties() const;
    void setPrecision();
private:
    const std::string readHeader(Core::BinaryInputStream &);
    bool writeHeader(Core::BinaryOutputStream &) const;
    const std::string getMagic() const;
    bool isConsistent(const std::string &magic) const;
    static bool checkConsistency(bool object, bool file, std::string description);
    static bool isValid(const std::string &magic);
    static bool hasBaseStatistics(const std::string &magic);
    static bool hasMeanAndVariance(const std::string &magic);
    static bool hasClassCounts(const std::string &magic);
    static bool hasGradient(const std::string &magic);
    static bool hasSinglePrecision(const std::string &magic);

};

template<typename T>
template<typename S>
void Statistics<T>::initialize(const NeuralNetwork<S> &neuralNetwork) {

    layerIndexToTrainableLayerIndex_.resize(neuralNetwork.nLayers());
    for (u32 layer = 0; layer < neuralNetwork.nLayers(); layer++)
	layerIndexToTrainableLayerIndex_[layer] = neuralNetwork.layerIndexToTrainableLayerIndex(layer);

    for (u32 layer = 0; layer < neuralNetwork.nLayers(); layer++) {
	/* store which layer is trainable */
	if (neuralNetwork.getLayer(layer).isTrainable()) {
	    s32 trainableLayerIndex = layerIndexToTrainableLayerIndex_.at(layer);
	    // initialize weights/bias gradient with the corresponding size
	    if (hasGradient_) {
		// different layer types have different structures of the
		// trainable parameters, so let them decide on the structure
		// of the gradient accumulators
		gradientWeights_[trainableLayerIndex].resize(neuralNetwork.getLayer(layer).nInputActivations());
		for (u32 stream = 0; stream < neuralNetwork.getLayer(layer).nInputActivations(); stream++) {
		    neuralNetwork.getLayer(layer).resizeWeightsGradient(gradientWeights_.at(trainableLayerIndex).at(stream), stream);
		}
		neuralNetwork.getLayer(layer).resizeBiasGradient(gradientBias_.at(trainableLayerIndex));
	    }
	}
    }
    if (hasGradient_) {
	gradientWeights_.resize(neuralNetwork.nTrainableLayers());
	gradientBias_.resize(neuralNetwork.nTrainableLayers());
    }
    if (hasNnOutput_)
	nnOutput_.resize(neuralNetwork.getTopLayer().getOutputDimension());
}

template<>
inline void Statistics<f32>::setPrecision(){
    singlePrecision_ = true;
}

template<>
inline void Statistics<f64>::setPrecision(){
    singlePrecision_ = false;
}

template<typename T>
template<typename S>
void Statistics<T>::add(const Statistics<S> &rhs){
    require(isComputing_);
    require(rhs.isComputing_);
    require_eq(hasBaseStatistics_, rhs.hasBaseStatistics_);
    require_eq(hasMeanAndVariance_, rhs.hasMeanAndVariance_);
    require_eq(hasGradient_, rhs.hasGradient_);
    require_eq(hasClassCounts_, rhs.hasClassCounts_);

    nObservations_ += rhs.nObservations_;
    totalWeight_ += rhs.totalWeight_;
    // add base statistics
    if (hasBaseStatistics_) {
	nClassificationErrors_ += rhs.nClassificationErrors_;
	objectiveFunction_ += rhs.objectiveFunction_;
    }
    // add mean and variance
    if (hasMeanAndVariance_){
	sum_.add(rhs.sum_);
	sumOfSquares_.add(rhs.sumOfSquares_);
    }
    // add class counts
    if (hasClassCounts_) {
	for (std::map<u32,u32>::const_iterator it = rhs.classCount_.begin(); it != rhs.classCount_.end() ; it++) {
	    incClassCount(it->first, it->second);
	}
    }
    // add gradient
    if (hasGradient_) {
	require_eq(gradientWeights_.size(), rhs.gradientWeights_.size());
	for (u32 l = 0; l < gradientWeights_.size(); l++) {
	    require_eq(gradientWeights_[l].size(), rhs.gradientWeights_[l].size());
	    for (u32 s = 0; s < gradientWeights_[l].size(); s++) {
		gradientWeights_[l][s].add(rhs.gradientWeights_[l][s]);
	    }
	    gradientBias_[l].add(rhs.gradientBias_[l]);
	}
    }
}

template<typename T>
template<typename S>
void Statistics<T>::copyStructure(const Statistics<S> &statistics){

    hasClassCounts_ = statistics.hasClassCounts_;
    hasBaseStatistics_ = statistics.hasBaseStatistics_;
    hasGradient_ = statistics.hasGradient_;
    gradientWeights_.resize(statistics.gradientWeights_.size());
    gradientBias_.resize(statistics.gradientBias_.size());
    layerIndexToTrainableLayerIndex_ = statistics.layerIndexToTrainableLayerIndex_;
    statisticTypes_ = statistics.statisticTypes_;

    if (hasMeanAndVariance_){
	sum_.resize(statistics.sum_.size());
	sumOfSquares_.resize(statistics.sumOfSquares_.size());
    }
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

    reset();

}

template<typename T>
Core::BinaryInputStream& operator>>(Core::BinaryInputStream &i, Statistics<T> &s){
    if (not s.read(i)) i.addState(std::ios::badbit); return i;
}

template<typename T>
Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream &o, const Statistics<T> &s){
    if (not s.write(o)) o.addState(std::ios::badbit); return o;
}


} // namespace Nn

namespace Core {

    template <>
    class NameHelper<Nn::Statistics<f32> > : public std::string {
    public:
	NameHelper() : std::string("nn-statistics-f32") {}
    };

    template <>
    class NameHelper<Nn::Statistics<f64> > : public std::string {
    public:
	NameHelper() : std::string("nn-statistics-f64") {}
    };


} //namespace Core




#endif // _NN_NEURAL_NETWORK_STATISTICS_HH
