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
#include "Module.hh"
#include <Core/Application.hh>
#include <Core/VectorParser.hh>
#include <Core/MatrixParser.hh>
#include "Lapack/EigenvalueProblem.hh"
#include "EigenvalueProblem.hh"

using namespace Math;

Module_::Module_() :
    formats_(0)
{}

Module_::~Module_()
{
    delete formats_;
}

Core::FormatSet &Module_::formats()
{
    if (!formats_) {
	formats_ = new Core::FormatSet(Core::Configuration(Core::Application::us()->getConfiguration(), "file-format-set"));
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<Vector<f32> >());
	formats_->registerFormat("xml",
				 new Core::XmlFormat<Vector<s32>,
				 Core::XmlVectorDocument<s32> >(), true);
	formats_->registerFormat("xml",
				 new Core::XmlFormat<Vector<f32>,
				 Core::XmlVectorDocument<f32> >(), true);
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<Vector<f64> >());
	formats_->registerFormat("xml",
				 new Core::XmlFormat<Vector<f64>,
				 Core::XmlVectorDocument<f64> >(), true);
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<Matrix<f32> >());
	formats_->registerFormat("xml",
				 new Core::XmlFormat<Matrix<f32>,
				 Core::XmlMatrixDocument<f32> >(), true);
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<Matrix<f64> >());
	formats_->registerFormat("xml",
				 new Core::XmlFormat<Matrix<f64>,
				 Core::XmlMatrixDocument<f64> >(), true);
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<RefernceCountedMatrix<f32> >());
	formats_->registerFormat("xml",
				 new Core::XmlFormat<RefernceCountedMatrix<f32>,
				 Core::XmlMatrixDocument<f32> >(), true);
	formats_->registerFormat("bin",
				 new Core::BinaryFormat<RefernceCountedMatrix<f64> >());
	formats_->registerFormat("xml",
				 new Core::XmlFormat<RefernceCountedMatrix<f64>,
				 Core::XmlMatrixDocument<f64> >(), true);
    }
    return *formats_;
}

EigenvalueProblem *Module_::createEigenvalueProblem(
    const Core::Configuration &configuration) const
{
    EigenvalueProblem *result = 0;
    switch((EigenvalueProblem::Type)EigenvalueProblem::paramType(configuration)) {
    case EigenvalueProblem::typeSymmetric:
	result = new Lapack::SymmetricEigenvalueProblem(configuration);
	break;
    default:
	Core::Application::us()->error("This type of eigenvalue problem is not implemented yet.");
    }
    return result;
}

GeneralizedEigenvalueProblem *Module_::createGeneralizedEigenvalueProblem(
    const Core::Configuration &configuration) const
{
    GeneralizedEigenvalueProblem *result = 0;
    switch((EigenvalueProblem::Type)EigenvalueProblem::paramType(configuration)) {
    case EigenvalueProblem::typeGeneral:
	result = new Lapack::GenEigenProblemWithSchurDecomposition(configuration);
	break;
    case EigenvalueProblem::typeSymmetricPositiveDefinite:
	result = new Lapack::GenSymmetricDefiniteEigenProblem(configuration);
	break;
    default:
	Core::Application::us()->error("This type of generalized eigenvalue problem is not implemented yet.");
    }
    return result;
}
