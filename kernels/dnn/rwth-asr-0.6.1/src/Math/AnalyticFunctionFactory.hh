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
#ifndef _MATH_ANALYTIC_FUNCTION_FACTORY_HH
#define _MATH_ANALYTIC_FUNCTION_FACTORY_HH

#include "SimpleAnalyticFunctions.hh"
#include "AcousticalAnalyticFunctions.hh"
#include <Core/Component.hh>

namespace Math {

    /** Analytic function factory */
    class AnalyticFunctionFactory : public Core::Component {
	typedef AnalyticFunctionFactory Self;
    private:
	typedef Core::Ref<const AnalyticFunction> AnalyticFunctionRef;

	/** Prototype of a function declaration parser.
	 *  @return is invalid if declaration at position @param position does not match the syntax.
	 *  @param position is set to the furthermost unparsed character (independent of success).
	 */
	typedef AnalyticFunctionRef (Self::*ParserFunction)(
	    const std::string &, std::string::size_type &) const;
    public:
	enum DomainType {
	    /** unit of argument is an unknown continuous unit */
	    continuousDomain,
	    /** argument is discrete index corresponding to sampleRate_ */
	    discreteDomain,
	    /** omega = 2 * pi * n / N, where n is discrete frequency index and N is number of FFT points */
	    normalizedOmegaDomain
	};
    private:
	UnaryAnalyticFunction::Argument sampleRate_;
	UnaryAnalyticFunction::Argument maximalArgument_;
	DomainType domainType_;
	std::vector<ParserFunction> parserFunctions_;
    private:
	UnaryAnalyticFunctionRef castToUnaryFunction(AnalyticFunctionRef) const;
	BinaryAnalyticFunctionRef castToBinaryFunction(AnalyticFunctionRef) const;
    private:
	/** Matches @param s in @param declaration starting at position @param position.
	 *  @return if true if match was successfull.
	 *  If successfully matched @param position will point at the first unparsed position.
	 *  Whitespaces are ignored.
	 */
	bool accept(const std::string &s, const std::string &declaration,
		    std::string::size_type &position) const;
	/** Matches a number starting at position @param position.
	 *  @return if true if match was successfull.
	 *  If successfully matched @param position will point at the first unparsed position.
	 *  Whitespaces are ignored.
	 */
	bool accept(UnaryAnalyticFunction::Argument &a,
		    const std::string &declaration, std::string::size_type &position) const;
	/** Accepts leading whitespaces starting at position @param position.
	 *  @param position is set the position if the first non-whitespace character
	 *  in @param declaration starting from @param position
	 */
	void acceptWhitespaces(const std::string &declaration, std::string::size_type &position) const;
    private:
	void registerFunctionParser(ParserFunction function) { parserFunctions_.push_back(function); }

	/** Creates a function by parsing @param declaration.
	 *  @return is invalid if parsing failed.
	 */
	AnalyticFunctionRef create(const std::string &declaration) const;

	/** Creates a function by parsing @param declaration starting at position @param position.
	 *  @return is invalid if parsing failed.
	 */
	AnalyticFunctionRef create(const std::string &declaration, std::string::size_type &position) const;

	/** Creates a nested function object f(g(x)).
	 *  Declaration syntax: nest(g, f).
	 *  Remark: f and g are swaped in declaration syntax since g changes maximal argument of f.
	 *          Parsing could be improved...
	 */
	AnalyticFunctionRef parseNesting(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a inverse function object f(x)^(-1).
	 *  Declaration syntax: invert(f).
	 */
	AnalyticFunctionRef parseInvertion(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a maximum normalization function object f(x)^(-1).
	 *  Declaration syntax: norm-max(f).
	 */
	AnalyticFunctionRef parseMaximumNormalization(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates an identity function object f(x) = x.
	 *  Declaration syntax: identity.
	 */
	AnalyticFunctionRef parseIdentity(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a constant function object f(x) = c.
	 *  Declaration syntax: const(c).
	 */
	AnalyticFunctionRef parseConstant(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates an offset function object f(x) = x + b.
	 *  Declaration syntax: offset(b).
	 */
	AnalyticFunctionRef parseOffset(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates an scaling function object f(x) = a * x.
	 *  Declaration syntax: scaling(a).
	 */
	AnalyticFunctionRef parseScaling(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates a addition function object f(x, y) = x + y.
	 *  Declaration syntax: plus.
	 */
	AnalyticFunctionRef parseAddition(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a addition function object f(x, y) = x - y.
	 *  Declaration syntax: minus.
	 */
	AnalyticFunctionRef parseSubstraction(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a addition function object f(x, y) = x * y.
	 *  Declaration syntax: multiplies.
	 */
	AnalyticFunctionRef parseMultiplication(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a addition function object f(x, y) = x / y.
	 *  Declaration syntax: divides.
	 */
	AnalyticFunctionRef parseDivision(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates an Sinh function object f(x) = sinh(x).
	 *  Declaration syntax: sinh.
	 */
	AnalyticFunctionRef parseSinh(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates an ArcSinh function object f(x) = asinh(x).
	 *  Declaration syntax: asinh.
	 */
	AnalyticFunctionRef parseArcSinh(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates an Sinh function object f(x) = cosh(x).
	 *  Declaration syntax: cosh.
	 */
	AnalyticFunctionRef parseCosh(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates an ArcSinh function object f(x) = acosh(x).
	 *  Declaration syntax: acosh.
	 */
	AnalyticFunctionRef parseArcCosh(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates an scaling function object f(x) = x / sampleRate_.
	 *  Declaration syntax: disc-to-cont.
	 */
	AnalyticFunctionRef parseDiscreteToContinuous(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates an scaling function object f(x) = x * sampleRate_.
	 *  Declaration syntax: cont-to-disc.
	 */
	AnalyticFunctionRef parseContinuousToDiscrete(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates a mel warping function object (@see createMelWarpingFunction).
	 *  Declaration syntax: mel or mel(discretize-input).
	 */
	AnalyticFunctionRef parseMelWarpingFunction(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a bark warping function object (@see createBarkWarpingFunction).
	 *  Declaration syntax: bark.
	 */
	AnalyticFunctionRef parseBarkWarpingFunction(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a two-piece linear function function object (@see createTwoPieceLinearFunction).
	 *  Declaration syntax: linear-2(a, limit).
	 */
	AnalyticFunctionRef parseTwoPieceLinearFunction(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a bilinear warping function object (@see createBilinearWarpingFunction).
	 *  Declaration syntax: bilinear(a).
	 */
	AnalyticFunctionRef parseBilinearWarpingFunction(
	    const std::string &declaration, std::string::size_type &position) const;
	/** Creates a three-piece affine function function object (@see createThreePieceAffineFunction).
	 *  Declaration syntax: affine-3(a, a-shift, lower-limit, upper-limit).
	 */
	AnalyticFunctionRef parseThreePieceAffineFunction(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates a equal loudness preemphasis object
	 *  Declaration syntax: mel or equal-loudness-preemphasis.
	 *  For details @see createEqualLoudnessPreemphasis.
	 */
	AnalyticFunctionRef parseEqualLoudnessPreemphasis(
	    const std::string &declaration, std::string::size_type &position) const;

	/** Creates a equal loudness preemphasis object for 40dB
	 *  For details @see createEqualLoudnessPreemphasis40dB.
	 */
	AnalyticFunctionRef parseEqualLoudnessPreemphasis40dB(
	    const std::string &declaration, std::string::size_type &position) const;
  public:
	AnalyticFunctionFactory(const Core::Configuration &);

	/** Creates a unary function by parsing @param declaration.
	 *  @return is invalid if parsing failed.
	 */
	UnaryAnalyticFunctionRef createUnaryFunction(const std::string &declaration) const {
	    return castToUnaryFunction(create(declaration));
	}
	/** Creates a binary function by parsing @param declaration.
	 *  @return is invalid if parsing failed.
	 */
	BinaryAnalyticFunctionRef createBinaryFunction(const std::string &declaration) const {
	    return castToBinaryFunction(create(declaration));
	}

	void setSampleRate(UnaryAnalyticFunction::Argument sampleRate) { sampleRate_ = sampleRate; }
	void setMaximalArgument(UnaryAnalyticFunction::Argument maximum) { maximalArgument_ = maximum; }
	void setDomainType(DomainType type) { domainType_ = type; }
    public:
	/** Creates a ConstantFunction object. */
	static UnaryAnalyticFunctionRef createConstant(UnaryAnalyticFunction::Argument c) {
	    return Math::createConstant(c);
	}
	/** Creates a IdentityFunction object. */
	static UnaryAnalyticFunctionRef createIdentity() {
	    return UnaryAnalyticFunctionRef(new IdentityFunction);
	}
	/** Creates a ScalingFunction object. */
	static UnaryAnalyticFunctionRef createScaling(UnaryAnalyticFunction::Argument a) {
	    return UnaryAnalyticFunctionRef(new ScalingFunction(a));
	}
	/** Creates a OffsetFunction object. */
	static UnaryAnalyticFunctionRef createOffset(UnaryAnalyticFunction::Argument b) {
	    return UnaryAnalyticFunctionRef(new OffsetFunction(b));
	}
	/** Creates an object for f(x) = a * x + b */
	static UnaryAnalyticFunctionRef createLinear(
	    UnaryAnalyticFunction::Argument a, UnaryAnalyticFunction::Argument b) {
	    return nest(createOffset(b), createScaling(a));
	}
	/** Creates a FloorFloatFunction object. */
	static UnaryAnalyticFunctionRef createFloorFloat() {
	    return UnaryAnalyticFunctionRef(new FloorFloatFunction);
	}
	/** Result is f' = constant * f where constant ensures f'(maxInput) = maxInput */
	UnaryAnalyticFunctionRef createMaximumNormalization(UnaryAnalyticFunctionRef f) const {
	    return nest(createScaling(maximalArgument_ / f->value(maximalArgument_)), f);
	}
    public:
	/** Creates a AdditionFunction object. */
	static BinaryAnalyticFunctionRef createAddition() {
	    return BinaryAnalyticFunctionRef(new AdditionFunction);
	}
	/** Creates a createSubstraction object. */
	static BinaryAnalyticFunctionRef createSubstraction() {
	    return BinaryAnalyticFunctionRef(new SubstractionFunction);
	}
	/** Creates a MultiplicationFunction object. */
	static BinaryAnalyticFunctionRef createMultiplication() {
	    return BinaryAnalyticFunctionRef(new MultiplicationFunction);
	}
	/** Creates a DivisionFunction object. */
	static BinaryAnalyticFunctionRef createDivision() {
	    return BinaryAnalyticFunctionRef(new DivisionFunction);
	}
    public:
	/** Creates an object of Sinh */
	static UnaryAnalyticFunctionRef createSinh() { return UnaryAnalyticFunctionRef(new Sinh); }
	/** Creates an object of Cosh */
	static UnaryAnalyticFunctionRef createCosh() { return UnaryAnalyticFunctionRef(new Cosh); }
    public:
	/** Creates an object of MelWarpingCore */
	static UnaryAnalyticFunctionRef createMelWarpingCore() {
	    return UnaryAnalyticFunctionRef(new MelWarpingCore);
	}
	/** Creates an object implementing the Mel warping function
	 *  f_{Mel} = 2595 * log10 (1 + f / 700)
	 *
	 *  Discrete input:
	 *    i_{Mel} = 2595 * log10 (1 + i / frequencySampleRate / 700) * frequencySampleRate
	 *
	 *    if @param discretizeArgument is true then floorf is applied to the frequency:
	 *      -'i / frequencySampleRate' replaced by 'floorf(i / frequencySampleRate)'
	 *      -floorf gives less skipped warped indices
	 *      -Acctually applied for combatibility with the old standard system.
	 *
	 *    Note: sampleRate has to be initialized by sampling rate on frequency axis:
	 *          i.e.: #FFT-points / time-domain-sampling-rate.
	 *
	 *  Normalized omega input:
	 *    omega_{Mel} = 2595 * log10 (1 + omega * sampleRate / (2 * pi) / 700) * (2 * pi) / sampleRate
	 *    Note: sampleRate is the one in time domain.
	 */
	UnaryAnalyticFunctionRef createMelWarpingFunction(bool discretizeArgument = false) const;
	/** Creates an object implementing the Bark warping function
	 *  Continuous input:
	 *    f_{Bark} = 6 * ln(f / 600 + sqrt((f / 600)^2 + 1)) =  6 * asinh(f / 600);
	 *    Remark: @see ArchSinh
	 *  Discrete input:
	 *    i_{Bark} = 6 * asinh(i / frequencySampleRate / 600) * frequencySampleRate
	 *  Normalized omega input:
	 *    omega_{Mel} = 6 * asinh(omega * sampleRate / (2 * pi) / 600) * (2 * pi) / sampleRate
	 *    Note: sampleRate is the one in time domain.
	 */
	UnaryAnalyticFunctionRef createBarkWarpingFunction() const;
	/** Two piece linear warping function
	 *
	 * a <= 1:
	 *   f < limit : f_warped = a * f
	 *   f >= limit : f_warped = (1.0 - a * limit) / (1.0 - limit) * f
	 *
	 * a > 1
	 *   f < limit / a : f_warped = a * f
	 *   f >= limit / a : (a - a * limit) / (a - limit) * f
	 *   Note: this case is equal to the mathematical inverse of the warping function set up
	 *   with (limit, 1/a).
	 */
	UnaryAnalyticFunctionRef createTwoPieceLinearFunction(
	    UnaryAnalyticFunction::Argument warpingFactor, UnaryAnalyticFunction::Argument limit) const;
	/** Bilinear Warping Function.
	 *  x_{B} = x - T / pi * arctan(a * sin(2 * pi / T * x) / (1 + a * cos(2 * pi / T * x))),
	 *  where a = 1 - warping-factor, and T/2 is the point which is mapped on itself.
	 */
	UnaryAnalyticFunctionRef createBilinearWarpingFunction(
	    UnaryAnalyticFunction::Argument warpingFactor) const {
	    return UnaryAnalyticFunctionRef(
		new BilinearTransform(1 - warpingFactor, 2 * maximalArgument_));
	}
	/** Three piece affine warping function
	 *
	 * a <= 1:
	 *   f < lowerLimit : f_warped = ((a * lowerLimit + aShift * (a - 1))/lowerLimit) * f
	 *   lowerLimit <= f < upperLimit : f_warped = a * f + aShift * (a-1)
	 *   f >= upperLimit : f_warped =  m * f + c
	 *   where m = (1 - (a * upperLimit + aShift * (a - 1)))(1 - upperLimit)
	 *
	 * a > 1
	 *   Note: this case is equal to the mathematical inverse of the warping function set up
	 *   with (lowerLimit, upper_limit, 1/a).
	 */
	UnaryAnalyticFunctionRef createThreePieceAffineFunction(
	    UnaryAnalyticFunction::Argument warpingFactor,
	    UnaryAnalyticFunction::Argument aShift,
	    UnaryAnalyticFunction::Argument lowerLimit,
	    UnaryAnalyticFunction::Argument upperLimit) const;

	/** Creates equal loudness preemphasis object
	 *  Note: If maximal-argument <= 4000 (i.e.: 4000 KHz)
	 *    an optimized object (EqualLoudnessPreemphasis4Khz) is created.
	 */
	UnaryAnalyticFunctionRef createEqualLoudnessPreemphasis() const;

	/** Creates equal loudness preemphasis object with 40dB
	 */
	UnaryAnalyticFunctionRef createEqualLoudnessPreemphasis40dB() const;
};

} // namespace Math

#endif //_MATH_ANALYTIC_FUNCTION_FACTORY_HH
