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
#include "AutoregressionToSpectrum.hh"
#include "ArEstimator.hh"

using namespace Signal;

//===============================================================================================
const f64 AutoregressionToSpectrumNode::Pi = 3.141592653589793238;

const Core::ParameterInt AutoregressionToSpectrumNode::paramOutputSize(
		"nr-outputs", "number of outputs");
const Core::ParameterBool AutoregressionToSpectrumNode::paramSlimSpectrum(
		"slim", "slim spectrum to skip bad band/channels (yes/no)", true);

AutoregressionToSpectrumNode::AutoregressionToSpectrumNode(const Core::Configuration &c) :
		Core::Component(c), Precursor(c), outputSize_(0),
		needInit_(true) {
	setOutputSize(paramOutputSize(c));
	setSlimSpectrum(paramSlimSpectrum(c));
}

AutoregressionToSpectrumNode::~AutoregressionToSpectrumNode() {}

bool AutoregressionToSpectrumNode::configure() {
	Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
	getInputAttributes(0, *attributes);
	if (!configureDatatype(attributes, Signal::AutoregressiveCoefficients::type()))
		return false;
	attributes->set("sample-rate", 1);
	attributes->set("datatype", Flow::Vector<Signal::AutoregressiveCoefficients::Coefficient>::type()->name());
	return putOutputAttributes(0, attributes);
}

bool AutoregressionToSpectrumNode::setParameter(const std::string &name, const std::string &value) {
	if (paramOutputSize.match(name))
		setOutputSize(paramOutputSize(value));
	else if (paramSlimSpectrum.match(name))
		setSlimSpectrum(paramSlimSpectrum(value));
	else return false;

	return true;
}

void AutoregressionToSpectrumNode::init(size_t autoregressiveCoefficients) {
	// check the size of the incoming/outgoing elements, report errors
	if (outputSize_ == 0 ) {
		error("Incorrect output size (%zd). Output size > %zd.", outputSize_, autoregressiveCoefficients +1);
	}
	respondToDelayedErrors();

	// resize the real and imaginary parts
	size_t nBands = outputSize_ + ((slimSpectrum_) ? 2 : 0); // 2 depending on the bark skala
	realPart_.resize(nBands, autoregressiveCoefficients +1);
	imaginaryPart_.resize(nBands, autoregressiveCoefficients +1);

	// Pre-calculation of the real and imaginary part
	Value omega;
	for (size_t i = 0; i < nBands; i++) {
		for (size_t j = 0; j < autoregressiveCoefficients +1; j++) {
			omega = i * j * (Pi / (nBands - 1));
			realPart_[i][j] = cos(omega);
			imaginaryPart_[i][j] = sin(omega);
		}
	}

	// all inits done
	needInit_ = false;
}

bool AutoregressionToSpectrumNode::work(Flow::PortId p) {
	Flow::DataPtr<Signal::AutoregressiveCoefficients> arCoefficients;
	if (getData(0, arCoefficients)) {
		if (needInit_) init(arCoefficients->a().size());
		Flow::Vector<Value> *out = new Flow::Vector<Value>(outputSize_);
		out->setTimestamp(*arCoefficients);
		autoregressionToSpectrum(arCoefficients->gain(), arCoefficients->a(), *out);
		return putData(0, out);
	}
	return putData(0, arCoefficients.get());
};

/*	Icsi code:
 *	- real += (a[j-1] / sc) * cos((i+1)*(j)*(Pi/(nBands -1)));
 *	- imaginary -= (a[j-1] / sc) * sin((i+1)*(j)*(Pi/(nBands -1)));
 *  */
void AutoregressionToSpectrumNode::autoregressionToSpectrum(Value gain, const std::vector<Value> &a, std::vector<Value> &c) {
	require(c.size() > (a.size() + 1));

	// get the spectrum/crbe values
	for (size_t i = 0; i < c.size(); i++) {
		Value real = 1.0;
		Value imaginary = 0.0;

		for (size_t j = 1; j <= a.size(); j++) { // regression coefficients (12)
			if (slimSpectrum_) {
				real += a[j-1]  * realPart_[i+1][j];
				imaginary -= a[j-1] * imaginaryPart_[i+1][j];
			} else {
				real += a[j-1] * realPart_[i][j];
				imaginary -= a[j-1] * imaginaryPart_[i][j];
			}
		}
		c[i] = gain * gain / (real * real + imaginary * imaginary);
	}
}
