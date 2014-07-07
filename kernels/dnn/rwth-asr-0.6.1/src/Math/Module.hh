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
#ifndef _MATH_APPLICATION_HH
#define _MATH_APPLICATION_HH

#include <Core/Singleton.hh>
#include <Core/FormatSet.hh>

namespace Math {

    class EigenvalueProblem;
    class GeneralizedEigenvalueProblem;

    class Module_
    {
    private:
	Core::FormatSet *formats_;
    public:
	Module_();
	~Module_();

	/**
	 *  Set of file format class.
	 */
	Core::FormatSet &formats();

	/**
	 *  Creates object for solving eigenvalue problem
	 *  with the given configuration.
	 */
	EigenvalueProblem *createEigenvalueProblem(
	    const Core::Configuration &) const;
	/**
	 *  Creates  object for solving generalized eigenvalue problem
	 *  with the given configuration.
	 */
	GeneralizedEigenvalueProblem *createGeneralizedEigenvalueProblem(
	    const Core::Configuration &) const;
    };

    typedef Core::SingletonHolder<Module_> Module;
}

#endif // _MATH_APPLICATION_HH
