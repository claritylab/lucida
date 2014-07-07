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
#include <Modules.hh>
#include "MixtureSet.hh"
#include <Core/Application.hh>
#include <Core/Statistics.hh>
#include <typeinfo>

using namespace Mm;

/**
 * AbstractMixtureSet
 */
AbstractMixtureSet::AbstractMixtureSet(ComponentIndex dimension) :
    dimension_(dimension)
{}

AbstractMixtureSet::~AbstractMixtureSet()
{
    clear();
}

void AbstractMixtureSet::clear()
{
    mixtureTable_.clear();
    densityTable_.clear();
}

void AbstractMixtureSet::setOffset(ComponentIndex offset)
{
    verify(0); // Not implemented in this scorer
}

void AbstractMixtureSet::setDimension(ComponentIndex dimension)
{
    dimension_ = dimension;
}

void AbstractMixtureSet::removeDensitiesWithLowWeight(Weight minWeight, bool normalizeWeights)
{
    // remove indices from mixtures but objects are not removed from tables
    for (MixtureIndex mix = 0; mix < nMixtures(); ++ mix) {
	mixture(mix)->removeDensitiesWithLowWeight(minWeight, normalizeWeights);
    }
}

/**
 * MixtureSet
 */
MixtureSet::MixtureSet(ComponentIndex dimension) :
    Precursor(dimension)
{}


MixtureSet::~MixtureSet()
{}


MixtureSet* MixtureSet::createOneMixtureCopy() const {
    Mixture* oneMixture = new Mixture(*this);
    MixtureSet* oneMixtureSet = new MixtureSet(dimension_);
    oneMixtureSet->addMixture(oneMixture);

    // Deep copy the mean table
    for (MeanIndex i=0; i<nMeans(); ++i) {
	Mean* newMean = new Mean(*(mean(i)));
	oneMixtureSet->addMean(i, newMean);
    }

    // Deep copy the covariance table
    for (CovarianceIndex i=0; i<nCovariances(); ++i) {
	Covariance* newCovariance = covariance(i)->clone();
	oneMixtureSet->addCovariance(i, newCovariance);
    }

    // Deep copy the density table
    for (DensityIndex i=0; i<nDensities(); ++i) {
	GaussDensity* newDensity = new GaussDensity(*(density(i)));
	oneMixtureSet->addDensity(i, newDensity);
    }

    return oneMixtureSet;
}


MixtureSet* MixtureSet::createOneMixtureClusterCopy(const Core::Configuration& clusteringConfiguration) const {
    /*! @todo Move this method to somewhere else */
#if 1
    defect();
    return 0;
#endif
}

/**
 * Overwrites the means of a given mixture set (for debugging purposes).
 * @param newMean New mean.
 */
void MixtureSet::replaceMeans(const Mean& newMean) {
    for (MeanIndex i=0; i<nMeans(); ++i) {
	*(mean(i)) = newMean;
    }
}

void MixtureSet::clear()
{
    Precursor::clear();
    meanTable_.clear();
    covarianceTable_.clear();
}

void MixtureSet::setOffset(ComponentIndex offset)
{
    PointerVector<Mean>::ConstantIterator m;
    for (m = meanTable_.begin(); m != meanTable_.end(); ++m)
	(*m)->erase((*m)->begin(), (*m)->begin()+offset);
    PointerVector<Covariance>::ConstantIterator c;
    for (c= covarianceTable_.begin(); c != covarianceTable_.end(); ++c)
	(*c)->setOffset(offset);
}

void MixtureSet::setDimension(ComponentIndex dimension)
{
    Precursor::setDimension(dimension);
    PointerVector<Mean>::ConstantIterator m;
    for (m = meanTable_.begin(); m != meanTable_.end(); ++m)
	(*m)->resize(dimension, 0.0);
    PointerVector<Covariance>::ConstantIterator c;
    for (c= covarianceTable_.begin(); c != covarianceTable_.end(); ++c)
	(*c)->setDimension(dimension);
}

void MixtureSet::writeLogNormStatistics(Core::XmlWriter &os) const
{
    Core::Statistics<Score> logNormStatistics("mixture-set-log-norm-factor");
    PointerVector<Covariance>::ConstantIterator c;
    for(c = covarianceTable_.begin(); c != covarianceTable_.end(); ++ c)
	logNormStatistics += (*c)->halfeLogNormFactor();
    logNormStatistics.write(os);
    if (logNormStatistics.minimum() < 0) {
	Core::Application::us()->error(
	    "The minimal log norm factor of the mixture set is negative " \
	    "thus negative score are possible which is not supported by " \
	    "several search algorithms. To avoid this review the applied " \
	    " variance normalization, e.g. Signal::ScatterTransform::paramTransformScale");
    }
}

bool MixtureSet::write(std::ostream& o) const
{
    o << "#Version: 2.0" << std::endl;
    std::string covtype;
    if ( typeid((*(covariance(0)))) == typeid(DiagonalCovariance) ) {
	covtype = "DiagonalCovariance";
    } else {
	Core::Application::us()->error("Covariance Type <%s> not implemented",
				       typeid((*(covariance(0)))).name());
    }
    o << "#CovarianceType: " << covtype << std::endl;
    o << dimension() << " "
      << nMixtures() << " "
      << nDensities() << " "
      << nMeans() << " "
      << nCovariances() << std::endl;
    for (MixtureIndex i=0; i<nMixtures();++i)
	o << *(mixture(i));
    for (DensityIndex i=0; i<nDensities();++i)
	o << *(density(i));
    for (MeanIndex i=0; i<nMeans();++i)
	o << *(mean(i));
    for (CovarianceIndex i=0; i<nCovariances();++i) {
	o  << " " << (*(covariance(i)));
    }

    return o.good();
}
bool MixtureSet::read(std::istream& i)
{
    clear();
    ComponentIndex dim;
    MixtureIndex nMix;
    DensityIndex nDns;
    MeanIndex nMean;
    CovarianceIndex nCov;
    // Header
    std::string line;
    getline(i, line); // version
    f32 version = (f32)atof(line.substr(10).c_str());
    if (version > 2.0) {
	Core::Application::us()->criticalError("version \"") << version << "\" not supported";
    }
    getline(i, line); // CovType
    std::string covtype = line.substr(17);
    if (covtype.compare("DiagonalCovariance") != 0) {
	Core::Application::us()->error("No correct Covariance Type set: %s", covtype.c_str());
    }
    i >> dim >> nMix >> nDns >> nMean >> nCov;
    setDimension(dim);
    // Data
    while (0<nMix--) {
	Mixture* mix = new Mixture();
	mix->read(i, version);
	addMixture(mix);
    }
    while (0<nDns--) {
	GaussDensity* dns = new GaussDensity();
	i >> *dns;
	addDensity(dns);
    }
    while (0<nMean--) {
	Mean* mean = new Mean();
	i >> *mean;
	addMean(mean);
    }
    while (0<nCov--) {
	Covariance *cov = new DiagonalCovariance();
	i >> *cov;
	addCovariance(cov);
    }

    return i.good();
}
bool MixtureSet::write(Core::BinaryOutputStream& o) const
{
    Core::Application::us()->error("binaryoutput not yet implemented");
    return false;

}

bool MixtureSet::read(Core::BinaryInputStream& i)
{
    //return Legacy::readMixtureSet(i, *this, paramRepeatedCovariance(configuration));
#if 1
    Core::Application::us()->criticalError("reading of binary mixture sets is not supported in this version");
    return false;
#endif
}

