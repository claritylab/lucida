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
#ifndef PRIOR_HH_
#define PRIOR_HH_

#include <Math/Vector.hh>
#include <Mm/MixtureSet.hh>

#include "ClassLabelWrapper.hh"
#include "Statistics.hh"
#include "Types.hh"


namespace Nn {

/**
 *  Handles the class prior probabilities.
 *  Priors can be read and written to file in Math::Vector formats.
 *  Alternatively, the priors can be estimated from a mixture set or from the class counts accumulated in a statistics object.
 *  A float parameter for scaling the prior can be specified.
 *
 *  In order to make old experiments reproducible, a compatibility mode is provided for estimating the priors from a mixture set.
 */
template<typename T>
class Prior : public Core::Component {
    typedef Core::Component Precursor;
    typedef typename Types<T>::NnVector NnVector;
    typedef typename Types<T>::NnMatrix NnMatrix;
    static const Core::ParameterString paramPriorFile;
    static const Core::ParameterFloat paramPrioriScale;
    static const Core::ParameterBool paramCompatibilityMode;
protected:
    const std::string priorFilename_;
    const bool compatibilityMode_;
    const T scale_;
    Math::Vector<T> logPrior_;
    Core::XmlChannel statisticsChannel_;
public:
    Prior(const Core::Configuration &c);
    virtual ~Prior() {}

    T scale() const { return scale_; }
    std::string fileName() const { return priorFilename_; }
    size_t size() const { return logPrior_.size(); }

    T& at(size_t n) { return logPrior_.at(n); }
    const T& at(size_t n) const { return logPrior_.at(n); }

    void resize(size_t nClasses) { logPrior_.resize(nClasses); }
    // set from statistics
    void setFromClassCounts(const Statistics<T> &statistics, const Math::Vector<T> &classWeights);
    // calculate prior from mixture set
    // option "normalize-mixture-weights" needs to be set to false in configuration!
    // use of class weighting not supported yet
    void setFromMixtureSet(Core::Ref<const Mm::MixtureSet> mixtureSet, const ClassLabelWrapper &labelWrapper);

    void getVector(NnVector &prior) const;

    // IO
    bool read();
    bool read(const std::string &filename);
    bool write() const;
    bool write(const std::string &filename) const;
};

} /* namespace Nn */

#endif /* PRIOR_HH_ */
