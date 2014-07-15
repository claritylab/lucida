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
// $Id: Regression.cc 9227 2013-11-29 14:47:07Z golik $

#include "Regression.hh"

using namespace Signal;

Regression::Regression() {}

Regression::~Regression() {}

void Regression::regressFirstOrder(const std::vector<const Frame*> &in, Frame &out) {
    // see header file for details on the regression coefficients
    for (u32 c = 0; c < out.size(); ++c)
	out[c] = 0.0;
    f32 tm = 0.0;
    for (u32 i = 0; i < in.size(); ++i) {
	const Frame &f(*in[i]);
	require(f.size() >= out.size());
	f32 dt = f32(i) - f32(in.size()-1) / 2.0;
	for (u32 c = 0; c < out.size(); ++c) {
	    out[c] += dt * f[c];
	}
	tm += dt * dt;
    }
    for (u32 c = 0; c < out.size(); ++c)
	out[c] /= tm;
}

void Regression::regressSecondOrder(const std::vector<const Frame*> &in, Frame &out) {
    for (u32 c = 0; c < out.size(); ++c)
	out[c] = 0.0;
    f32 tm = 0.0;
    f32 ns = 0.0;
    for (u32 i = 0; i < in.size(); ++i) {
	f32 dt = f32(i) - f32(in.size()-1) / 2.0;
	tm += dt * dt;
	ns += dt * dt * dt * dt;
    }
    ns = tm * tm - f32(in.size()) * ns;
    for (u32 i = 0; i < in.size(); ++i) {
	const Frame &f(*in[i]);
	require(f.size() >= out.size());
	f32 dt = f32(i) - f32(in.size()-1) / 2.0;
	for (u32 c = 0; c < out.size(); ++c) {
	    out[c] += f[c] * tm;
	    out[c] -= f[c] * dt * dt * f32(in.size());
	}
    }
    for (u32 c = 0; c < out.size(); ++c)
	out[c] *= 2.0 / ns;
}

const Core::ParameterInt RegressionNode::parameterOrder(
    "order", "order of derivative to calculate", 1, 0);

RegressionNode::RegressionNode(const Core::Configuration &c) :
    Core::Component(c), Precursor(c)
{
    order_ = parameterOrder(config);
}

RegressionNode::~RegressionNode() {}

bool RegressionNode::setParameter(const std::string &name, const std::string &value) {
    if (parameterOrder.match(name))
	order_ = parameterOrder(value);
    else
	return Precursor::setParameter(name, value);
    return true;
}

/**
 * \todo This would be more efficient, if vector dimensions were advertised attributes.
 */
RegressionNode::OutputData *RegressionNode::merge(std::vector<Precursor::InputFrame> &inputData) {
    std::vector<const Regression::Frame*> in(inputData.size());
    size_t d = Core::Type<u32>::max;
    for (u32 i = 0; i < inputData.size(); ++i) {
	d = std::min(d, inputData[i]->size());
	in[i] = inputData[i].get();
    }

    OutputData *out = new OutputData(d);
    switch (order_) {
    case 1:
	regressFirstOrder(in, *out);
	break;
    case 2:
	regressSecondOrder(in, *out);
	break;
    default:
	criticalError("signal-regression only implemented for 1st and 2nd order derivatives");
    }
    return out;
}
