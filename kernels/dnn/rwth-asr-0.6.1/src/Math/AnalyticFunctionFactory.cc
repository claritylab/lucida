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
#include "AnalyticFunctionFactory.hh"
#include "PiecewiseLinearFunction.hh"

using namespace Math;


//=====================================================================================================
AnalyticFunctionFactory::AnalyticFunctionFactory(const Core::Configuration &c) :
    Core::Component(c),
    sampleRate_(1.0),
    maximalArgument_(Core::Type<UnaryAnalyticFunction::Argument>::max),
    domainType_(continuousDomain)
{
    // basic unary functions
    registerFunctionParser(&Self::parseNesting);
    registerFunctionParser(&Self::parseInvertion);
    registerFunctionParser(&Self::parseMaximumNormalization);
    registerFunctionParser(&Self::parseIdentity);
    registerFunctionParser(&Self::parseConstant);
    registerFunctionParser(&Self::parseOffset);
    registerFunctionParser(&Self::parseScaling);

    // basic binary functions
    registerFunctionParser(&Self::parseAddition);
    registerFunctionParser(&Self::parseSubstraction);
    registerFunctionParser(&Self::parseMultiplication);
    registerFunctionParser(&Self::parseDivision);

    // trigonometric functions
    registerFunctionParser(&Self::parseSinh);
    registerFunctionParser(&Self::parseArcSinh);
    registerFunctionParser(&Self::parseCosh);
    registerFunctionParser(&Self::parseArcCosh);

    // acoustical functions
    registerFunctionParser(&Self::parseDiscreteToContinuous);
    registerFunctionParser(&Self::parseContinuousToDiscrete);
    registerFunctionParser(&Self::parseMelWarpingFunction);
    registerFunctionParser(&Self::parseBarkWarpingFunction);
    registerFunctionParser(&Self::parseTwoPieceLinearFunction);
    registerFunctionParser(&Self::parseBilinearWarpingFunction);
    registerFunctionParser(&Self::parseThreePieceAffineFunction);
    registerFunctionParser(&Self::parseEqualLoudnessPreemphasis);

    // advanced acoustical functions
    registerFunctionParser(&Self::parseEqualLoudnessPreemphasis40dB);
}

bool AnalyticFunctionFactory::accept(
    const std::string &s, const std::string &declaration, std::string::size_type &position) const
{
    acceptWhitespaces(declaration, position);
    if (position != std::string::npos) {
	if (declaration.find(s, position) == position) {
	    position += s.size();
	    require(position <= declaration.size());
	    if (position == declaration.size())
		position = std::string::npos;
	    return true;
	}
    }
    return false;
}

bool AnalyticFunctionFactory::accept(
    UnaryAnalyticFunction::Argument &a, const std::string &declaration, std::string::size_type &position) const
{
    acceptWhitespaces(declaration, position);
    if (position != std::string::npos) {
	std::string s = declaration.substr(position);
	char *endptr;
	UnaryAnalyticFunction::Argument newA = strtod(s.c_str(), &endptr);
	if (endptr == 0) {
	    position = std::string::npos;
	    a = newA;
	    return true;
	}
	if (s.c_str() != endptr) {
	    position += (endptr - s.c_str());
	    a = newA;
	    return true;
	}
    }
    return false;
}

void AnalyticFunctionFactory::acceptWhitespaces(
    const std::string &declaration, std::string::size_type &position) const
{
    if (position != std::string::npos)
	position = declaration.find_first_not_of(utf8::whitespace, position);
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::castToUnaryFunction(AnalyticFunctionRef r) const
{
    const UnaryAnalyticFunction *p = dynamic_cast<const UnaryAnalyticFunction*>(r.get());
    if (p == 0) {
	if (r) error("Function is not unary.");
	return UnaryAnalyticFunctionRef();
    }
    return UnaryAnalyticFunctionRef(p);
}

BinaryAnalyticFunctionRef AnalyticFunctionFactory::castToBinaryFunction(AnalyticFunctionRef r) const
{
    const BinaryAnalyticFunction *p = dynamic_cast<const BinaryAnalyticFunction*>(r.get());
    if (p == 0) {
	if (r) error("Function is not binary.");
	return BinaryAnalyticFunctionRef();
    }
    return BinaryAnalyticFunctionRef(p);
}

AnalyticFunctionFactory::AnalyticFunctionRef
AnalyticFunctionFactory::create(const std::string &declaration) const
{
    AnalyticFunctionRef result;
    if (!declaration.empty()) {
	std::string::size_type position = 0;
	if (result = create(declaration, position)) {
	    acceptWhitespaces(declaration, position);
	    if (position == std::string::npos)
		return result;
	}
	if (position != std::string::npos) {
	    error("Failed to parse function declaration from position %zd.\nUnparsed ending '%s'.",
		  position, declaration.substr(position).c_str());
	} else {
	    error("End of string reached during parsing '%s'.", declaration.c_str());
	}
    } else {
	error("Function declaration is empty.");
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::create(
    const std::string &declaration, std::string::size_type &position) const
{
    AnalyticFunctionRef result;
    std::string::size_type maximalPosition = 0;
    for(size_t i = 0; i < parserFunctions_.size(); ++ i) {
	std::string::size_type newPosition = position;
	if (result = (this->*parserFunctions_[i])(declaration, newPosition)) {
	    position = newPosition;
	    return result;
	} else {
	    maximalPosition = std::max(maximalPosition, newPosition);
	}
    }
    position = maximalPosition;
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseNesting(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("nest(", declaration, position)) {
	UnaryAnalyticFunctionRef g = castToUnaryFunction(create(declaration, position));
	if (g) {
	    if (accept(",", declaration, position)) {
		AnalyticFunctionFactory factory(*this);
		factory.setMaximalArgument(g->value(maximalArgument_));

		UnaryAnalyticFunctionRef f = castToUnaryFunction(factory.create(declaration, position));
		if (f) {
		    if (accept(")", declaration, position))
			result = nest(f, g);
		}
	    }
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseInvertion(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("invert(", declaration, position)) {
	UnaryAnalyticFunctionRef f = castToUnaryFunction(create(declaration, position));
	if (f) {
	    if (accept(")", declaration, position)) {
		result = f->invert();
		if (!result)
		    error("Function not invertable.");
	    }
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseMaximumNormalization(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("norm-max(", declaration, position)) {
	UnaryAnalyticFunctionRef f = castToUnaryFunction(create(declaration, position));
	if (f) {
	    if (accept(")", declaration, position))
		result = createMaximumNormalization(f);
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseIdentity(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("identity", declaration, position))
	result = createIdentity();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseConstant(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("const(", declaration, position)) {
	AnalyticFunction::Argument value;
	if (accept(value, declaration, position)) {
	    if (accept(")", declaration, position))
		result = createConstant(value);
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseOffset(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("offset(", declaration, position)) {
	AnalyticFunction::Argument value;
	if (accept(value, declaration, position)) {
	    if (accept(")", declaration, position))
		result = createOffset(value);
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseScaling(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("scaling(", declaration, position)) {
	AnalyticFunction::Argument value;
	if (accept(value, declaration, position)) {
	    if (accept(")", declaration, position))
		result = createScaling(value);
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseAddition(
    const std::string &declaration, std::string::size_type &position) const
{
    BinaryAnalyticFunctionRef result;
    if (accept("plus", declaration, position))
	result = createAddition();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseSubstraction(
    const std::string &declaration, std::string::size_type &position) const
{
    BinaryAnalyticFunctionRef result;
    if (accept("minus", declaration, position))
	result = createSubstraction();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseMultiplication(
    const std::string &declaration, std::string::size_type &position) const
{
    BinaryAnalyticFunctionRef result;
    if (accept("multiplies", declaration, position))
	result = createMultiplication();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseDivision(
    const std::string &declaration, std::string::size_type &position) const
{
    BinaryAnalyticFunctionRef result;
    if (accept("divides", declaration, position))
	result = createDivision();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseSinh(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("sinh", declaration, position))
	result = createSinh();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseArcSinh(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("asinh", declaration, position))
	result = createSinh()->invert();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseCosh(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("cosh", declaration, position))
	result = createCosh();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseArcCosh(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("acosh", declaration, position))
	result = createCosh()->invert();
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseDiscreteToContinuous(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("disc-to-cont", declaration, position))
	result = createScaling(1 / sampleRate_);
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseContinuousToDiscrete(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("cont-to-disc", declaration, position))
	result = createScaling(sampleRate_);
    return result;
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::createMelWarpingFunction(bool discretizeArgument) const
{
    switch (domainType_) {
    case continuousDomain:
	return nest(createScaling(2595.0), createMelWarpingCore());
    case discreteDomain: {
	// sampleRate_: sample rate of the frequency axis
	UnaryAnalyticFunctionRef scaleInput = createScaling(1 / sampleRate_);
	if (discretizeArgument)
	    scaleInput = nest(createFloorFloat(), scaleInput); // creates result uninvertible
	return nest(createScaling(2595.0 * sampleRate_), nest(createMelWarpingCore(), scaleInput));
    }
    case normalizedOmegaDomain:
	// sampleRate_: sample rate in time domain
	return nest(createScaling(2595.0 * 2 * M_PI / sampleRate_),
		    nest(createMelWarpingCore(), createScaling(sampleRate_ / (2 * M_PI))));
    default:
	error("Input type not supported by mel warping.");
	return UnaryAnalyticFunctionRef();
    }
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseMelWarpingFunction(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("mel", declaration, position)) {
	bool discretizeArgument = accept("(discretize-argument)", declaration, position);
	result = createMelWarpingFunction(discretizeArgument);
    }
    return result;
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::createBarkWarpingFunction() const
{
    UnaryAnalyticFunctionRef continuous =
	nest(createScaling(6.0), nest(createSinh()->invert(), createScaling(1.0 / 600.0)));
    switch (domainType_) {
    case continuousDomain:
	return continuous;
    case discreteDomain:
	// sampleRate_: sample rate of the frequency axis
	return nest(createScaling(sampleRate_), nest(continuous, createScaling(1 / sampleRate_)));
    case normalizedOmegaDomain:
	// sampleRate_: sample rate in time domain
	return nest(createScaling((2 * M_PI) / sampleRate_),
		    nest(continuous, createScaling(sampleRate_ / (2 * M_PI))));
    default:
	error("Input type not supported by bark warping.");
	return UnaryAnalyticFunctionRef();
    }
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseBarkWarpingFunction(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("bark", declaration, position))
	result = createBarkWarpingFunction();
    return result;
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::createTwoPieceLinearFunction(
    UnaryAnalyticFunction::Argument warpingFactor, UnaryAnalyticFunction::Argument limit) const
{
    require(0 < limit && limit < 1);
    if (warpingFactor <= 1) {
	PiecewiseLinearFunction *result = new PiecewiseLinearFunction;
	result->add(limit * maximalArgument_, warpingFactor);
	result->normalize(maximalArgument_);
	return UnaryAnalyticFunctionRef(result);
    } else {
	PiecewiseLinearFunction inverse;
	inverse.add(limit * maximalArgument_, 1 / warpingFactor);
	inverse.normalize(maximalArgument_);
	return inverse.invert();
    }
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseTwoPieceLinearFunction(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("linear-2(", declaration, position)) {
	UnaryAnalyticFunction::Argument warpingFactor;
	if (accept(warpingFactor, declaration, position)) {
	    if (warpingFactor > 0) {
		if (accept(",", declaration, position)) {
		    UnaryAnalyticFunction::Argument limit;
		    if (accept(limit, declaration, position)) {
			if (0 < limit && limit < 1) {
			    if (accept(")", declaration, position))
				result = createTwoPieceLinearFunction(warpingFactor, limit);
			} else {
			    error("Limit has to lie in the interval (0, 1).");
			}
		    }
		}
	    } else {
		error("Warping factor is smaller or equal to zero.");
	    }
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseBilinearWarpingFunction(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("bilinear(", declaration, position)) {
	UnaryAnalyticFunction::Argument warpingFactor;
	if (accept(warpingFactor, declaration, position)) {
	    if (warpingFactor > 0 && warpingFactor < 2) {
		if (accept(")", declaration, position))
		    result = createBilinearWarpingFunction(warpingFactor);
	    } else {
		error("Warping factor is not within the interval (0..2).");
	    }
	}
    }
    return result;
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::createThreePieceAffineFunction(
    UnaryAnalyticFunction::Argument warpingFactor,
    UnaryAnalyticFunction::Argument aShift,
    UnaryAnalyticFunction::Argument lowerLimit,
    UnaryAnalyticFunction::Argument upperLimit) const
{
    require(0 < lowerLimit && lowerLimit < upperLimit && upperLimit < 1 && aShift >= 0);
    if (warpingFactor <= 1) {
	PiecewiseLinearFunction *result = new PiecewiseLinearFunction;
	result->add(lowerLimit * maximalArgument_,
		    (warpingFactor * lowerLimit * maximalArgument_ +
		     (aShift * (warpingFactor - 1))) / (lowerLimit * maximalArgument_));
	result->add(upperLimit * maximalArgument_, warpingFactor);
	result->normalize(maximalArgument_);
	return UnaryAnalyticFunctionRef(result);
    } else {
	PiecewiseLinearFunction inverse;
	inverse.add(lowerLimit * maximalArgument_,
		    ((1 / warpingFactor) * lowerLimit * maximalArgument_ + aShift *
		     ((1 / warpingFactor) - 1)) / (lowerLimit * maximalArgument_));
	inverse.add(upperLimit * maximalArgument_, 1 / warpingFactor);
	inverse.normalize(maximalArgument_);
	return inverse.invert();
   }
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseThreePieceAffineFunction(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("affine-3(", declaration, position)) {
	UnaryAnalyticFunction::Argument warpingFactor;
	if (accept(warpingFactor, declaration, position)) {
	    if (0.7 < warpingFactor && warpingFactor < 1.3) {
		if (accept(",", declaration, position)) {
		    UnaryAnalyticFunction::Argument aShift;
		    if (accept(aShift, declaration, position)) {
			if (0 <= aShift && aShift <= 1500) {
			    if (accept(",", declaration, position)) {
				UnaryAnalyticFunction::Argument lowerLimit;
				if (accept(lowerLimit, declaration, position)) {
				    if (0.0350 < lowerLimit && lowerLimit < 1) {
					if (accept(",", declaration, position)) {
					    UnaryAnalyticFunction::Argument upperLimit;
					    if (accept(upperLimit, declaration, position)) {
						if (lowerLimit < upperLimit && upperLimit < 1) {
						    if (accept(")", declaration, position)) {
							result = createThreePieceAffineFunction(
							    warpingFactor, aShift, lowerLimit, upperLimit);
						    }
						} else {
						    error("Upper-Limit has to be greater than 'lower-limit' " \
							  "and less than 1.");
						}
					    }
					}
				    } else {
					error("Lower-Limit has to lie in the interval (0.0350, 1).");
				    }
				}
			    }
			} else {
			    error("Reasonable a-shift are usually expected in the interval [0, 1500].");
			}
		    }
		}
	    } else {
		error("Warping factor has to lie in the interval (0.7, 1.3).");
	    }
	}
    }
    return result;
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseEqualLoudnessPreemphasis(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("equal-loudness-preemphasis", declaration, position))
	result = createEqualLoudnessPreemphasis();
    return result;
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::createEqualLoudnessPreemphasis() const
{
    UnaryAnalyticFunctionRef result;
    switch (domainType_) {
    case continuousDomain: {
	if (Core::isSignificantlyGreater(maximalArgument_, (UnaryAnalyticFunction::Argument)4000, 1e12))
	    return UnaryAnalyticFunctionRef(new EqualLoudnessPreemphasis);
	else
	    return UnaryAnalyticFunctionRef(new EqualLoudnessPreemphasis4Khz);
    } break;
    default:
	error("Input type not supported by equal loudness preemphasis.");
	return UnaryAnalyticFunctionRef();
    }
}

AnalyticFunctionFactory::AnalyticFunctionRef AnalyticFunctionFactory::parseEqualLoudnessPreemphasis40dB(
    const std::string &declaration, std::string::size_type &position) const
{
    UnaryAnalyticFunctionRef result;
    if (accept("equal-loudness-40dB", declaration, position))
	result = createEqualLoudnessPreemphasis40dB();
    return result;
}

UnaryAnalyticFunctionRef AnalyticFunctionFactory::createEqualLoudnessPreemphasis40dB() const
{
    UnaryAnalyticFunctionRef result;
    switch (domainType_) {
    case continuousDomain: {
	return UnaryAnalyticFunctionRef(new EqualLoudnessPreemphasis40dB);
    } break;
    default:
	error("Input type not supported by equal loudness preemphasis 40dB.");
	return UnaryAnalyticFunctionRef();
    }
}
