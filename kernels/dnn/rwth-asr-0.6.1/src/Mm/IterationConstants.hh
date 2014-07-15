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
#ifndef _MM_ITERATION_CONSTANTS_HH
#define _MM_ITERATION_CONSTANTS_HH

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include "Types.hh"
#include "EbwDiscriminativeMixtureSetEstimator.hh"
#include "EbwDiscriminativeMixtureEstimator.hh"

namespace InternalIc {
    struct AugmentedDensityEstimator;
    class CovarianceToDensitySetMap;
}

namespace Mm {

    /**
     * IterationConstants: base class
     */
    class IterationConstants : public Core::Component
    {
	typedef Core::Component Precursor;
    protected:
	typedef AbstractMixtureSetEstimator::MixtureEstimators MixtureEstimators;
    private:
	enum Type { globalRwth, localRwth, cambridge };
	static Core::Choice choiceType;
	static Core::ParameterChoice paramType;
	static const Core::ParameterFloat paramBeta;
	static const Core::ParameterFloat paramStepSize;
	static const Core::ParameterFloat paramMinimumIcd;
    protected:
	const IterationConstant default_;
	IterationConstant beta_;
	IterationConstant stepSize_;
    private:
	void dumpIterationConstants(const MixtureEstimators &);
    protected:
	virtual void initialize(
	    const MixtureEstimators &, const InternalIc::CovarianceToDensitySetMap &,
	    const CovarianceToMeanSetMap &) = 0;
	virtual void set(const InternalIc::AugmentedDensityEstimator &) = 0;
    public:
	IterationConstants(const Core::Configuration &);
	virtual ~IterationConstants() {}

	/**
	 *  Set iteration constant for each density.
	 */
	void set(const MixtureEstimators &, const CovarianceToMeanSetMap &);

	static IterationConstants* createIterationConstants(const Core::Configuration &);
    };

} //namespace Mm

#endif //_MM_ITERATION_CONSTANTS_HH
