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
#ifndef _MATH_LAPACK_EIGENVALUE_PROBLEM_HH
#define _MATH_LAPACK_EIGENVALUE_PROBLEM_HH

#include "Lapack.hh"
#include <Math/EigenvalueProblem.hh>

namespace Math { namespace Lapack {

    /**
     *  Helper class for LAPACK eigenvalue problems.
     */
    class EigenvalueProblem : public virtual Core::Component {
    protected:
	/**
	 *  Different implementations types of the same algorithm
	 *  -"expert" driver supports all features (e.g. eigenvalue selection,
	 *   condition numbers, etc.)
	 *  -"relatively-robust" driver chooses optimal drivers depending on the input.
	 *  -"divide-and-conquer" driver is faster than other routines but
	 *   consumes more memory.
	 */
	enum DriverType {
	    driverExpert,
	    driverRelativelyRobust,
	    driverDivideAndConquer,
	};
    private:
	static const Core::ParameterFloat paramEigenvalueLowerBound;
	static const Core::ParameterFloat paramEigenvalueUpperBound;
	static const Core::Choice choiceDriverType;
	static const Core::ParameterChoice paramDriverType;
	static const Core::ParameterBool paramShouldSupportEigenvectors;
    private:
	double eigenvalueLowerBound_;
	double eigenvalueUpperBound_;
	DriverType driverType_;
	bool shouldSupportEigenvectors_;
    protected:
	bool isEigenvalueRangeValid() const;
	char translateEigenvalueRange() const;
	double eigenvalueLowerBound() const { return eigenvalueLowerBound_; }
	double eigenvalueUpperBound() const { return eigenvalueUpperBound_; }
	DriverType driverType() const { return driverType_; }
	bool shouldSupportEigenvectors() const { return shouldSupportEigenvectors_; }

	void supportEigenvectors(const Matrix<int, RowByRowStorage> &isuppz,
				 Matrix<double> &eigenvectors) const;
    public:
	EigenvalueProblem(const Core::Configuration &);
    };

    /**
     *  Wrapper for symmetric eigenvalue problem.
     */
    class SymmetricEigenvalueProblem :
	public Math::EigenvalueProblem, public EigenvalueProblem {
    private:
	bool solveWithExpertOrRelativelyRobustDriver(
	    const Math::Matrix<ValueType> &a,
	    Math::Vector<ValueType> &eigenvalues,
	    Math::Matrix<ValueType> &eigenvectors) const;
	/**
	 *  Expert LAPACK driver which solves the symmetric eigenvalue problem.
	 *  This expert driver function can calculate a set of eigenvalues defined by
	 *  (eigen-value-lower-bound..eigen-value-upper-bound]. Number of calculated eigenvalues is
	 *  returned in @c nEigenvalues. Eigenvectors are returned in @c eigenvectors,
	 *  eigenvalues in @c eigenvalues.
	 */
	bool dsyevx(Matrix<double> &a,
		    int &nEigenvalues, Vector<double> &eigenvalues,
		    Matrix<double> &eigenvectors) const;
	/**
	 *  Relatively robust LAPACK driver which solves the symmetric eigenvalue problem.
	 *  This driver function can calculate a set of eigenvalues defined by
	 *  (eigen-value-lower-bound..eigen-value-upper-bound]. Number of calculated eigenvalues is
	 *  returned in @c nEigenvalues. Eigenvectors are returned in @c eigenvectors,
	 *  eigenvalues in @c eigenvalues.
	 *  If  shouldSupportEigenvectors() is true, elements of the eigenvectors smaller than
	 *  a threshold are set to zero. (@see also LAPACK manual variable izuppz).
	 */
	bool dsyevr(Matrix<double> &a,
		    int &nEigenvalues, Vector<double> &eigenvalues,
		    Matrix<double> &eigenvectors) const;

	bool solveWithDivideAndConquerDriver(const Math::Matrix<ValueType> &a,
					     Math::Vector<ValueType> &eigenvalues,
					     Math::Matrix<ValueType> &eigenvectors) const;
	/**
	 *  Divide and conquer LAPACK driver which solves the symmetric eigenvalue problem.
	 *  Eigenvectors are returned in @c a, eigenvalues in @c eigenvalues.
	 */
	bool dsyevd(Matrix<double> &a, Vector<double> &eigenvalues) const;
    public:
	SymmetricEigenvalueProblem(const Core::Configuration &);

	virtual bool solveSymmetric(const Math::Matrix<ValueType> &a,
				    Math::Vector<ValueType> &eigenvalues,
				    Math::Matrix<ValueType> &eigenvectors) {
	    return solve(a, eigenvalues, eigenvectors);
	}
	bool solve(const Math::Matrix<ValueType> &a,
		   Math::Vector<ValueType> &eigenvalues,
		   Math::Matrix<ValueType> &eigenvectors);
    };

    /**
     *  Wrapper for generalized generalized eigenvalue problem.
     */
    class GenEigenProblemWithSchurDecomposition :
	public Math::GeneralizedEigenvalueProblem, public EigenvalueProblem {
    private:
	/**
	 *  If input matrices have widely varying magnitudes
	 *  then they should be first balanced.
	 *  The algorithm is O(n^2).
	 *  Balancing a matrix pair (A,B) includes, first,
	 *  permuting rows and columns to isolate eigenvalues,
	 *  second, applying diagonal similarity transformation
	 *  to the rows and columns to make the rows and columns
	 *  as close in norm as possible. The computed reciprocal
	 *  condition numbers correspond to the balanced matrix.
	 *  Permuting rows and columns will not change the condition
	 *  numbers (in exact arithmetic) but diagonal scaling will.
	 *  For further explanation of balancing, see section 4.11.1.2
	 *  of LAPACK Users' Guide.
	 */
	enum BalancingType {
	    noneBalancing,
	    permute,
	    scale,
	    permuteAndScale
	};
    private:
	static const Core::ParameterFloat paramRelativeAlphaMinimum;
	static const Core::ParameterFloat paramRelativeBetaMinimum;
	static const Core::Choice choiceBalancing;
	static const Core::ParameterChoice paramBalancing;
    private:
	double relativeAlphaMinimum_;
	double relativeBetaMinimum_;
	mutable Core::XmlChannel conditionNumberChannel_;
    private:
	bool solveWithExpertDriver(
	    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
	    Math::Vector<std::complex<ValueType> > &eigenvalues,
	    Math::Matrix<std::complex<ValueType> > &leftEigenvectors,
	    Math::Matrix<std::complex<ValueType> > &rightEigenvectors) const;
	/** LAPACK function which solves the generalized eigenvalue problem.
	 *  Eigenvalues are returned in @c alphar, @c alphar, and @c beta.
	 *  Left and right eigenvectors are returned in @c vl and @c vr.
	 *  @see copyEigenvalues and copyEigenvectors.
	 */
	bool dggevx(
	    Matrix<double> &a, Matrix<double> &b,
	    Vector<double> &realAlphas, Vector<double> &imaginaryAlphas, Vector<double> &betas,
	    Matrix<double> &leftEigenvectors, Matrix<double> &rightEigenvectors,
	    Vector<double> &eigenvalueReciprocalConditionNumbers,
	    Vector<double> &eigenvectorReciprocalConditionNumbers,
	    double &oneNormOfBalancedA, double &oneNormOfBalancedB,
	    double &leftMaxPerMinScaling, double &rightMaxPerMinScaling) const;

	double maxPerMinScaling(const Vector<double> &scales, int ilo, int ihi) const;

	/**
	 *  Copies alphar and alphai in std::complex array.
	 */
	void copyAlphas(const Vector<double> &realAlphas, const Vector<double> &imaginaryAlphas,
			Math::Vector<std::complex<ValueType> > &betas) const;
	/**
	 *  Copies eigenvalues form the LAPACK data structures @c alphas,
	 *  and @c betas.
	 */
	void copyEigenvalues(
	    const Math::Vector<std::complex<ValueType> > &alphas, const Vector<double> &betas,
	    Math::Vector<std::complex<ValueType> > &eigenvalues) const;
	/**
	 *  Calculates the eigenvalue from alpha and beta.
	 *  Warnings are generated if abs(alpha) < alphaMinimum or abs(beta) < betaMinimum.
	 *  For these pairs resulting eigenvalue is set to zero.
	 */
	std::complex<ValueType> calculateEigenvalue(
	    std::complex<ValueType> alpha, ValueType alphaMinimum,
	    ValueType beta, ValueType betaMinimum,
	    std::string &warningDescription) const;
	/**
	 *  Copies eigenvectors form the LAPACK data structure @c v.
	 *  @c alphas are necessary to decide if column of @c v contain
	 *  real values or complex conjugate pairs.
	 */
	void copyEigenvectors(
	    const Math::Vector<std::complex<ValueType> > &alphas, const Matrix<ValueType> &v,
	    Math::Matrix<std::complex<ValueType> > &eigenvectors) const;

	void writeConditionNumbers(
	    Core::XmlWriter &os,
	    const Math::Vector<std::complex<ValueType> > &alphas,
	    const Vector<double> &betas,
	    const Math::Vector<std::complex<ValueType> > &eigenvalues,
	    const Vector<double> &eigenvalueReciprocalConditionNumbers,
	    const Vector<double> &eigenvectorReciprocalConditionNumbers,
	    double oneNormOfBalancedA, double oneNormOfBalancedB,
	    double &leftMaxPerMinScaling, double &rightMaxPerMinScaling) const;

	char translateBalancingType(BalancingType) const;
	char translateSensitivity() const;
    public:
	GenEigenProblemWithSchurDecomposition(const Core::Configuration &);

	virtual bool solveSymmetric(const Math::Matrix<ValueType> &a,
				    const Math::Matrix<ValueType> &b,
				    Math::Vector<ValueType> &eigenvalues,
				    Math::Matrix<ValueType> &eigenvectors);

	bool solve(const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
		   Math::Vector<std::complex<ValueType> > &eigenvalues,
		   Math::Matrix<std::complex<ValueType> > &leftEigenvectors,
		   Math::Matrix<std::complex<ValueType> > &rightEigenvectors);
    };

    /**
     *  Wrapper for generalized symmetric positive definite eigenvalue problem.
     */
    class GenSymmetricDefiniteEigenProblem :
	public Math::GeneralizedEigenvalueProblem, public EigenvalueProblem {
    private:
	bool solveWithExpertDriver(
	    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
	    Math::Vector<ValueType> &eigenvalues, Math::Matrix<ValueType> &eigenvectors) const;
	/**
	 *  LAPACK driver function which solves the generalized symmetric definite eigenvalue problem.
	 *  This expert driver function can calculate a set of eigenvalues defined by
	 *  (eigen-value-lower-bound..eigen-value-upper-bound]. Number of calculated eigenvalues is
	 *  returned in @c nEigenvalues. Eigenvectors are returned in @c eigenvectors,
	 *  eigenvalues in @c eigenvalues.
	 */
	bool dsygvx(Matrix<double> &a, Matrix<double> &b,
		    int &nEigenvalues, Vector<double> &eigenvalues,
		    Matrix<double> &eigenvectors) const;
	bool isEigenvalueRangeValid() const;

	bool solveWithDivideAndConquerDriver(
	    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
	    Math::Vector<ValueType> &eigenvalues, Math::Matrix<ValueType> &eigenvectors) const;
	/**
	 *  LAPACK driver function which solves the generalized symmetric definite eigenvalue problem
	 *  Eigenvectors are returned in @c a, eigenvalues in @c eigenvalues.
	 *
	 *  A divide-and-conquer driver (name ending -GVD) solves the same problem as the simple
	 *  driver. It is much faster than the simple driver for large matrices, but uses more
	 *  workspace. The name divide-and-conquer refers to the underlying algorithm.
	 */
	bool dsygvd(Matrix<double> &a, Matrix<double> &b, Vector<double> &eigenvalues) const;
    public:
	GenSymmetricDefiniteEigenProblem(const Core::Configuration &c);

	virtual bool solveSymmetric(const Math::Matrix<ValueType> &a,
				    const Math::Matrix<ValueType> &b,
				    Math::Vector<ValueType> &eigenvalues,
				    Math::Matrix<ValueType> &eigenvectors) {
	    return solve(a, b, eigenvalues, eigenvectors);
	}

	bool solve(const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
		   Math::Vector<ValueType> &eigenvalues, Math::Matrix<ValueType> &eigenvectors);
    };

} } // namespace Math::Lapack

#endif // _MATH_LAPACK_EIGENVALUE_PROBLEM_HH
