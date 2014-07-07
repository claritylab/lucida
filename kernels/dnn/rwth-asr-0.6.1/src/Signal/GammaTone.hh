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
#ifndef _SIGNAL_GAMMATONE_HH
#define _SIGNAL_GAMMATONE_HH
#include <Flow/Data.hh>
#include <Flow/Vector.hh>
#include <Flow/StringExpressionNode.hh>

namespace Signal {

class WarpingFunction
{
private:
    f32 warpingFactor_;
    f32 freqBreak_, maxFreq_;
    f32 beta_, b_;
    f32 warpedFreqBreak_;
    bool needInit_;
    void init();
public:
    WarpingFunction(f32 warpingFactor, f32 freqBreak, f32 maxFreq);
    void setWarpingFactor(f32 warpingFactor) { warpingFactor_ = warpingFactor; reset(); }
    void setFreqBreak(f32 freqBreak) { freqBreak_ = freqBreak; reset(); }
    void setMaxFreq(f32 maxFreq) { maxFreq_ = maxFreq; reset(); }
    bool checkParam();
    void reset() { needInit_ = true; }

    f32 warping(f32 f);
    f32 inverseWarping(f32 f);
};

/**
  * Provides an auditory-based Gammatone filter bank. The filters are
  * implemented as IIR filters in the time domain
  *
  * For details, see
  *   X. Zhang, M. G. Heinz, I. C. Bruce, L. H. Carney
  *     A phenomenological model for the responses of auditory-nerve fibers:
  *     I. Nonlinear tuning with compression and suppression
  *     JASA, February 2001 109(2), pp 648-670
  *   R. Schlueter, I. Bezrukov, H. Wagner, and H. Ney.
  *     Gammatone Features and Feature Combination for Large Vocabulary
  *     Speech Recognition.
  *     IEEE ICASSP, Honululu, HI, USA, April 2007.
  *
  * Center frequencies of the filters are calculated with the Greenwood
  * function with human parameters:
  *     c = (165.4*10^2.1 * x -1), where x = proportion of basilar membrane length
  * The filter bandwidth is calculated from the center frequency according to
  * the Equivalent Rectangular Bandwidth measure:
  *      ERB(c) = 1.019 * 24.7 * ( (4.37*c)/1000 +1 )
  *
  * The reduction of temporal and spectral dimensionality of the filter bank
  * outputs is performed by the Flow nodes TemporalIntegration and SpectralIntegration.
  */
class GammaTone {
public:
    // structure to store the coefficients of the filter
    struct Coefficient {
	f32 a0, a1, b1, b2;
    };

    // Type of center frequency distribution mode
    enum CenterFrequencyModeType { Human };

private:
    WarpingFunction warp_;

    // configurable parameters
    f32 minFreq_;
    f32 maxFreq_;
    CenterFrequencyModeType centerFrequencyMode_;
    u32 channels_;

    Flow::Time sampleRate_ ;
    u32 cascade_;
    bool needInit_;

    std::vector<std::vector< std::vector<f32> > > buffer_;
    std::vector<f32> centerFrequencyList_;
    std::vector<Coefficient> coefficients_;
    std::vector<f32> bandWidthList_;

    /**
     * Initializes the center frequency list, which is needed for the other
     * initialization functions
     */
    void initializeCenterFrequencyList();

    /**
     * Initializes the bandwidths of each filter to the ERB bandwidth
     */
    void initBandWidths();

    /**
    initialize the coefficients of each filter
    */
    void initCoefficients();

    /**
     * Inverse Greenwood function
     */
    f32 invGreenWoodFunction(f32 cf, const std::vector<f32> &parameters);

public:

    void setCascade(const u32 &cascade) {
	if(cascade_ != cascade) { cascade_ = cascade ; reset(); }
    }
    void setCenterFrequencyMode(CenterFrequencyModeType centerFrequencyMode) {
	if (centerFrequencyMode_ != centerFrequencyMode) { centerFrequencyMode_ = centerFrequencyMode; reset(); }
    }
    void setMinFreq(const f32 &minFreq){
	if (minFreq_ != minFreq) { minFreq_ = minFreq; reset(); }
    }
    void setMaxFreq(const f32 &maxFreq){
	if (maxFreq_ != maxFreq) { maxFreq_ = maxFreq; reset(); }
    }
    void setChannels( const u32 &channels){
	if (channels_ != channels) { channels_ = channels; reset(); }
    }
    void setSampleRate(const Flow::Time &sampleRate){
	if (sampleRate_ != sampleRate) { sampleRate_ = sampleRate; reset(); }
    }

    void setWarpWarpingFactor(const f32 &warpingFactor) { warp_.setWarpingFactor(warpingFactor); reset(); }
    void setWarpFreqBreak(const f32 &freqBreak) { warp_.setFreqBreak(freqBreak); reset(); }
    void setWarpMaxFreq(const f32 &maxFreq) { warp_.setMaxFreq(maxFreq); reset(); }

    GammaTone();
    virtual ~GammaTone() {}

    // calls the other private init() function
    virtual void init(); // There is no check, whether GammaTone is OK. The child object has to do this.
    void reset() { needInit_ = true; }
    bool checkParam() { return warp_.checkParam(); }
    void apply(const Flow::Vector<f32> &in, Flow::Vector<Flow::Vector<f32> > &out);
}; // class GammaTone


/**
  *   Gammatone filter bank node
  *
  *   Parameters:
  *   cascade: Order of the Gammatone filter.
  *     4th or 3rd order filters best represent the response of
  *     human basilar membrane best
  *   minFreq, maxFreq: lower and upper frequency limits
  *   channels: number of the filter bank channels
  *   centerFrequencyMode: mode of center frequency distribution;
  *     currently only human is avaible
  */
class GammaToneNode : public Flow::StringExpressionNode, GammaTone {
private:
    static const Core::ParameterInt paramCascade;
    static const Core::ParameterFloat paramMinFreq;
    static const Core::ParameterFloat paramMaxFreq;
    static const Core::ParameterInt paramChannels;

    static const Core::Choice choiceCenterFrequencyMode;
    static const Core::ParameterChoice paramCenterFrequencyMode;

    static const Core::ParameterFloat paramWarpingFreqBreak;
    static const Core::ParameterString paramWarpingFactor;

public:
    typedef Flow::Vector<Flow::Vector<f32> >  BasiliarMembraneData;
    static std::string filterName() { return "signal-gammatone"; }

    GammaToneNode(const Core::Configuration &c);
    virtual ~GammaToneNode() {}

    Flow::PortId getInput(const std::string &name) {
	return name.empty() ? 0 : StringExpressionNode::getInput(name);
    }
    Flow::PortId getOutput(const std::string &name) { return 0; }
    virtual bool setParameter(const std::string &name, const std::string &value);
    virtual bool configure();
    virtual bool work(Flow::PortId p);

    void reset() { GammaTone::reset(); }
    virtual void init();
};
} // namespace Signal

#endif
