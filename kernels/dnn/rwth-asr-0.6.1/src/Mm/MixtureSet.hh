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
#ifndef _MM_MIXTURE_SET_HH
#define _MM_MIXTURE_SET_HH

#include "Utilities.hh"
#include "PointerVector.hh"
#include "Mixture.hh"
#include "GaussDensity.hh"
#include <Core/ReferenceCounting.hh>
#include <Core/BinaryStream.hh>

namespace Mm {

    class Mixture;

    /**
     * AbstractMixtureSet administrates mixture objects and
     *  their relations.
     *
     *  Data structure:
     *    -objects pointers are stored in tables;
     *    -objects points reference others by integer indexes.
     *    Advantage of pointer representation:
     *      -object types can be extended be deriving from the base types and
     *       without changing (much :-) ) in other parts of this library
     *    Advantages to graph representation:
     *      -direct access to set of elements (e.g.: list of all covariances)
     *      -storing and load is straight forward
     *    Disadvantages:
     *      -hard to remove elements from the structure
     *      -since the tables contain pointers, each access needs +1 indirection.
     *       Thus this representation is not suitable for fast computations.
     */
    class AbstractMixtureSet : public Core::ReferenceCounted {
    protected:
	PointerVector<Mixture> mixtureTable_;
	PointerVector<AbstractDensity> densityTable_;

	ComponentIndex dimension_;
    public:
	AbstractMixtureSet(ComponentIndex dimension = 0);
	~AbstractMixtureSet();

	virtual void clear();

	ComponentIndex dimension() const { return dimension_; }
	virtual void setOffset(ComponentIndex offset);
	virtual void setDimension(ComponentIndex dimension);

	MixtureIndex addMixture(MixtureIndex index, Mixture* mixture) {
	    return mixtureTable_.pushBack(index, mixture);
	}
	MixtureIndex addMixture(Mixture* mixture) {
	    return addMixture(mixtureTable_.size(), mixture);
	}
	MixtureIndex nMixtures() const { return mixtureTable_.size(); }
	Mixture* mixture(MixtureIndex index) { return mixtureTable_[index]; }
	const Mixture* mixture(MixtureIndex index) const { return mixtureTable_[index]; }

	DensityIndex addDensity(DensityIndex index, AbstractDensity* density) {
	    return densityTable_.pushBack(index, density);
	}
	DensityIndex addDensity(AbstractDensity* density) {
	    return addDensity(densityTable_.size(), density);
	}
	DensityIndex nDensities() const { return densityTable_.size(); }
	virtual AbstractDensity* density(DensityIndex index) { return densityTable_[index]; }
	virtual const AbstractDensity* density(DensityIndex index) const { return densityTable_[index]; }
	/**
	 * print/scan methods for std::iostreams
	 * used in operator<< resp. operator>>
	 * overwrite these methodes in inherited classes
	 */
	virtual bool write(Core::BinaryOutputStream& o) const { return false; }
	virtual bool read(Core::BinaryInputStream& i) { return false; }
	virtual bool write(std::ostream& o) const { return false; }
	virtual bool read(std::istream& i) { return false; }
	virtual void removeDensitiesWithLowWeight(Weight minWeight, bool normalizeWeights = true);
    };
    inline Core::BinaryOutputStream& operator<< (Core::BinaryOutputStream& o, const AbstractMixtureSet& ms) {
	if (!ms.write(o))
	    Core::Application::us()->error("an error occures while printing AbstractMixtureSet (or inherited) on binarystream..");
	return o;
    };
    inline Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, AbstractMixtureSet& ms) {
	if (!ms.read(i))
	    Core::Application::us()->error("an error occures while scaning AbstractMixtureSet (or inherited) from binarystream..");
	return i;
    };
    inline std::ostream& operator<< (std::ostream& o, const AbstractMixtureSet& ms) {
	if (!ms.write(o))
	    Core::Application::us()->error("an error occures while printing AbstractMixtureSet (or inherited) on asciistream..");
	return o;
    };
    inline std::istream& operator>> (std::istream& i, AbstractMixtureSet& ms) {
	if (!ms.read(i))
	    Core::Application::us()->error("an error occures while scaning AbstractMixtureSet (or inherited) from asciistream..");
	return i;
    };

    /** MixtureSet administrates mixture, Gauss-density, mean, and covariance objects and
     *  their relations.
     */
    class MixtureSet : public AbstractMixtureSet {
	typedef AbstractMixtureSet Precursor;
    protected:
	PointerVector<Mean> meanTable_;
	PointerVector<Covariance> covarianceTable_;
    public:
	MixtureSet(ComponentIndex dimension = 0);
	~MixtureSet();

	MixtureSet* createOneMixtureCopy() const;
	MixtureSet* createOneMixtureClusterCopy(const Core::Configuration& clusteringConfiguration) const ;

	virtual void clear();
	virtual void setOffset(ComponentIndex offset);
	virtual void setDimension(ComponentIndex dimension);

	void replaceMeans(const Mm::Mean& newMean);

	virtual GaussDensity* density(DensityIndex index) {
	    return required_cast(GaussDensity*, densityTable_[index]);
	}
	virtual const GaussDensity* density(DensityIndex index) const {
	    return required_cast(const GaussDensity*, densityTable_[index]);
	}

	MeanIndex addMean(MeanIndex index, Mean* mean) {
	    require(mean->size() == dimension_); return meanTable_.pushBack(index, mean);
	}
	MeanIndex addMean(Mean* mean) {
	    return addMean(meanTable_.size(), mean);
	}

	MeanIndex nMeans() const { return meanTable_.size(); }
	Mean* mean(MeanIndex index) { return meanTable_[index]; }
	const Mean* mean(MeanIndex index) const { return meanTable_[index]; }

	CovarianceIndex addCovariance(CovarianceIndex index, Covariance* covariance) {
	    require(covariance->dimension() == dimension_);
	    return covarianceTable_.pushBack(index, covariance);
	}
	CovarianceIndex addCovariance(Covariance* covariance) {
	    return addCovariance(covarianceTable_.size(), covariance); }
	CovarianceIndex nCovariances() const { return covarianceTable_.size(); }
	Covariance* covariance(CovarianceIndex index) { return covarianceTable_[index]; }
	void setFeatureWeights(const std::vector<f64> w) {
	    for (size_t index=0; index < covarianceTable_.size(); ++index)
		(*covarianceTable_[index]).setFeatureWeights(w);
	}
	const Covariance* covariance(CovarianceIndex index) const { return covarianceTable_[index]; }

	void writeLogNormStatistics(Core::XmlWriter &) const;

	virtual bool write(Core::BinaryOutputStream& o) const;
	virtual bool read(Core::BinaryInputStream& i);
	virtual bool write(std::ostream& o) const;
	virtual bool read(std::istream& i);

    };

} // namespace Mm

namespace Core {

    template <>
    class NameHelper<Mm::MixtureSet> : public std::string {
    public:
	NameHelper() : std::string("mixtureset") {}
    };

    template <>
    class NameHelper<Mm::AbstractMixtureSet> : public std::string {
    public:
	NameHelper() : std::string("abstractmixtureset") {}
    };

} //namespace Core

#endif // _MM_MIXTURE_SET_HH
