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
#ifndef _MATH_EIGENVALUE_PROBLEM_HH
#define _MATH_EIGENVALUE_PROBLEM_HH

#include <complex>
#include <Core/Component.hh>
#include <Math/Matrix.hh>
#include <Math/Vector.hh>

namespace Math {

    /**
     *  Base class for eigenvalue problems.
     */
    class EigenvalueProblem : public virtual Core::Component {
    public:
	typedef f64 ValueType;
    private:
	/**
	 *  Normalization type eigenvectors.
	 *  Eigenvectors are scaled such that:
	 *  -nonEigenvectorNormalization: no scaling;
	 *  -unityLength: 2nd norm of eigenvectors is one;
	 *  -unityDiagonal: eigenvector^T * A * eigenvector = I.
	 */
	enum EigenvectorNormalizationType {
	    nonEigenvectorNormalization,
	    unityLength,
	    unityDiagonal,
	};
	static const Core::Choice choiceEigenvectorNormalizationType;
	static const Core::ParameterChoice paramEigenvectorNormalization;
	static const Core::ParameterFloat paramDiagonalNormalizationTolerance;

	static const Core::ParameterFloat paramVerificationTolerance;
	static const Core::ParameterFloat paramRelativeImaginaryMaximum;

	enum EigenvalueSortType {
	    noSort,
	    decreasing,
	    increasing,
	};
	static const Core::Choice choiceEigenvalueSortType;
	static const Core::ParameterChoice paramEigenvalueSort;
    public:
	enum Type {
	    typeGeneral,
	    typeSymmetric,
	    typeSymmetricPositiveDefinite
	};
	static const Core::Choice choiceType;
	static const Core::ParameterChoice paramType;
    private:
	EigenvectorNormalizationType eigenvectorNormalizationType_;
	ValueType diagonalNormalizationTolerance_;
	ValueType relativeImaginaryMaximum_;
	ValueType verificationTolerance_;
	EigenvalueSortType eigenvalueSortType_;
    protected:
	/**
	 *  @return is true if x and y are equal with a floating point
	 *  tolerance of verificationTolerance_.
	 *  If @return is false, error message is generated.
	 */
	bool areEqual(const Matrix<ValueType> &x, const Matrix<ValueType> &y);

	/**
	 *  Eigenvectors are scaled so that eigenvector^T * A * eigenvector = I.
	*/
	bool normalizeDiagonal(const Matrix<ValueType> &a,
			       Matrix<ValueType> &eigenvectors);
    public:
	EigenvalueProblem(const Core::Configuration &);

	/**
	 *  Interface function for symmetric eigenvalue problem.
	 */
	virtual bool solveSymmetric(const Matrix<ValueType> &a,
				    Vector<ValueType> &eigenvalues,
				    Matrix<ValueType> &eigenvectors) = 0;
	/**
	 *  Solves the symmetric eigenvalue problem and performs finalization of results.
	 *  If @return is false, depending on the type of error, results can be stored in
	 *  @c eigenvalues and @c eigenvectors
	 */
	bool solveSymmetricAndFinalize(const Matrix<ValueType> &a,
				       Vector<ValueType> &eigenvalues,
				       Matrix<ValueType> &eigenvectors);

	/**
	 *  Performs final processing and verifications.
	 *  1) Performs final processing of eigenvalues and eigenvectors.
	 *  2) Verifies solution.
	 */
	bool finalize(const Matrix<ValueType> &a,
		      Vector<ValueType> &eigenvalues,
		      Matrix<ValueType> &eigenvectors);
	/**
	 *  Performs final processing of eigenvalues and eigenvectors.
	 *  1) Normalizes eigenvectors (configurable)
	 *  2) Sorts eigenvalues
	 */
	bool finalizeEigenvaluesAndVectors(
	    const Matrix<ValueType> &a,
	    Vector<ValueType> &eigenvalues,
	    Matrix<ValueType> &eigenvectors);

	/**
	 *  Verifies if eigenvectors and eigenvalues satisfy A*X = Lambda*B*X
	 */
	bool isCorrectSolution(const Matrix<ValueType> &a,
			       const Vector<ValueType> &eigenvalues,
			       const Matrix<ValueType> &eigenvectors);
	/**
	 *  Sorts eigenvalues in descending order.
	 *  Eigenvectors are permuted according to eigenvalues.
	 */
	void sortEigenvalues(Vector<ValueType> &eigenvalues,
			     Matrix<ValueType> &eigenvectors);

	/**
	 *  Extracts real part of the input vector.
	 *  If one of the imaginary parts are larger than imaginaryMaximum_
	 *  then @return is false.
	 */
	bool real(const Vector<std::complex<ValueType> > &,
		  Vector<ValueType> &);
	/**
	 *  Extracts real part of the input matrix.
	 *  If one of the imaginary parts are larger than imaginaryMaximum_
	 *  then @return is false.
	 */
	bool real(const Matrix<std::complex<ValueType> > &,
		  Matrix<ValueType> &);
    };

    /**
     *  Base class for generalized eigenvalue problems.
     */
    class GeneralizedEigenvalueProblem : public EigenvalueProblem {
    private:
	static const Core::ParameterBool paramNormalizeEigenvectorsUsingB;
    private:
	bool normalizeEigenvectorsUsingB_;
    public:
	GeneralizedEigenvalueProblem(const Core::Configuration &);

	virtual bool solveSymmetric(const Matrix<ValueType> &a,
				    Vector<ValueType> &eigenvalues,
				    Matrix<ValueType> &eigenvectors) {
	    return solveSymmetric(a, makeDiagonalMatrix(a.nColumns(), (ValueType)1.0),
				  eigenvalues, eigenvectors);
	}

	/**
	 *  Interface function for generalized symmetric eigenvalue problem.
	 */
	virtual bool solveSymmetric(const Matrix<ValueType> &a,
				    const Matrix<ValueType> &b,
				    Vector<ValueType> &eigenvalues,
				    Matrix<ValueType> &eigenvectors) = 0;
	/**
	 *  Solves the symmetric generalized eigenvalue problem and performs finalization of results.
	 *  If @return is false, depending on the type of error, results can be stored in
	 *  @c eigenvalues and @c eigenvectors
	 */
	bool solveSymmetricAndFinalize(const Matrix<ValueType> &a,
				       const Matrix<ValueType> &b,
				       Vector<ValueType> &eigenvalues,
				       Matrix<ValueType> &eigenvectors);

	/**
	 *  Performs final processing of eigenvalues and eigenvectors.
	 *  1) Normalizes eigenvectors (configurable)
	 *  2) Sorts eigenvalues
	 */
	bool finalizeEigenvaluesAndVectors(
	    const Matrix<ValueType> &a,
	    const Matrix<ValueType> &b,
	    Vector<ValueType> &eigenvalues,
	    Matrix<ValueType> &eigenvectors);

	/**
	 *  Performs final processing and verifications.
	 *  1) Performs final processing of eigenvalues and eigenvectors.
	 *  2) Verifies solution.
	 */
	bool finalize(const Matrix<ValueType> &a,
		      const Matrix<ValueType> &b,
		      Vector<ValueType> &eigenvalues,
		      Matrix<ValueType> &eigenvectors);

	/**
	 *  Verifies if eigenvectors and eigenvalues satisfy A*X = Lambda*B*X
	 */
	bool isCorrectSolution(const Matrix<ValueType> &a,
			       const Matrix<ValueType> &b,
			       const Vector<ValueType> &eigenvalues,
			       const Matrix<ValueType> &eigenvectors);
    };
 } // namespace Math

#endif // _MATH_EIGENVALUE_PROBLEM_HH
