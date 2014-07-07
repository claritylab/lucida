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
// $Id: Parameter.cc 9621 2014-05-13 17:35:55Z golik $

#include <cstdlib>
#include <iomanip>
#include <ostream>

#include "Parameter.hh"
#include "Assertions.hh"
#include "Configuration.hh"
#include "Utility.hh"
#include "Application.hh"



using namespace Core;

// ===========================================================================
// AbstractParameter

AbstractParameter::AbstractParameter(const AbstractParameter &p) {
    ident      = p.ident;
    abbrev     = p.abbrev;
    short_desc = p.short_desc;
    long_desc  = p.long_desc;
    needs_arg_ = p.needs_arg_;
}

AbstractParameter::AbstractParameter(
    const char *_ident,
    char _abbrev,
    const char *_short_desc,
    const char *_long_desc,
    bool needs_arg) :
    ident(_ident), g_param_max_ident(0)
{
    require(!ident.empty());
    require(ident.find(Configuration::resource_wildcard_char)==std::string::npos);
    require(ident.find(Configuration::resource_separation_char)==std::string::npos);

    abbrev     = _abbrev;
    short_desc = _short_desc;
    long_desc  = _long_desc;
    needs_arg_ = needs_arg;

    if (strlen(_ident) > g_param_max_ident) g_param_max_ident = strlen(_ident);
}

void AbstractParameter::printShortHelp(std::ostream &ostr /*= std::cerr*/) const {
    ostr << "  ";
    if (hasAbbreviation()) ostr << "-" << abbrev << ", ";
    else ostr << "    ";
    ostr << "--"  << std::setiosflags(std::ios::left) << std::setw(g_param_max_ident + 3)
	 << ident.c_str();
    ostr << short_desc;
    ostr << std::endl;
}

// ===========================================================================
// Parameter

template <class T>
Parameter<T>::Parameter(
    const char *ident,
    const char *s_desc,
    const Value &defaultValue,
    const char *l_desc) :
    AbstractParameter(ident, '\0', s_desc, l_desc, false),
    defaultValue_(defaultValue)
{}

template <class T>
typename Parameter<T>::Value Parameter<T>::getValue(const std::string& str) const {

    Value value;
    if (!parseString(str, value)) {
	std::string msg = Core::form("configuration error: unparsable value %s for parameter %s, substituting default.",
				     str.c_str(), name().c_str());
	Application::us()->reportLowLevelError(msg);
	std::cerr << msg << std::endl;
	printParseHelp(std::cerr);
	return defaultValue_;
    }
    if (!isValid(value)) {
	std::string msg = Core::form("configuration error: invalid value \"%s\" for parameter \"%s\", substituting default.",
				     str.c_str(), name().c_str());
	Application::us()->reportLowLevelError(msg);
	std::cerr << msg << std::endl;
	printValidityHelp(std::cerr);
	return defaultValue_;
    }
    return value;
}

template <class T>
typename Parameter<T>::Value Parameter<T>::getValue(
    const class Configuration &c,
    const Value &_defaultValue) const
{
    std::string s;
    if (c.get(name(), s)) return getValue(s);
    else return _defaultValue;
}

namespace Core {
// explicit template instances
template class Parameter<bool>;
template class Parameter<s32>;
template class Parameter<f64>;
template class Parameter< std::vector<bool> >;
template class Parameter<std::string>;
}

// ===========================================================================
// AbstractChoiceParameter

template <class T>
void AbstractChoiceParameter<T>::printParseHelp(std::ostream &os /*= std::cerr*/) const {
    os << "\"" << this->name() << "\" must be one of: ";
    choice().printIdentifiers(os);
    os << std::endl;
}

template <class T>
bool AbstractChoiceParameter<T>::parseString(const std::string &in, Value &out) const {
    return strconv(in, out, choice());
}

// ===========================================================================
// RangedParameter

template <class T>
RangedParameter<T>::RangedParameter(
    const char *ident,
    const char *s_desc,
    const Value &defaultValue,
    const Value &minValue,
    const Value &maxValue,
    const char *l_desc) :
    Parameter<T>(ident, s_desc, defaultValue, l_desc),
    minValue_(minValue),
    maxValue_(maxValue)
{
    require(isValid(defaultValue));
}

template <class T>
void RangedParameter<T>::printValidityHelp(std::ostream &os) const {
    os << "\"" << this->name() << "\" must be in the range of "
       << minValue_ << " - " << maxValue_ << std::endl;
};

namespace Core {
// explicit template instances
template class RangedParameter<s32>;
template class RangedParameter<f64>;
}


// ===========================================================================
// ParameterBool

ParameterBool::ParameterBool(
    const char *ident,
    const char *s_desc,
    bool defaultValue,
    const char *l_desc) :
    AbstractChoiceParameter<bool>(ident, s_desc, defaultValue, l_desc)
{}

// ===========================================================================
// ParameterInt

ParameterInt::ParameterInt(
    const char *ident,
    const char *s_desc,
    s32 defaultValue,
    s32 minValue,
    s32 maxValue,
    const char *l_desc) :
    RangedParameter<s32>(ident, s_desc, defaultValue, minValue, maxValue, l_desc)
{}

void ParameterInt::printParseHelp(std::ostream &os /*= std::cerr*/) const {
    os << "\"" << name() << "\" must be an integer value (cf. strtol(3))." << std::endl;
}

bool ParameterInt::parseString(const std::string &in, Value &out) const {
    return strconv(in, out);
}

// ===========================================================================
// ParameterFloat

ParameterFloat::ParameterFloat(
    const char *ident,
    const char *s_desc,
    f64 defaultValue,
    f64 minValue,
    f64 maxValue,
    const char *l_desc) :
    RangedParameter<f64>(ident, s_desc, defaultValue, minValue, maxValue, l_desc)
{}

void ParameterFloat::printParseHelp(std::ostream &os /*= std::cerr*/) const {
    os << "\"" << name() << "\" must be an floating-point value (cf. strtod(3))." << std::endl;
}

bool ParameterFloat::parseString(const std::string &in, Value &out) const {
    return strconv(in, out);
}

// ===========================================================================
// ParameterChoice

ParameterChoice::ParameterChoice(
    const char *ident,
    const Choice *c,
    const char *s_desc,
    s32 defaultValue,
    const char *l_desc) :
    AbstractChoiceParameter<s32>(ident, s_desc, defaultValue, l_desc),
    choice_(c)
{}

// ===========================================================================
// ParameterString

ParameterString::ParameterString(
    const char *ident,
    const char *s_desc,
    const char *defaultValue,
    const char *l_desc) :
    Parameter<std::string>(ident, s_desc, defaultValue, l_desc)
{}

bool ParameterString::parseString(const std::string &in, Value &out) const {
    out = in;
    return true;
}

// ===========================================================================
// ParameterBitVector

ParameterBitVector::ParameterBitVector(
    const char *ident,
    const char *s_desc,
    const std::vector<bool> &defaultValue,
    const char *l_desc) :
    Parameter<Value>(ident, s_desc, defaultValue, l_desc)
{}


bool ParameterBitVector::parseRangeList(const std::string &in, Value &out) {
    std::string tmp = in;

    out.clear();
    do {
	std::string range;
	std::string::size_type pos = tmp.find(',');

	if (pos == std::string::npos) {
	    range = tmp.substr(0, tmp.length());
	    tmp.erase(0, tmp.length());
	} else {
	    range = tmp.substr(0, pos);
	    tmp.erase(0, pos + 1);
	}

	u32 from, to;
	char* error;

	pos = range.find('-');
	if (pos == std::string::npos) {
	    from = to = strtol(range.c_str(), &error, 10);
	    if (*error != '\0')
		return false;
	} else {
	    std::string tmp2 = range.substr(0, pos);
	    from = strtol(tmp2.c_str(), &error, 10);
	    if (*error != '\0')
		return false;
	    tmp2 = range.substr(pos + 1, range.length() - pos - 1);
	    to = strtol(tmp2.c_str(), &error, 10);
	    if (*error != '\0')
		return false;
	}

	for(u32 i = from; i <= to; i++) {
	    if (i >= out.size())
		out.insert(out.end(), i - out.size() + 1, false);

	    out[i] = true;
	}
    } while (tmp.length() > 0);

    return true;
}

bool ParameterBitVector::parseBinary(const std::string &in, Value &out) {
    out.clear();
    for(std::string::const_iterator c = in.begin(); c != in.end(); c++) {
	switch (*c) {
	case '0':
	    out.push_back(false);
	    break;
	case '1':
	    out.push_back(true);
	    break;
	default:
	    return false;
	}
    }
    return true;
}

bool ParameterBitVector::parseString(const std::string &in, Value &out) const {
    return parseBinary(in, out) || parseRangeList(in, out);
}

// ===========================================================================
// VectorParameter
template <class T>
VectorParameter<T>::VectorParameter(
    const char *ident,
    const char *s_desc,
    const std::string delimiter,
    const Value minValue,
    const Value maxValue,
    const Size minSize,
    const Size maxSize,
    const char *l_desc) :
    AbstractParameter(ident, '\0', s_desc, l_desc, false),
    delimiter_(delimiter),
    minValue_(minValue),
    maxValue_(maxValue),
    minSize_(minSize),
    maxSize_(maxSize)
{
    //    require(delimiter.find_first_of(utf8::whitespace) == std::string::npos);
    if (delimiter_ == "") delimiter_ = " ";
    require(minValue_ <= maxValue_);
    require(minSize_ <= maxSize_);
}

template <class T>
typename VectorParameter<T>::Vector VectorParameter<T>::getVector(const std::string& str) const
{
    Vector vector;
    if (!parseString(str, vector)) {
	std::string msg = Core::form("configuration error: unparsable vector \"%s\" for parameter \"%s\", substituting empty vector.",
				     str.c_str(), name().c_str());
	Application::us()->reportLowLevelError(msg);
	std::cerr << msg << std::endl;
	printParseHelp(std::cerr);
	return Vector();
    }
    if (!isValid(vector)) {
	std::string msg = Core::form("configuration error: invalid vector \"%s\" for parameter \"%s\", substituting empty vector.",
				     str.c_str(), name().c_str());
	Application::us()->reportLowLevelError(msg);
	std::cerr << msg << std::endl;
	printValidityHelp(std::cerr);
	return Vector();
    }
    return vector;
}

template <class T>
typename VectorParameter<T>::Vector VectorParameter<T>::getVector(const Configuration &c) const
{
    std::string s;
    if (c.get(name(), s)) return getVector(s);
    else return Vector();
}

/*
template <class T>
bool VectorParameter<T>::split(const std::string &in, std::vector<std::string> &out) const
{
    out.clear();
    if (in.empty()) {
	std::cout << "configuration warning: ";
	std::cout << "the given string is empty." << std::endl;
	return true;
    }

    std::string tmp = in;
    if (delimiter_ == " ") {
	normalizeWhitespace(tmp);
	stripWhitespace(tmp);
    }

    std::string::size_type b = 0;
    std::string::size_type c=tmp.find(delimiter_, b);
    while (1) {
	out.push_back(tmp.substr(b, c-b));
	stripWhitespace(out.back());
	b = std::min(c, tmp.size()) + delimiter_.size();
	c = tmp.find(delimiter_, b);
	if (b > tmp.size()) break;
    }

    return true;
}
*/

namespace Core {
// explicit template instances
template class VectorParameter<std::string>;
template class VectorParameter<s32>;
template class VectorParameter<f64>;
}

// ===========================================================================
// ParameterStringVector

ParameterStringVector::ParameterStringVector(
    const char *ident,
    const char *s_desc,
    const std::string delimiter,
    const Size minSize,
    const Size maxSize,
    const char *l_desc) :
    VectorParameter<std::string>(ident, s_desc, delimiter, "", "", minSize, maxSize, l_desc)
{}

bool ParameterStringVector::parseString(const std::string &in, Vector &out) const {
    if (in.empty()) {
	std::cout << "configuration warning: ";
	std::cout << "the given string is empty." << std::endl;
	return true;
    }
    return str2vector(in, out, delimiter_);
    /*
    return split(in, out);
    */
}

// ===========================================================================
// ParameterIntVector

ParameterIntVector::ParameterIntVector(
    const char *ident,
    const char *s_desc,
    const std::string delimiter,
    const s32 minValue,
    const s32 maxValue,
    const Size minSize,
    const Size maxSize,
    const char *l_desc) :
    VectorParameter<s32>(ident, s_desc, delimiter, minValue, maxValue, minSize, maxSize, l_desc)
{}

bool ParameterIntVector::parseString(const std::string &in, Vector &out) const
{
    if (in.empty()) {
	std::cout << "configuration warning: ";
	std::cout << "the given string is empty." << std::endl;
	return true;
    }
    return str2vector(in, out, delimiter_, ":", minValue_, maxValue_, minSize_, maxSize_, 1);

    /*
    std::vector<std::string> tmp;
    if (!split(in, tmp)) return false;
    if (in.empty()) return true;
    for (std::vector<std::string>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
	if (it->empty()) return false;
	char *endptr = 0;
	out.push_back(strtol(it->c_str(), &endptr, 10));
	if (*endptr != '\0') return false;
	if (errno == ERANGE) {
	    std::cout << "configuration warning: ";
	    std::cout << "the given string was out of range; ";
	    std::cout << "the value converted has been clamped." << std::endl;
	}
    }

    return true;
    */
}

// ===========================================================================
// ParameterFloatVector

ParameterFloatVector::ParameterFloatVector(
    const char *ident,
    const char *s_desc,
    const std::string delimiter,
    const f64 minValue,
    const f64 maxValue,
    const Size minSize,
    const Size maxSize,
    const char *l_desc) :
    VectorParameter<f64>(ident, s_desc, delimiter, minValue, maxValue, minSize, maxSize, l_desc)
{}

bool ParameterFloatVector::parseString(const std::string &in, Vector &out) const
{
    if (in.empty()) {
	std::cout << "configuration warning: ";
	std::cout << "the given string is empty." << std::endl;
	return true;
    }
    return str2vector(in, out, delimiter_, ":", minValue_, maxValue_, minSize_, maxSize_, f64(1.0));

    /*
    std::vector<std::string> tmp;
    if (!split(in, tmp)) return false;
    if (in.empty()) return true;
    for (std::vector<std::string>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
	if (it->empty()) return false;
	char *endptr = 0;
	out.push_back(strtod(it->c_str(), &endptr));
	if (*endptr != '\0') return false;
	if (errno == ERANGE) {
	    std::cout << "configuration warning: ";
	    std::cout << "the given string was out of range; ";
	    std::cout << "the value converted has been clamped." << std::endl;
	}
    }

    return true;
    */
}

// ===========================================================================
// ParameterWeightVector

ParameterWeightVector::ParameterWeightVector(
    const char *ident,
    const char *s_desc,
    const std::string delimiter,
    const std::string assignDelimiter,
    const f64 minValue,
    const f64 maxValue,
    const Size minSize,
    const Size maxSize,
    const char *l_desc) :
    VectorParameter<f64>(ident, s_desc, delimiter, minValue, maxValue, minSize, maxSize, l_desc),
    assignDelimiter_(assignDelimiter)
{}

bool ParameterWeightVector::parseString(const std::string &in, Vector &out) const
{
    if (in.empty()) {
	std::cout << "configuration warning: ";
	std::cout << "the given string is empty." << std::endl;
	return true;
    }
    return str2vector(in, out, delimiter_, "-", ":", minValue_, maxValue_, minSize_, maxSize_, f64(1.0));

    /*
    std::vector<std::string> tmp;
    if (!split(in, tmp)) return false;
    if (in.empty()) return true;
    for (std::vector<std::string>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
	if (it->empty()) return false;
	char *endptr = 0;
	out.push_back(strtod(it->c_str(), &endptr));
	if (*endptr != '\0') return false;
	if (errno == ERANGE) {
	    std::cout << "configuration warning: ";
	    std::cout << "the given string was out of range; ";
	    std::cout << "the value converted has been clamped." << std::endl;
	}
    }

    return true;
    */
}
