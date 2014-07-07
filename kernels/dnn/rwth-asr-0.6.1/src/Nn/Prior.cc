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
#include "Prior.hh"

#include <sstream>

#include <Math/Module.hh>

using namespace Nn;


template<typename T>
const Core::ParameterString Prior<T>::paramPriorFile(
	"prior-file", "", "");

template<typename T>
const Core::ParameterFloat Prior<T>::paramPrioriScale(
	"priori-scale", "scaling of the logarithmized state priori probability", 1.0);

template<typename T>
const Core::ParameterBool Prior<T>::paramCompatibilityMode(
	"compatibility-mode", "calculate prior as in old version", false);

template<typename T>
Prior<T>::Prior(const Core::Configuration &c) :
    Core::Component(c),
    priorFilename_(paramPriorFile(c)),
    compatibilityMode_(paramCompatibilityMode(c)),
    scale_(paramPrioriScale(c)),
    statisticsChannel_(c, "statistics")
{
    this->log("using priori scale ") << scale_;
    if (compatibilityMode_)
	this->log("using compatibility mode for prior calculation");
}

template<typename T>
void Prior<T>::setFromClassCounts(const Statistics<T> &statistics, const Math::Vector<T> &classWeights){
    logPrior_.resize(classWeights.size());
    log("calculating prior from class counts");
    T totalWeight = 0.0;
    for (u32 c = 0; c < logPrior_.size(); c++) {
	logPrior_.at(c) = statistics.classCount(c) * classWeights.at(c);
	if (statisticsChannel_.isOpen()){
	    std::stringstream ss;
	    ss << "class-" << c;
	    std::string xmlName = ss.str();
	    statisticsChannel_ << Core::XmlOpen(xmlName.c_str())
	    << Core::XmlFull("number-of-observations", statistics.classCount(c))
	    << Core::XmlFull("weighted-number-of-observations", logPrior_.at(c))
	    << Core::XmlClose(xmlName.c_str());
	}
	if (logPrior_.at(c) == 0)
	    this->warning("zero observations for class: ") << c;
	totalWeight += logPrior_.at(c);
    }
    for (u32 c = 0; c < logPrior_.size(); c++)
	logPrior_.at(c) = logPrior_.at(c) == 0 ?  Core::Type<T>::min : std::log(logPrior_.at(c) / totalWeight);
    if (statisticsChannel_.isOpen())
	statisticsChannel_ << logPrior_;
}

template<typename T>
void Prior<T>::setFromMixtureSet(Core::Ref<const Mm::MixtureSet> mixtureSet, const ClassLabelWrapper &labelWrapper){
    log("calculating prior from mixture set");
    // get counts from mixture set
    std::vector<f32> priorFromMixtureSet;
    priorFromMixtureSet.resize(mixtureSet->nMixtures());
    require_eq(priorFromMixtureSet.size(), labelWrapper.nClasses());
    for (size_t m = 0; m < mixtureSet->nMixtures(); ++m) {
	const Mm::Mixture *mixture = mixtureSet->mixture(m);
	size_t nDensities = mixture->nDensities();
	for(size_t dns = 0; dns < nDensities; ++ dns) {
	    priorFromMixtureSet.at(m) += mixture->weight(dns);  /// returns exp(logWeights_[densityInMixture]), @see Mm/Mixture.cc
	}
    }

    // map counts corresponding to order in output layer
    logPrior_.resize(labelWrapper.nClassesToAccumulate());
    for (u32 m = 0; m < mixtureSet->nMixtures(); m++){
	if (labelWrapper.isClassToAccumulate(m))
	    logPrior_.at(labelWrapper.getOutputIndexFromClassIndex(m)) = priorFromMixtureSet.at(m);
    }

    f32 observationWeight = 0.0;
    // normalize and apply log
    if (compatibilityMode_)
	observationWeight = std::accumulate(priorFromMixtureSet.begin(), priorFromMixtureSet.end(), 0.0);
    else
	observationWeight = std::accumulate(logPrior_.begin(), logPrior_.end(), 0.0);

    for (u32 c = 0; c < logPrior_.size(); ++ c)
	logPrior_.at(c) = std::log(logPrior_.at(c) / observationWeight);
    if (statisticsChannel_.isOpen())
	statisticsChannel_ << logPrior_;
}

template<typename T>
void Prior<T>::getVector(NnVector &prior) const {
    prior.copy(logPrior_);
}


template<typename T>
bool Prior<T>::read() {
    require(priorFilename_ != "");
    return read(priorFilename_);
}

template<typename T>
bool Prior<T>::read(const std::string &filename){
    this->log("reading prior from file ") << filename;
    return Math::Module::instance().formats().read(filename, logPrior_);
}

template<typename T>
bool Prior<T>::write() const {
    require(priorFilename_ != "");
    return write(priorFilename_);
}

template<typename T>
bool Prior<T>::write(const std::string &filename) const {
    this->log("writing prior to file ") << filename;
    return Math::Module::instance().formats().write(filename, logPrior_, 20);
}


namespace Nn {
template class Prior<f32>;
template class Prior<f64>;
}
