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
#ifndef _SIGNAL_AUTOREGRESSION_TO_SPECTRUM_HH
#define _SIGNAL_AUTOREGRESSION_TO_SPECTRUM_HH

#include <Flow/Node.hh>
#include <Math/Matrix.hh>

namespace Signal {

/** Calculate spectrum coefficients (CRBE) from autoregressive coefficients.
 *  Input: autoregressive-parameter.
 *  Output spectrum coefficients.
 *  Parameter: number of spectrum coefficients.
 */
class AutoregressionToSpectrumNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
	typedef f32 Value;
private:
	// Defined for numerical compatibility against NR implementation, see FastForierTransformation
	static const f64 Pi;
private:
	size_t outputSize_;
	bool slimSpectrum_;
	bool needInit_;

	// Real and imaginary part
	Math::Matrix<f32> realPart_;
	Math::Matrix<f32> imaginaryPart_;
private:
	static const Core::ParameterInt paramOutputSize;
	static const Core::ParameterBool paramSlimSpectrum;
private:
	void setOutputSize(size_t size) { if (outputSize_ != size) { outputSize_ = size; needInit_ = true; } };
	void setSlimSpectrum(bool slim) { if (slimSpectrum_ != slim) { slimSpectrum_ = slim; needInit_ = true;} };

	void init(size_t autoregressiveCoefficients);
public:
	static std::string filterName() { return std::string("nn-autoregression-to-spectrum"); };

	AutoregressionToSpectrumNode(const Core::Configuration &c);
	virtual ~AutoregressionToSpectrumNode();

	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(Flow::PortId p);

private:
	/** Calculate cepstrum coefficients from autoregressive coefficients.
	 *  c[n] = a[n] + sum_{k=1}^{n-1} ((n-k)/n c[n-k] a[k]),
	 *  c[0] = 2 log(gain), where
	 *  c contains the cepstrum coefficients, a contains the autoregressive coefficients (starting at a1),
	 *  gain is autoregression gain = sqrt(prediction error).
	 */
	void autoregressionToSpectrum(Value gain, const std::vector<Value> &a, std::vector<Value> &c);
};

}

#endif // _SIGNAL_AUTOREGRESSION_TO_SPECTRUM_HH
