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
#include <Core/Assertions.hh>

#include "Normalization.hh"

using namespace Signal;
using namespace Core;
using namespace Flow;

//===================================================================================================
Normalization::Normalization() :
    length_(0),
    right_(0),
    sumWeight_(0),
    changed_(true) {
}

bool Normalization::init(size_t length, size_t right, size_t dimension) {
    if (slidingWindow_.init(length, right)) {
	if (init(dimension)) {
	    reset();
	    return true;
	}
    }
    return false;
}

void Normalization::reset() {
    sumWeight_ = 0.0;
    changed_ = true;
    slidingWindow_.clear();
}

bool Normalization::update(const Frame &in, Frame &out) {
    if (in) {
	slidingWindow_.add(in);

	Frame removed;
	slidingWindow_.removed(removed);

	updateStatistics(in, removed);

    } else
	slidingWindow_.flushOut();

    out = Frame();
    if (slidingWindow_.out(out)) {
	out.makePrivate();
	normalize(out);
	return true;
    }

    return false;
}

bool Normalization::flush(Frame &out) {
    return update(Frame(), out);
}

void Normalization::updateStatistics(const Frame &add, const Frame &remove) {
    if (add) {
	sumWeight_ ++;
	changed_ = true;
    }

    if (remove) {
	sumWeight_ --;
	changed_ = true;
    }
}

void Normalization::finalize() {
    verify(changed_);
    changed_ = false;
}

void Normalization::normalize(Frame &out) {
    require(out);

    if (changed_)
	finalize();

    apply(out);
}

//===================================================================================================
void LevelNormalization::finalize() {
    max_ = Type<Value>::min;
    for(u32 i = 0; i < slidingWindow_.size(); i++)
	max_ = std::max((*slidingWindow_[i])[index_], max_);
    Precursor::finalize();
}

void LevelNormalization::apply(Frame &out) {
    (*out)[index_] -= max_;
}

//===================================================================================================
bool MeanNormalization::init(size_t dimension) {
    mean_.resize(dimension);
    sum_.resize(dimension);
    return Precursor::init(dimension);
}

void MeanNormalization::reset() {
    std::fill(sum_.begin(), sum_.end(), (Value)0);
    Precursor::reset();
}

void MeanNormalization::updateStatistics(const Frame &add, const Frame &remove) {
    if (add)
	std::transform(sum_.begin(), sum_.end(), add->begin(), sum_.begin(), std::plus<Sum>());
    if (remove)
	std::transform(sum_.begin(), sum_.end(), remove->begin(), sum_.begin(), std::minus<Sum>());
    Precursor::updateStatistics(add, remove);
}

void MeanNormalization::finalize() {
    if (sumWeight_ > 0) {
	std::transform(sum_.begin(), sum_.end(), mean_.begin(),
		       std::bind2nd(std::divides<Sum>(), sumWeight_));
    }
    Precursor::finalize();
}

void MeanNormalization::apply(Frame &out) {
    std::transform(out->begin(), out->end(), mean_.begin(), out->begin(), std::minus<Value>());
}

//===================================================================================================
MeanAndVarianceNormalization::MeanAndVarianceNormalization(const Core::Configuration &c) :
    Core::Component(c) {
}

bool MeanAndVarianceNormalization::init(size_t dimension) {
    standardDeviation_.resize(dimension);
    sumSquare_.resize(dimension);
    return Precursor::init(dimension);
}

void MeanAndVarianceNormalization::reset() {
    std::fill(sumSquare_.begin(), sumSquare_.end(), (Value)0);
    Precursor::reset();
}

void MeanAndVarianceNormalization::updateStatistics(const Frame &add, const Frame &remove) {
    if (add) {
	for(size_t d = 0; d < add->size(); d++)
	    sumSquare_[d] += (Sum)(*add)[d] * (Sum)(*add)[d];
    }

    if (remove) {
	for(size_t d = 0; d < add->size(); d++)
	    sumSquare_[d] -= (Sum)(*remove)[d] * (Sum)(*remove)[d];
    }
    Precursor::updateStatistics(add, remove);
}

void MeanAndVarianceNormalization::finalize() {
    if (sumWeight_ > 0) {
	for(size_t d = 0; d < standardDeviation_.size(); d++) {
	    standardDeviation_[d] =
		(Value)sqrt( (sumSquare_[d] - sum_[d] * sum_[d] / sumWeight_) / sumWeight_);
	    if(standardDeviation_[d] == 0) {
		// Due to mean normalization, the final value will be zero anyway, so we need no variance normalization
		standardDeviation_[d] = 1.0;
		warning("standard deviation is zero.");
	    }else if (standardDeviation_[d] < 0)
		criticalError("standard deviation is smaller than zero.");
	}
    }
    Precursor::finalize();
}

void MeanAndVarianceNormalization::apply(Frame &out) {
    Precursor::apply(out);
    std::transform(out->begin(), out->end(), standardDeviation_.begin(),
		   out->begin(), std::divides<Value>());
}

//===================================================================================================
MeanAndVarianceNormalization1D::MeanAndVarianceNormalization1D(const Core::Configuration &c) :
    Core::Component(c) {
}

bool MeanAndVarianceNormalization1D::init(size_t dimension) {
    standardDeviation_ = 0;
    sumSquare_ = 0;
    mean_ = 0;
    sum_ = 0;
    return Precursor::init(dimension);
}

void MeanAndVarianceNormalization1D::reset() {
    sumSquare_ = 0;
    sum_ = 0;
    Precursor::reset();
}

void MeanAndVarianceNormalization1D::updateStatistics(const Frame &add, const Frame &remove) {
    if (add) {
	for(size_t d = 0; d < add->size(); d++) {
	    sumSquare_ += (Sum)(*add)[d] * (Sum)(*add)[d];
	    sum_       += (Sum)(*add)[d];
	}
	changed_ = true;
	sumWeight_ += add->size();
    }

    if (remove) {
	for(size_t d = 0; d < add->size(); d++) {
	    sumSquare_ -= (Sum)(*remove)[d] * (Sum)(*remove)[d];
	    sum_       -= (Sum)(*remove)[d];
	}
	changed_ = true;
	sumWeight_ -= remove->size();
    }
}

void MeanAndVarianceNormalization1D::finalize() {
    if (sumWeight_ > 0) {
	standardDeviation_ = (Value)sqrt( (sumSquare_ - sum_ * sum_ / sumWeight_) / sumWeight_);
	mean_ = (Value) sum_ / sumWeight_;
	//std::cout << "mvn finalize with mean == " << mean_ << " and std == " << standardDeviation_ << std::endl;
	if(standardDeviation_ == 0) {
	    standardDeviation_ = 1.0;
	    warning("standard deviation is zero.");
	}
    }
    Precursor::finalize();
}

void MeanAndVarianceNormalization1D::apply(Frame &out) {
    //std::cout << "1D mvn applying to input of dimension " << out->size() << std::endl;
    for(size_t d = 0; d < out->size(); d++)
	out->at(d) = (out->at(d) - mean_) / standardDeviation_;
}

//===================================================================================================
DivideByMean::DivideByMean(const Core::Configuration &c) :
    Core::Component(c) {
}

void DivideByMean::finalize() {
    Precursor::finalize();
    if (std::find(mean_.begin(), mean_.end(), (Value)0) != mean_.end())
	criticalError("One of the mean components is zero.");
}

void DivideByMean::apply(Frame &out) {
    std::transform(out->begin(), out->end(), mean_.begin(), out->begin(), std::divides<Value>());
}

//===================================================================================================
MeanNormNormalization::MeanNormNormalization(const Core::Configuration &c, f32 norm) :
    Core::Component(c), sumOfNorms_(0), averageNorm_(0), norm_(norm), normFunction_(c) {
}

void MeanNormNormalization::reset() {
    sumOfNorms_ = 0;
    Precursor::reset();
}

void MeanNormNormalization::updateStatistics(const Frame &add, const Frame &remove) {
    if (add)
	sumOfNorms_ += normFunction_.apply(*add, norm_);
    if (remove)
	sumOfNorms_ -= normFunction_.apply(*remove, norm_);
    Precursor::updateStatistics(add, remove);
}

void MeanNormNormalization::finalize() {
    averageNorm_ = sumOfNorms_ / sumWeight_;
    if (averageNorm_ == 0) criticalError("Average norm became zero.");
    Precursor::finalize();
}

void MeanNormNormalization::apply(Frame &out) {
    std::transform(out->begin(), out->end(), out->begin(),
		   std::bind2nd(std::divides<Value>(), averageNorm_));
}

//===================================================================================================
const Choice NormalizationNode::typeChoice(
    "level", typeLevel,
    "mean", typeMean,
    "mean-and-variance",  typeMeanAndVariance,
    "mean-and-variance-1D",  typeMeanAndVariance1D,
    "divide-by-mean", typeDivideByMean,
    "mean-norm", typeMeanNorm,
    Choice::endMark());
const ParameterChoice  NormalizationNode::paramType(
    "type", &typeChoice, "type of normalization", typeMean);

const ParameterInt NormalizationNode::paramLength(
    "length", "length of the sliding window in frames");
const ParameterInt NormalizationNode::paramRight(
    "right", "output point");

const ParameterInt NormalizationNode::paramLevelIndex(
    "level", "index of level normalization.", 0);
const ParameterFloat NormalizationNode::paramNorm(
    "norm", "norm of mean norm normalization.", 2);

NormalizationNode::NormalizationNode(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c),
    algorithm_(0),
    type_(typeMean),
    length_(0),
    right_(0),
    levelIndex_(0),
    norm_(0),
    needInit_(true) {

    setType((Type)paramType(c));
    setLength(paramLength(c));
    setRight(paramRight(c));
    setLevelIndex(paramLevelIndex(c));
    setNorm(paramNorm(c));
    addDatatype(Flow::Vector<Normalization::Value>::type());
}

NormalizationNode::~NormalizationNode() {
    delete algorithm_;
}

bool NormalizationNode::setParameter(const std::string &name, const std::string &value) {
    if (paramType.match(name))
	setType((Type)paramType(value));
    else if (paramLength.match(name))
	setLength(paramLength(value));
    else if (paramRight.match(name))
	setRight(paramRight(value));
    else if (paramLevelIndex.match(name))
	setLevelIndex(paramLevelIndex(value));
    else if (paramNorm.match(name))
	setNorm(paramNorm(value));
    else
	return false;

    return true;
}

bool NormalizationNode::configure() {
    reset();
    return Precursor::configure();
}

bool NormalizationNode::init(size_t dimension) {
    delete algorithm_;
    switch(type_) {
    case typeLevel:
	algorithm_ = new LevelNormalization(levelIndex_);
	break;
    case typeMean:
	algorithm_ = new MeanNormalization;
	break;
    case typeMeanAndVariance:
	algorithm_ = new MeanAndVarianceNormalization(config);
	break;
    case typeMeanAndVariance1D:
	algorithm_ = new MeanAndVarianceNormalization1D(config);
	break;
    case typeDivideByMean:
	algorithm_ = new DivideByMean(config);
	break;
    case typeMeanNorm:
	algorithm_ = new MeanNormNormalization(config, norm_);
	break;
    default:
	defect();
    }

    if (!algorithm_->init(length_, right_, dimension)) {
	error("Cannot initialize with parameters lenght (%zd), right (%zd), and dimension (%zd).",
	      length_, right_, dimension);
	return false;
    }

    needInit_ = false;
    return true;
}
void NormalizationNode::reset() {
    if (algorithm_ != 0)
	algorithm_->reset();
}

bool NormalizationNode::update(const Normalization::Frame &in, Normalization::Frame &out) {
    require(in);
    if (!needInit_ || init(in->size()))
	return algorithm_->update(in, out);
    return false;
}

bool NormalizationNode::flush(Normalization::Frame &out) {
    if (needInit_)
	return false;
    return algorithm_->flush(out);
}

bool NormalizationNode::work(Flow::PortId p) {
    Normalization::Frame in, out;

    while(getData(0, in)) {
	if (update(in, out))
	    return putData(0, out.get());
    }

    // in is invalid
    if (in == Flow::Data::eos()) {
	while(flush(out))
	    putData(0, out.get());
	reset();
    }
    return putData(0, in.get());
}
