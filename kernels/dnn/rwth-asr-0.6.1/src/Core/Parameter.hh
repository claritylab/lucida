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
// $Id: Parameter.hh 6911 2008-10-30 14:35:27Z rybach $

#ifndef _CORE_PARAMETER_HH
#define _CORE_PARAMETER_HH


#include <cstdio>
#include <cstring>

#include <vector>
#include <sstream>
#include <algorithm>

#include "Choice.hh"
#include "StringUtilities.hh"
#include "Types.hh"

namespace Core {

    class Configuration;

    /**
     * Abstract base class for parameter declaration.
     *
     * Parameters must be static const members!
     */
    class AbstractParameter {
    protected:
	std::string ident;
	char        abbrev;
	const char* short_desc;
	const char* long_desc;
	bool        needs_arg_;

    public:
	AbstractParameter(const AbstractParameter& p);
	AbstractParameter(
	    const char* ident,
	    const char abbrev,
	    const char* short_desc,
	    const char* long_desc,
	    const bool needs_arg);
	virtual ~AbstractParameter() {}

	// query methods
	std::string name() const { return ident; }
	bool match(const std::string &query) const { return (ident == query); }
	bool hasAbbreviation() const { return (abbrev != '\0'); }
	char getAbbreviation() const { return abbrev; }
	std::string getShortDescription() const { return short_desc; }
	std::string getLongDescription() const { return long_desc; }

	// dump methods
	void printShortHelp(std::ostream& ostr) const;

	/**
	 * Print help on how a wellformed setting looks.
	 * printParseHelp() is called whenever parseString() fails
	 */
	virtual void printParseHelp(std::ostream &os) const {};

	/**
	 * Print help on what values are acceptable.
	 * printParseHelp() is called whenever isValid() fails
	 */
	virtual void printValidityHelp(std::ostream &os) const {};

	// operator
	bool operator == (const AbstractParameter& p) const {
	    return (ident == p.ident);
	}
	bool operator <  (const AbstractParameter& p) const {
	    return (ident < p.ident);
	}

    private:
	u32 g_param_max_ident;
    };

    /**
     * Base class template for parameter declaration
     */
    template <class T>
    class Parameter :
	public AbstractParameter
    {
    public:
	typedef T Value;
    protected:
	Value defaultValue_;
	virtual bool parseString(const std::string &in, Value &out) const = 0;
	Value getValue(const std::string& str) const;
	Value getValue(const class Configuration &c, const Value &_defaultValue) const;
	Value getValue(const class Configuration &c) const {
	    return getValue(c, defaultValue_);
	}
    public:
	virtual bool isValid(const Value &v) const {
	    return true;
	}

	Parameter(
	    const char *ident,
	    const char *s_desc,
	    const Value &defaultValue,
	    const char *l_desc = "");

	virtual ~Parameter() {}

	const Value &defaultValue() const { return defaultValue_; }

	/**
	 * Obtain parameter value from string.
	 * Just string conversion, no Configuration involved.
	 */
	Value operator()(const std::string& str) const { return getValue(str); }

	/**
	 * Retrieve parameter value from configuration.
	 * Use this version only if a static default is not sufficient.
	 * @return value of parameter in the configuration or given default
	 * value @c _defaultValue if not configured.
	 */
	Value operator()(const class Configuration& c, const Value &_defaultValue) const {
	    return getValue(c, _defaultValue);
	}

	/**
	 * Retrieve parameter value from configuration.
	 * This is the recommended way of getting a paramter value.
	 * @return value of parameter in the configuration or the default
	 * value if not configured.
	 */
	Value operator()(const class Configuration& c) const {
	    return getValue(c);
	}
    };

    /**
     * Base class template for multiple-choice parameters.
     */
    template <class T>
    class AbstractChoiceParameter :
	public Parameter<T>
    {
    public:
	typedef T Value;

	virtual const Choice &choice() const = 0;
	virtual void printParseHelp(std::ostream &os) const;
	virtual bool parseString(const std::string &in, Value &out) const;

	AbstractChoiceParameter(
	    const char *ident,
	    const char *s_desc,
	    const Choice::Value &defaultValue,
	    const char *l_desc = "") :
	    Parameter<T>(ident, s_desc, defaultValue, l_desc) {}
    };

    /**
     * Base class template for parameters with lower und upper bounds.
     **/
    template <class T>
    class RangedParameter :
	public Parameter<T>
    {
    public:
      typedef T Value;

    protected:
	Value minValue_;
	Value maxValue_;
    public:
	virtual bool isValid(const Value &v) const {
	    return minValue_ <= v && v <= maxValue_;
	}

	virtual void printValidityHelp(std::ostream &os) const;

	RangedParameter(
	    const char *ident,
	    const char *s_desc,
	    const Value &defaultValue,
	    const Value &minValue,
	    const Value &maxValue,
	    const char *l_desc = "");
    };

    /**
     * Declaration class for boolean valued parameters.
     */
    class ParameterBool :
	public AbstractChoiceParameter<bool>
    {
    public:
	virtual const Choice &choice() const { return boolChoice; };
	ParameterBool(
	    const char* ident,
	    const char* s_desc,
	    const bool defaultValue = false,
	    const char* l_desc = "");
    };

    /**
     * Declaration class for integer valued parameters.
     */
    class ParameterInt :
	public RangedParameter<s32> {
    public:
	virtual void printParseHelp(std::ostream &os) const;
	virtual bool parseString(const std::string &in, Value &out) const;
	ParameterInt(
	    const char* ident,
	    const char* s_desc,
	    s32 defaultValue = 0,
	    s32 minValue = Core::Type<s32>::min,
	    s32 maxValue = Core::Type<s32>::max,
	    const char* l_desc = "");
    };

    /**
     * Declaration class for floating point parameters.
     */
    class ParameterFloat :
	public RangedParameter<f64>
    {
    public:
	virtual void printParseHelp(std::ostream &os) const;
	virtual bool parseString(const std::string &in, Value &out) const;
	ParameterFloat(
	    const char *ident,
	    const char *s_desc,
	    f64 defaultValue = 0.0,
	    f64 minValue = Core::Type<f64>::min,
	    f64 maxValue = Core::Type<f64>::max,
	    const char *l_desc = "");
    };

    /**
     * Declaration class for discrete multiple-choice parameters.
     */
    class ParameterChoice :
	public AbstractChoiceParameter<s32>
    {
    private:
	const Choice *choice_;
    public:
	virtual const Choice &choice() const { return *choice_; };
	ParameterChoice(
	    const char *ident,
	    const Choice *c,
	    const char *s_desc,
	    const s32 defaultValue = 0,
	    const char *l_desc="");
    };

    /**
     * Declaration class for string parameters
     */
    class ParameterString :
	public Parameter<std::string>
    {
    public:
	virtual bool parseString(const std::string &in, Value &out) const;
	ParameterString(
	    const char *ident,
	    const char *s_desc,
	    const char *defaultValue = "",
	    const char *l_desc = "");
    };

    /**
     * Declaration class for bit vector parameters.
     * Values can be specified either as sequence of zeroes and ones, or as
     * as comma-separated list of ranges.
     */
    class ParameterBitVector :
	public Parameter< std::vector<bool> >
    {
    public:
	/** Parse comma-separated list of ranges.
	 * Each comma-seperated item can be either a single number or
	 * range (a-b).  All bits mentioned in the list are true, all
	 * others are false */
	static bool parseRangeList(const std::string &in, Value &out);

	/**
	 * Parse sequence of zeroes and ones.
	 */
	static bool parseBinary(const std::string &in, Value &out);

	/**
	 * Parse bit-vector specification.
	 * Try to interpret as binary representation first, if that
	 * fails parse as range list.
	 */
	virtual bool parseString(const std::string &in, Value &out) const;

	ParameterBitVector(
	    const char *ident,
	    const char *s_desc,
	    const Value &defaultValue = Value(),
	    const char *l_desc = "");
    };


    /**
     * Base class template for vector parameter declaration
     */
    template <class T>
    class VectorParameter :
	public AbstractParameter
    {
    public:
	typedef T Value;
	typedef u32 Size;
	typedef std::vector<T> Vector;
    protected:
	std::string delimiter_;
	Value minValue_;
	Value maxValue_;
	Size minSize_;
	Size maxSize_;
	virtual bool parseString(const std::string &in, Vector &out) const = 0;
	Vector getVector(const std::string &) const;
	Vector getVector(const Configuration &) const;
    protected:
	bool split(const std::string &in, std::vector<std::string> &out) const;
    public:
	virtual bool isValid(const Vector &v) const {
	    return minSize_<=v.size() && v.size()<=maxSize_;
	}

	VectorParameter(
	    const char *ident,
	    const char *s_desc,
	    const std::string delimiter = "",
	    const Value minValue = Core::Type<Value>::min,
	    const Value maxValue = Core::Type<Value>::max,
	    const Size minSize = Core::Type<Size>::min,
	    const Size maxSize = Core::Type<Size>::max,
	    const char *l_desc = "");

	virtual ~VectorParameter() {}

	/**
	 * Obtain vector parameter from string.
	 * Just string conversion, no Configuration involved.
	 */
	Vector operator()(const std::string& str) const { return getVector(str); }

	/**
	 * Retrieve vector parameter from configuration.
	 * This is the recommended way of getting a vector parameter.
	 * @return vector of parameters in the configuration
	 */
	Vector operator()(const class Configuration& c) const {
	    return getVector(c);
	}
    };

    /**
     * Declaration class for string vector parameters.
     */
    class ParameterStringVector :
	public VectorParameter<std::string>
    {
    protected:
	virtual bool parseString(const std::string &in, Vector &out) const;

    public:
	ParameterStringVector(
	    const char *ident,
	    const char *s_desc,
	    const std::string delimiter = "",
	    const Size minSize = Core::Type<Size>::min,
	    const Size maxSize = Core::Type<Size>::max,
	    const char *l_desc = "");
    };

    /**
     * Declaration class for int vector parameters.
     */
    class ParameterIntVector :
	public VectorParameter<s32>
    {
    protected:
	virtual bool parseString(const std::string &in, Vector &out) const;

    public:
	ParameterIntVector(
	    const char *ident,
	    const char *s_desc,
	    const std::string delimiter = "",
	    const s32 minValue = Core::Type<s32>::min,
	    const s32 maxValue = Core::Type<s32>::max,
	    const Size minSize = Core::Type<Size>::min,
	    const Size maxSize = Core::Type<Size>::max,
	    const char *l_desc = "");
    };

    /**
     * Declaration class for float vector parameters.
     */
    class ParameterFloatVector :
	public VectorParameter<f64>
    {
    protected:
	virtual bool parseString(const std::string &in, Vector &out) const;

    public:
	ParameterFloatVector(
	    const char *ident,
	    const char *s_desc,
	    const std::string delimiter = "",
	    const f64 minValue = Core::Type<f64>::min,
	    const f64 maxValue = Core::Type<f64>::max,
	    const Size minSize = Core::Type<Size>::min,
	    const Size maxSize = Core::Type<Size>::max,
	    const char *l_desc = "");
    };

    /**
     * Declaration class for weight vector parameters.
     */
    class ParameterWeightVector :
	public VectorParameter<f64>
    {
    protected:
	virtual bool parseString(const std::string &in, Vector &out) const;

    protected:
	std::string assignDelimiter_;
    public:
	ParameterWeightVector(
	    const char *ident,
	    const char *s_desc,
	    const std::string delimiter = "",
	    const std::string assignDelimiter = "",
	    const f64 minValue = Core::Type<f64>::min,
	    const f64 maxValue = Core::Type<f64>::max,
	    const Size minSize = Core::Type<Size>::min,
	    const Size maxSize = Core::Type<Size>::max,
	    const char *l_desc = "");
    };

} // namespace Core

#endif // _CORE_PARAMETER_HH
