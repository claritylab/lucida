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
#ifndef _SIGNAL_AUTOREGRESSION_TO_CEPSTRUM_HH
#define _SIGNAL_AUTOREGRESSION_TO_CEPSTRUM_HH

#include <Flow/Node.hh>

namespace Signal
{
    /** Calculate cepstrum coefficients from autoregressive coefficients.
     *  c[n] = a[n] + sum_{k=1}^{n-1} ((n-k)/n c[n-k] a[k]),
     *  c[0] = 2 log(gain), where
     *  c contains the cepstrum coefficients, a constains the autoregressive coefficients (starting at a1),
     *  gain is autoregression gain = sqrt(prediction error).
     */
    void autoregressionToCepstrum(f32 gain, const std::vector<f32> &a, std::vector<f32> &c);

    /** Calculate cepstrum coefficients from autoregressive coefficients.
     *  Input: autoregressive-parameter.
     *  Output cepstrum coefficients.
     *  Parameter: number of cepstrum coeffitients. (It has to be smaller than number of
     *    autoregressive coefficients + 1.)
     */
    class AutoregressionToCepstrumNode : public Flow::SleeveNode {
	typedef Flow::SleeveNode Precursor;
    private:
	static const Core::ParameterInt paramOutputSize;
    private:
	size_t outputSize_;
	bool needInit_;
    private:
	void setOutputSize(size_t size) { if (outputSize_ != size) { outputSize_ = size; needInit_ = true; } }
	void init(size_t autoregressiveCoefficients);
    public:
	static std::string filterName() {
	    return std::string("signal-autoregression-to-cepstrum");
	}
	AutoregressionToCepstrumNode(const Core::Configuration &);
	virtual ~AutoregressionToCepstrumNode();

	virtual bool configure();
	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool work(Flow::PortId p);
    };
}

#endif // _SIGNAL_AUTOREGRESSION_TO_CEPSTRUM_HH
