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
#ifndef _SIGNAL_NORMALIZATION_HH
#define _SIGNAL_NORMALIZATION_HH

#include <Core/Parameter.hh>
#include <Flow/Vector.hh>
#include <Flow/VectorScalarFunction.hh>
#include "Node.hh"
#include "SlidingWindow.hh"

namespace Signal {

    /** Base class for normalization algorithms.
     *  Processing:
     *    updateStatistics is called if no new data entered or old data left the sliding window
     *    finalize() called right before normalization is applied to an output vector
     *    apply() called to perform normalization
     *  Interface:
     *    Use update(..) to put new data into the sliding window and get the latest normalized output
     *    Use flush to get normalized output without any input
     */
    class Normalization {
    public:
	typedef f32 Value;
	typedef f64 Sum;
	typedef Flow::DataPtr<Flow::Vector<Value> > Frame;
    protected:
	SlidingWindow<Frame> slidingWindow_;
	u32 length_;
	u32 right_;

	Sum sumWeight_;

	bool changed_;
    private:
	void normalize(Frame &out);
    protected:
	virtual bool init(size_t dimension) { return true; }
	virtual void updateStatistics(const Frame &add, const Frame &remove);
	virtual void finalize();
	virtual void apply(Frame &out) = 0;
    public:
	Normalization();
	virtual ~Normalization() {}

	/** init:
	 *  @param length is length of sliding window.
	 *  @param right is the output point calculated from the right side of the sliding window.
	 *    (Note: data flows into the sliding window from right to left.)
	 *  @param dimension is dimension of the input vectors.
	 */
	bool init(size_t length, size_t right, size_t dimension);

	/**
	 *  Puts in into the sliding window and
	 *  normalizes the output vector if any.
	 *  @return is true if out is valid
	 */
	bool update(const Frame &in, Frame &out);
	/**
	 *   Normalizes the next remaining output vector if any.
	 *   @return is true if out is valid
	 */
	bool flush(Frame &out);
	virtual void reset();
    };

    /** Maximum of the sliding window gets substracted from the output.
     *  Typical application is normalization of zeroth ceptrum coefficient.
     */
    class LevelNormalization : public Normalization {
	typedef Normalization Precursor;
    protected:
	size_t index_;
	Value max_;
    protected:
	virtual void finalize();
	virtual void apply(Frame &out);
    public:
	LevelNormalization(size_t index) : index_(index), max_(0) {}
    };

    /** Arithmetic average of the sliding window is substracted from the output.
     */
    class MeanNormalization : public Normalization {
	typedef Normalization Precursor;
    protected:
	std::vector<Sum> sum_;
	std::vector<Value> mean_;
    protected:
	virtual bool init(size_t dimension);
	virtual void reset();
	virtual void updateStatistics(const Frame &add, const Frame &remove);
	virtual void finalize();
	virtual void apply(Frame &out);
    };

    /** Arithmetic average of the sliding window is substracted from the output,
     *  followed by division with standard deviation.
     */
    class MeanAndVarianceNormalization : public Core::Component, public MeanNormalization {
	typedef MeanNormalization Precursor;
    protected:
	std::vector<Sum> sumSquare_;
	std::vector<Value> standardDeviation_;
    protected:
	virtual bool init(size_t dimension);
	virtual void reset();
	virtual void updateStatistics(const Frame &add, const Frame &remove);
	virtual void finalize();
	virtual void apply(Frame &out);
    public:
	MeanAndVarianceNormalization(const Core::Configuration&);
    };

    /** 1D MVN.
     */
    class MeanAndVarianceNormalization1D : public Core::Component, public Normalization {
	typedef Normalization Precursor;
    protected:
	Sum sum_;
	Sum sumSquare_;
	Value mean_;
	Value standardDeviation_;
    protected:
	virtual bool init(size_t dimension);
	virtual void reset();
	virtual void updateStatistics(const Frame &add, const Frame &remove);
	virtual void finalize();
	virtual void apply(Frame &out);
    public:
	MeanAndVarianceNormalization1D(const Core::Configuration&);
    };

    /**
     *  Output is divided by arithmetic mean of the sliding window.
     */
    class DivideByMean : public Core::Component, public MeanNormalization {
	typedef MeanNormalization Precursor;
    protected:
	virtual void finalize();
	virtual void apply(Frame &out);
    public:
	DivideByMean(const Core::Configuration &);
    };

    /**
     *  Output is divided by the arithmetic mean of the norm of the elements of the sliding window.
     *
     *  In case of the 2nd norm, this normalization ensures that average frame energy will be 1.
     */
    class MeanNormNormalization : public Core::Component, public Normalization {
	typedef Normalization Precursor;
    protected:
	Sum sumOfNorms_;
	Value averageNorm_;

	f32 norm_;
	Flow::NormFunction<Value> normFunction_;
    protected:
	virtual void reset();
	virtual void updateStatistics(const Frame &add, const Frame &remove);
	virtual void finalize();
	virtual void apply(Frame &out);
    public:
	MeanNormNormalization(const Core::Configuration&, f32 norm);
    };

    /** NormalizationNode
     */
    class NormalizationNode : public SleeveNode {
	typedef SleeveNode Precursor;
    private:
	enum Type {
	    typeLevel,
	    typeMean,
	    typeMeanAndVariance,
	    typeMeanAndVariance1D,
	    typeDivideByMean,
	    typeMeanNorm
	};

	static const Core::Choice typeChoice;
	static const Core::ParameterChoice paramType;
	static const Core::ParameterInt paramLength;
	static const Core::ParameterInt paramRight;
	static const Core::ParameterInt paramLevelIndex;
	static const Core::ParameterFloat paramNorm;
    private:
	Normalization *algorithm_;

	Type type_;
	void setType(Type type) {
	    if (type_ != type) { type_ = type; needInit_ = true; }
	}

	size_t length_;
	void setLength(size_t length) {
	    if (length_ != length) { length_ = length; needInit_ = true; }
	}

	size_t right_;
	void setRight(size_t right) {
	    if (right_ != right) { right_ = right; needInit_ = true; }
	}

	size_t levelIndex_;
	void setLevelIndex(size_t index) {
	    if (levelIndex_ != index) { levelIndex_ = index; needInit_ = true; }
	}

	f32 norm_;
	void setNorm(f32 norm) {
	    if (norm_ != norm) { norm_ = norm; needInit_ = true; }
	}

	bool needInit_;
	bool init(size_t dimension);
	bool update(const Normalization::Frame &in, Normalization::Frame &out);
	bool flush(Normalization::Frame &out);
	void reset();
    public:
	static std::string filterName() { return "signal-normalization"; }
	NormalizationNode(const Core::Configuration &c);
	virtual ~NormalizationNode();

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };

}


#endif // _SIGNAL_NORMALIZATION_HH
