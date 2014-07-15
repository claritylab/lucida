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
#ifndef _SIGNAL_SCATTER_ESTIMATOR_HH
#define _SIGNAL_SCATTER_ESTIMATOR_HH

#include <Core/Component.hh>
#include <Math/Matrix.hh>
#include <numeric>

namespace Signal {

    extern const std::string totalScatterType;
    extern const std::string betweenClassScatterType;
    extern const std::string withinClassScatterType;

    /**
     *  Base class for scatter matrix estimators.
     *  Features:
     *   -Supports accumulation of x_i*x_i^T.
     *   -Basic I/O
     */
    class ScatterMatrixEstimator : virtual public Core::Component, public Core::ReferenceCounted {
	typedef Core::Component Precursor;
    public:
	typedef u32 ClassIndex;
	typedef f32 Data;
	typedef f64 Sum;
	typedef f64 Count;
	typedef Math::Matrix<Sum> ScatterMatrix;
    public:
	static const Core::ParameterInt paramOutputPrecision;
	static const Core::ParameterBool paramShallNormalize;
    protected:
	size_t featureDimension_;

	/** Accumulates covariance matrix
	 *  Remark: accumulate only lower triangle, because of symmetry
	 */
	Math::Matrix<Sum> vectorSquareSum_;

	bool needInit_;
    private:
	/** Copies the lower (not accumulated) triangle to the upper one. */
	void finalizeVectorSquareSum();
    protected:
	void initialize();
	void accumulate(const Math::Vector<Data> &, f32 weight = 1.0);
	bool accumulate(const ScatterMatrixEstimator &);
	bool finalize();
	bool write(const std::string &filename, const std::string &scatterType,
		   const ScatterMatrix &scatterMatrix);
	virtual bool read(Core::BinaryInputStream &);
	virtual bool write(Core::BinaryOutputStream &);
    public:
	ScatterMatrixEstimator(const Core::Configuration &c);
	~ScatterMatrixEstimator();

	void setDimension(size_t dimension);
    };

    /**
     *  Scatter matrix type;
     */
    typedef ScatterMatrixEstimator::ScatterMatrix ScatterMatrix;

    /**
     *  Estimator class for total scatter(covariance) matrix.
     */
    class TotalScatterMatrixEstimator :
	public ScatterMatrixEstimator
    {
	typedef ScatterMatrixEstimator Precursor;
    private:
	static const Core::ParameterString paramFilename;
	static const Core::ParameterFloat paramElementThresholdMin;
    protected:
	/* Accumulates vector sum. */
	Math::Vector<Sum> vectorSum_;
	/* Number of observations. */
	Count count_;
    protected:
	void initialize();
    public:
	TotalScatterMatrixEstimator(const Core::Configuration &c);
	~TotalScatterMatrixEstimator();

	void accumulate(const Math::Vector<Data>&);
	bool finalize(ScatterMatrix &covarianceMatrix);
	/**
	 *  Saves covariance matrix.
	 *  Calls finalize and saves covariance matrix.
	 */
	bool write();
    };

    /**
     *  Estimator class for between and within class scatter matrices.
     */
    class ScatterMatricesEstimator: public ScatterMatrixEstimator {
	typedef ScatterMatrixEstimator Precursor;
    public:
	static const Core::ParameterString paramBetweenClassScatterFilename;
	static const Core::ParameterString paramWithinClassScatterFilename;
	static const Core::ParameterString paramTotalScatterFilename;

	static const Core::ParameterString paramOldAccumulatorFilename;
	static const Core::ParameterString paramNewAccumulatorFilename;
	static const Core::ParameterStringVector paramAccumulatorFilesToCombine;

    private:
	ClassIndex nClasses_;

	/* Accumulates vector per class */
	std::vector<Math::Vector<Sum> > vectorSums_;
	/* Class frequency */
	std::vector<Count> counts_;
    private:
	void initialize(bool deepInitialization = true);

	Count getTotalCount() const { return std::accumulate(counts_.begin(), counts_.end(), Count(0)); }
	Math::Vector<Sum> getTotalVectorSum() const;
	bool writeAccumulators();
	bool writeMatrices();
    protected:
	virtual bool read(Core::BinaryInputStream &);
	virtual bool write(Core::BinaryOutputStream &);
    public:
	ScatterMatricesEstimator(const Core::Configuration &c);
	~ScatterMatricesEstimator();

	void setNumberOfClasses(size_t nClasses);

	void accumulate(ClassIndex classIndex, const Math::Vector<Data>&, f32 weight = 1);
	bool accumulate(const ScatterMatricesEstimator &);
	bool finalize(ScatterMatrix &betweenClassScatterMatrix,
		      ScatterMatrix &withinClassScatterMatrix,
		      ScatterMatrix &totalScatterMatrix);
	/**
	 *  Saves scatter matrices.
	 *  Calls finalize and saves scatter  matrices.
	 */
	bool write();
	bool load();

	bool loadAccumulatorFile(const std::string &);
	void addAccumulatorFiles(const std::vector<std::string> &);
    };

} //namespace Signal

#endif // _SIGNAL_SCATTER_ESTIMATOR_HH
