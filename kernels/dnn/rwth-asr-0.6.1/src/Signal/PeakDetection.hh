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
#ifndef _SIGNAL_PEAK_DETECTION_HH
#define _SIGNAL_PEAK_DETECTION_HH


#include <Core/Choice.hh>
#include <Core/Parameter.hh>
#include <Flow/Vector.hh>

#include "Node.hh"


namespace Signal {


    /** PeakDetection
     */

    class PeakDetection {
    public:
	typedef f32 Amplitude;
    private:
	f32 continuousMinPosition_;
	u32 minPosition_;
	f32 continuousMaxPosition_;
	u32 maxPosition_;
	f32 continuousMaxWidth_;
	u32 maxWidthHalf_;
	f32 continuousHeightAverageWidth_;
	u32 heightAverageWidthHalf_;
	f32 widthHeightFactor_;
	bool mountainWidth_;
	f64 sampleRate_;
	bool needInit_;
    private:
	u32 getMaximumIndex(const std::vector<Amplitude> &v) const;
	u32 getMaximalPeakIndex(const std::vector<Amplitude> &v) const;
	Amplitude getAverageHeight(const std::vector<Amplitude> &v, const u32 peakIndex) const;
	f32 getHeightConfidence(const Amplitude peakValue, const Amplitude averageHeight) const;
	Amplitude getWidthHeight(const Amplitude peakValue, const Amplitude averageHeight) const;
	u32 getPeakWidth(const std::vector<Amplitude> &v,
			     const u32 peakPos,
			     const Amplitude widthHeight) const;
	u32 getMountainWidth(const std::vector<Amplitude> &v,
			     const u32 peakIndex,
			     const Amplitude widthHeight) const;
	f32 getWidthConfidence(const std::vector<Amplitude> &v,
				   const u32 peakIndex,
				   const Amplitude peakValue,
				   const Amplitude averageHeight) const;
	void init();
    public:
	PeakDetection();

	/** getMaximumPosition = position of maximum in continous unit depending on previous nodes */
	f32 getMaximumPosition(const std::vector<Amplitude> &v);

	/** getMaximalPeakPosition = position of maximal peak
	    in continous unit depending on previous nodes */
	f32 getMaximalPeakPosition(const std::vector<Amplitude> &v);
	/** getMaximum = maximum */
	Amplitude getMaximum(const std::vector<Amplitude> &v);

	/** getMaximalPeakValue = value of maximal peak */
	Amplitude getMaximalPeakValue(const std::vector<Amplitude> &v);

	/** getNormalizedMaximum = (maximal value) / (average value) */
	Amplitude getNormalizedMaximum(const std::vector<Amplitude> &v);
	/** getHeightConfidence = min((max value) / (average value), 2) - 1 */
	f32 getHeightConfidence(const std::vector<Amplitude> &v);
	/** getWidthConfidence = (width of peak) / maxWidth */
	f32 getWidthConfidence(const std::vector<Amplitude> &v);
	/** getConfidence = heightConfidence * widthConfidence */
	f32 getConfidence(const std::vector<Amplitude> &v);
    public:
	void setContinuousMinPosition(const f32 continuousMinPosition) {
	    continuousMinPosition_ = continuousMinPosition; needInit_ = true;
	}

	f32 continuousMinPosition() const { return continuousMinPosition_; }
	u32 minPosition();

	void setContinuousMaxPosition(const f32 continuousMaxPosition) {
	    continuousMaxPosition_ = continuousMaxPosition; needInit_ = true;
	}

	f32 continuousMaxPosition() const { return continuousMaxPosition_; }
	u32 maxPosition();

	void setContinuousMaxWidth(const f32 continuousMaxWidth) {
	    continuousMaxWidth_ = continuousMaxWidth; needInit_ = true;
	}

	void setContinuousHeightAverageWidth(const f32 continuousHeightAverageWidth) {
	    continuousHeightAverageWidth_ = continuousHeightAverageWidth; needInit_ = true;
	}

	void setWidthHeightFactor(const f32 widthHeightFactor) { widthHeightFactor_ = widthHeightFactor; }
	void setMountainWidth(const bool mountainWidth) { mountainWidth_ = mountainWidth; }

	void setSampleRate(const f64 sampleRate) { sampleRate_ = sampleRate; needInit_ = true; }
	f64 sampleRate() { return sampleRate_; }
    };


    /** PeakDetectionNode
     */

    class PeakDetectionNode : public Flow::Node, public PeakDetection {
    private:
	static Core::ParameterFloat paramMinPosition;
	static Core::ParameterFloat paramMaxPosition;
	static Core::ParameterFloat paramMaxWidth;
	static Core::ParameterFloat paramHeightAverageWidth;
	static Core::ParameterFloat paramWidthHeightFactor;
	static Core::ParameterBool paramLogInput;
	static Core::ParameterBool paramMountainWidth;
    private:
	bool putData(const Flow::PortId port, const Flow::Timestamp &timestamp, const f32 v);
	bool putData(Flow::Data *d);
    public:
	static std::string filterName() { return "signal-peak-detection"; }
	PeakDetectionNode(const Core::Configuration &c);
	virtual ~PeakDetectionNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual Flow::PortId getInput(const std::string &name)  { return 0; }
	virtual Flow::PortId getOutput(const std::string &name);

	virtual bool work(Flow::PortId p);
    };
}



#endif // _SIGNAL_PEAK_DETECTION_HH
