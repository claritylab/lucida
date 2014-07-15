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
#ifndef _SIGNAL_FAST_FOURIER_TRANSFORM_HH
#define _SIGNAL_FAST_FOURIER_TRANSFORM_HH


#include <Core/Types.hh>
#include <Math/FastFourierTransform.hh>
#include "ComplexVectorFunction.hh"

namespace Signal {


    /** FastFourierTransform: base class to perform fast fourier transform.
     */
    class FastFourierTransform {
    public:
	typedef f32 Data;
    protected:
	u32 length_;
	/** sample rate of input vector. */
	f64 sampleRate_;
	Math::FastFourierTransform fft_;
	std::string lastError_;

    protected:
	virtual bool zeroPadding(std::vector<Data> &data);
	virtual bool applyAlgorithm(std::vector<Data> &data) = 0;
	/** Calculates an estimation of the continuous values.
	 *  Each element is multiplied by {\Delta t} resp. {\Delta \omega}.
	 */
	virtual bool estimateContinuous(std::vector<Data> &data);
    public:
	FastFourierTransform(u32 length = 0, Data sampleRate = 1);
	virtual ~FastFourierTransform() {}

	virtual bool transform(std::vector<Data> &data);

	/** @return number of FFT points. */
	u32 length() const { return length_; }
	/** sets the number of FFT points (length_) as the
	 *  -smallest number which is
	 *  -power of 2
	 *  -larger than @param length
	 *
	 * @return number of FFT points.
	 */
	u32 setLength(u32 length);

	/** @return maximum input size for the number of FFT points.
	 */
	virtual u32 maximalInputSize() const = 0;
	/** Sets the number of FFT points necesseary for transforming
	 *  an input of length @param inputSize.
	 */
	virtual void setInputSize(u32 inputSize) = 0;
	virtual u32 outputSize() const = 0;

	/** Sample-rate of input vector. (@see outputSampleRate() too.)
	 */
	void setInputSampleRate(f64 sampleRate) { require(sampleRate > 0); sampleRate_ = sampleRate; }
	/** Sample-rate of output vector.
	 *  I.e.: inverse of distance between two output samples. (length_ / sampleRate_).
	 */
	f64 outputSampleRate() const { return length_ / sampleRate_; }

	/** Returns the description of the last error. */
	const std::string& lastError() const { return lastError_; }
    };


    /** ComplexFastFourierTransform: performs complex fast fourier transform
     * Algorithm taken from "Numerical Recepies in C++"
     * Delivers N complex values real and imaginary part alternating,
     * where N is number of FFT points.
     */
    class ComplexFastFourierTransform : public FastFourierTransform {
	typedef FastFourierTransform Predecessor;
    protected:
	virtual bool applyAlgorithm(std::vector<Data> &data);
    public:
	static std::string name() { return "complex-fast-fourier-transform"; }

	ComplexFastFourierTransform(u32 length = 0, Data sampleRate = 1) :
	    Predecessor(length, sampleRate) {}
	virtual ~ComplexFastFourierTransform() {}

	virtual u32 maximalInputSize() const { return 2 * length(); }
	virtual void setInputSize(u32 inputSize) { setLength(inputSize / 2); }
	virtual u32 outputSize() const { return 2 * length(); }
    };


    /** ComplexInverseFastFourierTransform: performs complex inverse fast fourier transform
     * Algorithm taken from "Numerical Recepies in C++"
     * Delivers N complex values real and imaginary part alternating,
     * where N is number of FFT points.
     */
    class ComplexInverseFastFourierTransform : public ComplexFastFourierTransform {
	typedef ComplexFastFourierTransform Predecessor;
    protected:
	virtual bool applyAlgorithm(std::vector<Data> &data);
    public:
	static std::string name() { return "complex-inverse-fast-fourier-transform"; }

	ComplexInverseFastFourierTransform(u32 length = 0, Data sampleRate = 1) :
	    Predecessor(length, sampleRate) {}
	virtual ~ComplexInverseFastFourierTransform() {}
    };


    /** RealFastFourierTransform: performs fourier transform of real vector
     * Algorithm taken from "Numerical Recepies in C++"
     * Input: N real values;
     * Output N / 2 + 1 complex values real and imaginary part alternating;
     * where N is number of FFT points.
     */
    class RealFastFourierTransform : public FastFourierTransform {
	typedef FastFourierTransform Predecessor;
    protected:
	void unpack(std::vector<f32> &data);
	virtual bool applyAlgorithm(std::vector<Data> &data);
    public:
	static std::string name() { return "real-fast-fourier-transform"; }

	RealFastFourierTransform(u32 length = 0, Data sampleRate = 1):
	    Predecessor(length, sampleRate) {}
	virtual ~RealFastFourierTransform() {}

	virtual u32 maximalInputSize() const { return length(); }
	virtual void setInputSize(u32 inputSize) { setLength(inputSize); }
	virtual u32 outputSize() const { return length() + 2; }
    };


    /** RealInverseFastFourierTransform: performs inverse fourier transform of real vector
     * Algorithm taken from "Numerical Recepies in C++"
     * Input: N / 2 + 1 complex values real and imaginary part alternating;
     * Output N real values;
     * where N is number of FFT points.
     */
    class RealInverseFastFourierTransform : public FastFourierTransform {
	typedef FastFourierTransform Predecessor;
    protected:
	bool pack(std::vector<f32> &data);
	virtual bool applyAlgorithm(std::vector<Data> &data);
	virtual bool estimateContinuous(std::vector<Data> &data);
    public:
	static std::string name() { return "real-inverse-fast-fourier-transform"; }

	RealInverseFastFourierTransform(u32 length = 0, Data sampleRate = 1) :
	    Predecessor(length, sampleRate) {}

	virtual u32 maximalInputSize() const { return length() + 2; }
	virtual void setInputSize(u32 inputSize) { setLength(inputSize - 2); }
	virtual u32 outputSize() const { return length(); }
    };

    extern const Core::ParameterInt paramFftLength;
    extern const Core::ParameterFloat paramFftMaximumInputSize;

    /** FastFourierTransformNode
     */
    template<class Algorithm>
    class FastFourierTransformNode : public SleeveNode {
    private:
	Algorithm algorithm_;

	u32 length_;
	f64 maximumInputSize_;
	u32 length(f64 sampleRate) const;
    public:
	static std::string filterName() { return "signal-" + Algorithm::name(); }

	FastFourierTransformNode(const Core::Configuration &c);
	virtual ~FastFourierTransformNode() {}

	virtual bool setParameter(const std::string &name, const std::string &value);
	virtual bool configure();
	virtual bool work(Flow::PortId p);
    };

    template<class Algorithm>
    FastFourierTransformNode<Algorithm>::FastFourierTransformNode(const Core::Configuration &c) :
	Component(c),
	SleeveNode(c),
	length_(paramFftLength(c)),
	maximumInputSize_(paramFftMaximumInputSize(c))
    {}

    template<class Algorithm>
    bool FastFourierTransformNode<Algorithm>::setParameter(
	const std::string &name, const std::string &value)
    {
	if (paramFftLength.match(name))
	    length_ = paramFftLength(value);
	else if (paramFftMaximumInputSize.match(name))
	    maximumInputSize_ = paramFftMaximumInputSize(value);
	else
	    return false;
	return true;
    }

    template<class Algorithm>
    bool FastFourierTransformNode<Algorithm>::configure()
    {
	Core::Ref<Flow::Attributes> a(new Flow::Attributes());
	getInputAttributes(0, *a);
	if (!configureDatatype(a, Flow::Vector<f32>::type()))
	    return false;

	f64 sampleRate = atof(a->get("sample-rate").c_str());
	if (sampleRate <= 0)
	    criticalError("Sample rate (%f) is smaller or equal to 0.", sampleRate);

	algorithm_.setInputSampleRate(sampleRate);
	algorithm_.setLength(length(sampleRate));
	a->set("sample-rate", algorithm_.outputSampleRate());

	return putOutputAttributes(0, a);
    }

    template<class Algorithm>
    u32 FastFourierTransformNode<Algorithm>::length(f64 sampleRate) const
    {
	u32 maximumLength = (u32)ceil(maximumInputSize_ * sampleRate);
	if (length_ == 0)
	    return maximumLength;
	else if (maximumLength != 0) {
	    warning("FFT length given by maximum-input-size (%d) will be overwitten by parameter length (%d).",
		    maximumLength, length_);
	}
	return length_;
    }

    template<class Algorithm>
    bool FastFourierTransformNode<Algorithm>::work(Flow::PortId p)
    {
	Flow::DataPtr<Flow::Vector<f32> > in;
	if (!getData(0, in))
	    return putData(0, in.get());

	in.makePrivate();
	if (!algorithm_.transform(*in))
	    criticalError("%s", algorithm_.lastError().c_str());
	return putData(0, in.get());
    }
}

#endif // _SIGNAL_FAST_FOURIER_TRANSFORM_HH
