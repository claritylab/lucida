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
#include <ext/functional>
#include "EigenvalueProblem.hh"

using namespace Math;

using __gnu_cxx::select1st;
using __gnu_cxx::select2nd;

//--------------------------------------------------------------------------------------------------------
const Core::Choice EigenvalueProblem::choiceEigenvectorNormalizationType(
    "none", nonEigenvectorNormalization,
    "unity-length", unityLength,
    "unity-diagonal", unityDiagonal,
    Core::Choice::endMark());
const Core::ParameterChoice EigenvalueProblem::paramEigenvectorNormalization(
    "eigenvector-normalization-type", &choiceEigenvectorNormalizationType,
    "type of eigenvector normalization", unityLength);
const Core::ParameterFloat EigenvalueProblem::paramDiagonalNormalizationTolerance(
    "eigenvector-normalization-tolerance",
    "tolerance used when inverting the diagonal.",
    0, 0);
const Core::ParameterFloat EigenvalueProblem::paramVerificationTolerance(
    "verification-tolerance",
    "tolerance used when verifying the solution.",
    Core::Type<f64>::max, 0);
const Core::ParameterFloat EigenvalueProblem::paramRelativeImaginaryMaximum(
    "relative-imaginary-maximum",
    "while copiing real part of a container, values are reported which abs(imag) > max(abs(container)) * this.",
    1, 0, 1);
const Core::Choice EigenvalueProblem::choiceType(
    "general", typeGeneral,
    "symmetric", typeSymmetric,
    "symmetric-positive-definite", typeSymmetricPositiveDefinite,
    Core::Choice::endMark());
const Core::ParameterChoice EigenvalueProblem::paramType(
    "type", &choiceType, "type of eigenvalue problem", typeGeneral);
const Core::Choice EigenvalueProblem::choiceEigenvalueSortType(
    "none", noSort,
    "decreasing", decreasing,
    "increasing", increasing,
    Core::Choice::endMark());
const Core::ParameterChoice EigenvalueProblem::paramEigenvalueSort(
    "eigenvalue-sort-type", &choiceEigenvalueSortType,
    "eigenvalues are sorted in this order", decreasing);

EigenvalueProblem::EigenvalueProblem(const Core::Configuration &c) :
    Core::Component(c),
    eigenvectorNormalizationType_((EigenvectorNormalizationType)paramEigenvectorNormalization(c)),
    diagonalNormalizationTolerance_(paramDiagonalNormalizationTolerance(c)),
    relativeImaginaryMaximum_(paramRelativeImaginaryMaximum(c)),
    verificationTolerance_(paramVerificationTolerance(c)),
    eigenvalueSortType_((EigenvalueSortType)paramEigenvalueSort(c))
{}

bool EigenvalueProblem::solveSymmetricAndFinalize(
    const Matrix<ValueType> &a,
    Vector<ValueType> &eigenvalues,
    Matrix<ValueType> &eigenvectors)
{
    bool success = true;
    if (!solveSymmetric(a, eigenvalues, eigenvectors))
	success = false;
    if (!finalize(a, eigenvalues, eigenvectors))
	success = false;
    return success;
}

bool EigenvalueProblem::finalize(const Matrix<ValueType> &a,
				 Vector<ValueType> &eigenvalues,
				 Matrix<ValueType> &eigenvectors)
{
    if (!finalizeEigenvaluesAndVectors(a, eigenvalues, eigenvectors))
	return false;
    if (!isCorrectSolution(a, eigenvalues, eigenvectors)) {
	error("Verifying the calculation of eigenvalues and eigenvectors failed.");
	return false;
    }
    return true;
}

bool EigenvalueProblem::finalizeEigenvaluesAndVectors(
    const Matrix<ValueType> &a,
    Vector<ValueType> &eigenvalues,
    Matrix<ValueType> &eigenvectors)
{
    bool result = true;

    eigenvectors.normalizeColumnDirection();
    sortEigenvalues(eigenvalues, eigenvectors);

    switch (eigenvectorNormalizationType_) {
    case nonEigenvectorNormalization:
	break;
    case unityLength:
	eigenvectors.normalizeColumns(2);
	break;
    case unityDiagonal:
	if (!normalizeDiagonal(a, eigenvectors))
	    result = false;
	break;
    default:
	error("Eigenvector normalization type not supported.");
	result = false;
    }
    return result;
}

bool EigenvalueProblem::normalizeDiagonal(
    const Matrix<ValueType> &a, Matrix<ValueType> &eigenvectors)
{
    bool result = true;
    Vector<ValueType> diagonal =
	(eigenvectors.transpose() * a * eigenvectors).diagonal();
    for (size_t i = 0; i < diagonal.size(); ++ i) {
	if (diagonal[i] < 0 && Core::abs(diagonal[i]) > diagonalNormalizationTolerance_) {
	    error("Failed to normalize eigenvectors because "		\
		  "because %zu diagonal element %e is negative.", i, diagonal[i]);
	    result = false;
	}
    }
    if (result) {
	diagonal.takeSquareRoot(diagonalNormalizationTolerance_);
	Message scalingMessage(log("Eigenvector scaling factors relative to their lengths:\n"));
	for (size_t i = 0; i < diagonal.size(); ++ i) {
	    if (diagonal[i] == 0) scalingMessage << "no-scaling ";
	    else scalingMessage << (eigenvectors.columnNorm(i, 2) / diagonal[i]) << " ";
	}
	diagonal.takeReciprocal(diagonalNormalizationTolerance_);
	eigenvectors = eigenvectors * makeDiagonalMatrix(diagonal);
    }
    return result;
}

bool EigenvalueProblem::isCorrectSolution(
    const Matrix<ValueType> &a,
    const Vector<ValueType> &eigenvalues, const Matrix<ValueType> &eigenvectors)
{
    if ((a.nColumns() != eigenvectors.nRows()) ||
	(eigenvectors.nColumns() != eigenvalues.size())) {
	error("Dimension mismatch while verifying solution: A[%zd, %zd], X[%zd, %zd], lambda[%zd].)",
	      a.nRows(), a.nColumns(), eigenvectors.nRows(), eigenvectors.nColumns(), eigenvalues.size());
	return false;
    }
    Matrix<ValueType> aTimesEigenvectors(a * eigenvectors);
    Matrix<ValueType> eigenvectorsTimesValues(
	eigenvectors * makeDiagonalMatrix(eigenvalues));
    return areEqual(aTimesEigenvectors, eigenvectorsTimesValues);
}

bool EigenvalueProblem::areEqual(
    const Matrix<ValueType> &x, const Matrix<ValueType> &y)
{
    std::pair<size_t, size_t> notEqual =
	x.isAlmostEqual(y, verificationTolerance_);

    if (notEqual.first != x.nRows()) {
	error("Checking solution failed in row=%zd columns=%zd (%e != %e at tolerance %e).",
	      notEqual.first, notEqual.second,
	      x[notEqual.first][notEqual.second],
	      y[notEqual.first][notEqual.second],
	      verificationTolerance_);
	return false;
    }
    return true;
}

void EigenvalueProblem::sortEigenvalues(
    Vector<ValueType> &eigenvalues, Matrix<ValueType> &eigenvectors)
{
    if (eigenvalueSortType_ != noSort) {
	typedef std::pair<ValueType, size_t> MapItem;
	std::vector<MapItem> eigenvalueIndexMap(eigenvalues.size());
	for(size_t i = 0; i < eigenvalues.size(); ++ i)
	    eigenvalueIndexMap[i] = std::make_pair(Core::abs(eigenvalues[i]), i);

	if (eigenvalueSortType_ == decreasing) {
	    std::sort(eigenvalueIndexMap.begin(), eigenvalueIndexMap.end(),
		      Core::composeBinaryFunction(std::greater<ValueType>(),
						  select1st<MapItem>(),
						  select1st<MapItem>()));
	} else if (increasing) {
	    std::sort(eigenvalueIndexMap.begin(), eigenvalueIndexMap.end(),
		      Core::composeBinaryFunction(std::less<ValueType>(),
						  select1st<MapItem>(),
						  select1st<MapItem>()));
	} else {
	    error("undefined sort type for eigenvalues");
	}

	std::vector<size_t> permutation(eigenvalues.size());
	std::transform(eigenvalueIndexMap.begin(), eigenvalueIndexMap.end(), permutation.begin(),
		       select2nd<MapItem>());
	eigenvalues.permuteElements(permutation);
	eigenvectors.permuteColumns(permutation);
    }
}

bool EigenvalueProblem::real(
    const Vector<std::complex<ValueType> > &c,
    Vector<ValueType> &r)
{
    ValueType imaginaryMaximum = Core::maxAbsoluteElement(c) * relativeImaginaryMaximum_;
    bool success = true;
    r.resize(c.size());
    Vector<std::complex<ValueType> >::const_iterator ci;
    Vector<ValueType>::iterator ri;
    for(ci = c.begin(), ri = r.begin(); ci != c.end(); ++ ci, ++ ri) {
	ValueType absImag = ci->imag();
	if (absImag > imaginaryMaximum) {
	    error("The %zdth imaginary element (%e) is larger than imaginary-maximum (%e).",
		  size_t(ci - c.begin()), absImag, imaginaryMaximum);
	    success = false;
	}
	*ri = ci->real();
    }
    return success;
}

bool EigenvalueProblem::real(
    const Matrix<std::complex<ValueType> > &c,
    Matrix<ValueType> &r)
{
    ValueType imaginaryMaximum = Math::maxAbsoluteElement(c) * relativeImaginaryMaximum_;
    bool success = true;
    r.resize(c.nRows(), c.nColumns());
    for(size_t row = 0; row < r.nRows(); ++ row) {
	for(size_t column = 0; column < r.nColumns(); ++ column) {
	    ValueType absImag = Core::abs(c[row][column].imag());
	    if (absImag > imaginaryMaximum) {
		error("The (%zd, %zd) imaginary element (%e) is larger than imaginary-maximum (%e).",
		      row, column, absImag, imaginaryMaximum);
		success = false;
	    }
	    r[row][column] = c[row][column].real();
	}
    }
    return success;
}

//--------------------------------------------------------------------------------------------------------
const Core::ParameterBool GeneralizedEigenvalueProblem::paramNormalizeEigenvectorsUsingB(
    "normalize-eigenvectors-using-B",
    "If yes, matrix B is used in eigenvector normalization instead of A.",
    false);

GeneralizedEigenvalueProblem::GeneralizedEigenvalueProblem(const Core::Configuration &c) :
    Core::Component(c),
    EigenvalueProblem(c),
    normalizeEigenvectorsUsingB_(paramNormalizeEigenvectorsUsingB(c))
{}

bool GeneralizedEigenvalueProblem::solveSymmetricAndFinalize(
    const Matrix<ValueType> &a,
    const Matrix<ValueType> &b,
    Vector<ValueType> &eigenvalues,
    Matrix<ValueType> &eigenvectors)
{
    bool success = true;
    if (!solveSymmetric(a, b, eigenvalues, eigenvectors))
	success = false;
    if (!finalize(a, b, eigenvalues, eigenvectors))
	success = false;
    return success;
}

bool GeneralizedEigenvalueProblem::finalize(const Matrix<ValueType> &a,
					    const Matrix<ValueType> &b,
					    Vector<ValueType> &eigenvalues,
					    Matrix<ValueType> &eigenvectors)
{
    if (!finalizeEigenvaluesAndVectors(a, b, eigenvalues, eigenvectors))
	return false;
    if (!isCorrectSolution(a, b, eigenvalues, eigenvectors)) {
	error("Verifying the calculation of eigenvalues and eigenvectors failed.");
	return false;
    }
    return true;
}

bool GeneralizedEigenvalueProblem::finalizeEigenvaluesAndVectors(
    const Matrix<ValueType> &a,
    const Matrix<ValueType> &b,
    Vector<ValueType> &eigenvalues,
    Matrix<ValueType> &eigenvectors)
{
    const Matrix<ValueType> *m = &a;
    if (normalizeEigenvectorsUsingB_) m = &b;
    return EigenvalueProblem::finalizeEigenvaluesAndVectors(*m, eigenvalues, eigenvectors);
}

bool GeneralizedEigenvalueProblem::isCorrectSolution(
    const Matrix<ValueType> &a, const Matrix<ValueType> &b,
    const Vector<ValueType> &eigenvalues, const Matrix<ValueType> &eigenvectors)
{
    if ((a.nColumns() != eigenvectors.nRows()) ||
	(b.nColumns() != eigenvectors.nRows()) ||
	(eigenvectors.nColumns() != eigenvalues.size())) {
	error("Dimension mismatch while verifying solution: A[%zd, %zd], B[%zd, %zd], X[%zd, %zd], lambda[%zd].)",
	      a.nRows(), a.nColumns(), b.nRows(), b.nColumns(),
	      eigenvectors.nRows(), eigenvectors.nColumns(), eigenvalues.size());
	return false;
    }
    Matrix<ValueType> aTimesEigenvectors(a * eigenvectors);
    Matrix<ValueType> bTimesEigenvectorsAndValues(
	b * eigenvectors * makeDiagonalMatrix(eigenvalues));
    return areEqual(aTimesEigenvectors, bTimesEigenvectorsAndValues);
}
