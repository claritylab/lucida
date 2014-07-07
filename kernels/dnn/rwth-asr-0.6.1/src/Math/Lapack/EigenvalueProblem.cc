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
#include "EigenvalueProblem.hh"
#include <Math/Complex.hh>

using namespace Math::Lapack;

//--------------------------------------------------------------------------------------------------------
const Core::ParameterFloat EigenvalueProblem::paramEigenvalueLowerBound(
    "eigenvalue-lower-bound",
    "eigenvalues larger than this value are only searched.",
    Core::Type<double>::min);
const Core::ParameterFloat EigenvalueProblem::paramEigenvalueUpperBound(
    "eigenvalue-upper-bound",
    "eigenvalues smaller or equal this value are only searched.",
    Core::Type<double>::min);
const Core::Choice EigenvalueProblem::choiceDriverType(
    "expert", driverExpert,
    "relatively-robust", driverRelativelyRobust,
    "divide-and-conquer", driverDivideAndConquer,
    Core::Choice::endMark());
const Core::ParameterChoice EigenvalueProblem::paramDriverType(
    "driver", &choiceDriverType, "type of driver routine", driverExpert);
const Core::ParameterBool EigenvalueProblem::paramShouldSupportEigenvectors(
    "support-eigenvectros",
    "zero those eigenvector elements which the algorithm proposes.",
    false);

EigenvalueProblem::EigenvalueProblem(const Core::Configuration &c) :
    Core::Component(c),
    eigenvalueLowerBound_(paramEigenvalueLowerBound(c)),
    eigenvalueUpperBound_(paramEigenvalueUpperBound(c)),
    driverType_((DriverType)paramDriverType(c)),
    shouldSupportEigenvectors_(paramShouldSupportEigenvectors(c))
{}

bool EigenvalueProblem::isEigenvalueRangeValid() const
{
    if (eigenvalueLowerBound_ < eigenvalueUpperBound_)
	return true;
    if (eigenvalueLowerBound_ == Core::Type<double>::min &&
	eigenvalueLowerBound_ == eigenvalueUpperBound_)
	return false;
    warning("Inconsistent eigenvalue range (%f, %f]. All eigenvalues will be calculated.",
	    (double)eigenvalueLowerBound_, (double)eigenvalueUpperBound_);
    return false;
}

char EigenvalueProblem::translateEigenvalueRange() const
{
    char result = 'A';
    if (isEigenvalueRangeValid())
	result = 'V';
    return result;
}

void EigenvalueProblem::supportEigenvectors(
    const Matrix<int, RowByRowStorage> &isuppz, Matrix<double> &eigenvectors) const
{
    size_t n = eigenvectors.nColumns();
    for (size_t i = 0; i < n; ++ i) {
	size_t firstNoneZero = isuppz(i, 0);
	verify_(firstNoneZero < n);
	size_t lastNoneZero = isuppz(i, 1);
	verify_(firstNoneZero <= lastNoneZero && lastNoneZero < n);
	for(size_t j = 1; j < firstNoneZero; ++ j)
	    eigenvectors(j - 1, i) = 0;
	for(size_t j = lastNoneZero + 1; j <= n; ++ j)
	    eigenvectors(j - 1, i) = 0;
    }
}

//--------------------------------------------------------------------------------------------------------
SymmetricEigenvalueProblem::SymmetricEigenvalueProblem(const Core::Configuration &c) :
    Core::Component(c),
    Math::EigenvalueProblem(c),
    Math::Lapack::EigenvalueProblem(c)
{}

bool SymmetricEigenvalueProblem::solve(
    const Math::Matrix<ValueType> &a,
    Math::Vector<ValueType> &eigenvalues,
    Math::Matrix<ValueType> &eigenvectors)
{
    switch(driverType()) {
    case driverExpert:
    case driverRelativelyRobust:
	return solveWithExpertOrRelativelyRobustDriver(a, eigenvalues, eigenvectors);
    case driverDivideAndConquer:
	return solveWithDivideAndConquerDriver(a, eigenvalues, eigenvectors);
    default:
	defect();
    }
}

bool SymmetricEigenvalueProblem::solveWithExpertOrRelativelyRobustDriver(
    const Math::Matrix<ValueType> &a,
    Math::Vector<ValueType> &eigenvalues,
    Math::Matrix<ValueType> &eigenvectors) const
{
    Matrix<double> lapackA; copy(a, lapackA);
    int nEigenvalues;
    Vector<double> lapackEigenvalues;
    Matrix<double> lapackEigenvectors;
    switch(driverType()) {
    case driverExpert:
	if (!dsyevx(lapackA, nEigenvalues, lapackEigenvalues, lapackEigenvectors))
	    return false;
	break;
    case driverRelativelyRobust:
	if (!dsyevr(lapackA, nEigenvalues, lapackEigenvalues, lapackEigenvectors))
	    return false;
	break;
    default:
	defect();
    }
    copy(lapackEigenvalues, eigenvalues, nEigenvalues);
    copy(lapackEigenvectors, eigenvectors, nEigenvalues);
    return true;
}

bool SymmetricEigenvalueProblem::dsyevx(Matrix<double> &a,
					int &nEigenvalues, Vector<double> &eigenvalues,
					Matrix<double> &eigenvectors) const
{
    char jobz = 'V';
    char range = translateEigenvalueRange();
    char uplo = 'U';

    int n = a.nRows();
    int lda = n;
    double vl = eigenvalueLowerBound();
    double vu = eigenvalueUpperBound();
    int il, iu;
    double abstol = 0;
    eigenvalues.resize(n);
    eigenvectors.resize(n, n);
    int ldz = n;
    Vector<double> work(1);
    int lwork = -1;
    Vector<int> iwork(5 * n);
    Vector<int> ifail(n);
    int info;

    Lapack::dsyevx(&jobz, &range, &uplo, &n, a.buffer(), &lda, &vl, &vu, &il, &iu, &abstol, &nEigenvalues,
		   eigenvalues.buffer(), eigenvectors.buffer(), &ldz, work.buffer(), &lwork,
		   iwork.buffer(), ifail.buffer(), &info);
    if (info != 0) {
	error("Lapack error code %d recieved while workspace query.", info);
	return false;
    }
    work.resize(lwork = (int)work[0]);

    Lapack::dsyevx(&jobz, &range, &uplo, &n, a.buffer(), &lda, &vl, &vu, &il, &iu, &abstol, &nEigenvalues,
		   eigenvalues.buffer(), eigenvectors.buffer(), &ldz, work.buffer(), &lwork,
		   iwork.buffer(), ifail.buffer(), &info);
    if (info != 0) {
	error("Lapack error code %d recieved.", info);
	return false;
    }
    return true;
}

bool SymmetricEigenvalueProblem::dsyevr(Matrix<double> &a,
					int &nEigenvalues, Vector<double> &eigenvalues,
					Matrix<double> &eigenvectors) const
{
    char jobz = 'V';
    char range = translateEigenvalueRange();
    char uplo = 'U';

    int n = a.nRows();
    int lda = n;
    double vl = eigenvalueLowerBound();
    double vu = eigenvalueUpperBound();
    int il, iu;
    double abstol = 0;
    eigenvalues.resize(n);
    eigenvectors.resize(n, n);
    int ldz = n;
    Matrix<int, RowByRowStorage> isuppz(n, 2);
    Vector<double> work(1);
    int lwork = -1;
    Vector<int> iwork(1);
    int liwork = -1;
    int info;

    Lapack::dsyevr(&jobz, &range, &uplo, &n, a.buffer(), &lda, &vl, &vu, &il, &iu, &abstol, &nEigenvalues,
		   eigenvalues.buffer(), eigenvectors.buffer(), &ldz, isuppz.buffer(), work.buffer(), &lwork,
		   iwork.buffer(), &liwork, &info);
    if (info != 0) {
	error("Lapack error code %d recieved while workspace query.", info);
	return false;
    }
    work.resize(lwork = (int)work[0]);
    iwork.resize(liwork = iwork[0]);

    Lapack::dsyevr(&jobz, &range, &uplo, &n, a.buffer(), &lda, &vl, &vu, &il, &iu, &abstol, &nEigenvalues,
		   eigenvalues.buffer(), eigenvectors.buffer(), &ldz, isuppz.buffer(), work.buffer(),
		   &lwork, iwork.buffer(), &liwork, &info);
    if (info != 0) {
	error("Lapack error code %d recieved.", info);
	return false;
    }
    if (info == 0 && shouldSupportEigenvectors())
	supportEigenvectors(isuppz, eigenvectors);
    return true;
}

bool SymmetricEigenvalueProblem::solveWithDivideAndConquerDriver(
    const Math::Matrix<ValueType> &a,
    Math::Vector<ValueType> &eigenvalues,
    Math::Matrix<ValueType> &eigenvectors) const
{
    Matrix<double> lapackA; copy(a, lapackA);
    Vector<double> lapackEigenvalues;
    if (!dsyevd(lapackA, lapackEigenvalues))
	return false;
    copy(lapackEigenvalues, eigenvalues);
    copy(lapackA, eigenvectors);
    return true;
}

bool SymmetricEigenvalueProblem::dsyevd(Matrix<double> &a, Vector<double> &eigenvalues) const
{
    char jobz = 'V';
    char uplo = 'U';

    int n = a.nRows();
    int lda = n;
    eigenvalues.resize(n);
    Vector<double> work(1);
    int lwork = -1;
    Vector<int> iwork(1);
    int liwork = -1;
    int info;

    Lapack::dsyevd(&jobz, &uplo, &n, a.buffer(), &lda, eigenvalues.buffer(), work.buffer(), &lwork,
		   iwork.buffer(), &liwork, &info);
    if (info != 0) {
	error("Lapack error code %d recieved while workspace query.", info);
	return false;
    }
    work.resize(lwork = (int)work[0]);
    iwork.resize(liwork = iwork[0]);

    Lapack::dsyevd(&jobz, &uplo, &n, a.buffer(), &lda, eigenvalues.buffer(), work.buffer(), &lwork,
		   iwork.buffer(), &liwork, &info);
    if (info != 0) {
	error("Lapack error code %d recieved.", info);
	return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------------
const Core::ParameterFloat GenEigenProblemWithSchurDecomposition::paramRelativeAlphaMinimum(
    "relative-alpha-minimum",
    "If abs(alpha) smaller than max(abs(alphas)) * this, eigenvalue is set to zero.",
    Core::Type<f64>::epsilon, 0, 1);
const Core::ParameterFloat GenEigenProblemWithSchurDecomposition::paramRelativeBetaMinimum(
    "relative-beta-minimum",
    "If abs(beta) smaller than max(abs(betas)) * this, eigenvalue is set to zero.",
    Core::Type<f64>::epsilon, 0, 1);
const Core::Choice GenEigenProblemWithSchurDecomposition::choiceBalancing(
    "none", noneBalancing,
    "permute", permute,
    "scale", scale,
    "permute-and-scale", permuteAndScale,
    Core::Choice::endMark());
const Core::ParameterChoice GenEigenProblemWithSchurDecomposition::paramBalancing(
    "balancing", &choiceBalancing, "type of balancing", permuteAndScale);

GenEigenProblemWithSchurDecomposition::GenEigenProblemWithSchurDecomposition(const Core::Configuration &c) :
    Core::Component(c),
    Math::GeneralizedEigenvalueProblem(c),
    Math::Lapack::EigenvalueProblem(c),
    relativeAlphaMinimum_(paramRelativeAlphaMinimum(c)),
    relativeBetaMinimum_(paramRelativeBetaMinimum(c)),
    conditionNumberChannel_(c, "condition-numbers", Core::Channel::disabled)
{}

bool GenEigenProblemWithSchurDecomposition::solveSymmetric(
    const Math::Matrix<ValueType> &a,
    const Math::Matrix<ValueType> &b,
    Math::Vector<ValueType> &eigenvalues,
    Math::Matrix<ValueType> &eigenvectors)
{
    bool success = true;
    Math::Vector<std::complex<ValueType> > complexEigenvalues;
    Math::Matrix<std::complex<ValueType> > complexLeftEigenvectors;
    Math::Matrix<std::complex<ValueType> > complexRightEigenvectors;
    if (!solve(a, b, complexEigenvalues,
	       complexLeftEigenvectors, complexRightEigenvectors))
	success = false;
    if (!real(complexEigenvalues, eigenvalues))
	success = false;
    if (!real(complexRightEigenvectors, eigenvectors))
	success = false;
    return success;
}

bool GenEigenProblemWithSchurDecomposition::solve(
    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
    Math::Vector<std::complex<ValueType> > &eigenvalues,
    Math::Matrix<std::complex<ValueType> > &leftEigenvectors,
    Math::Matrix<std::complex<ValueType> > &rightEigenvectors)
{
    switch(driverType()) {
    case driverExpert:
	return solveWithExpertDriver(a, b, eigenvalues, leftEigenvectors, rightEigenvectors);
    default:
	error("Driver type not supported.");
	return false;
    }
}

bool GenEigenProblemWithSchurDecomposition::solveWithExpertDriver(
    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
    Math::Vector<std::complex<ValueType> > &eigenvalues,
    Math::Matrix<std::complex<ValueType> > &leftEigenvectors,
    Math::Matrix<std::complex<ValueType> > &rightEigenvectors) const
{
    Matrix<double> lapackA; copy(a, lapackA);
    Matrix<double> lapackB; copy(b, lapackB);
    Vector<double> realAlphas;
    Vector<double> imaginaryAlphas;
    Vector<double> betas;
    Matrix<double> lapackLeftEigenvectors;
    Matrix<double> lapackRightEigenvectors;
    Vector<double> eigenvalueReciprocalConditionNumbers;
    Vector<double> eigenvectorReciprocalConditionNumbers;
    double oneNormOfBalancedA;
    double oneNormOfBalancedB;
    double leftMaxPerMinScaling;
    double rightMaxPerMinScaling;

    if (!dggevx(lapackA, lapackB, realAlphas, imaginaryAlphas, betas,
		lapackLeftEigenvectors, lapackRightEigenvectors,
		eigenvalueReciprocalConditionNumbers, eigenvectorReciprocalConditionNumbers,
		oneNormOfBalancedA, oneNormOfBalancedB,
		leftMaxPerMinScaling, rightMaxPerMinScaling))
	return false;

    Math::Vector<std::complex<ValueType> > alphas;
    copyAlphas(realAlphas, imaginaryAlphas, alphas);
    copyEigenvalues(alphas, betas, eigenvalues);
    copyEigenvectors(alphas, lapackLeftEigenvectors, leftEigenvectors);
    copyEigenvectors(alphas, lapackRightEigenvectors, rightEigenvectors);
    if (conditionNumberChannel_.isOpen()) {
	writeConditionNumbers(conditionNumberChannel_, alphas, betas, eigenvalues,
			      eigenvalueReciprocalConditionNumbers,
			      eigenvectorReciprocalConditionNumbers,
			      oneNormOfBalancedA, oneNormOfBalancedB,
			      leftMaxPerMinScaling, rightMaxPerMinScaling);
    }
    return true;
}

bool GenEigenProblemWithSchurDecomposition::dggevx(
    Matrix<double> &a, Matrix<double> &b,
    Vector<double> &realAlphas, Vector<double> &imaginaryAlphas, Vector<double> &betas,
    Matrix<double> &leftEigenvectors, Matrix<double> &rightEigenvectors,
    Vector<double> &eigenvalueReciprocalConditionNumbers,
    Vector<double> &eigenvectorReciprocalConditionNumbers,
    double &oneNormOfBalancedA, double &oneNormOfBalancedB,
    double &leftMaxPerMinScaling, double &rightMaxPerMinScaling) const
{
    require(a.nRows() > 0);
    require(a.nRows() == a.nColumns());
    require(b.nRows() == b.nColumns());
    require(a.nRows() == b.nRows());

    char balanc = translateBalancingType((BalancingType)paramBalancing(config));
    char jobvl = 'V';
    char jobvr = 'V';
    char sense = translateSensitivity();

    int n = a.nRows();
    int lda = n; // lda >= max(1,n_)
    int ldb = n; // ldb >= max(1,n_)

    realAlphas.resize(n);
    imaginaryAlphas.resize(n);
    betas.resize(n);
    int ldvl = n; // ldvl>=1, and if jobvl='V' then ldvl >= 'N'
    leftEigenvectors.resize(ldvl, n);
    int ldvr = n;
    rightEigenvectors.resize(ldvr, n);

    oneNormOfBalancedA = 0;
    oneNormOfBalancedB = 0;

    int ilo;
    int ihi;

    Vector<double> leftBalancingScales(n + 1);
    std::fill(leftBalancingScales.begin(), leftBalancingScales.end(), 1);
    leftMaxPerMinScaling = 1;
    Vector<double> rightBalancingScales(n + 1);
    std::fill(rightBalancingScales.begin(), rightBalancingScales.end(), 1);
    rightMaxPerMinScaling = 1;

    eigenvalueReciprocalConditionNumbers.resize(n);
    eigenvectorReciprocalConditionNumbers.resize(n);

    int lwork = -1;
    Vector<double> work(1);
    Vector<int> iwork(n + 6);
    Vector<int> bwork(n);

    int info;
    Lapack::dggevx(&balanc, &jobvl, &jobvr, &sense, &n, a.buffer(), &lda, b.buffer(), &ldb,
		   realAlphas.buffer(), imaginaryAlphas.buffer(), betas.buffer(),
		   leftEigenvectors.buffer(), &ldvl, rightEigenvectors.buffer(), &ldvr,
		   &ilo, &ihi, leftBalancingScales.buffer(), rightBalancingScales.buffer(),
		   &oneNormOfBalancedA, &oneNormOfBalancedB,
		   eigenvalueReciprocalConditionNumbers.buffer(),
		   eigenvectorReciprocalConditionNumbers.buffer(),
		   work.buffer(), &lwork, iwork.buffer(), bwork.buffer(),
		   &info);
    if (info != 0) {
	error("Lapack error code %d recieved while workspace query.", info);
	return false;
    }
    work.resize(lwork = (int)work[0]);

    Lapack::dggevx(&balanc, &jobvl, &jobvr, &sense, &n, a.buffer(), &lda, b.buffer(), &ldb,
		   realAlphas.buffer(), imaginaryAlphas.buffer(), betas.buffer(),
		   leftEigenvectors.buffer(), &ldvl, rightEigenvectors.buffer(), &ldvr,
		   &ilo, &ihi, leftBalancingScales.buffer(), rightBalancingScales.buffer(),
		   &oneNormOfBalancedA, &oneNormOfBalancedB,
		   eigenvalueReciprocalConditionNumbers.buffer(),
		   eigenvectorReciprocalConditionNumbers.buffer(),
		   work.buffer(), &lwork, iwork.buffer(), bwork.buffer(),
		   &info);
    if (info != 0) {
	error("Lapack error code %d recieved.", info);
	return false;
    }

    leftMaxPerMinScaling = maxPerMinScaling(leftBalancingScales, ilo, ihi);
    rightMaxPerMinScaling = maxPerMinScaling(rightBalancingScales, ilo, ihi);

    return true;
}

double GenEigenProblemWithSchurDecomposition::maxPerMinScaling(
    const Vector<double> &scales, int ilo, int ihi) const
{
    double result = (*std::max_element(scales.begin() + (ilo - 1), scales.begin() + ihi) /
		     *std::min_element(scales.begin() + (ilo - 1), scales.begin() + ihi));
    return result;
}


void GenEigenProblemWithSchurDecomposition::copyAlphas(
    const Vector<double> &realAlphas, const Vector<double> &imaginaryAlphas,
    Math::Vector<std::complex<ValueType> > &alphas) const
{
    require(realAlphas.size() == imaginaryAlphas.size());
    alphas.resize(realAlphas.size());
    std::transform(realAlphas.begin(), realAlphas.end(), imaginaryAlphas.begin(),
		   alphas.begin(), Math::makeComplex<ValueType>());
}

void GenEigenProblemWithSchurDecomposition::copyEigenvalues(
    const Math::Vector<std::complex<ValueType> > &alphas, const Vector<double> &betas,
    Math::Vector<std::complex<ValueType> > &eigenvalues) const
{
    require(alphas.size() == betas.size());

    ValueType alphaMinimum = Core::maxAbsoluteElement(alphas) * relativeAlphaMinimum_;
    ValueType betaMinimum = maxAbsoluteElement(betas) * relativeBetaMinimum_;

    std::string warningDescription;
    static const std::string definitions = "Where alpha is diagonal element of Alpha=Q^H*A*Z; " \
	"beta is diagonal element of Beta=Q^H*B*Z.";
    eigenvalues.resize(alphas.size());
    for(size_t i = 0; i < alphas.size(); ++ i) {
	warningDescription.clear();
	eigenvalues[i] = calculateEigenvalue(alphas[i], alphaMinimum,
					     betas[i], betaMinimum,
					     warningDescription);
	if (!warningDescription.empty()) {
	    warning("While calculating %zdth eigenvalue, %s. %s",
		    i + 1, warningDescription.c_str(), definitions.c_str());
	}
    }
}

std::complex<Math::EigenvalueProblem::ValueType>
GenEigenProblemWithSchurDecomposition::calculateEigenvalue(
    std::complex<ValueType> alpha, ValueType alphaMinimum,
    ValueType beta, ValueType betaMinimum,
    std::string &warningDescription) const
{
    ValueType absAlpha = Core::abs(alpha);
    ValueType absBeta = Core::abs(beta);
    if (absAlpha < alphaMinimum) {
	warningDescription =
	    Core::form("eigenvalue set to zero due to abs(alpha)=%le smaller %le (abs(beta)=%le)",
		       (double)absAlpha, (double)alphaMinimum, (double)absBeta);
	return std::complex<ValueType>(0, 0);
    } else if (absBeta < betaMinimum) {
	warningDescription =
	    Core::form("eigenvalue set to zero due to abs(beta)=%le smaller %le (abs(alpha)=%le)",
		       (double)absBeta, (double)betaMinimum, (double)absAlpha);
	return std::complex<ValueType>(0, 0);
    }
    return alpha / beta;
}

void GenEigenProblemWithSchurDecomposition::copyEigenvectors(
    const Math::Vector<std::complex<ValueType> > &alphas, const Matrix<ValueType> &v,
    Math::Matrix<std::complex<ValueType> > &eigenvectors) const
{
    /*
     *   The eigenvectors v(j) are stored one
     *   after another in the columns of VR, in the same order as
     *   their eigenvalues. If the j-th eigenvalue is real, then
     *   v(j) = VR(:,j), the j-th column of VR. If the j-th and
     *   (j+1)-th eigenvalues form a complex conjugate pair, then
     *   v(j) = VR(:,j)+i*VR(:,j+1) and v(j+1) = VR(:,j)-i*VR(:,j+1).
     *   Each eigenvector will be scaled so the largest component have
     *   abs(real part) + abs(imag. part) = 1.
     *   Not referenced if JOBVR = 'N'.
     */
    require(alphas.size() == v.nColumns());

    eigenvectors.resize(v.nRows(), v.nColumns());
    for(size_t i = 0; i < eigenvectors.nColumns(); ++ i) {
	if (alphas[i].imag() == 0) {
	    for(u32 d = 0; d < v.nRows(); ++ d)
		eigenvectors[d][i] = std::complex<ValueType>(v(d, i), 0);
	} else {
	    verify((i + 1) < eigenvectors.nColumns());
	    for(u32 d = 0; d < v.nRows(); ++ d) {
		eigenvectors[d][i] = std::complex<ValueType>(v(d, i), v(d, i + 1));
		eigenvectors[d][i + 1] = std::complex<ValueType>(v(d, i), -v(d, i + 1));
	    }
	    ++ i;
	}
    }
}

void GenEigenProblemWithSchurDecomposition::writeConditionNumbers(
    Core::XmlWriter &os,
    const Math::Vector<std::complex<ValueType> > &alphas,
    const Vector<double> &betas,
    const Math::Vector<std::complex<ValueType> > &eigenvalues,
    const Vector<double> &eigenvalueReciprocalConditionNumbers,
    const Vector<double> &eigenvectorReciprocalConditionNumbers,
    double oneNormOfBalancedA, double oneNormOfBalancedB,
    double &leftMaxPerMinScaling, double &rightMaxPerMinScaling) const
{
    require(alphas.size() == betas.size());
    require(alphas.size() == eigenvalues.size());
    require(alphas.size() == eigenvalueReciprocalConditionNumbers.size());
    require(alphas.size() == eigenvectorReciprocalConditionNumbers.size());

    Math::Vector<std::complex<ValueType> >::const_iterator alpha = alphas.begin();
    Vector<double>::ConstIterator beta = betas.begin();
    Math::Vector<std::complex<ValueType> >::const_iterator eigenvalue = eigenvalues.begin();
    Vector<double>::ConstIterator e = eigenvalueReciprocalConditionNumbers.begin();
    Vector<double>::ConstIterator v = eigenvectorReciprocalConditionNumbers.begin();

    double perturbance = Core::Type<double>::epsilon *
	sqrt(oneNormOfBalancedA * oneNormOfBalancedA +
	     oneNormOfBalancedB * oneNormOfBalancedB);

    os << Core::XmlOpen("generalized-eigen-sensitivity");
    os << Core::XmlOpen("perturbation")
       << Core::XmlFull("epsilon", Core::Type<double>::epsilon)
       << Core::XmlFull("one-norm-A", oneNormOfBalancedA)
       << Core::XmlFull("one-norm-B", oneNormOfBalancedB)
       << Core::XmlFull("left-max-per-min-balancing-scale-ratio", leftMaxPerMinScaling)
       << Core::XmlFull("right-max-per-min-balancing-scale-ratio", rightMaxPerMinScaling)
       << Core::XmlClose("perturbation");
    for(; alpha != alphas.end(); ++ alpha, ++ beta, ++ eigenvalue, ++ e, ++ v) {
	os << Core::XmlOpen("component")
	   << Core::XmlFull("alpha", *alpha)
	   << Core::XmlFull("beta", *beta)
	   << Core::XmlFull("eigenvalue", *eigenvalue)
	   << Core::XmlFull("eigenvalue-reciprocal-condition-number", *e)
	   << Core::XmlFull("eigenvector-reciprocal-condition-number", *v)
	   << Core::XmlFull("eigenvalue-asymptotic-error-bound",  perturbance / *e)
	   << Core::XmlFull("left-eigenvector-asymptotic-error-bound",
			    asin(leftMaxPerMinScaling * sin(perturbance / *v)))
	   << Core::XmlFull("right-eigenvector-asymptotic-error-bound",
			    asin(rightMaxPerMinScaling * sin(perturbance / *v)))
	   << Core::XmlClose("component");
    }
    os << Core::XmlClose("generalized-eigen-sensitivity");
}

char GenEigenProblemWithSchurDecomposition::translateBalancingType(BalancingType type) const
{
    switch(type) {
    case noneBalancing: return 'N';
    case permute: return 'P';
    case scale: return 'S';
    case permuteAndScale: return 'B';
    }
    defect();
}

char GenEigenProblemWithSchurDecomposition::translateSensitivity() const
{
    char result = 'N';
    if (conditionNumberChannel_.isOpen())
	result = 'B';
    return result;
}

//--------------------------------------------------------------------------------------------------------
GenSymmetricDefiniteEigenProblem::GenSymmetricDefiniteEigenProblem(
    const Core::Configuration &c) :
    Core::Component(c),
    Math::GeneralizedEigenvalueProblem(c),
    Math::Lapack::EigenvalueProblem(c)
{}

bool GenSymmetricDefiniteEigenProblem::solve(
    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
    Math::Vector<ValueType> &eigenvalues, Math::Matrix<ValueType> &eigenvectors)
{
    switch(driverType()) {
    case driverExpert:
	return solveWithExpertDriver(a, b, eigenvalues, eigenvectors);
    case driverDivideAndConquer:
	return solveWithDivideAndConquerDriver(a, b, eigenvalues, eigenvectors);
    default:
	error("Driver type not supported.");
	return false;
    }
}

bool GenSymmetricDefiniteEigenProblem::solveWithExpertDriver(
    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
    Math::Vector<ValueType> &eigenvalues, Math::Matrix<ValueType> &eigenvectors) const
{
    Matrix<double> lapackA; copy(a, lapackA);
    Matrix<double> lapackB; copy(b, lapackB);
    int nEigenvalues;
    Vector<double> lapackEigenvalues;
    Matrix<double> lapackEigenvectors;
    if (!dsygvx(lapackA, lapackB, nEigenvalues, lapackEigenvalues, lapackEigenvectors))
	return false;
    copy(lapackEigenvalues, eigenvalues, nEigenvalues);
    copy(lapackEigenvectors, eigenvectors, nEigenvalues);
    return true;
}

bool GenSymmetricDefiniteEigenProblem::dsygvx(
    Matrix<double> &a, Matrix<double> &b, int &nEigenvalues,
    Vector<double> &eigenvalues, Matrix<double> &eigenvectors) const
{
    require(a.nRows() > 0);
    require(a.nRows() == a.nColumns());
    require(a.nRows() == b.nRows());
    require(b.nRows() == b.nColumns());

    int itype = 1;
    char jobz = 'V';
    char range = translateEigenvalueRange();
    char uplo = 'U';
    int n = a.nRows();
    int lda = n; // LDA >= max(1,N)
    int ldb = n; // LDB >= max(1,N)
    double vl = eigenvalueLowerBound();
    double vu = eigenvalueUpperBound();
    int il;
    int iu;
    double abstol = 0;
    eigenvalues.resize(n);
    eigenvectors.resize(n, n);
    int ldz = n;
    int lwork = -1;
    Vector<double> work(1);
    Vector<int> iwork(5 * n);
    Vector<int> ifail(n);
    int info;
    eigenvalues.resize(n);

    Lapack::dsygvx(&itype, &jobz, &range, &uplo, &n, a.buffer(), &lda, b.buffer(), &ldb,
		   &vl, &vu, &il, &iu, &abstol, &nEigenvalues, eigenvalues.buffer(),
		   eigenvectors.buffer(), &ldz, work.buffer(), &lwork, iwork.buffer(),
		   ifail.buffer(), &info);
    if (info != 0) {
	error("Lapack error code %d recieved while workspace query.", info);
	return false;
    }
    work.resize(lwork = (int)work[0]);

    Lapack::dsygvx(&itype, &jobz, &range, &uplo, &n, a.buffer(), &lda, b.buffer(), &ldb,
		   &vl, &vu, &il, &iu, &abstol, &nEigenvalues, eigenvalues.buffer(),
		   eigenvectors.buffer(), &ldz, work.buffer(), &lwork, iwork.buffer(),
		   ifail.buffer(), &info);
    if (info != 0) {
	error("Lapack error code %d recieved.", info);
	return false;
    }
    return true;
}

bool GenSymmetricDefiniteEigenProblem::solveWithDivideAndConquerDriver(
    const Math::Matrix<ValueType> &a, const Math::Matrix<ValueType> &b,
    Math::Vector<ValueType> &eigenvalues, Math::Matrix<ValueType> &eigenvectors) const
{
    Matrix<double> lapackA; copy(a, lapackA);
    Matrix<double> lapackB; copy(b, lapackB);
    Vector<double> lapackEigenvalues;
    if (!dsygvd(lapackA, lapackB, lapackEigenvalues))
	return false;
    copy(lapackEigenvalues, eigenvalues);
    copy(lapackA, eigenvectors);
    return true;
}

bool GenSymmetricDefiniteEigenProblem::dsygvd(
    Matrix<double> &a, Matrix<double> &b, Vector<double> &eigenvalues) const
{
    require(a.nRows() > 0);
    require(a.nRows() == a.nColumns());
    require(a.nRows() == b.nRows());
    require(b.nRows() == b.nColumns());

    char jobz = 'V';
    char uplo = 'U';
    int itype = 1;
    int n = a.nRows();
    int lda = n; // LDA >= max(1,N)
    int ldb = n; // LDB >= max(1,N)
    int lwork = 1 + 6 * n + 3 * n * n; // If JOBZ = 'V' and N > 1, LWORK >= 1 + 6*N + 2*N**2.
    Vector<double> work(lwork);
    int liwork = 3 + 5 * n; //If JOBZ  = 'V' and N > 1, LIWORK >= 3 + 5*N
    Vector<int> iwork(liwork);
    int info;
    eigenvalues.resize(n);

    Lapack::dsygvd(&itype, &jobz, &uplo, &n, a.buffer(), &lda, b.buffer(), &ldb,
		   eigenvalues.buffer(), work.buffer(), &lwork, iwork.buffer(), &liwork, &info);
    if (info != 0) {
	error("Lapack error code %d recieved.", info);
	return false;
    }
    return true;
}
