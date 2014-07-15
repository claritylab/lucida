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
#ifndef _SIGNAL_SEGMENT_CLUSTERING_HH
#define _SIGNAL_SEGMENT_CLUSTERING_HH

/** Segment clustering.
 *
 * @todo: a lot of public functions should be declared as private
 */

#include <Core/Parameter.hh>
#include <Core/ProgressIndicator.hh>
#include <Math/Vector.hh>
#include <Math/Matrix.hh>
#include <Math/Lapack/MatrixTools.hh>
#include <Flow/Vector.hh>
#include "Node.hh"
#include <fstream>
#include <ext/numeric>

namespace ext = ::__gnu_cxx;

#define MAXFLOAT 999999999 // happily large number
#define LN_2PI 1.83787706640934533908193770912475883960723876953125

namespace Signal {

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    class DiagCovMonoGaussianModel {
    protected:
	Math::Vector<f64> mean_;
	Math::Vector<f64> variance_;
	u32 nFrames_;
    public:
	DiagCovMonoGaussianModel(u32 dim): nFrames_(0)
	{
	    mean_.resize(dim);
	    variance_.resize(dim);
	}
	~DiagCovMonoGaussianModel() {}

	const u32 dim() const { return mean_.size(); }

	u32 nFrames() const { return nFrames_; }
	bool accumulate(const std::vector<f32> &vec);
	bool finalize();
	void mergeModels(const DiagCovMonoGaussianModel &x, const DiagCovMonoGaussianModel &y);
	void mergeMeans(const DiagCovMonoGaussianModel &x, const DiagCovMonoGaussianModel &y);
	void mergeVariance(const DiagCovMonoGaussianModel &x, const DiagCovMonoGaussianModel &y);
	void reset(f32 x);
    };

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    /**
     * @todo: this class should be derived from the abstract DiagCovMonoGaussianModel class
     */
    class FullCovMonoGaussianModel {
    protected:
	Math::Vector<f64>  mean_;
	Math::Matrix<f64>  variance_;
	u32                nFrames_;
	f32                likelihood_; /// represented as N(logDeterminant(variance))
    public:
	FullCovMonoGaussianModel(u32 dim): nFrames_(0)
	{
	    mean_.resize(dim);
	    variance_.resize(dim);
	}
	~FullCovMonoGaussianModel() {}

	/** returns the feature dimension */
	const u32 dim() const { return mean_.size(); }

	/** returns the number of frames seen so far */
	const u32  nFrames() const  { return nFrames_; }

	/** returns the mean vector of size dim() */
	const Math::Vector<f64>& mean() const  { return mean_; }

	/** returns the variance matrix of size dim()xdim() */
	const Math::Matrix<f64>& variance() const { return variance_; }

	/** update the mean vector, the variance matrix, and the frame count */
	bool accumulate(const std::vector<f32> &vec);

	/** finalize the mean vector and variance matrix */
	bool finalize();

	/** merges the mean vectors and variance matrices of the left
	 * FullCovMonoGaussianModel x and right
	 * FullCovMonoGaussianModel y into the current model.
	 */
	void mergeModels(const FullCovMonoGaussianModel &x, const FullCovMonoGaussianModel &y);

	/** merges the mean vectors of the left
	 * FullCovMonoGaussianModel x and right
	 * FullCovMonoGaussianModel y
	 *
	 * @todo: this should be a private function?
	 */
	void mergeMeans(const FullCovMonoGaussianModel &x, const FullCovMonoGaussianModel &y);

	/** merges the variance matrices of the left
	 * FullCovMonoGaussianModel x and right
	 * FullCovMonoGaussianModel y
	 *
	 * @todo: this should be a private function?
	 */
	void mergeVariance(const FullCovMonoGaussianModel &x, const FullCovMonoGaussianModel &y);

	/** ? */
	void reset(f32 x) {}

	/** BIC is a likelihood criterion penalized by the model
	 *  complexity: the number of parameters in the model.
	 *
	 *  The number of parameters for each cluster is d+d/2(d+1)
	 *
	 *  with penalty function P = (1/2(d+d/2(d+1))) * log(N)
	 *                          = (2d/4 + d^2/4 + d/4) * log(N)
	 *                          = (1/4(d*(3+d))) * log(N)
	 */
	const f32 computeComplexity();

	/** compute the likelihood as N(logDeterminant(variance)) */
	void computeL();

	/**  compute the relative likelihood */
	const f32 relativeLikelihood(const FullCovMonoGaussianModel &x) const;
    };


    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    class BICFullCovMonoGaussianModel : public FullCovMonoGaussianModel {
    private:
	f32 glr_;
    public:

	BICFullCovMonoGaussianModel(u32 dim):
	    FullCovMonoGaussianModel::FullCovMonoGaussianModel(dim), glr_(0) {}
	BICFullCovMonoGaussianModel(u32 dim, f32 alpha):
	    FullCovMonoGaussianModel::FullCovMonoGaussianModel(dim), glr_(0) {}

	BICFullCovMonoGaussianModel(u32 dim, const std::vector<BICFullCovMonoGaussianModel> &models):
	    FullCovMonoGaussianModel::FullCovMonoGaussianModel(dim), glr_(0) {}
	BICFullCovMonoGaussianModel(u32 dim, f32 alpha, const std::vector<BICFullCovMonoGaussianModel> &models):
	    FullCovMonoGaussianModel::FullCovMonoGaussianModel(dim), glr_(0) {}

	/** @return the N(logDeterminant(variance)) likelihood */
	const f32 likelihood() const { return likelihood_;}

	/** @return the generalized likelihood ratio (GLR) (or Gish distance) */
	const f32 score() const {return glr_;}

	/** @return the GLR stop score for the BIC criterion */
	const f32 stopscore() const {return glr_;}

	/** compute Brandt's generalized likelihood ratio (GLR) for
	 * the BIC criterion.
	 *
	 * GLR is calculated as 1/2(alldataLikelihood - leftLikelihood - rightLikelihood)
	 */
	void computeGLR(const BICFullCovMonoGaussianModel &x, const BICFullCovMonoGaussianModel &y);

	/** useless?? */
	void finalize(const std::vector<BICFullCovMonoGaussianModel>){ finalize(); };

	/** finalize the mean vector and variance matrix */
	void finalize();

	/**
	 * First, the mean vectors and variance matrices of the left
	 * FullCovMonoGaussianModel x and right
	 * FullCovMonoGaussianModel y are merged into the current
	 * model. Second, the likelihood of the current model is
	 * calculated, and finally the GLR is calculated for the
	 * merged, the left, and the right model.
	 *
	 */
	void mergeModels(const BICFullCovMonoGaussianModel &x, const BICFullCovMonoGaussianModel &y);

	~BICFullCovMonoGaussianModel(){};
    };


    // ------------------------------------------------------------------------
    class KL2FullCovMonoGaussianModel : public BICFullCovMonoGaussianModel{
    private:
	f32 kl2_;
    public:

	KL2FullCovMonoGaussianModel(u32 dim):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim), kl2_(0) {}
	KL2FullCovMonoGaussianModel(u32 dim, f32 alpha):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim), kl2_(0) {}

	KL2FullCovMonoGaussianModel(u32 dim, const std::vector<KL2FullCovMonoGaussianModel> &models):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim), kl2_(0) {}
	KL2FullCovMonoGaussianModel(u32 dim, f32 alpha, const std::vector<KL2FullCovMonoGaussianModel> &models):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim), kl2_(0) {}

	/** @return the KL2 distance */
	const f32 score() const { return kl2_; }

	/** compute the KL2 distance using relative likelihoods of the left and right model */
	void computeKL2(const KL2FullCovMonoGaussianModel &x, const KL2FullCovMonoGaussianModel &y);

	/**
	 * First, the mean vectors and variance matrices of the left
	 * FullCovMonoGaussianModel x and right
	 * FullCovMonoGaussianModel y are merged into the current
	 * model. Second, the KL2 distance is calculated for the
	 * merged, the left, and the right model.
	 *
	 */
	void mergeModels(const KL2FullCovMonoGaussianModel &x, const KL2FullCovMonoGaussianModel &y);

	~KL2FullCovMonoGaussianModel() {}
    };


    // ------------------------------------------------------------------------
    class CorrFullCovMonoGaussianModel : public BICFullCovMonoGaussianModel{
    private:
	Math::Vector<f32> relativeFeature_;
	bool refresh_;
	f32 corrdist_;
	f32 alpha_;
	const std::vector<CorrFullCovMonoGaussianModel> *models_;
    public:
	CorrFullCovMonoGaussianModel(u32 dim):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim),
	    refresh_(1), alpha_(0.0012)
	{}

	CorrFullCovMonoGaussianModel(u32 dim, f32 alpha):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim),
	    refresh_(1), alpha_(alpha)
	{}

	CorrFullCovMonoGaussianModel( u32 dim, f32 alpha,
	    const std::vector<CorrFullCovMonoGaussianModel> &models)
	:
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim),
	    refresh_(1), alpha_(alpha)
	{
	    models_ = &models;
	}

	CorrFullCovMonoGaussianModel(u32 dim, const std::vector<CorrFullCovMonoGaussianModel> &models):
	    BICFullCovMonoGaussianModel::BICFullCovMonoGaussianModel(dim),
	    refresh_(1), alpha_(0.0012)
	{
	    models_ = &models;
	}

	~CorrFullCovMonoGaussianModel(){};


	/** @return the relative feature vector */
	const Math::Vector<f32> relativeFeature() const { return relativeFeature_; }

	/** @return the correlation based distance */
	const f32 score() const { return corrdist_; }

	/** updates the alpha weighting parmameter for the relative feature normalizrtion */
	void reset(f32 alpha) { alpha_ = alpha; }

	/** weights and normalizes the relative feature vector
	 * (i.e. the relative likelihoods of all models are weighted
	 * by a factor alpha )
	 *
	 * @todo: this should be a private function
	 */
	void relativeFeatureNormalize(f32 weight);

	/** is rather an init function which initializes the relative
	 * features by the weighted and normalized relative
	 * likelihoods of its models
	 *
	 * @todo: this should be a private function
	 */
	void refresh();

	/** calulate the correlation-based distance between the left model x and right model y */
	void correlationDistance(CorrFullCovMonoGaussianModel &x, CorrFullCovMonoGaussianModel &y);

	/** merges the left model x and right model y into the current
	 * model and calculate the correlation distance
	 */
	void mergeModels( CorrFullCovMonoGaussianModel &x, CorrFullCovMonoGaussianModel &y);

	/** finalize the current model  and assign a new model-vector models */
	void finalize(const std::vector<CorrFullCovMonoGaussianModel> &models);
    };


    // ------------------------------------------------------------------------
    class Tracker {
    private:
	u32 leftIdx_;
	u32 rightIdx_;
	f32 score_;
    public:
	Tracker(){};
	~Tracker(){};

	void update(u32 left, u32 right, f32 score)
	{
	    leftIdx_  = left;
	    rightIdx_ = right;
	    score_    = score;
	}
	const u32 leftIdx() const { return leftIdx_; }
	const u32 rightIdx() const { return rightIdx_; }
	const f32 score() const { return score_; }
    };


    // ------------------------------------------------------------------------
    template<class T>
    class SegmentModelEstimator {
    public:
	typedef T Model;
	typedef f32 Value;
    private:
	std::vector<Model> models_;

    public:
	SegmentModelEstimator() {}
	virtual ~SegmentModelEstimator() {}

	void pushBackSegment(const u32 dim, const f32 alpha) { models_.push_back(Model(dim, alpha)); }
	void accumulate(const Flow::Vector<Value> &in) {
	    models_.back().accumulate(in);
	}

	/** finalize all segment models */
	std::vector<Model>& finalize()
	{
	    for (typename std::vector<Model>::iterator iter= models_.begin(); iter != models_.end(); ++iter) {
		iter->finalize(models_);
	    }
	    return models_;
	}
    };


    // ------------------------------------------------------------------------
    /** Segment Clustering
     *
     * @todo: class is derive from component now, so ppplication
     * logging should be done via the info channel
     */
    template<class Model>
    class SegmentClustering : public virtual Core::Component {
    private:
	std::vector<Model> modelTracker_;
	Math::Matrix<f32> distMatrix_;
	std::vector<s32> matrixTracker_;

	u32 parent_; // parent has idx smaller than son
	u32 child_;  // child has idx larger than parent

	f32 bestscore_;
	Math::Vector<u32> classid_;
	u32 nrsegments_;
	u32 nrcluster_;
	u32 totalframes_;

	u32 minframes_;
	u32 mincluster_;
	u32 maxcluster_;
	f32 threshold_;

	std::string outfile_;
	f32 lambda_;
	f32 alpha_;
	f32 alphaMin_;
	f32 alphaMax_;
	u32 amalgamation_;
	std::ifstream infile_;

	std::string segmentId_;

	std::vector<std::string> segmentIds_;
	// Ids of segments which should be assigned to the default cluster
	std::vector<std::string> defaultIds_;

	std::vector<s32> predefinedClusters_;
	std::vector<Tracker> mergeTracker_;
    private:
	mutable Core::XmlChannel infoChannel_;

    public:
	/** @todo: -99999.9 is maybe not the best value ... */
	SegmentClustering(const Core::Configuration &c):
	    Core::Component(c),
	    bestscore_(-99999.9),
	    totalframes_(0),
	    infoChannel_(c, "cluster-info"),
	    minframes_(0)
	    {};
	virtual ~SegmentClustering() {}

	/** minimum number of frames to consider the segment for segment clustering */
	void setMinFrames(int frames) { minframes_ = frames; }

	u32 minFrames() const {
	  return minframes_;
	}

	/** set the minimum number of clusters */
	void setMincluster(int value) { mincluster_ = value; }

	/** set the maximum number of clusters */
	void setMaxcluster(int value) { maxcluster_ = value; }

	/** */
	void setThreshold(float value) { threshold_ = value; }

	/** set penalty weight in BIC criterion, although only lambda=1 corresponds to the definition of BIC */
	void setLambda (float value) { lambda_ = value; }

	/** set bayesian amalgamation value */
	void setAmalgamation (int value) { amalgamation_ = value; }

	/** set the output filename to print the clustering info */
	void setFilename(std::string value) { outfile_= value; }

	/** set the input filename to read an existing clustering */
	void setInFile(std::string value)
	{
	    infile_.open(value.c_str(), std::ios::in );
	    if (infile_.is_open()) {
		loadClustering();
	    };
	}

	/** BBN proposed a log-likelihood ratio distance measure with
	 * distances between consecutive segments scaled down by a
	 * parameter alpha - is it the same value here ??? */
	void setAlpha(float value) { alpha_ = value; }

	/** set the minimum alpha scaling value to be used for alpha optimization in computeAlpha() method */
	void setAlphaMax(float value) { alphaMax_ = value; }

	/** set the maximum alpha scaling value to be used for alpha optimization in computeAlpha() method */
	void setAlphaMin(float value) { alphaMin_ = value; }

	void setId(std::string value) { segmentId_= value; }
	void pushBackId() { segmentIds_.push_back(segmentId_); }
	void pushDefaultId() { defaultIds_.push_back(segmentId_); }

	/** @return the alpha scaling parameter for the relative faeture weighting */
	const f32 alpha() const { return(alpha_); }

	/** Initializing distance matrix for clustering and initialize the model trackers */
	void initDistMatrix(std::vector<Model> &models)
	{
	    u32 dim = models.back().dim();
	    distMatrix_.resize(models.size());
	    totalframes_ = 0;
	    if (infoChannel_.isOpen()) {
		infoChannel_ << Core::XmlEmpty("init-distance-matrix ") + Core::XmlAttribute("segments", models.size());
	    }
	    for (u32 i = 0; i < models.size(); i++) {
		totalframes_ += models[i].nFrames();
		Model newModel(dim, alpha_, models);
		modelTracker_.push_back(models[i]);
		for (u32 j = 0; j < i; j++) {
		    newModel.mergeModels(models[i], models[j]);
		    distMatrix_[i][j] = newModel.score();
		}
	    }
	}

	void initMatrixTracker()
	{
	    for (u32 i = 0; i < matrixTracker_.size(); i++)
		matrixTracker_[i] = 1;
	}

	void initTracker(u32 dim)
	{
	    classid_.resize(dim);
	    ext::iota(classid_.begin(), classid_.end(),0);  //Iota assigns sequentially increasing values to a range
	    matrixTracker_.resize(dim);
	    initMatrixTracker();
	}

	/** computes the penalty P of the BIC criterion
	 * P = \lambda * log(N) * (1/2(d+d/2(d+1)))
	 */
	void computePenalty(std::vector<Model> &models)
	{
	    f32 penalty;
	    penalty = models.back().computeComplexity();
	    penalty *= lambda_;
	    penalty *= Core::log((f32)totalframes_);
	    threshold_ += penalty;
	}

	/** compute the average (score?) for a given scaling factor alpha */
	f32 computeAverage(const Math::Matrix<f32> &relativeFeature, const f32 alpha)
	{
	    u32 dim = relativeFeature.nRows();
	    f32 total = 0.0f;
	    f32 NormFac;
	    Math::Vector<f32> scaledFeature;
	    for (u32 i = 0 ; i < dim; i++) {
		scaledFeature = relativeFeature[i] * alpha;
		NormFac       = scaledFeature.logSum();
		total         += exp(scaledFeature[i] - NormFac);
	    }
	    total /= (f32)dim;
	    return(total);
	}

	/** compute relative features to be used for alpha optimization */
	void computeRelativeFeatures(const std::vector<Model> &models, Math::Matrix<f32> &relativeFeature) {
	    // first compute the relative likelihoods of the models
	    Core::WithProgressIndicator progressIndicator("computing relative likelihood features");
	    progressIndicator.start(models.size());
	    for (u32 i = 0; i < models.size(); i++) {
		for (u32 j = 0; j < models.size(); j++) {
		    relativeFeature[i][j] = models[i].relativeLikelihood(models[j]);
		}
		progressIndicator.notify(i);
	    }
	    progressIndicator.finish(false);
	}

	/** compute alpha scaling value for correlation-based distance scaling */
	void computeAlpha(std::vector<Model> &models)
	{
	    f32 alpha = 0.0;
	    f32 step = 0.5f;
	    Math::Matrix<f32> relativeFeature(models.size());
	    f32 average= 0;

	    // compute the relative features to be used for alpha optimization
	    computeRelativeFeatures(models, relativeFeature);

	    // optimize the best alpha value
	    while (((average < alphaMin_)||(average > alphaMax_ )) && !(Core::isAlmostEqualUlp(step, 0.0f, 100)) ) {
		if (infoChannel_.isOpen()) {
		    infoChannel_ << Core::XmlEmpty("compute-alpha")
			+ Core::XmlAttribute("alpha", alpha)
			+ Core::XmlAttribute("average", average)
			+ Core::XmlAttribute("alphaMin", alphaMin_)
			+ Core::XmlAttribute("alphaMax", alphaMax_)
			+ Core::XmlAttribute("step", step);
		}
		if (average < alphaMin_) {
		    alpha += step;
		}
		else {
		    alpha -=step;
		}
		step /= 2.0;
		average = computeAverage(relativeFeature, alpha);
	    }
	    if ((average < alphaMin_)||(average > alphaMax_ )) {
		warning("Clustering goals with current alpha settings not reachable, computation of alpha failed: alpha=%lf, average=%lf, alphaMin_=%lf, alphaMax_=%lf, step=%lf",
			alpha, average, alphaMin_, alphaMax_, step);
	    }
	    else {
		if (infoChannel_.isOpen()) {
		    infoChannel_ << Core::XmlEmpty("compute-alpha")
			+ Core::XmlAttribute("alpha", alpha)
			+ Core::XmlAttribute("average", average)
			+ Core::XmlAttribute("alphaMin", alphaMin_)
			+ Core::XmlAttribute("alphaMax", alphaMax_)
			+ Core::XmlAttribute("step", step);
		}
	    }

	    // update the scaling factor
	    for (u32 i = 0; i < models.size(); i++)
		models[i].reset(alpha);
	    alpha_ = alpha;
	}

	/** initializes the clustering process */
	void initSegmentClustering(std::vector<Model> &models)
	{
	    if (models.size() == 0){
		criticalError("no segments received for clustering");
	    }
	    if (alpha_ < 0) {
		if (infoChannel_.isOpen()) {
		    infoChannel_ << Core::XmlEmpty("init-segment-clustering") + Core::XmlAttribute("alpha", alpha_);
		}
		computeAlpha(models);
	    }
	    initTracker(models.size());
	    initDistMatrix(models);
	    computePenalty(models);
	    nrcluster_ = models.size();
	    nrsegments_ = models.size();
	    if (infoChannel_.isOpen()) {
		infoChannel_ << Core::XmlEmpty("init-segment-clustering") + Core::XmlAttribute("number-of-clusters", nrcluster_);
	    }
	}

	// Penalty --> how do i record it?

	/** find the best two segments to be merged */
	void findBestPair()
	{
	    bestscore_ = MAXFLOAT;
	    for (u32 i = 0; i < nrsegments_; i++)
		for (u32 j = 0 ;j< i ; j++)
		    if ((matrixTracker_[i]) && (matrixTracker_[j])) {
			if (distMatrix_[i][j] < bestscore_) {
			    bestscore_ = distMatrix_[i][j];
			    parent_ = j;
			    child_ = i;
			}
		    }
	}

	void maxLinkage()
	{
	    for (u32 i = child_ +1; i < nrsegments_; i++)
		if (matrixTracker_[i])
		    distMatrix_[i][parent_] = std::max(distMatrix_[i][parent_], distMatrix_[i][child_]);
	    for (u32 i = parent_+1; i < child_; i++)
		if (matrixTracker_[i])
		    distMatrix_[i][parent_] = std::max(distMatrix_[i][parent_], distMatrix_[child_][i]);
	    for (u32 i = 0 ; i < parent_; i++)
		if (matrixTracker_[i])
		    distMatrix_[parent_][i] = std::max(distMatrix_[parent_][i], distMatrix_[child_][i]);

	}

	void concatenation(std::vector<Model> &models)
	{
	    Model tmpModel(models.back().dim(),alpha_ , models);
	    for(u32 i = 0; i < nrsegments_; i++)
		if (i != parent_){
		    if (matrixTracker_[i])
			tmpModel.mergeModels(modelTracker_[i], modelTracker_[parent_]);
		    if (i < parent_)
			distMatrix_[parent_][i] = tmpModel.score();
		    else distMatrix_[i][parent_] = tmpModel.score();
		}
	}

	void updateDistMat(std::vector<Model> &models)
	{
	    if (amalgamation_ == 1) {//maximum linkage
		maxLinkage();
	    }
	    else { // concatenation
		concatenation(models);
	    }
	}

	void insertTracker()
	{
	    Tracker node;
	    node.update(parent_, child_, bestscore_);
	    mergeTracker_.push_back(node);
	}

	/** merge current parent @c parent_ and child @c child_
	 * previously determined by findBestPair() method
	 */
	void condenseDistMatrix(std::vector<Model> &models)
	{
	    Model newModel(modelTracker_[parent_].dim(), alpha_, models);
	    newModel.mergeModels(modelTracker_[parent_] , modelTracker_[child_]);
	    bestscore_ = newModel.stopscore();
	    bestscore_ -=threshold_; // threshold_ contains also the model-complexity-based penalty P scaled by \lambda
	    --nrcluster_;
	    modelTracker_[parent_] = newModel;
	    matrixTracker_[child_] = 0;
	    classid_[child_] = parent_;
	    /*merge cluster*/
	    if (infoChannel_.isOpen()) {
		infoChannel_ << Core::XmlEmpty("merging-clusters") + Core::XmlAttribute("parent", parent_) + Core::XmlAttribute("child", child_);
	    }
	    insertTracker();
	    updateDistMat(models);
	}

	void loadClustering()
	{
	    char clusterN[8];
	    while (infile_.good()){
		infile_.getline(clusterN, 8);
		if (strcmp(clusterN,""))
		    predefinedClusters_.push_back(atoi(clusterN));
	    }
	    infile_.close();
	}

	void printClustering()
	{
	    Core::XmlOutputStream outfile(outfile_.c_str());
	    if (outfile) {
		outfile << Core::XmlOpen("coprus-key-map");
		for(u32 i = 0; i < defaultIds_.size(); i++)
		{
		    outfile << Core::XmlEmpty("map-item") +
			Core::XmlAttribute("key", defaultIds_[i]) +
			Core::XmlAttribute("value", 0);
		}
		for (u32 i = 0; i<nrsegments_; i++) {
		    outfile << Core::XmlEmpty("map-item") +
			Core::XmlAttribute("key", segmentIds_[i]) +
			Core::XmlAttribute("value", classid_[i] + (defaultIds_.size() ? 1 : 0));
		}
		outfile << Core::XmlClose("coprus-key-map");
	    }
	    else error("error in opening file output file '%s'", outfile_.c_str());
	}

	/** find the best segment index w.r.t. the BIC scores */
	const u32 findBestBIC() const
	{
	    s32 bestIdx = -1;
	    f32 bestScore = 0;
	    f32 currentScore = 0 ;
	    u32 stop = std::max(nrsegments_ - mincluster_, (u32)0);
	    if (nrsegments_ > maxcluster_)
		bestIdx = nrsegments_ - maxcluster_ -1;

	    for (s32 i = bestIdx +1; i < (s32)stop ; i++) {
		currentScore += mergeTracker_[i].score();
		if (currentScore < bestScore) {
		    bestScore = currentScore;
		    bestIdx = i;
		    if (infoChannel_.isOpen()) {
			infoChannel_ << Core::XmlEmpty("find-best-BIC") + Core::XmlAttribute("score", bestScore) + Core::XmlAttribute("cluster", bestIdx) + Core::XmlAttribute("min-cluster", mincluster_) + Core::XmlAttribute("max-cluster", maxcluster_) + Core::XmlAttribute("segments", nrsegments_) +  Core::XmlAttribute("stop", stop);
		    }
		}
	    }
	    return(bestIdx);
	}

	void returnClustering() {
	    s32 stopIdx;
	    stopIdx = findBestBIC();
	    initTracker(nrsegments_);

	    for (s32 i = 0; i <= stopIdx; i++) {
		matrixTracker_[mergeTracker_[i].rightIdx()] = 0;
		classid_[mergeTracker_[i].rightIdx()] = mergeTracker_[i].leftIdx();
	    }

	    for (u32 i=nrsegments_-1; i > 0; i--) {
		u32 j = i;
		while (matrixTracker_[j] ==  0)
		    j = classid_[j];
		classid_[i] = j;
	    }
	    //nrcluster_ = nextclusterid;
	    printClustering();
	}

	void clusterUpdate(std::vector<Model> &models) {
	    if (predefinedClusters_[parent_] >=0)
		for (child_= parent_+1; child_< predefinedClusters_.size(); child_++)
		    if (predefinedClusters_[child_] == predefinedClusters_[parent_]) {
			predefinedClusters_[child_] = -1;
			condenseDistMatrix(models);
		    }
	}


	void integratePredefinedClusters(std::vector<Model> &models)
	{
	    u32 tmpMaxCluster = maxcluster_;
	    maxcluster_ = 1;
	    for ( parent_ =0; parent_ < predefinedClusters_.size(); parent_++)
		clusterUpdate(models);
	    maxcluster_= tmpMaxCluster;
	}

	std::vector<Model> filterModelsByFrames(const std::vector<Model>& models)
	{
	    std::vector<Model> newModels;
	    std::vector<std::string> newSegmentIds;
	    for(u32 i = 0; i< models.size(); ++i)
	    {
	      if(models[i].nFrames() >= minframes_)
	      {
		  newModels.push_back(models[i]);
		  newSegmentIds.push_back(segmentIds_[i]);
	      }else{
		defaultIds_.push_back(segmentIds_[i]);
	      }
	    }
	    segmentIds_ = newSegmentIds;
	    return newModels;
	}

	/** clustering of the finalized models */
	bool cluster(std::vector<Model> &models)
	{
	    models = filterModelsByFrames(models);

	    initSegmentClustering(models);
	    if (predefinedClusters_.size() == models.size())
		integratePredefinedClusters(models);
	    findBestPair();
	    while (nrcluster_ > 1) {
		if (infoChannel_.isOpen()) {
		    infoChannel_ << Core::XmlEmpty("clustering ") + Core::XmlAttribute("number-of-clusters", nrcluster_);
		}
		condenseDistMatrix(models);
		findBestPair();
	    }
	    returnClustering();
	    return(0);
	}
    };

    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    template<class Model>
    class SegmentClusteringNode : public SleeveNode, SegmentClustering<Model> {
    private:
	SegmentModelEstimator<Model> modelEstimator_;
	bool startNewSegment_;

	static Core::ParameterInt paramMinFrames;
	static Core::ParameterInt paramMincluster;
	static Core::ParameterInt paramMaxcluster;
	static Core::ParameterFloat paramThreshold;
	static Core::ParameterFloat paramLambda;
	static Core::ParameterString paramFilename;
	static Core::ParameterString paramInFile;
	static Core::ParameterFloat paramAlpha;
	static Core::ParameterFloat paramAlphaMax;
	static Core::ParameterFloat paramAlphaMin;
	static Core::ParameterInt paramAmalgamation;

	static Core::ParameterString paramId;

    public:
	static std::string filterName() { return "signal-segment-clustering"; }

	SegmentClusteringNode(const Core::Configuration &c):
	    Core::Component(c),
	    SleeveNode(c),
	    SegmentClustering<Model>(c),
	    startNewSegment_(true)
	{
	    addDatatype(Flow::Vector<f32>::type());

	    this->setMinFrames(paramMinFrames(c));
	    this->setMincluster(paramMincluster(c));
	    this->setMaxcluster(paramMaxcluster(c));
	    this->setThreshold(paramThreshold(c));
	    this->setLambda(paramLambda(c));
	    this->setFilename(paramFilename(c));
	    this->setInFile(paramInFile(c));
	    this->setAlpha(paramAlpha(c));
	    this->setAlphaMin(paramAlphaMin(c));
	    this->setAlphaMax(paramAlphaMax(c));
	    this->setAmalgamation(paramAmalgamation(c));
	    this->setId(paramId(c));
	}

	virtual ~SegmentClusteringNode()
	{
	    SegmentClustering<Model>::cluster(modelEstimator_.finalize()); /// @todo: ugly, this should be moved to the work method, but how?
	}

	virtual bool setParameter(const std::string &name, const std::string &value)
	{
	    if (paramMinFrames.match(name))
		this->setMinFrames(paramMinFrames(value));
	    else if (paramMincluster.match(name))
		this->setMincluster(paramMincluster(value));
	    else if (paramMaxcluster.match(name))
		this->setMaxcluster(paramMaxcluster(value));
	    else if (paramThreshold.match(name))
		this->setThreshold(paramThreshold(value));
	    else if (paramLambda.match(name))
		this->setLambda(paramLambda(value));
	    else if (paramFilename.match(name))
		this->setFilename(paramFilename(value));
	    else if (paramInFile.match(name))
		this->setInFile(paramInFile(value));
	    else if (paramAlpha.match(name))
		this->setAlpha(paramAlpha(value));
	    else if (paramAlphaMin.match(name))
		this->setAlphaMin(paramAlphaMin(value));
	    else if (paramAlphaMax.match(name))
		this->setAlphaMax(paramAlphaMax(value));
	    else if (paramAmalgamation.match(name))
		this->setAmalgamation(paramAmalgamation(value));
	    else if (paramId.match(name))
		this->setId(paramId(value));
	    else
		return false;
	    return true;
	}

	virtual bool work(Flow::PortId p)
	{
	    Flow::DataPtr<Flow::Vector<f32> > in;

	    if (getData(0, in)) {
		if (startNewSegment_) {
		    modelEstimator_.pushBackSegment(in->size(),this->alpha());
		    SegmentClustering<Model>::pushBackId();
		    startNewSegment_ = false;
		}

		modelEstimator_.accumulate(*in);
	    } else
		startNewSegment_ = true;

	    return putData(0, in.get());
	}
    };

    template <typename T>
	Core::ParameterInt SegmentClusteringNode<T>::paramMinFrames
	("minframes", "minimum number of frames in a segment to consider the segment for clustering", 0);

    template <typename T>
	Core::ParameterInt SegmentClusteringNode<T>::paramMincluster
	("mincluster", "minimum number of clusters", 2);

    template <typename T>
	Core::ParameterInt SegmentClusteringNode<T>::paramMaxcluster
	("maxcluster", "maximum number of clusters",100000);

    template <typename T>
	Core::ParameterFloat SegmentClusteringNode<T>::paramThreshold
	("threshold", "Threshold for BIC which is added to the model-complexity based penalty", 0);

    template <typename T>
	Core::ParameterFloat SegmentClusteringNode<T>::paramLambda
	("lambda", "Weight for the model-complexity-based penalty (only lambda=1 corresponds to the definition of BIC)", 1);

    template <typename T>
	Core::ParameterString SegmentClusteringNode<T>::paramFilename
	("file", "Name of the cluster outputfile", "cluster.dump");

    template <typename T>
	Core::ParameterString SegmentClusteringNode<T>::paramInFile
	("infile", "Name of inputfile of clusters", "spk.cl");

    template <typename T>
	Core::ParameterFloat SegmentClusteringNode<T>::paramAlphaMin
	("minalpha", "Minimum Alpha scaling value used within distance scaling optimization", 0.40);

    template <typename T>
	Core::ParameterFloat SegmentClusteringNode<T>::paramAlphaMax
	("maxalpha", "Maximum Alpha scaling value used within distance scaling optimization", 0.60);

    template <typename T>
	Core::ParameterFloat SegmentClusteringNode<T>::paramAlpha
	("alpha", "Weighting Factor for correlation-based distance (default is automatic alpha estimation using minalpha and maxalpha values)", -1);

    template <typename T>
	Core::ParameterInt SegmentClusteringNode<T>::paramAmalgamation
	("amalgamation", "Amalgamation Rule 1=Max Linkage, 0=Concatenation ", 0);

    template <typename T>
	Core::ParameterString SegmentClusteringNode<T>::paramId
	("id", "changing the id resets the cached features");

	}
#endif
