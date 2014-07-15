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
#ifndef _SIGNAL_EIGEN_TRANSFORM_HH
#define _SIGNAL_EIGEN_TRANSFORM_HH

#include "ScatterTransform.hh"
#include <Math/EigenvalueProblem.hh>

namespace Signal {

    /**
     *  Base class for eigenvalue analysis based transformations.
     *  Features:
     *   -Generation of projector matrix.
     *   -Basic I/O
     */
    class EigenTransform : public ScatterTransform {
	typedef ScatterTransform Precursor;
    public:
	typedef Math::EigenvalueProblem::ValueType EigenType;
    private:
	static const Core::ParameterInt paramReducedDimension;
	static const Core::ParameterFloat paramReducedDimensionByThreshold;
	static const Core::ParameterFloat paramVarianceProportion;

	mutable Core::XmlChannel resultsChannel_;
    protected:
	Math::Vector<EigenType> eigenvalues_;
	Math::Matrix<EigenType> eigenvectors_;
    private:
	void writeResults(Core::XmlWriter &os) const;
    protected:
	/**
	 *  Performs final steps of calculating the transformation.
	 *  Steps:
	 *    -creating projector
	 */
	bool work();

	void writeResults() const {
	    if (resultsChannel_.isOpen()) writeResults(resultsChannel_);
	}
	bool createProjector();
    public:
	EigenTransform(const Core::Configuration &c);
	~EigenTransform();

	const Math::Vector<EigenType> &eigenvalues() const { return eigenvalues_; }
	const Math::Matrix<EigenType> &eigenvectors() const { return eigenvectors_; }
    };

    /**
     *  Principal Component Analysis.
     *  Input: total scatter matrices (covariance matrix).
     *  Output: projector matrix.
     */
    class PrincipalComponentAnalysis :
	public EigenTransform
    {
	typedef EigenTransform Precursor;
    private:
	Math::EigenvalueProblem *eigenvalueProblem_;
	mutable Core::XmlChannel covarianceMatrixChannel_;
    public:
	PrincipalComponentAnalysis(const Core::Configuration &);
	~PrincipalComponentAnalysis();
	/**
	 *  Solves generalized eigenvalue problem on the parameter matrices.
	 */
	bool work(const ScatterMatrix &covarianceMatrix);
	/**
	 *  Solves generalized eigenvalue problem.
	 *  covariance matrix are read from files.
	 */
	bool work();
    };

    /**
     *  Linear Discriminant Analysis
     *  Input: between class and within class scatter matrices.
     *  Output: projector matrix.
     */
    class LinearDiscriminantAnalysis :  public EigenTransform {
	typedef EigenTransform Precursor;
    private:
	/**
	 *  Generalized eigenvalue problem solver used for calculating LDA matrix.
	 */
	Math::GeneralizedEigenvalueProblem *generalizedEigenvalueProblem_;
	/**
	 *  Eigenvalue problem solver used when analyzing scatter matrices.
	 */
	Math::EigenvalueProblem *eigenvalueProblem_;

	mutable Core::XmlChannel betweenClassScatterMatrixChannel_;
	mutable Core::XmlChannel withinClassScatterMatrixChannel_;
    public:
	LinearDiscriminantAnalysis(const Core::Configuration &c);
	~LinearDiscriminantAnalysis();

	/**
	 *  Solves generalized eigenvalue problem on the parameter matrices.
	 */
	bool work(const ScatterMatrix &betweenClassScatterMatrix,
		  const ScatterMatrix &withinClassScatterMatrix);
	/**
	 *  Solves generalized eigenvalue problem.
	 *  Scatter matrices are read form files.
	 */
	bool work();
    };

} //namespace Signal

#endif // _SIGNAL_EIGEN_TRANSFORM_HH
