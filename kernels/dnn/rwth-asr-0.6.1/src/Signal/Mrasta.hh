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
#ifndef _SIGNAL_MRASTA_FILTERING_HH
#define _SIGNAL_MRASTA_FILTERING_HH

#include <string>
#include <vector>

#include <Core/Parameter.hh>
#include <Math/Vector.hh>
#include <Math/Matrix.hh>
#include <Flow/Node.hh>
#include <Flow/Vector.hh>

namespace Signal {

static const u32 maxFilter = 8;

/** Implementation of the MRasta filtering as introduced in
 *  "Multi-resolution RASTA filtering for TANDEM-based ASR" by
 *  Hynek Hermansky, Petr Fousek in Interspeech 2005
 */
class MrastaFiltering {
public:
    typedef f32 Value;
private:
    Math::Vector<Value> inputBuffer_;
    Math::Vector<Value> outputBuffer_;

    bool needInit_;
    static const float sigma_[];

    u32 nFeatures_;   // feature dimension
    u32 nFrames_;     // number of concatenated frames in a sliding window
    u32 nDerivatives_; // number of derivatives (0, 1, 2) w.r.t. filter bands (not time)
    u32 nFilters_;     // number of MRASTA filters (up to 8)

    Math::Vector<Math::Matrix<Value> > filters_;

    bool init();
    inline float sigma(int index) const;

    void setFilters(Math::Vector<Math::Matrix<Value> >& filters);
    Math::Vector<Math::Matrix<Value> >& filters() { return filters_; }
    void initGaussianFilters(size_t nFilter, size_t nFrames);
    void resizeFilters(size_t nFilter, size_t nFrames);
    inline void normalizeFilterResponse(Math::Matrix<Value>& G, int filter_num);
    Math::Matrix<Value>& getG0() { return filters_[0]; }
    Math::Matrix<Value>& getG1() { return filters_[1]; }
    Math::Matrix<Value>& getG2() { return filters_[2]; }

    void getBand(size_t band, std::vector<Value> &in, std::vector<Value> &out);
    void setBand(size_t band, std::vector<Value> &in, std::vector<Value> &out);
    void filterEnergyBand(Math::Vector<Value> &in, Math::Vector<Value> &response);
    void appendDerivatives(std::vector<Value> &in);

    void setFeatures(u32 nFeatures) { if (nFeatures_ != nFeatures) { nFeatures_ = nFeatures; needInit_ = true; } }
    void setFrames(u32 nFrames) { if (nFrames_ != nFrames) { nFrames_ = nFrames; needInit_ = true; } }
    void setDerivatives(u32 nDerivatives) {
	if ( ((int) nDerivatives_ >= 0) && ((int) nDerivatives_ <= 2) ) {
	    nDerivatives_ = nDerivatives;
	} else {
	    nDerivatives_ = 0;
	}
	needInit_ = true;
    }
    void setFilter(u32 nFilter) { if (nFilters_ != nFilter) { nFilters_ = nFilter; needInit_ = true; } }

public:
    void init(size_t nFeatures, size_t nFrames, size_t nFilters, size_t nDerivatives) {
	setFeatures(nFeatures);
	setFrames(nFrames);
	setFilter(nFilters);
	setDerivatives(nDerivatives);
    }

    MrastaFiltering();
    virtual ~MrastaFiltering();

    /** Applies the Gaussian filtering to @param in. */
    bool apply(std::vector<Value> &in, std::vector<Value> &out);
    /** result is s32, so that we can detect overflows/wraps due to bad nFeatures_ values */
    s32 getOutputDimension() { return ( 2 * nFilters_ * (nFeatures_ + ( (nFeatures_-2) * nDerivatives_ ) ) ); }
};


/** Apply the MRASTA filtering to critical band energies (CRBE)
 *  Input:  sequence of concatenated CRBE vectors.
 *  Output: MRASTA filtered bands.
 *  Parameters:
 *  - number of Gaussian filters
 *  - context length of input sequence
 *  - number of derivatives
 */
class MrastaFilteringNode : public Flow::SleeveNode, MrastaFiltering {
    typedef Flow::SleeveNode Precursor;
    typedef f32 Value;
private:
    // parameter
    size_t contextLength_;
    size_t nDerivatives_;
    size_t nGaussFilters_;
    bool needInit_;

    static const Core::ParameterInt paramContextLength;
    static const Core::ParameterInt paramDerivatives;
    static const Core::ParameterInt paramGaussFilters;

    void setContextLength(size_t length) { if (contextLength_ != length) { contextLength_ = length; needInit_ = true; } };
    void setDerivative(size_t number) { if (nDerivatives_ != number) { nDerivatives_ = number; needInit_ = true; } };
    void setGaussFilter(size_t number) { if (nGaussFilters_ != number) { nGaussFilters_ = number; needInit_ = true; } };

    void init(size_t length);
public:
    static std::string filterName() { return std::string("mrasta-filtering"); };

    MrastaFilteringNode(const Core::Configuration &c);
    virtual ~MrastaFilteringNode();

    virtual bool configure();
    virtual bool setParameter(const std::string &name, const std::string &value);
    virtual bool work(Flow::PortId p);
};
}

#endif // _SIGNAL_MRASTA_FILTERING_HH
