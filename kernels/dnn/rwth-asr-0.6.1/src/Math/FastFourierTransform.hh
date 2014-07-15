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
#ifndef _MATH_FASTFOURIERTRANSFORM_HH
#define _MATH_FASTFOURIERTRANSFORM_HH

#include <Core/Types.hh>
#include <vector>

namespace Math
{
    /**
     * Fast Fourier Transformation.
     * Based on the implementation in "Numerical Recipes in C"
     * @todo Optimization, e.g. use of SSE instructions
     */
    class FastFourierTransform
    {
    public:
	/**
	 * Initialize DFFT for arrays of size  @c size
	 * @param size size of arrays to transform, has to be 2^n
	 */
	FastFourierTransform();

	/**
	 * Apply discrete Fourier transform to complex values in @c v.
	 * Transformation is done in place.
	 *
	 * @param v vector of complex values, has to be of size 2^n
	 * @param inverse apply inverse discrete fourier transform
	 */
	void transform(std::vector<f32> &v, bool inverse = false);

	/**
	 * Apply discrete Fourier transform to real values in @c v.
	 * Transformation is done in place.
	 *
	 * @param v values, vector has to be of size 2^n
	 * @param inverse apply inverse discrete fourier transform
	 */
	void transformReal(std::vector<float> &v, bool inverse = false);

    protected:
	void setSize(u32 size);
	/**
	 * Reorder values in the array &v by bit reversal.
	 * @param v vector of complex values
	 */
	void bitReversalReordering(std::vector<f32> &v);

	/**
	 * create the mapping for bit reversal reording
	 * @param length number of complex values (array size / 2)
	 */
	void createBitReversalReordering(u32 length);
	std::vector<size_t> reording_;
	u32 size_;

    private:
	// Defined for numerical compatibility against NR implementation
	static const f64 Pi, DPi;
    };

}

#endif // _MATH_FASTFOURIERTRANSFORM_HH
