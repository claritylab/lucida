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
#ifndef _MM_MLLR_ADAPTATION_HH
#define _MM_MLLR_ADAPTATION_HH

#include <vector>
#include <map>
#include <string>
#include <Core/BinaryTree.hh>
#include <Core/Component.hh>
#include <Math/Matrix.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/XmlStream.hh>
#include <Speech/Alignment.hh>
#include <Am/AdaptationTree.hh>
#include <Am/AcousticModelAdaptor.hh>
#include "MixtureSet.hh"
#include "AssigningFeatureScorer.hh"
#include "Types.hh"
#include "AbstractAdaptationAccumulator.hh"

namespace Mm {
    typedef Core::BinaryTree::Id NodeId;
    typedef Core::BinaryTree::LeafNumber LeafNumber;
    typedef Core::Ref<Math::ReferenceCountedVector<LeafNumber> > LeafIndexVector;
    typedef Math::Matrix<double>  Matrix;
    typedef std::map<NodeId,Matrix >::const_iterator MatrixConstIterator;
    typedef std::map<NodeId,Matrix >::iterator MatrixIterator;

    typedef div_t my_div_t;

    struct IdSetEntry{
	NodeId id;
	bool isActive;
	IdSetEntry(NodeId i, bool a=true):id(i),isActive(a){};
    };
    struct IdSetEntryLess
	: public std::binary_function<const IdSetEntry&, const IdSetEntry&, bool>
    {
	bool operator()(const IdSetEntry &a, const IdSetEntry &b) const
	{
	    return a.id < b.id;
	}
    };
    static inline bool operator< (const Mm::IdSetEntry& ls, const Mm::IdSetEntry& rs) {
	return ls.id<rs.id;
    }

    typedef std::set<IdSetEntry>::const_iterator IdSetConstIterator;
    typedef std::set<IdSetEntry>::iterator IdSetIterator;

    Math::Matrix<double> adaptationUnitMatrix(ComponentIndex dim);


    /**
     * Base class for adaptors.
     */
    class Adaptor: public Core::ReferenceCounted, public Core::Component {
    protected:
	ClusterId clusterId_;
    public:
	Adaptor(const Core::Configuration&);
	virtual ~Adaptor(){}

	ClusterId& clusterId(){return clusterId_;}
	const ClusterId& clusterId() const { return clusterId_; }
	virtual void setMatrix(std::map<NodeId,Matrix >&, std::vector<NodeId>& ) = 0;
	virtual void adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet) const = 0;
	//virtual void adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet, Core::Ref<Mm::Adaptor>) const = 0;

	// virtual Adaptor* clone() const = 0;
	virtual std::string typeName() const { return "adaptor"; }
	virtual bool write(Core::BinaryOutputStream &o) const;
	virtual bool read(Core::BinaryInputStream &i);
	virtual bool dump(Core::XmlOutputStream& xos) { defect(); }
    };


    /**
     * Adaptor for shift adaptation.
     */
    class ShiftAdaptor: public Adaptor {
    public:
	typedef Adaptor Precursor;
    private:
	std::map<NodeId,Matrix > adaptationMatrices_;
	Math::Vector<NodeId> adaptationMatrixIndex_;
    public:
	ShiftAdaptor(const Core::Configuration&);
	virtual ~ShiftAdaptor(){};

	virtual void setMatrix(std::map<NodeId,Matrix >& , std::vector<NodeId>& );
	virtual void adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet) const;
	void adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet, Core::Ref<Mm::ShiftAdaptor> preAdaptor) const;
	const std::map<NodeId,Matrix >& adaptationMatrices() const {return  adaptationMatrices_;}
	const Math::Vector<double>& shiftVector(MixtureIndex mix);

	virtual std::string typeName() const { return "shift-adaptor"; }
	virtual bool write(Core::BinaryOutputStream &o) const;
	virtual bool read(Core::BinaryInputStream &i);
    } ;


    /**
     * Adaptor for full matrix MLLR.
     */
    class FullAdaptor: public Adaptor {
    public:
	typedef Adaptor Precursor;
    private:
	std::map<NodeId,Matrix > adaptationMatrices_;
	Math::Vector<NodeId> adaptationMatrixIndex_;
    public:
	FullAdaptor(const Core::Configuration&);
	virtual ~FullAdaptor(){};

	virtual void setMatrix(std::map<NodeId,Matrix >& , std::vector<NodeId>& );
	virtual void adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet) const;
	const std::map<NodeId,Matrix >& adaptationMatrices() const {return  adaptationMatrices_;}

	// virtual Adaptor* clone() const;
	virtual std::string typeName() const { return "full-adaptor"; }
	virtual bool write(Core::BinaryOutputStream &o) const;
	virtual bool read(Core::BinaryInputStream &i);
    } ;


    /**
     * Base class for adaptor estimators.
     */
    class AdaptorEstimator: public AbstractAdaptationAccumulator, public Core::Component {
    protected:
	AdaptorEstimator(const Core::Configuration&, ComponentIndex dimension,
		const Core::Ref<Am::AdaptationTree> adaptationTree);
	virtual void estimateWMatrices()=0;
	template <class T2>
	void propagate(const Math::Vector<T2>& leafData, Math::Vector<T2> &nodeData, NodeId id);

	ClusterId clusterId_;
	ComponentIndex dimension_;
	Math::Vector<Sum> count_;

	Sum minAdaptationObservations_;
	Sum minSilenceObservations_;

	LeafIndexVector leafIndex_;	// MixtureToLeafIndex
	u32 nLeafs_;
	const Core::Ref<Core::BinaryTree> tree_ ;
	const std::set<MixtureIndex> silenceMixtures_;

	mutable Core::XmlChannel adaptationDumpChannel_;

	static const Core::ParameterFloat paramMinAdaptationObservations;
	static const Core::ParameterFloat paramMinSilenceObservations;
    public:
	AdaptorEstimator(const Core::Configuration&,
		const Core::Ref<Am::AdaptationTree> adaptationTree);
	AdaptorEstimator(const Core::Configuration&, const Core::Ref<const Mm::MixtureSet> mixtureSet,
		const Core::Ref<Am::AdaptationTree> adaptationTree);
	virtual ~AdaptorEstimator();

	ClusterId& clusterId(){return clusterId_;}
	const ClusterId& clusterId()const{return clusterId_;}
	virtual Sum minAdaptationObservations()=0;
	virtual Core::Ref<Adaptor> adaptor(void)=0;
	virtual void accumulate(
		Core::Ref<const Feature::Vector>,
		DensityIndex,
		MixtureIndex,
		Core::Ref<MixtureSet>) = 0;
	virtual void accumulate(
		Core::Ref<const Feature::Vector>,
		DensityIndex,
		MixtureIndex,
		Core::Ref<MixtureSet>,
		Mm::Weight) = 0;
	virtual void accumulate(
	    const Speech::Alignment&,
	    const std::vector<Core::Ref<const Mm::Feature> >&,
	    Core::Ref<const Am::MixtureSetAdaptor>,
	    Core::Ref<const Mm::AssigningFeatureScorer>) = 0;

	virtual void reset()=0;
	// virtual AdaptorEstimator* clone() const =0;
	virtual std::string typeName() const { return "adaptor-estimator"; }
	virtual bool read(Core::BinaryInputStream &i);
	virtual bool write(Core::BinaryOutputStream &o) const;
    };


    /**
     * Estimator for shift adaptor.
     */
    class ShiftAdaptorViterbiEstimator: public AdaptorEstimator {
    public:
	typedef AdaptorEstimator Precursor;
    private:
	Math::Vector<Sum> countAccumulators_;
	Math::Vector<Math::Vector<Sum> > betaAccumulators_;
	Math::Vector<Math::Vector<Sum> > shiftAccumulators_;
	std::map<NodeId,Matrix > shift_;  //not a vector since Id:s need not be successive

	void init();

	virtual void estimateWMatrices();
    public:
	ShiftAdaptorViterbiEstimator(const Core::Configuration &c, ComponentIndex dimension,
				     const Core::Ref<Am::AdaptationTree> adaptationTree);
	ShiftAdaptorViterbiEstimator(const Core::Configuration&,
				    const Core::Ref<Am::AdaptationTree> adaptationTree);
	ShiftAdaptorViterbiEstimator(const Core::Configuration &c, const Core::Ref<const Mm::MixtureSet> mixtureSet,
				    const Core::Ref<Am::AdaptationTree> adaptationTree);

	virtual Sum minAdaptationObservations(){return minAdaptationObservations_;}
	virtual Core::Ref<Adaptor> adaptor(void);
	virtual void accumulate(
	    Core::Ref<const Feature::Vector> feature,
	    DensityIndex density,
	    MixtureIndex mixture,
	    Core::Ref<MixtureSet> mixtureSet);
	virtual void accumulate(
	    Core::Ref<const Feature::Vector> feature,
	    DensityIndex density,
	    MixtureIndex mixture,
	    Core::Ref<MixtureSet> mixtureSet,
	    Mm::Weight weight);
	virtual void accumulate(
	    const Speech::Alignment &alignment,
	    const std::vector<Core::Ref<const Mm::Feature> > &featureSequence,
	    Core::Ref<const Am::MixtureSetAdaptor> unadaptedAdaptor,
	    Core::Ref<const Mm::AssigningFeatureScorer> unadaptedFeatureScorer);

	virtual void reset();

	virtual std::string typeName() const { return "shift-adaptor-viterbi-estimator"; }
	virtual bool write(Core::BinaryOutputStream &o) const;
	virtual bool read(Core::BinaryInputStream &i);
    } ;


    /**
     * Base class for accumulators.
     */
    class AccumulatorBase{
    protected:
	Matrix matrix_;
	Sum count_;
    public:
	AccumulatorBase():count_(0){};
	virtual ~AccumulatorBase(){};

	Math::Matrix<double>  matrix() const {return matrix_;}
	virtual Math::Matrix<double> squareMatrix() const =0;
	Sum count() const {return count_;}
	void add(const Math::Matrix<double> & mat, const Sum c );
	void operator+=(const AccumulatorBase& a);

	void reset();
	virtual bool write(Core::BinaryOutputStream &o) const;
	virtual bool read(Core::BinaryInputStream &i);
    };

    Core::BinaryOutputStream& operator<< (Core::BinaryOutputStream& o, const AccumulatorBase &a);
    Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, AccumulatorBase &a);


    /**
     * Estimator for full matrix adaptor.
     */
    class FullAdaptorViterbiEstimator: public AdaptorEstimator {
    public:
	typedef AdaptorEstimator Precursor;
    private:
	void init();
    protected:
	FullAdaptorViterbiEstimator(const Core::Configuration &c, ComponentIndex dimension,
				    const Core::Ref<Am::AdaptationTree> adaptationTree);

	class ZAccumulator: public AccumulatorBase{
	public:
	    ZAccumulator(ComponentIndex dim);
	    ZAccumulator();
	    virtual ~ZAccumulator(){};
	    void accumulate(const Mm::Mean&,  const FeatureVector&);
	    void accumulate(const Mm::Mean&,  const FeatureVector&, Mm::Weight);
	    virtual Math::Matrix<double> squareMatrix() const;
	};

	class GAccumulator: public AccumulatorBase{
	public:
	    GAccumulator(ComponentIndex dim);
	    GAccumulator();
	    virtual ~GAccumulator(){};
	    void accumulate(const Mm::Mean&);
	    void accumulate(const Mm::Mean&, Mm::Weight);
	    virtual Math::Matrix<double> squareMatrix() const;
	};

	Math::Vector<ZAccumulator> leafZAccumulators_;
	Math::Vector<GAccumulator> leafGAccumulators_;
	std::map<NodeId,Matrix > w_;  //not a vector since Id:s need not be successive

	virtual void estimateWMatrices();

    public:
	FullAdaptorViterbiEstimator(const Core::Configuration&,
				    const Core::Ref<Am::AdaptationTree> adaptationTree);
	FullAdaptorViterbiEstimator(const Core::Configuration &c, const Core::Ref<const Mm::MixtureSet> mixtureSet,
				    const Core::Ref<Am::AdaptationTree> adaptationTree);

	virtual Sum minAdaptationObservations(){return minAdaptationObservations_;}
	virtual Core::Ref<Adaptor> adaptor(void);
	virtual void accumulate(
		Core::Ref<const Feature::Vector> feature,
		DensityIndex density,
		MixtureIndex mixture,
		Core::Ref<MixtureSet> mixtureSet);
	virtual void accumulate(
		Core::Ref<const Feature::Vector> feature,
		DensityIndex density,
		MixtureIndex mixture,
		Core::Ref<MixtureSet> mixtureSet,
		Mm::Weight weight);
	virtual void accumulate(
	    const Speech::Alignment &alignment,
	    const std::vector<Core::Ref<const Mm::Feature> > &featureSequence,
	    Core::Ref<const Am::MixtureSetAdaptor> unadaptedAdaptor,
	    Core::Ref<const Mm::AssigningFeatureScorer> unadaptedFeatureScorer);

	virtual void reset();
	// virtual AdaptorEstimator* clone() const;
	virtual std::string typeName() const { return "full-adaptor-viterbi-estimator"; }
	virtual bool write(Core::BinaryOutputStream &o) const;
	virtual bool read(Core::BinaryInputStream &i);
    } ;


} //namespace Mm



namespace {
    template<class T>
    std::set<T>& operator+=(std::set<T> &s1, const std::set<T> &s2)
    {
	typename std::set<T>::const_iterator p=s2.begin(), q=s2.end();
	s1.insert(p,q);
	return s1;
    }
}

#endif // _MM_MLLR_ADAPTATION_HH
