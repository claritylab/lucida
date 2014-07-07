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
#include "AutoregressionToCepstrum.hh"
#include "ArEstimator.hh"

using namespace Signal;

//===============================================================================================
void Signal::autoregressionToCepstrum(f32 gain, const std::vector<f32> &a, std::vector<f32> &c)
{
    require(c.size() >= 2);
    require(c.size() <= (a.size() + 1));

    size_t nCepstrum = c.size();

    c[0] = 2 * log(gain);
    c[1] = -a[0];

    for(size_t n = 2; n < nCepstrum; ++ n) {
	c[n] = n * a[n - 1];
	for(size_t k = 1; k < n; ++ k)
	    c[n] += (n - k) * c[n - k] * a[k - 1];
	c[n] /= (-(f32)n);
    }
}

//===============================================================================================
const Core::ParameterInt AutoregressionToCepstrumNode::paramOutputSize(
    "nr-outputs", "number of outputs");

AutoregressionToCepstrumNode::AutoregressionToCepstrumNode(const Core::Configuration &c) :
    Core::Component(c), Precursor(c),
    outputSize_(0),
    needInit_(true)
{
    setOutputSize(paramOutputSize(c));
}

AutoregressionToCepstrumNode::~AutoregressionToCepstrumNode()
{}

bool AutoregressionToCepstrumNode::configure()
{
    Core::Ref<Flow::Attributes> attributes(new Flow::Attributes());
    getInputAttributes(0, *attributes);
    if (!configureDatatype(attributes, AutoregressiveCoefficients::type()))
	return false;
    attributes->set("sample-rate", 1);
    attributes->set("datatype", Flow::Vector<f32>::type()->name());
    return putOutputAttributes(0, attributes);
}

bool AutoregressionToCepstrumNode::setParameter(const std::string &name, const std::string &value)
{
    if (paramOutputSize.match(name))
	setOutputSize(paramOutputSize(value));
    else
	return false;
    return true;
}

void AutoregressionToCepstrumNode::init(size_t autoregressiveCoefficients)
{
    if (outputSize_ < 2 || outputSize_ > (autoregressiveCoefficients + 1))
	error("Incorrect output size (%zd). 2 < nr-outputs <= %zd.", outputSize_, autoregressiveCoefficients + 1);
    respondToDelayedErrors();
    needInit_ = false;
}

bool AutoregressionToCepstrumNode::work(Flow::PortId p)
{
    Flow::DataPtr<AutoregressiveCoefficients> arCoefficients;
    if (getData(0, arCoefficients)) {
	if (needInit_) init(arCoefficients->a().size());
	Flow::Vector<f32> *out = new Flow::Vector<f32>(outputSize_);
	out->setTimestamp(*arCoefficients);
	autoregressionToCepstrum(arCoefficients->gain(), arCoefficients->a(), *out);
	return putData(0, out);
    }
    return putData(0, arCoefficients.get());
};
