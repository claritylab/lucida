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
#include <math.h>
#include <Core/Assertions.hh>
#include "SpectralIntegration.hh"

using namespace Flow;
using namespace Signal;







// SpectralIntegration
/////////

SpectralIntegration::SpectralIntegration() :
    length_(0),
    shift_(0),
    windowFunction_(0)
{}

SpectralIntegration::~SpectralIntegration() {

    if (windowFunction_)
	delete windowFunction_;
}


void SpectralIntegration::setWindowFunction(WindowFunction* windowFunction) {

    if (windowFunction_)
	delete windowFunction_;

    windowFunction_ = windowFunction;
}


void SpectralIntegration::setLength(u32 length) {
    if (length_ != length) {
	length_ = length;
    }
}


void SpectralIntegration::setShift(u32 shift) {

    if (shift_ != shift) {
	shift_ = shift;
    }
}



void SpectralIntegration::init() {
    verify(windowFunction_);

}


void SpectralIntegration::apply(const Flow::Vector<Sample> &in, Flow::Vector<Sample> &out) {

  windowFunction_->setLength(length_);
  /*** calculate the number of channels of the output signal ***/
  /*
  u32 tmp;
  tmp=in[0].size()/length_;
  std::cout << "tmp = " << tmp << std::endl;
  tmp*=length_;
  u32 channels=tmp/shift_;                            // this is wrong, bad channel nums calc.: u32 channels=tmp/shift_;    Gammatone with 48 filter, size 9, step 4 -> break-down
  std::cout << "channels = " << channels << std::endl;
  */

  u32 channels = (in[0].size() - length_) / shift_ + 1;

  out.resize(in.size());
  out.setTimestamp(in);
  for (u32 i=0; i< in.size(); i++){
    out[i].resize(channels);

    for (u32 ch=0; ch<channels; ch++){

      for (u32 w=0; w<length_; w++) {
	verify(ch*shift_+w < in[0].size() );
	out[i][ch]+=(windowFunction_->getWindow())[w] * in[i][ch*shift_+w];

      }
    }
  }
}


// SpectralIntegrationNode
/////////////

const Core::ParameterFloat SpectralIntegrationNode::paramShift(
    "shift", "shift of window");

const Core::ParameterFloat SpectralIntegrationNode::paramLength(
    "length", "length of window");

SpectralIntegrationNode::SpectralIntegrationNode(const Core::Configuration &c) :
    Component(c), Predecessor(c) {

    setWindowFunction(WindowFunction::create((WindowFunction::Type)WindowFunction::paramType(c)));
    setShift(u32(paramShift(c)));
    setLength(u32(paramLength(c)));
}

bool SpectralIntegrationNode::setParameter(const std::string &name, const std::string &value) {

    if (WindowFunction::paramType.match(name))
	setWindowFunction(WindowFunction::create((WindowFunction::Type)WindowFunction::paramType(value)));
    else if (paramShift.match(name))
      setShift(u32(paramShift(value)));
    else if (paramLength.match(name))
      setLength(u32(paramLength(value)));
    else
	return false;

    return true;
}


bool SpectralIntegrationNode::configure() {

    Core::Ref<Flow::Attributes> a(new Flow::Attributes());;
    getInputAttributes(0, *a);

    if (!configureDatatype(a, Flow::Vector<Sample>::type()))
	return false;
    reset();

    return putOutputAttributes(0, a);
}


bool SpectralIntegrationNode::work(Flow::PortId p) {

  Flow::DataPtr<Flow::Vector<Sample> > in;
  Flow::Vector<Sample>  *out = new Flow::Vector<Sample >();
    if (!getData(0, in)) {
	if (in == Flow::Data::eos())
	    reset();
	return putData(0, in.get());
    }

    in.makePrivate();
    apply(*in, *out);

    return putData(0, out);
}
