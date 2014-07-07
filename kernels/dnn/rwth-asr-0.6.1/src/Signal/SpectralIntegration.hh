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
#ifndef _SIGNAL_SPECTRALINTEGRATION_HH
#define _SIGNAL_SPECTRALINTEGRATION_HH


#include <Core/Parameter.hh>

#include <Flow/Data.hh>
#include <Flow/Vector.hh>
#include "Node.hh"
#include "WindowFunction.hh"



namespace Signal {

  /**
   * Performs a reduction of the spectral dimension of a (Gammatone) filterbank output.
   * A window of the length length is applied and the samples are
   * summed up, weighted by the window function. The window is then shifted along the spectral
   * axis.
   */

  class SpectralIntegration {
  public:
    typedef Flow::Time Time;
    typedef Flow::Vector<f32> Sample;
    typedef SleeveNode Predecessor;
    private:
      u32 length_;
      u32 shift_;
      WindowFunction* windowFunction_;
    protected:
	virtual void init();
    void apply(const Flow::Vector<Sample> &in, Flow::Vector<Sample> &out);
    public:
	SpectralIntegration();
	virtual ~SpectralIntegration();

	void setWindowFunction(WindowFunction* windowFunction);

	void setLength(u32 length);
	u32 length() const { return length_; }

	void setShift(u32 shift);
	u32 shift() const { return shift_; }
  };


    /**
     * Spectral Integration Node
     * Parameters:
     * shift: shift length
     * length: window length
     */
  class SpectralIntegrationNode : public SleeveNode, SpectralIntegration {
    public:
	typedef SleeveNode Predecessor;
    private:
	static const Core::ParameterFloat paramShift;
	static const Core::ParameterFloat paramLength;
    public:
	static std::string filterName() { return "signal-spectralintegration"; }

	SpectralIntegrationNode(const Core::Configuration &c);
	virtual ~SpectralIntegrationNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
      virtual bool work(Flow::PortId p);
    };

}

#endif
