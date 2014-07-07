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
#include "ScatterTransform.hh"
#include <Math/Module.hh>

using namespace Signal;

//----------------------------------------------------------------------------
const Core::ParameterInt ScatterTransform::paramOutputPrecision(
    "output-precision", "number of decimal digits in output files", 20);
const Core::ParameterString ScatterTransform::paramTotalScatterFilename(
    "covariance-file", "input filename for covariance matrix");
 const Core::ParameterString ScatterTransform::paramBetweenClassScatterFilename(
    "between-class-scatter-matrix-file", "input file name for between-class scatter matrix");
const Core::ParameterString ScatterTransform::paramWithinClassScatterFilename(
    "within-class-scatter-matrix-file", "input file name for within-class scatter matrix");
const Core::ParameterFloat ScatterTransform::paramTransformScale(
    "transform-scale",
    "as last step the transformation matrix is multiplied by this value",
    1.0);

ScatterTransform::ScatterTransform(
    const Core::Configuration &configuration,
    const std::string &transformationTypename) :
    Precursor(configuration),
    paramTransformationFilename((transformationTypename + "-file").c_str(),
				"output filename for transformation matrix"),
    transformScale_(paramTransformScale(configuration))
{}

ScatterTransform::~ScatterTransform()
{}

bool ScatterTransform::work()
{
    if (transformScale_ != (TransformationMatrix::Type)1)
	transform_ *= transformScale_;
    return true;
}

bool ScatterTransform::write()
{
    bool success = true;
    std::string filename(paramTransformationFilename(config));
    if (Math::Module::instance().formats().write(
	    filename, transform_, paramOutputPrecision(config))) {
	log("Transformation matrix written to '%s'.", filename.c_str());
    } else {
	error("Failed to store transformation matrix to file '%s'.", filename.c_str());
	success = false;
    }
    return success;
}

bool ScatterTransform::readScatterMatrix(
    const std::string &filename, const std::string &scatterType,
    ScatterMatrix &scatterMatrix) const
{
    if (!Math::Module::instance().formats().read(
	    filename, scatterMatrix)) {
	error("Failed to read %s scatter matrix from file '%s'.",
	      scatterType.c_str(), filename.c_str());
	return false;
    }
    return true;
}

void ScatterTransform::writeScatterMatrix(
    Core::XmlWriter &os, const std::string &scatterType,
    const ScatterMatrix &scatterMatrix, Math::EigenvalueProblem *eigenvalueProblem) const
{
    std::string name = scatterType + "-scatter-matrix";
    os << Core::XmlOpen(name);
    if (eigenvalueProblem) {
	os << Core::XmlOpen("scatter-matrix");
	os << scatterMatrix;
	os << Core::XmlClose("scatter-matrix");

	os << Core::XmlOpen("eigenvalues");
	Math::Vector<EigenType> eigenvalues;
	Math::Matrix<EigenType> eigenvectors;
	if (!eigenvalueProblem->solveSymmetricAndFinalize(scatterMatrix, eigenvalues, eigenvectors)) {
	    os << Core::XmlFull("error", "Error(s) occurred while calculating eigenvalues and eigenvectors.");
	}
	os << eigenvalues;
	os << Core::XmlClose("eigenvalues");
    } else {
	os << scatterMatrix;
    }
    os << Core::XmlClose(name);
}

//----------------------------------------------------------------------------

const Core::ParameterFloat ScatterDiagonalNormalization::paramTolerance(
    "tolerance", "tolerance used when inverting the diagonal", 0, 0);

ScatterDiagonalNormalization::ScatterDiagonalNormalization(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c, "normalization"),
    tolerance_(paramTolerance(c))
{}

bool ScatterDiagonalNormalization::work(const ScatterMatrix &scatter) {
    bool success = true;
    Math::Vector<ScatterMatrix::Type> diagonal = scatter.diagonal();
    for (size_t i = 0; i < diagonal.size(); ++ i) {
	if (diagonal[i] < 0 && Core::abs(diagonal[i]) > tolerance_) {
	    error("Normalization failed because "			\
		  "because %zu-th diagonal element %f is negative.", i, diagonal[i]);
	    success = false;
	}
    }
    if (success) {
	diagonal.takeSquareRoot(tolerance_);
	diagonal.takeReciprocal(tolerance_);
	Message scalingMessage(log("Scaling factors:\n"));
	for (size_t i = 0; i < diagonal.size(); ++ i) {
	    if (diagonal[i] == 0) scalingMessage << "no-scaling ";
	    else scalingMessage << diagonal[i] << " ";
	}
	Math::Vector<TransformationMatrix::Type> results = diagonal;
	transform_ = makeDiagonalMatrix(results);
	if (!Precursor::work()) success = false;
    }
    return success;
}

bool ScatterDiagonalNormalization::work() {
    ScatterMatrix scatterMatrix;
    return (readScatterMatrix(paramTotalScatterFilename(config),
			      totalScatterType, scatterMatrix) &&
	    work(scatterMatrix));
}

//----------------------------------------------------------------------------

const Core::ParameterFloat ScatterThresholding::paramElementThresholdMin("element-threshold-min",
    "minimum threshold for every covariance element", Core::Type<f32>::min);
const Core::ParameterString ScatterThresholding::paramInputScatterFilename("input-matrix-file",
    "input file name for scatter matrix");
const Core::ParameterString ScatterThresholding::paramOutputScatterFilename("output-matrix-file",
    "input file name for scatter matrix");

ScatterThresholding::ScatterThresholding(const Core::Configuration &c) :
    Core::Component(c),
    Precursor(c, "thresholding"),
    threshold_(paramElementThresholdMin(c)),
    inputScatterFilename_(paramInputScatterFilename(c)),
    outputScatterFilename_(paramOutputScatterFilename(c))
{ }

bool ScatterThresholding::work() {
	ScatterMatrix scatterMatrix;
	readScatterMatrix(inputScatterFilename_, totalScatterType, scatterMatrix);
    for (size_t i = 0; i < scatterMatrix.nRows(); ++i) {
	for (size_t j = 0; j < scatterMatrix.nColumns(); ++j) {
			if (scatterMatrix[i][j] < threshold_)
				scatterMatrix[i][j] = threshold_;
		}
	}
	scatterMatrix_ = scatterMatrix;
	return true;
}

bool ScatterThresholding::write() {
	if (Math::Module::instance().formats().write(outputScatterFilename_, scatterMatrix_, paramOutputPrecision(config))) {
		log("Transformation matrix written to '%s'.", outputScatterFilename_.c_str());
	return true;
	} else {
		error("Failed to store transformation matrix to file '%s'.", outputScatterFilename_.c_str());
	return false;
	}
}
