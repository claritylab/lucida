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
#include "MllrAdaptation.hh"
#include <Math/Lapack/Svd.hh>
#include <Core/Statistics.hh>
#include <Math/Lapack/MatrixTools.hh>
#include <cstdlib>

using namespace Mm;
using namespace Math::Lapack;


Math::Matrix<double> Mm::adaptationUnitMatrix(ComponentIndex dim)
{
    Math::Matrix<double> matrix(dim, dim+1);
    matrix = Math::makeDiagonalMatrix(dim, (double)1.0);
    matrix.insertColumn(0, Math::Vector<double>(dim, 0.0));
    return matrix;
}


// ===============================================================================
// Adaptor

Adaptor::Adaptor(const Core::Configuration &c):
    Core::Component(c),
    clusterId_("Dummy")
{}

bool Adaptor::write(Core::BinaryOutputStream &o) const
{
    o << clusterId_;
    return o.good();
}

bool Adaptor::read(Core::BinaryInputStream &i)
{
    i >> clusterId_;
    return i.good();
}

// ===============================================================================
// ShiftAdaptor

ShiftAdaptor::ShiftAdaptor(const Core::Configuration &c):
    Precursor(c)
{}

void ShiftAdaptor::setMatrix(std::map<NodeId,Matrix > &m, std::vector<NodeId> &mi)
{
    adaptationMatrices_=m;
    adaptationMatrixIndex_=mi;
}

void ShiftAdaptor::adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet) const
{
    // Adapt references
    log("adapting references, using ") << adaptationMatrices_.size() << " shifts.";
    for (MixtureIndex mix=0; mix<adaptableMixtureSet->nMixtures(); ++mix){
	MatrixConstIterator w=adaptationMatrices_.find(adaptationMatrixIndex_[mix]);
	ensure(w!=adaptationMatrices_.end());
	for (size_t dns=0; dns< adaptableMixtureSet->mixture(mix)->nDensities(); ++dns){

	    // Extract the mean vector
	    DensityIndex densityIndex = adaptableMixtureSet->mixture(mix)->densityIndex(dns);
	    MeanIndex meanIndex = adaptableMixtureSet->density(densityIndex)->meanIndex();
	    Mm::Mean mean = *(adaptableMixtureSet->mean(meanIndex));

	    // Shift
	    Math::Vector<Mm::MeanType> adaptedMean(w->second[0] + Math::Vector<Mm::MeanType>(mean));

	    // Update MixtureSet
	    // Should be efficient, as only the buffer pointers are swapped
	    adaptableMixtureSet->mean(meanIndex)->swap(adaptedMean);
	}
    }
}

void ShiftAdaptor::adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet, Core::Ref<Mm::ShiftAdaptor> preAdaptor) const
{
    if(adaptationMatrices_.size() > 0 || preAdaptor->adaptationMatrices().size() > 0) {
	// Adapt references
	log("adapting references, using ") << adaptationMatrices_.size() << " shifts.";
	for (MixtureIndex mix=0; mix<adaptableMixtureSet->nMixtures(); ++mix){
	    Math::Vector<double> shift;
	    Math::Vector<double> preShift;
	    if(adaptationMatrices_.size() > 0)
		shift = const_cast<ShiftAdaptor&>(*this).shiftVector(mix);
	    if(preAdaptor->adaptationMatrices().size() > 0)
		preShift = preAdaptor->shiftVector(mix);
	    for (size_t dns=0; dns< adaptableMixtureSet->mixture(mix)->nDensities(); ++dns){

		// Extract the mean vector
		DensityIndex densityIndex = adaptableMixtureSet->mixture(mix)->densityIndex(dns);
		MeanIndex meanIndex = adaptableMixtureSet->density(densityIndex)->meanIndex();
		Mm::Mean mean = *(adaptableMixtureSet->mean(meanIndex));

		// Shift
		Math::Vector<Mm::MeanType> adaptedMean;
		if(preShift.size() == 0)
		    adaptedMean = Math::Vector<Mm::MeanType>(mean);
		else
		    adaptedMean = Math::Vector<Mm::MeanType>(mean) - preShift;
		if(shift.size() > 0)
		    adaptedMean += shift;

		// Update MixtureSet
		// Should be efficient, as only the buffer pointers are swapped
		adaptableMixtureSet->mean(meanIndex)->swap(adaptedMean);
	    }
	}
    }
}

const Math::Vector<double>& ShiftAdaptor::shiftVector(MixtureIndex mix)
{
    MatrixConstIterator w=adaptationMatrices_.find(adaptationMatrixIndex_[mix]);
    ensure(w!=adaptationMatrices_.end());
    return w->second[0];
}

// Adaptor* FullAdaptor::clone() const
// {
//     return new FullAdaptor(getConfiguration());
// }

bool ShiftAdaptor::write(Core::BinaryOutputStream &o) const
{
    if (Precursor::write(o)) {
	o << adaptationMatrices_ << adaptationMatrixIndex_;
    }
    return o.good();
}

bool ShiftAdaptor::read(Core::BinaryInputStream &i)
{
    if (Precursor::read(i)) {
	i >> adaptationMatrices_ >> adaptationMatrixIndex_;
    }
    return i.good();
}


// ===============================================================================
// FullAdaptor

FullAdaptor::FullAdaptor(const Core::Configuration &c):
    Precursor(c)
{}

void FullAdaptor::setMatrix(std::map<NodeId,Matrix > &m, std::vector<NodeId> &mi)
{
    adaptationMatrices_=m;
    adaptationMatrixIndex_=mi;
}

void FullAdaptor::adaptMixtureSet(Core::Ref<Mm::MixtureSet> adaptableMixtureSet) const
{
    // Requires that mean-vectors are not shared between densitys. Should
    // explicity bail out if this is not true. Not implemented yet.

    //adapt references
    log("adapting references, using ") << adaptationMatrices_.size() << " matrices";
    for (MixtureIndex mix=0; mix<adaptableMixtureSet->nMixtures(); ++mix){
	MatrixConstIterator w=adaptationMatrices_.find(adaptationMatrixIndex_[mix]);
	ensure(w!=adaptationMatrices_.end());
	for (size_t dns=0; dns< adaptableMixtureSet->mixture(mix)->nDensities(); ++dns){

	    // Extract the mean vector
	    DensityIndex densityIndex = adaptableMixtureSet->mixture(mix)->densityIndex(dns);
	    MeanIndex meanIndex = adaptableMixtureSet->density(densityIndex)->meanIndex();
	    Mm::Mean extendedMean = *(adaptableMixtureSet->mean(meanIndex));

	    // Transform
	    extendedMean.insert(extendedMean.begin(),1.0);
	    Math::Vector<Mm::MeanType> adaptedMean(w->second*Math::Vector<Mm::MeanType>(extendedMean));

	    // Update MixtureSet
	    // Should be efficient, as only the buffer pointers are swapped
	    adaptableMixtureSet->mean(meanIndex)->swap(adaptedMean);
	}
    }
}

// Adaptor* FullAdaptor::clone() const
// {
//     return new FullAdaptor(getConfiguration());
// }

bool FullAdaptor::write(Core::BinaryOutputStream &o) const
{
    if (Precursor::write(o)) {
	o << adaptationMatrices_ << adaptationMatrixIndex_;
    }
    return o.good();
}

bool FullAdaptor::read(Core::BinaryInputStream &i)
{
    if (Precursor::read(i)) {
	i >> adaptationMatrices_ >> adaptationMatrixIndex_;
    }
    return i.good();
}


//===============================================================================
// AdaptorEstimator

const Core::ParameterFloat AdaptorEstimator::paramMinAdaptationObservations(
    "min-observation",
    "minimum number of observations for MLLR estimation",
    1000,1);

const Core::ParameterFloat AdaptorEstimator::paramMinSilenceObservations(
    "min-silence-observation",
    "minimum number of silence observations for MLLR estimation",
    500,1);

AdaptorEstimator::AdaptorEstimator(
    const Core::Configuration &c,
    const Core::Ref<const Mm::MixtureSet> mixtureSet,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    Core::Component(c),
    dimension_(mixtureSet->dimension() ),
    leafIndex_(adaptationTree->leafIndex()),
    nLeafs_(adaptationTree->nLeafs()),
    tree_(adaptationTree->tree()),
    silenceMixtures_(adaptationTree->silenceMixtures()),
    adaptationDumpChannel_(config, "dump-adaptation")
{
    minAdaptationObservations_=paramMinAdaptationObservations(config);
    log("minimum number of observations for MLLR estimation: ")
	<< minAdaptationObservations_;
    minSilenceObservations_= paramMinSilenceObservations(config);
    log("minimum number of silence observations for MLLR estimation: ")
	<< minSilenceObservations_;

    if ((*leafIndex_).size() != (mixtureSet->nMixtures())) {
       criticalError("size of adaptation class array does not match size of mixture set");
    }
}

AdaptorEstimator::AdaptorEstimator(
    const Core::Configuration &c,
    ComponentIndex dim,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    Core::Component(c),
    dimension_(dim),
    leafIndex_(adaptationTree->leafIndex()),
    nLeafs_(adaptationTree->nLeafs()),
    tree_(adaptationTree->tree()),
    silenceMixtures_(adaptationTree->silenceMixtures()),
    adaptationDumpChannel_(config, "dump-adaptation")
{
    minAdaptationObservations_=paramMinAdaptationObservations(config);
    //no logging because only used for clone()
}

AdaptorEstimator::AdaptorEstimator(
    const Core::Configuration &c,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    Core::Component(c),
    leafIndex_(adaptationTree->leafIndex()),
    nLeafs_(adaptationTree->nLeafs()),
    tree_(adaptationTree->tree()),
    silenceMixtures_(adaptationTree->silenceMixtures()),
    adaptationDumpChannel_(config, "dump-adaptation")
{
//    minAdaptationObservations_=paramMinAdaptationObservations(config);
//    minSilenceObservations_= paramMinSilenceObservations(config);
}

AdaptorEstimator::~AdaptorEstimator() {}

template <class T2>
void  AdaptorEstimator::propagate(const Math::Vector<T2>& leafData,
				      Math::Vector<T2> &nodeData, NodeId id)
{
    verify( id<size_t(tree_->numberOfNodes()+tree_->numberOfLeafs()) );

    if (nodeData.size()!=size_t(tree_->numberOfNodes()+tree_->numberOfLeafs())) {
	nodeData.resize(tree_->numberOfNodes()+tree_->numberOfLeafs());
    }
    if (tree_->isLeaf(id)) {
	verify( tree_->leafNumber(id) < leafData.size() );
	nodeData[id]= leafData[tree_->leafNumber(id)];
	return;
    }
    else {
	ensure(id!=tree_->invalidId);
	propagate(leafData,nodeData,tree_->left(id));
	propagate(leafData,nodeData,tree_->right(id));
	nodeData[id]= nodeData[tree_->left(id)];
	nodeData[id]+= nodeData[tree_->right(id)];
    }
}


bool AdaptorEstimator::write(Core::BinaryOutputStream &o) const
{
    o << clusterId_ << dimension_ << count_ << minAdaptationObservations_ << minSilenceObservations_;
    return o.good();
}

bool AdaptorEstimator::read(Core::BinaryInputStream &i)
{
    i >> clusterId_ >> dimension_ >> count_ >> minAdaptationObservations_ >> minSilenceObservations_;

    //Override values if present in configuration.
    minAdaptationObservations_=paramMinAdaptationObservations(config, minAdaptationObservations_);
    minSilenceObservations_= paramMinSilenceObservations(config, minSilenceObservations_);

    return i.good();
}


//===============================================================================
// AccumulatorBase

void AccumulatorBase::add(const Math::Matrix<double> & mat, const Sum c )
{
    matrix_= matrix_+mat;
    count_+= c;
}

void AccumulatorBase::reset()
{
    matrix_.fill(0.0);
}

void AccumulatorBase::operator+=(const AccumulatorBase& a)
{
    matrix_+= a.matrix_;
    count_+= a.count_;
}

bool AccumulatorBase::write(Core::BinaryOutputStream &o) const
{
    o << matrix_ << count_;
    return o.good();
}

bool AccumulatorBase::read(Core::BinaryInputStream &i)
{
    i >> matrix_ >> count_;
    return i.good();
}

Core::BinaryOutputStream& Mm::operator<< (Core::BinaryOutputStream& o, const AccumulatorBase &a)
{
    a.write(o);
    return o;
}

Core::BinaryInputStream& Mm::operator>> (Core::BinaryInputStream& i, AccumulatorBase &a)
{
    a.read(i);
    return i;
}


//===============================================================================
// ShiftAdaptorViterbiEstimator

ShiftAdaptorViterbiEstimator::ShiftAdaptorViterbiEstimator(
    const Core::Configuration& c,
    Core::Ref<const Mm::MixtureSet> m,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    AdaptorEstimator(c, m, adaptationTree)
{
    init();
}

ShiftAdaptorViterbiEstimator::ShiftAdaptorViterbiEstimator(
    const Core::Configuration& c,
    ComponentIndex dim,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    AdaptorEstimator(c, dim, adaptationTree)
{
    init();
}

ShiftAdaptorViterbiEstimator::ShiftAdaptorViterbiEstimator(
    const Core::Configuration& c,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    AdaptorEstimator(c, adaptationTree)
{ }

void ShiftAdaptorViterbiEstimator::init()
{
    countAccumulators_.resize(nLeafs_, 0); //number of leafs in MLLR tree
    betaAccumulators_.resize(nLeafs_, Math::Vector<Sum>(dimension_));
    shiftAccumulators_.resize(nLeafs_, Math::Vector<Sum>(dimension_));

    std::stack<NodeId> stack;
    stack.push(tree_->root());
    while (!stack.empty()) {
	Core::BinaryTree::Id id = stack.top(); stack.pop();
	shift_[id]=Matrix();
	if (tree_->right(id) != tree_->invalidId) {
	    stack.push(tree_->right(id));
	}
	if (tree_->left(id) != tree_->invalidId) {
	    stack.push(tree_->left(id));
	}
    }

    for (MatrixIterator p=shift_.begin(); p!=shift_.end(); ++p) {
	p->second=Math::Matrix<Sum>();
	p->second.addRow(Math::Vector<Sum>(dimension_));
    }
}

void ShiftAdaptorViterbiEstimator::reset(void)
{
    log("resetting adaptation data");
    for (u16 i=0; i< betaAccumulators_.size(); i++) {
	countAccumulators_[i] = 0;
	betaAccumulators_[i] = Math::Vector<Sum>(dimension_);
	shiftAccumulators_[i] = Math::Vector<Sum>(dimension_);
    }
    for (MatrixIterator p=shift_.begin(); p!=shift_.end(); ++p) {
	p->second=Math::Matrix<Sum>();
	p->second.addRow(Math::Vector<Sum>(dimension_));
    }
}

void ShiftAdaptorViterbiEstimator::estimateWMatrices()
{
    Math::Vector<Sum> count;
    Math::Vector<Math::Vector<Sum> > beta;
    Math::Vector<Math::Vector<Sum> > shift;

    propagate(countAccumulators_, count, tree_->root());
    propagate(betaAccumulators_, beta, tree_->root());
    propagate(shiftAccumulators_, shift, tree_->root());

    ensure(beta.size()==shift.size());
    count_.resize(count.size());
    for (u32 id=0; id<beta.size(); ++id) { //loop over all nodes of tree_
	    count_[id]=count[id];
	if (count_[id] > minAdaptationObservations_) {
	    for (u32 d=0; d<dimension_; d++)
		shift_[id][0][d] = shift[id][d] / beta[id][d];
	}
	else {
	    shift_.erase(id);
	}
    }
}

void ShiftAdaptorViterbiEstimator::accumulate(
	Core::Ref<const Feature::Vector> feature,
	DensityIndex density,
	MixtureIndex mixture,
	Core::Ref<MixtureSet> mixtureSet)
{
    MeanIndex meanIndex = mixtureSet->density(density)->meanIndex();
    const Mm::Mean &mean = *(mixtureSet->mean(meanIndex));

    CovarianceIndex covIndex = mixtureSet->density(density)->covarianceIndex();
    const Mm::Covariance &cov = *(mixtureSet->covariance(covIndex));

    countAccumulators_[(*leafIndex_)[mixture]] += 1.0;
    for (u32 d=0; d<dimension_; d++) {
	betaAccumulators_[(*leafIndex_)[mixture]][d] += 1.0 / cov.diagonal()[d];
	shiftAccumulators_[(*leafIndex_)[mixture]][d] +=
	    ((*feature)[d] - mean[d]) / cov.diagonal()[d];
    }
}

void ShiftAdaptorViterbiEstimator::accumulate(
	Core::Ref<const Feature::Vector> feature,
	DensityIndex density,
	MixtureIndex mixture,
	Core::Ref<MixtureSet> mixtureSet,
	Mm::Weight weight)
{
    MeanIndex meanIndex = mixtureSet->density(density)->meanIndex();
    const Mm::Mean &mean = *(mixtureSet->mean(meanIndex));

    CovarianceIndex covIndex = mixtureSet->density(density)->covarianceIndex();
    const Mm::Covariance &cov = *(mixtureSet->covariance(covIndex));

    countAccumulators_[(*leafIndex_)[mixture]] += weight;
    for (u32 d=0; d<dimension_; d++) {
	betaAccumulators_[(*leafIndex_)[mixture]][d] += weight / cov.diagonal()[d];
	shiftAccumulators_[(*leafIndex_)[mixture]][d] +=
	    weight * ((*feature)[d] - mean[d]) / cov.diagonal()[d];
    }
}

void ShiftAdaptorViterbiEstimator::accumulate(
    const Speech::Alignment &alignment,
    const std::vector<Core::Ref<const Mm::Feature> > &featureSequence,
    Core::Ref<const Am::MixtureSetAdaptor> unadaptedAdaptor,
    Core::Ref<const Mm::AssigningFeatureScorer> unadaptedFeatureScorer)
{
    require(alignment.size() == featureSequence.size());
    for (size_t item=0; item < alignment.size(); ++item) {
	if (alignment[item].weight < 0.5)
	    continue;
	MixtureIndex mixture=
	unadaptedAdaptor->acousticModel()->emissionIndex(alignment[item].emission);
	size_t density=
	unadaptedFeatureScorer->getAssigningScorer(featureSequence[item])->bestDensity(mixture);

	// Extract the mean vector
	DensityIndex densityIndex =
	unadaptedAdaptor->mixtureSet()->mixture(mixture)->densityIndex(density);
	MeanIndex meanIndex = unadaptedAdaptor->mixtureSet()->density(densityIndex)->meanIndex();
	const Mm::Mean &mean = *(unadaptedAdaptor->mixtureSet()->mean(meanIndex));
	CovarianceIndex covIndex = unadaptedAdaptor->mixtureSet()->density(densityIndex)->covarianceIndex();
	const Mm::Covariance &cov = *(unadaptedAdaptor->mixtureSet()->covariance(covIndex));

	countAccumulators_[(*leafIndex_)[mixture]] += 1.0;
	for (u32 d=0; d<dimension_; d++) {
	    betaAccumulators_[(*leafIndex_)[mixture]][d] += 1.0 / cov.diagonal()[d];
	    shiftAccumulators_[(*leafIndex_)[mixture]][d] +=
		((*featureSequence[item]->mainStream())[d] - mean[d]) / cov.diagonal()[d];
	}
    }
}

Core::Ref<Adaptor> ShiftAdaptorViterbiEstimator::adaptor(void)
{
    Core::Ref<Adaptor> adaptor(new ShiftAdaptor(config));

    estimateWMatrices();

    std::vector<NodeId> adaptationMatrixIndex(leafIndex_->size());
    std::vector<NodeId> matrixId(nLeafs_);
    std::map<NodeId,Matrix > adaptationMatrices;
    NodeId id;

    id=tree_->root();
    if (count_[id] <= minAdaptationObservations_) {
	//not enough observations in complete tree
	ensure(id==tree_->root()); //the only case this can happen
	log("too few observations for adaptation\n")
	    << minAdaptationObservations_ << " observations needed, "
	    << count_[id] << " seen.\n" << "resetting adaptation to unity";
	adaptationMatrices[id]=Math::Matrix<Sum>();
	adaptationMatrices[id].addRow(Math::Vector<Sum>(dimension_));
	for (Core::BinaryTree::LeafList::const_iterator p=tree_->leafList().begin();
	    p!=tree_->leafList().end(); ++p)
	{
	    matrixId[tree_->leafNumber(p)]=id;
	}
    }
    else {
	for (Core::BinaryTree::LeafList::const_iterator p=tree_->leafList().begin();
	    p!=tree_->leafList().end(); ++p)
	{
	    id=tree_->id(p);
	    NodeId leafId=tree_->leafNumber(id);
	    while (id!=tree_->root() && count_[id] <= minAdaptationObservations()) {
		id=tree_->previous(id);
	    }
	    ensure(id!=Core::BinaryTree::invalidId);
	    ensure(count_[id] > minAdaptationObservations());
	    if (adaptationMatrices.find(id) == adaptationMatrices.end()) {
		MatrixConstIterator p=shift_.find(id);
		ensure(p!=shift_.end()); //something has gone wrong in estimateWMatrices()
		adaptationMatrices[id]=p->second;
	    }
	    matrixId[leafId]=id;
	 }
    }

    Core::BinaryTree::Id silenceId=tree_->numberOfNodes()+tree_->numberOfLeafs();

    //estimate WMatrix for silence mixtures
    for (std::set<MixtureIndex>::const_iterator p= silenceMixtures_.begin();
	p!=silenceMixtures_.end(); ++p)
    {
	Core::BinaryTree::Id silenceLeaf=(*leafIndex_)[*p];
	if (countAccumulators_[silenceLeaf] > minSilenceObservations_) {
	    adaptationMatrices[silenceId].clear();
	    adaptationMatrices[silenceId].addRow(Math::Vector<Sum>(dimension_));
	    for (u32 d=0; d<dimension_; d++)
		adaptationMatrices[silenceId][0][d] =
			shiftAccumulators_[silenceLeaf][d] / betaAccumulators_[silenceLeaf][d];
	}
	else {
	    log("too few observations for silence adaptation\n")
		<< minSilenceObservations_ << " observations needed, "
		<< int(countAccumulators_[silenceLeaf]) << " seen.\n"
		<< "resetting matrix to unity";
	    adaptationMatrices[silenceId].clear();
	    adaptationMatrices[silenceId].addRow(Math::Vector<Sum>(dimension_));
	}
	matrixId[silenceLeaf] = silenceId;
	++silenceId;
    }

    log("Estimated ") << u32(adaptationMatrices.size()) << " adaptation shifts";
    if (adaptationDumpChannel_.isOpen()) {
	adaptationDumpChannel_<< Core::XmlOpen("mixture-shift-mapping");
    }
    for (u32 mixture = 0; mixture< leafIndex_->size(); mixture++) {

#ifndef __w_all__
	adaptationMatrixIndex[mixture]=matrixId[(*leafIndex_)[mixture]];
#else
	log("w_all estimation");
	adaptationMatrixIndex[mixture]=matrixId[tree_->root()];
#endif
	if (adaptationDumpChannel_.isOpen()) {
	    adaptationDumpChannel_<< Core::XmlEmpty("shift")
		+ Core::XmlAttribute("shift-id", mixture)
		+ Core::XmlAttribute("shift-id", adaptationMatrixIndex[mixture]);
	}
    }
    if (adaptationDumpChannel_.isOpen()) {
	adaptationDumpChannel_ << Core::XmlClose("mixture-shift-mapping");
    }
    adaptor->setMatrix(adaptationMatrices, adaptationMatrixIndex);
    return adaptor;
}

bool ShiftAdaptorViterbiEstimator::write(Core::BinaryOutputStream &o) const
{
    if (Precursor::write(o)) {
	o << countAccumulators_ << betaAccumulators_ << shiftAccumulators_ << shift_;
    }
    return o.good();
}

bool ShiftAdaptorViterbiEstimator::read(Core::BinaryInputStream &i)
{
    if (Precursor::read(i)) {
	i >> countAccumulators_ >> betaAccumulators_ >> shiftAccumulators_ >> shift_;
    }
    return i.good();
}
//===============================================================================
// FullAdaptorViterbiEstimator

FullAdaptorViterbiEstimator::FullAdaptorViterbiEstimator(
    const Core::Configuration& c,
    Core::Ref<const Mm::MixtureSet> m,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    AdaptorEstimator(c, m, adaptationTree)
{
    init();
}

FullAdaptorViterbiEstimator::FullAdaptorViterbiEstimator(
    const Core::Configuration& c,
    ComponentIndex dim,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    AdaptorEstimator(c, dim, adaptationTree)
{
    init();
}

FullAdaptorViterbiEstimator::FullAdaptorViterbiEstimator(
    const Core::Configuration& c,
    const Core::Ref<Am::AdaptationTree> adaptationTree)
    :
    AdaptorEstimator(c, adaptationTree)
{ }

void FullAdaptorViterbiEstimator::init()
{
    leafZAccumulators_.resize(nLeafs_, ZAccumulator(dimension_)); //number of leafs in MLLR tree
    leafGAccumulators_.resize(nLeafs_, GAccumulator(dimension_));

    //initialize matrices. Is this really needed??? Probably not...
    std::stack<NodeId> stack;
    stack.push(tree_->root());
    while (!stack.empty()) {
	Core::BinaryTree::Id id = stack.top(); stack.pop();
	w_[id]=Matrix();
	if (tree_->right(id) != tree_->invalidId) {
	    stack.push(tree_->right(id));
	}
	if (tree_->left(id) != tree_->invalidId) {
	    stack.push(tree_->left(id));
	}
    }
}

void FullAdaptorViterbiEstimator::reset(void)
{
    //todo
    log("resetting adaptation matrices");
    for (u16 i=0; i< leafZAccumulators_.size(); i++) {
	leafZAccumulators_[i].reset();
	leafGAccumulators_[i].reset();
    }
    for (MatrixIterator p=w_.begin(); p!=w_.end(); ++p) {
	p->second=adaptationUnitMatrix(dimension_);
    }
}

void FullAdaptorViterbiEstimator::ZAccumulator::accumulate(const Mm::Mean& mean,
							   const FeatureVector& vec)
{
    require(mean.size() == vec.size());
    count_++;
    for (u32 i=0; i<matrix_.nRows(); i++) {
	for (u32 j=0; j<matrix_.nColumns(); j++) {
	    matrix_[i][j]+= vec[i] * ((j == 0) ? 1.0 : mean[j-1]);
	}
    }
}

void FullAdaptorViterbiEstimator::ZAccumulator::accumulate(const Mm::Mean& mean,
							   const FeatureVector& vec,
							   Mm::Weight weight)
{
    require(mean.size() == vec.size());
    count_++;
    for (u32 i=0; i<matrix_.nRows(); i++) {
	for (u32 j=0; j<matrix_.nColumns(); j++) {
	    matrix_[i][j]+= weight * vec[i] * ((j == 0) ? 1.0 : mean[j-1]);
	}
    }
}

FullAdaptorViterbiEstimator:: ZAccumulator::ZAccumulator():AccumulatorBase() {}

FullAdaptorViterbiEstimator:: ZAccumulator::ZAccumulator(ComponentIndex dim):AccumulatorBase()
{
    matrix_.resize(dim, dim+1);
    reset();
}

Math::Matrix<double> FullAdaptorViterbiEstimator::ZAccumulator::squareMatrix() const
{
    Math::Matrix<double> m(matrix_);
    m.removeColumn(0);
    return m;
}

void FullAdaptorViterbiEstimator::GAccumulator::accumulate(const Mm::Mean& mean)
{
    count_++;
    for (u32 i=0; i<matrix_.nRows(); i++) {
	for (u32 j=0; j<matrix_.nColumns(); j++) {
	matrix_[i][j]+= ((i == 0) ? 1.0 : mean[i-1]) * ((j == 0) ? 1.0 : mean[j-1]);
	}
    }
}

void FullAdaptorViterbiEstimator::GAccumulator::accumulate(const Mm::Mean& mean, Mm::Weight weight)
{
    count_++;
    for (u32 i=0; i<matrix_.nRows(); i++) {
	for (u32 j=0; j<matrix_.nColumns(); j++) {
	matrix_[i][j]+= weight * ((i == 0) ? 1.0 : mean[i-1]) * ((j == 0) ? 1.0 : mean[j-1]);
	}
    }
}

FullAdaptorViterbiEstimator::GAccumulator::GAccumulator():AccumulatorBase() {}

FullAdaptorViterbiEstimator::GAccumulator::GAccumulator(ComponentIndex dim):AccumulatorBase()
{
    matrix_.resize(dim+1, dim+1);
    reset();
}

Math::Matrix<double> FullAdaptorViterbiEstimator::GAccumulator::squareMatrix() const
{
    Math::Matrix<double> m(matrix_);
    m.removeRow(0);
    m.removeColumn(0);
    return m;
}

void FullAdaptorViterbiEstimator::estimateWMatrices()
{
    Math::Vector<ZAccumulator> z;
    Math::Vector<GAccumulator> g;

    propagate(leafZAccumulators_, z, tree_->root());
    propagate(leafGAccumulators_, g, tree_->root());

    ensure(z.size()==g.size());
    count_.resize(z.size());
    for (u32 id=0; id<z.size(); ++id) { //loop over all nodes of tree_
	    count_[id]=g[id].count();
	if (count_[id] > minAdaptationObservations_) {
	    Matrix gInverse(g[id].matrix());
	    pseudoInvert(gInverse);
	    w_[id]= z[id].matrix()*gInverse;
	}
	else {
	    w_.erase(id);
	}
    }
}

void FullAdaptorViterbiEstimator::accumulate(
	Core::Ref<const Feature::Vector> feature,
	DensityIndex density,
	MixtureIndex mixture,
	Core::Ref<MixtureSet> mixtureSet)
{
    MeanIndex meanIndex = mixtureSet->density(density)->meanIndex();
    const Mm::Mean &mean = *(mixtureSet->mean(meanIndex));

    leafZAccumulators_[(*leafIndex_)[mixture]].accumulate(mean, *feature);
    leafGAccumulators_[(*leafIndex_)[mixture]].accumulate(mean);
}

void FullAdaptorViterbiEstimator::accumulate(
	Core::Ref<const Feature::Vector> feature,
	DensityIndex density,
	MixtureIndex mixture,
	Core::Ref<MixtureSet> mixtureSet,
	Mm::Weight weight)
{
    MeanIndex meanIndex = mixtureSet->density(density)->meanIndex();
    const Mm::Mean &mean = *(mixtureSet->mean(meanIndex));

    leafZAccumulators_[(*leafIndex_)[mixture]].accumulate(mean, *feature, weight);
    leafGAccumulators_[(*leafIndex_)[mixture]].accumulate(mean, weight);
}

void FullAdaptorViterbiEstimator::accumulate(
    const Speech::Alignment &alignment,
    const std::vector<Core::Ref<const Mm::Feature> > &featureSequence,
    Core::Ref<const Am::MixtureSetAdaptor> unadaptedAdaptor,
    Core::Ref<const Mm::AssigningFeatureScorer> unadaptedFeatureScorer)
{
    if (alignment.size() != featureSequence.size()) return;
    require(alignment.size() == featureSequence.size());
    for (size_t item=0; item < alignment.size(); ++item) {
	MixtureIndex mixture=
	unadaptedAdaptor->acousticModel()->emissionIndex(alignment[item].emission);
	size_t density=
	unadaptedFeatureScorer->getAssigningScorer(featureSequence[item])->bestDensity(mixture);

	// Extract the mean vector
	DensityIndex densityIndex =
	unadaptedAdaptor->mixtureSet()->mixture(mixture)->densityIndex(density);
	MeanIndex meanIndex = unadaptedAdaptor->mixtureSet()->density(densityIndex)->meanIndex();
	const Mm::Mean &mean = *(unadaptedAdaptor->mixtureSet()->mean(meanIndex));

	leafZAccumulators_[(*leafIndex_)[mixture]].accumulate(
		mean, *featureSequence[item]->mainStream());
	leafGAccumulators_[(*leafIndex_)[mixture]].accumulate(mean);
    }
}

Core::Ref<Adaptor> FullAdaptorViterbiEstimator::adaptor(void)
{
    Core::Ref<Adaptor> adaptor(new FullAdaptor(config));

    estimateWMatrices();

    std::vector<NodeId> adaptationMatrixIndex(leafIndex_->size());
    std::vector<NodeId> matrixId(nLeafs_);
    std::map<NodeId,Matrix > adaptationMatrices;
    NodeId id;

    id=tree_->root();
    if (count_[id] <= minAdaptationObservations_) {
	//not enough observations in complete tree
	ensure(id==tree_->root()); //the only case this can happen
	log("too few observations for adaptation\n")
	    << minAdaptationObservations_ << " observations needed, "
	    << count_[id] << " seen.\n" << "resetting matrix to unity";
	adaptationMatrices[id]=adaptationUnitMatrix(dimension_);
	for (Core::BinaryTree::LeafList::const_iterator p=tree_->leafList().begin();
	    p!=tree_->leafList().end(); ++p)
	{
	    matrixId[tree_->leafNumber(p)]=id;
	}
    }
    else {
	for (Core::BinaryTree::LeafList::const_iterator p=tree_->leafList().begin();
	    p!=tree_->leafList().end(); ++p)
	{
	    id=tree_->id(p);
	    NodeId leafId=tree_->leafNumber(id);
	    while (id!=tree_->root() && count_[id] <= minAdaptationObservations()) {
		id=tree_->previous(id);
	    }
	    ensure(id!=Core::BinaryTree::invalidId);
	    ensure(count_[id] > minAdaptationObservations());
	    if (adaptationMatrices.find(id) == adaptationMatrices.end()) {
		MatrixConstIterator p=w_.find(id);
		ensure(p!=w_.end()); //something has gone wrong in estimateWMatrices()
		adaptationMatrices[id]=p->second;
	    }
	    matrixId[leafId]=id;
	 }
    }

    /*! \todo change if silence is handled correctly by dectree */
    Core::BinaryTree::Id silenceId=tree_->numberOfNodes()+tree_->numberOfLeafs();

    //estimate WMatrix for silence mixtures
    for (std::set<MixtureIndex>::const_iterator p= silenceMixtures_.begin();
	p!=silenceMixtures_.end(); ++p)
    {
	Core::BinaryTree::Id silenceLeaf=(*leafIndex_)[*p];
	if (leafGAccumulators_[silenceLeaf].count() > minSilenceObservations_) {
	    Matrix gInverse(leafGAccumulators_[silenceLeaf].matrix());
	    pseudoInvert(gInverse);
	    adaptationMatrices[silenceId]=leafZAccumulators_[silenceLeaf].matrix()*gInverse;
	}
	else {
	    log("too few observations for silence adaptation\n")
		<< minSilenceObservations_ << " observations needed, "
		<< leafGAccumulators_[silenceLeaf].count() << " seen.\n"
		<< "resetting matrix to unity";
	    adaptationMatrices[silenceId] = adaptationUnitMatrix(dimension_);
	}
	matrixId[silenceLeaf] = silenceId;
	++silenceId;
    }

    log("Estimated ") << u32(adaptationMatrices.size()) << " adaptation matrices";
    if (adaptationDumpChannel_.isOpen()) {
	adaptationDumpChannel_<< Core::XmlOpen("mixture-matrix-mapping");
    }
    for (u32 mixture = 0; mixture< leafIndex_->size(); mixture++) {

#ifndef __w_all__
	adaptationMatrixIndex[mixture]=matrixId[(*leafIndex_)[mixture]];
#else
	log("w_all estimation");
	adaptationMatrixIndex[mixture]=matrixId[tree_->root()];
#endif
	if (adaptationDumpChannel_.isOpen()) {
	    adaptationDumpChannel_<< Core::XmlEmpty("mixture")
		+ Core::XmlAttribute("mixture-id", mixture)
		+ Core::XmlAttribute("matrix-id", adaptationMatrixIndex[mixture]);
	}
    }
    if (adaptationDumpChannel_.isOpen()) {
	adaptationDumpChannel_ << Core::XmlClose("mixture-matrix-mapping");
    }
    adaptor->setMatrix(adaptationMatrices, adaptationMatrixIndex);
    return adaptor;
}

// AdaptorEstimator* FullAdaptorViterbiEstimator::clone() const
// {
//     return new FullAdaptorViterbiEstimator(getConfiguration(), dimension_,
// 					   leafIndex_, nLeafs_, tree_, silenceMixtures_);
// }

bool FullAdaptorViterbiEstimator::write(Core::BinaryOutputStream &o) const
{
    if (Precursor::write(o)) {
	o << leafZAccumulators_ << leafGAccumulators_ << w_;
    }
    return o.good();
}

bool FullAdaptorViterbiEstimator::read(Core::BinaryInputStream &i)
{
    if (Precursor::read(i)) {
	i >> leafZAccumulators_ >> leafGAccumulators_ >> w_;
    }
    return i.good();
}
