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
#ifndef _MATH_LEVINSON_LSE_HH
#define _MATH_LEVINSON_LSE_HH

#include "Matrix.hh"

namespace Math {

    /** Levinson algorithm to estimate all-pole linear predication parameters.
     *  y[n] = gain delta(t) - a1 y[n-1] - a2 y[n-1] - ... - aN y[n-N]
     *  Algorithm see in Rabiner-Schafer: Digital Processing of Speech Signal, 1978, page 411.
     */
    class LevinsonLeastSquares {
    public:
	typedef f64 Data;
	typedef f32 InputData;
    private:
	Vector<Data> E_;
	Vector<Data> k_;
	Matrix<Data> alpha_;
    private:
	void setOrder(size_t);
	size_t order() const { return E_.size() - 1; }
    public: // constructor & destructor
	LevinsonLeastSquares() {}
	virtual ~LevinsonLeastSquares() {};

	/** Performs the Levinson algorithm on the autocorrelation coeffitients (@param R) */
	bool work(const std::vector<InputData> &R);

	/** Retrieves calculated parameter gain. */
	Data gain() const { return sqrt(predictionError()); }
	/** Retrieves calculated parameters a1..aN */
	template<class T> void a(std::vector<T> &result) const;
	/** Retrieves remaining prediction error. */
	Data predictionError() const { return E_[order()]; }
    };

    template<class T>
    void LevinsonLeastSquares::a(std::vector<T> &result) const {
	size_t N = order(); result.resize(N);
	for(size_t j = 1; j <= N; j++) result[j - 1] = alpha_[j][N];
    }

} // namespace Math

#endif // _MATH_LEVINSON_LSE_HH
