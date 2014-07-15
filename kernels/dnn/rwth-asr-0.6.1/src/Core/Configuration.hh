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
// $Id: Configuration.hh 9227 2013-11-29 14:47:07Z golik $

#ifndef _CORE_CONFIGURATION_HH
#define _CORE_CONFIGURATION_HH

#include "Assertions.hh"
#include "Types.hh"
#include "XmlStream.hh"
#include <iostream>
#include <set>
#include <string.h>
#include <sys/stat.h>
#include <vector>

namespace Core {

    class AbstractParameter;

    /**
     * @page Configuration
     *
     * All resources from
     *  - environment variables
     *  - configuration files
     *  - the command line
     *
     * are kept within an @c Configuration object. Each configurable
     * module (derived form @c Configurable) may ask this class for
     * resources using a parameter specification string of the form
     *
     * <selector1>.<selector2>. ... .<selectorN>.<name>
     *
     * A resources consists of a name and an associated value.  The name
     * of the resource has the form
     *
     * <selector1>.<selector2>. ... .<selectorN>
     *
     * where any of the selectors may by the wildcard "*".  The values
     * are stored as strings.  Conversion to appropriate type (flag,
     * bool, int, float, choice, ...) is done by parameter declaration
     * classes derive from Parameter.
     *
     * @see Core::Configuration
     * @see Core::Configurable
     * @see Core::Parameter
     *
     * @subsection Resource References
     * The value of a resource may contain a reference of the form
     * $(selector).  When this resource is looked up (i.e. matched
     * against a parameter specification), the reference is textually
     * replaced by its resolved value.  The resolved value is determined
     * by appending the resources selector to the matched parameter
     * specification and searching for a matching resource.  This implies
     * that resources are @em context @em dependent. If this failed the
     * matched parameter specification is iteratively truncated until
     * either a match is found or the resolution fails.  Example:
     * Given the resources
     * *.foo.abc   = cat
     * *.foo.xyz   = dog
     * foo.*.bar = /tmp/$(foo).txt
     * then looking up
     * foo.abc.bar     -> /tmp/cat.txt
     * foo.xyz.bar     -> /tmp/dog.txt
     * @see Core::Configuration::resolve()
     */


    /**
     * Central configuration class.
     * @ref Configuration
     */
    class Configuration {
    public:
	static const char resource_wildcard_char;
	static const char *resource_wildcard_string;
	static const char resource_separation_char;
	static const char *resource_separation_string;

	/**
	 * Test if argument is a well-formed resource name.
	 * A string is a well-formed resource name iff it matches the
	 * regular grammar: S   ->   '*' |  [a-zA-Z0-1]+  |  S '.' S
	 */
	static bool isWellFormedResourceName(const std::string&);

	/**
	 * Test if argument is a well-formed parameter name.
	 * A string is a well-formed resource name iff it matches the
	 * regular grammar: S   ->   [a-zA-Z0-1]+  |  S '.' S
	 */
	static bool isWellFormedParameterName(const std::string&);

	static std::string prepareResourceName(const std::string &selection, const std::string &name);

	class SourceDescriptor;
	class Resource;
	class ResourceDataBase;
    private:
	ResourceDataBase *db_;
	bool isDataBaseOwner_;
	std::string selection_;
	std::string name_;

	void warning(const std::string &filename, int lineNumber,
		     const std::string &description);
	void error(const std::string &filename, int lineNumber,
		   const std::string &description);

	/**
	 *  Parses include.
	 *  Syntax: include filename
	 *  Include can be used within a selection as well. In this case,
	 *  the including selection will be active until the first selection in the included file.
	 */
	bool parseInclude(const std::string &line,
			  std::string &includeFilename,
			  std::string &warningDescription);
	bool parseSelection(const std::string &line,
			    std::string &selection,
			    std::string &warningDescription);
	bool parseComment(const std::string &line,
			  std::string &warningDescription);
	bool parseResource(const std::string &line,
			   std::string &parameter, std::string &value,
			   std::string &warningDescription);

	static bool isWellFormedName(const std::string&, bool allowWildcards);
	bool setFromFile(const std::string &filename,
			 const std::string &currentSelection);
	std::vector<std::string> setFromCommandline(
	    const std::vector<std::string> &arguments,
	    const SourceDescriptor *source);
    public:
	// pseudo copy
	Configuration(const Configuration &c, const std::string &selection);
	Configuration(const Configuration &c);
	Configuration();
	~Configuration();

	Configuration& operator=(const Configuration &c);

	/**
	 * Enable logging facility.
	 **/
	void enableLogging();

	/**
	 * Report where configuration ressources came from.
	 **/
	void writeSources(XmlWriter&) const;

	/**
	 * Report all configuration resolved resources in the database.
	 */
	void writeResolvedResources(XmlWriter&) const;

	/**
	 * Report all configuration resources in the database.
	 */
	void writeResources(XmlWriter&) const;

	/**
	 * Report how configuration ressources where used by which
	 * parameters.
	 **/
	void writeUsage(XmlWriter&) const;

	const std::string &getSelection() const {
	    return selection_;
	};

	const std::string &getName() const {
	    return name_;
	};

	// manipulate configuration context
	void setSelection(const std::string &selection);

	/**
	 * Add a resource.
	 * If a resource with the same name already exists, its value is
	 * replaced.
	 * @param name of the resource to be added
	 * @param value of the resource
	 */
	void set(const std::string &name,
		 const std::string &value = "true",
		 const SourceDescriptor *source = 0);

	/**
	 * Try to add a resource.
	 * This is a safe veriosn of set(). If the resource name is not
	 * well-formed, an error message is printed and no resource is set.
	 * @return true is @c name is well-formed.
	 * @see set() */
	bool tryToSet(const std::string &name,
		      const std::string &value = "true",
		      const SourceDescriptor *source = 0);

	bool setFromEnvironment(const std::string &variable);
	bool setFromFile(const std::string &filename);
	std::vector<std::string> setFromCommandline(
	    const std::vector<std::string> &arguments);

	/**
	 * Query the configuration for a parameter.
	 *
	 * It is strongly discouraged to use get() directly use one of the
	 * Parameter classes instead. @see Parameter
	 *
	 * @param parameter is the parameter identification string
	 * @param value The value of the queried parameter is stored in @c value.
	 * Remains unchanged if the parameter is not specified in the
	 * configuration.
	 * @return true if the parameter was configured
	 **/
	bool get(const std::string &parameter, std::string &value) const;

	/**
	 * Substitutes all parameter references and arithmetic expressions
	 * in a string.
	 *
	 * @see Configuration::resolveReferences
	 * @see Configuration::resolveArithmeticExpressions
	 */
	std::string resolve(const std::string &value) const;

    private:
	/**
	 * Find the resource for a given parameter.
	 * @param parameter the parameter specification string
	 * @return the most specific resource matching @c parameter
	 */
	const Resource* find(const std::string &parameter) const;
	/**
	 * Substitute arithmetic expressions in a string
	 *
	 * All occurances of $[expression] are substituted.
	 * expression can be any valid arithmetic expression, including
	 * standard math funcions.
	 * To cast the result of the expression to a specific type, use
	 * $[expression,format]
	 * Format may be int or float, default is float.
	 * Parameter references are not resolved! So use resolveReferences
	 * before.
	 *
	 * @param value is a string which may contain arithmetic expressions
	 * @return @vc value with all arith. expressions subsituted
	 */
	std::string resolveArithmeticExpressions(const std::string &value) const;

	/**
	 * Substitute parameter references in a string.
	 *
	 * All occurances of $(config) are substituted based on the
	 * configuration.  resolve() works recursively if necessary.
	 * resolve() will first try to find a resource for the reference
	 * expanded by the current selection (local lookup).  If that
	 * fails the selection if succesively shortened until the
	 * unexpanded string is tried (global lookup).  (So we try
	 * A.B.C.REF, A.B.REF, A.REF and REF in this order until one of
	 * them is found.)  @warning Note that the result of local lookup
	 * depends on the Configuration which initiates the resolution!
	 *
	 * resolve() is intended for transformation of strings given by the
	 * user.  Do NOT use resolve() like this: resolve("$(fudge)"); use
	 * one of the Parameter classes instead. @see Parameter
	 *
	 * @param value is a string which may contain configuration references.
	 * @return @c value with all references substituted.
	 **/
	std::string resolveReferences(const std::string &value) const;

	std::string getResolvedValue(const Resource *resource) const;
    };

} // namespace Core

#endif // _CORE_CONFIGURATION_HH
