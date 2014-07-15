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
#ifndef _SIGNAL_SCATTER_TRANSFORM_HH
#define _SIGNAL_SCATTER_TRANSFORM_HH

#include <Core/Component.hh>
#include "ScatterEstimator.hh"
#include <Math/EigenvalueProblem.hh>

namespace Signal {

    /**
     *  Base class for scatter matrix based tranformations.
     *  Features:
     *   -Basic I/O
     */
    class ScatterTransform : virtual public Core::Component {
	typedef Core::Component Precursor;
    public:
	typedef Math::EigenvalueProblem::ValueType EigenType;
	typedef Math::Matrix<f32> TransformationMatrix;
    private:
	const Core::ParameterString paramTransformationFilename;
	static const Core::ParameterFloat paramTransformScale;
    protected:
	static const Core::ParameterInt paramOutputPrecision;
	static const Core::ParameterString paramTotalScatterFilename;
	static const Core::ParameterString paramBetweenClassScatterFilename;
	static const Core::ParameterString paramWithinClassScatterFilename;
    protected:
	TransformationMatrix transform_;
	TransformationMatrix::Type transformScale_;
    protected:
	/**
	 *  Performs final steps of calculating the transformation.
	 *  Steps:
	 *    -scaling
	 */
	bool work();

	bool readScatterMatrix(const std::string &filename,
			       const std::string &scatterType,
			       ScatterMatrix &scatterMatrix) const;
	/**
	 *  Writes given scatter matrix to the output stream.
	 *  @c scatterType is used when creating XML tag names.
	 *  If @c eigenvalueProblem not zero, eigenvalues of
	 *  scatter matrix are dumped as well.
	 */
	void writeScatterMatrix(Core::XmlWriter &os,
				const std::string &scatterType,
				const ScatterMatrix &scatterMatrix,
				Math::EigenvalueProblem *eigenvalueProblem = 0) const;
    public:
	/**
	 *  Consturctor.
	 *  @c transformationTypename specifies the parameter name of the output file.
	 *  E.g.: projector-matrix, normalization, etc.
	 */
	ScatterTransform(const Core::Configuration &c,
			 const std::string &transformationTypename);
	~ScatterTransform();

	const TransformationMatrix &transform() const { return transform_; }
	/**
	 *  Saves the transformation matrix under name given by param paramTransformationFilename;
	 */
	bool write();
    };

    /**
     *  Calculates a transformation which makes the diagonal
     *  of the input scatter matrix to be unity.
     */
    class ScatterDiagonalNormalization : public ScatterTransform {
	typedef ScatterTransform Precursor;
    private:
	static const Core::ParameterFloat paramTolerance;
    private:
	ScatterMatrix::Type tolerance_;
    public:
	ScatterDiagonalNormalization(const Core::Configuration &c);

	bool work(const ScatterMatrix &scatter);
	bool work();
    };

    class ScatterThresholding : public ScatterTransform {
	typedef ScatterTransform Precursor;
    private:
	Math::Matrix<f64> scatterMatrix_;
	static const Core::ParameterFloat paramElementThresholdMin;
	static const Core::ParameterString paramInputScatterFilename;
	static const Core::ParameterString paramOutputScatterFilename;
	f64 threshold_;
	std::string inputScatterFilename_;
	std::string outputScatterFilename_;
    public:
	ScatterThresholding(const Core::Configuration &c);
	bool work();
	bool write();
    };

} //namespace Signal

#endif // _SIGNAL_SCATTER_TRANSFORM_HH
