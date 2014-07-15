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
#ifndef _MM_INTEL_OPTIMAZATION_HH
#define _MM_INTEL_OPTIMAZATION_HH

#if (defined(PROC_intel) || defined(PROC_x86_64))
#include "IntelCodeGenerator.hh"
#include "SSE2CodeGenerator.hh"

#include "Utilities.hh"
#include "MixtureFeatureScorerElement.hh"
#include "GaussDensity.hh"
#include "CovarianceFeatureScorerElement.hh"

namespace Mm {

    /** FeatureScorerIntelOptimization
     */
    class FeatureScorerIntelOptimization : public Core::Configurable {
	typedef Configurable Precursor;
    public:
	typedef u8 QuantizedType;
	typedef QuantizedMixtureFeatureScorerElement<QuantizedType> MixtureElement;
	typedef std::vector<QuantizedType> PreparedFeatureVector;
    private:
#if !defined(DISABLE_SIMD)
#if defined(ENABLE_SSE2)
	SSE2L2NormCodeGenerator l2norm_;
	enum { BlockSize = 16 };
#else
	IntelMMXL2NormCodeGenerator l2norm_;
	IntelMMXResetCodeGenerator reset_;
	enum { BlockSize = 8 };
#endif // ENABLE_SSE2
#else
	enum { BlockSize = 8 };
#endif // DISABLE_SIMD

    private:
	/** @return is @param vectorSize rounded up to the next integer divisible by BlockSize. */
	static inline size_t optimalVectorSize(size_t vectorSize) {
	    return size_t((vectorSize + BlockSize - 1) / BlockSize) * (size_t)BlockSize;
	}

    public:
	FeatureScorerIntelOptimization(const Core::Configuration &c, ComponentIndex dimension);

	static void multiplyAndQuantize(const std::vector<FeatureType> &x,
					const std::vector<VarianceType> &y,
					PreparedFeatureVector &r);

	static void createDensityElement(Score scaledMinus2LogWeight,
					 const Mean &mean,
					 const CovarianceFeatureScorerElement &covarianceScorerElement,
					 MixtureElement::Density &result);

	int distanceNoMmx(const PreparedFeatureVector &mean,
			  const PreparedFeatureVector &featureVector) const;

#if defined(DISABLE_SIMD)
	int distance(const PreparedFeatureVector &mean,
		     const PreparedFeatureVector &featureVector) const;
	void resetFloatingPointCalculation() const {}
#else
	/**
	 * When this function is used the processor is set to optimized integer mode.
	 * Before using floating point operations resetFloatingPointCalculation HAS to be called.
	 */
	int distance(const PreparedFeatureVector &mean,
		     const PreparedFeatureVector &featureVector) const {
	    return l2norm_.run(&mean[0], &featureVector[0]);
	}
#if defined(ENABLE_SSE2)
	void resetFloatingPointCalculation() const {}
#else
	void resetFloatingPointCalculation() const { reset_.run(); }
#endif // ENABLE_SSE2

#endif // DISABLE_SIMD

    };

} // namespace Mm

#endif // PROC_intel
#endif //_MM_INTEL_OPTIMAZATION_HH
