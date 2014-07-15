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
// $Id: Configuration.cc 9621 2014-05-13 17:35:55Z golik $

#include "Application.hh"
#include "Configuration.hh"
#include "Utility.hh"
#include "Directory.hh"
#include "StringUtilities.hh"
#include "Tokenizer.hh"
#include "ArithmeticExpressionParser.hh"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <list>
#include <string>

using namespace Core;


/**
 * Describes where a resource comes from.
 */

class Configuration::SourceDescriptor {
public:
    std::string type, data;
    void write(XmlWriter &os) const {
	os << XmlFull("source", data) + XmlAttribute("type", type);
    }
};


/**
 * Item of configuration.
 *
 * A resource is a piece of configuration specified by the user.
 * It consists of a name and an associated value.  The name may
 * contain wildcards.  The value may contain references
 * (e.g. $(basedir) ), which are subject to substitution.
 */

class Configuration::Resource {
private:
    const SourceDescriptor *source_;
    std::string name_;
    std::string value_;
    mutable bool isBeingResolved_; /**< flag to trap circular reference */

    struct Usage {
	std::string fullParameterName;
	const AbstractParameter *parameter;
	std::string effectiveValue;
    };

    mutable std::vector<Usage> usage;

public:
    Resource(const std::string &_name, const std::string &_value, const SourceDescriptor *_source) :
	source_(_source),
	name_(_name), value_(_value),
	isBeingResolved_(false) {}

    inline const std::string& getName() const { return name_; };
    inline const std::string& getValue() const { return value_; };

    bool isBeingResolved() const { return isBeingResolved_; }
    void beginResolution() const { isBeingResolved_ = true; }
    void endResolution() const { isBeingResolved_ = false; }

    inline bool operator < (const Resource &r) const {
	return name_ < r.name_;
    }
    inline bool operator == (const Resource &r) const {
	return name_ == r.name_;
    }

    void write(std::ostream &os) const {
	os << name_ << " = " << value_;
    }

    /**
     * Determine if the resource matches a configurtion path.
     * @param components the coponents of the configuration path
     * @return the number of path components matched by the resource,
     * or -1 of the resource does not match.
     */
    s32 match(const std::vector<std::string> &components) const;

    void registerUsage(const std::string &n, const AbstractParameter *p, const std::string &v) const {
	Usage u;
	u.fullParameterName = n;
	u.parameter = p;
	u.effectiveValue = v;
	usage.push_back(u);
    }

    void writeUsage(XmlWriter&) const;
};


/**
 * Finds best match using dynamic programming:
 * Runtime is at most |Component| * |Resource|
 **/
s32 Configuration::Resource::match(const std::vector<std::string> &components) const {
    std::vector<s16> nMatches(components.size() + 1, Core::Type<s16>::min);
    nMatches.front() = 0;
    bool hasMatch = true;
    StringTokenizer tokenizer(getName(), resource_separation_string);
    for (StringTokenizer::Iterator itResource = tokenizer.begin(), endResource = tokenizer.end(); hasMatch && (itResource != endResource); ++itResource) {
	hasMatch = false;
	if (*itResource == resource_wildcard_string) {
	    for (std::vector<s16>::iterator itMatches = nMatches.begin() + 1; itMatches != nMatches.end(); ++itMatches)
		if (*(itMatches - 1) > *itMatches)
		    *itMatches = *(itMatches - 1);
	    hasMatch = true;
	} else {
	    std::vector<std::string>::const_reverse_iterator itComponent = components.rbegin();
	    for (std::vector<s16>::reverse_iterator itMatches = nMatches.rbegin(); itMatches != nMatches.rend() - 1; ++itMatches, ++itComponent)
		if ((*(itMatches + 1) >= 0) && (*itComponent == *itResource)) {
		    *itMatches = *(itMatches + 1) + 1;
		    hasMatch = true;
		} else
		    *itMatches = Core::Type<s16>::min;
	    nMatches.front() = Core::Type<s16>::min;
	}
    }
    return (nMatches.back() >= 0) ? nMatches.back() : -1;
}


void Configuration::Resource::writeUsage(XmlWriter &os) const {
    for (std::vector<Usage>::const_iterator u = usage.begin(); u != usage.end() ; ++u) {
	os << "! used as: " << u->fullParameterName
	   << " = " << u->effectiveValue;
	if (u->parameter) {
	    os << "\t" << u->parameter->getShortDescription();
	}
	os << "\n";
    }
}


/**
 * Central storage place for all resources.
 */

class Configuration::ResourceDataBase {
private:
    std::set<Resource> resources;
    Resource noResource_;
    bool isLogging_;

    typedef std::list<SourceDescriptor*> SourceList;
    SourceList sources_;

public:
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
     * Find the resource to be used for a given parameter.
     * @param parameter the parameter specification string
     * @return the most specific resource matching @c parameter
     */
    const Resource* find(const std::string &parameter) const;

	const std::set<Resource>& getResources() const { return resources; }

    const Resource* noResource() const {
	return &noResource_;
    }

    SourceDescriptor *addSource(const std::string &type, const std::string &data) {
	SourceDescriptor *sd = new SourceDescriptor;
	sd->type = type;
	sd->data = data;
	sources_.push_back(sd);
	return sd;
    }


    ResourceDataBase();
    ~ResourceDataBase();

    void enableLogging() { isLogging_ = true; }
    void writeSources(XmlWriter&) const;
    void writeUsage(XmlWriter&) const;
    void write(std::ostream&) const;
};

Configuration::ResourceDataBase::ResourceDataBase() :
    noResource_("DEFAULT", "DEFAULT", 0)
{
    isLogging_ = false;
}

Configuration::ResourceDataBase::~ResourceDataBase() {
    for (SourceList::const_iterator s = sources_.begin(); s != sources_.end(); ++s)
	delete *s;
}

void Configuration::ResourceDataBase::set(
    const std::string &name, const std::string &value, const SourceDescriptor *source)
{
    require(isWellFormedResourceName(name));

    // create new resource
    Resource res(name, value, source);

    // delete existing resources with the same name
    resources.erase(res);

    bool isInserted = resources.insert(res).second;

    ensure(isInserted);
}

const Configuration::Resource* Configuration::ResourceDataBase::find(
    const std::string &parameter) const
{
    require(isWellFormedParameterName(parameter));

    // split parameter string into components
    std::vector<std::string> components;
    StringTokenizer tokenizer(parameter, resource_separation_string);
    for (StringTokenizer::Iterator token = tokenizer.begin(); token != tokenizer.end(); ++token) {
	components.push_back(*token);
    }

    s32 specific = 0;
    u32 ties = 0;
    const Resource *result = 0;

    // try all resources to find best (most specific) match
    std::set<Resource>::const_iterator it;

    // debug
    // std::cerr << "match " << parameter << "(" << resources.size() << "):" << std::endl;

    for (it = resources.begin() ; it != resources.end() ; it++) {
	s32 m = it->match(components);

	// debug
	// if (m > 0) std::cerr << it->getName() << " matches with score " << m << std::endl;

	if (m > specific) {
	    specific = m;
	    ties = 0;
	    result = &(*it);
	} else if (m == specific) {
	    ++ties;
	}
    }

    if (ties > 0) {
	std::cerr << "configuration warning: \""
		  << parameter << "\" is matched by "
	     << ties + 1 << " equally specific resources. using \""
		  << result->getName() << "\"."
		  << std::endl;
    }

    if (isLogging_) {
	std::cerr << "configuration: \"";
	if (result)
	    std::cerr << parameter << "\" is matched by \""
		      << result->getName() << "\""
		      << std::endl;
	else
	    std::cerr << parameter << "\" is not matched."
		 << std::endl;
    }

    return result;
}

void Configuration::ResourceDataBase::write(std::ostream &os) const {
    for (std::set<Resource>::const_iterator ri = resources.begin(); ri != resources.end(); ++ri) {
	ri->write(os); os << "\n";
    }
}

void Configuration::ResourceDataBase::writeSources(XmlWriter &os) const {
    for (SourceList::const_iterator s = sources_.begin(); s != sources_.end(); ++s) {
	(*s)->write(os);
    }
}

void Configuration::ResourceDataBase::writeUsage(XmlWriter &os) const {
    for (std::set<Resource>::const_iterator r = resources.begin(); r != resources.end() ; ++r) {
	os << r->getName() << " = " << r->getValue() << "\n";
	r->writeUsage(os);
    }
    os << "!!! " << noResource_.getName() << "\n";
    noResource_.writeUsage(os);
}

// ===========================================================================
// Configuration static members

const char  Configuration::resource_wildcard_char     = '*';
const char *Configuration::resource_wildcard_string   = "*";
const char  Configuration::resource_separation_char   = '.';
const char *Configuration::resource_separation_string = ".";

bool Configuration::isWellFormedName(const std::string &s, bool allowWildcards) {
    enum {require_component, component, require_separator, error} state = require_component;
    for (std::string::size_type i = 0 ; i < s.size() && state != error ; ++i) {
	switch (state) {
	case require_component: {
	    if (s[i] == resource_separation_char)
		state = error;
	    else if (s[i] == resource_wildcard_char) {
		state = (allowWildcards) ? require_separator : error;
	    } else
		state = component;
	} break;
	case component: {
	    if (s[i] == resource_separation_char)
		state = require_component;
	    else if (s[i] == resource_wildcard_char)
		state = error;
	    else
		state = component;
	} break;
	case require_separator: {
	    if (s[i] == resource_separation_char)
		state = require_component;
	    else
		state = error;
	} break;
	default: break;
	}
    }

    switch (state) {
    case component:
    case require_separator:
	return true;
	break;
    case error:
    case require_component:
    default:
	return false;
	break;
    }
}

bool Configuration::isWellFormedResourceName(const std::string &s) {
    return isWellFormedName(s, true);
}

bool Configuration::isWellFormedParameterName(const std::string &s) {
    return isWellFormedName(s, false);
}


/*****************************************************************************/
Configuration::Configuration() :
/*****************************************************************************/
    db_(0),
    isDataBaseOwner_(true),
    selection_("UNNAMED"),
    name_("UNNAMED")
{
    // define new root configuration
    db_ = new ResourceDataBase;
}

/*****************************************************************************/
Configuration::Configuration(const Configuration &c) :
/*****************************************************************************/
    db_(c.db_),
    isDataBaseOwner_(false),
    selection_(c.selection_),
    name_(c.name_)
{
}

/*****************************************************************************/
Configuration::Configuration
(const Configuration &c, const std::string &add_selection) :
/*****************************************************************************/
    db_(c.db_),
    isDataBaseOwner_(false)
{
    require(add_selection.length());
    require(add_selection.find(resource_separation_char) == std::string::npos);
    require(add_selection.find(resource_wildcard_char)   == std::string::npos);

    selection_ = c.selection_ + resource_separation_string + add_selection;
    name_ = add_selection;
    ensure(isWellFormedParameterName(selection_));
}

/*****************************************************************************/
Configuration::~Configuration()
/*****************************************************************************/
{
  if (isDataBaseOwner_) delete db_;
}

/*****************************************************************************/
Configuration& Configuration::operator=(const Configuration &c)
/*****************************************************************************/
{
    if (isDataBaseOwner_) {
	delete db_;
	isDataBaseOwner_= false;
    }
    db_= c.db_;
    selection_= c.selection_;
    name_= c.name_;
    return *this;
}

/*****************************************************************************/
void Configuration::setSelection(const std::string &selection)
/*****************************************************************************/
{
    require(isWellFormedParameterName(selection));

    selection_ = selection;

    std::string::size_type pos = selection.find_last_of(resource_separation_char);
    if (pos != std::string::npos) {
	name_.assign(selection, pos + 1, selection.length() - pos - 1);
    } else {
	name_ = selection_;
    }
}

/*****************************************************************************/
std::string Configuration::prepareResourceName(
    const std::string &selection,
    const std::string &parameter)
/*****************************************************************************/
{
	if (!(selection.empty() || isWellFormedResourceName(selection))) {
		std::cerr << "selection is not valid: '" <<  selection << std::endl;
	}
    require(selection.empty() || isWellFormedResourceName(selection));
    if (!isWellFormedResourceName(parameter)) {
	std::cerr << "parameter name is not valid: '" << parameter << "'" << std::endl;
    }
    require(isWellFormedResourceName(parameter));

    std::string result;

    if (selection.empty()) {
	result = parameter;
    } else {
	result = selection;
	if ((result[result.length() - 1] == resource_wildcard_char) &&
	    (parameter[0]                == resource_wildcard_char)) {
	    result += std::string(parameter.begin()+1, parameter.end());
	} else {
	    result += resource_separation_string;
	    result += parameter;
	}
    }

    ensure(isWellFormedResourceName(result));
    return result;
}

void Configuration::set(
    const std::string &name, const std::string &value, const SourceDescriptor *source)
{
    require(isWellFormedResourceName(name));

    // value may be surrounded by " or '. remove these
    std::string val;
    std::string::size_type i = value.find_first_of("\"'");
    if (i == std::string::npos) {
	val = value;
    } else {
	std::string::size_type j = value.find_last_of("\"'");
	val = value.substr(i+1, j-i-1);
    }

    db_->set(name, val, source);
}

bool Configuration::tryToSet(
    const std::string &name, const std::string &value, const SourceDescriptor *source)
{
    if (isWellFormedResourceName(name)) {
	set(name, value, source);
	return true;
    } else {
	std::cerr << "configuration error: malformed resource name \""
		  << name << "\"" << std::endl;
	return false;
    }
}

/*****************************************************************************/
bool Configuration::setFromEnvironment(const std::string &variable)
/*****************************************************************************/
{
    // parse user supplied variable
    const char *env = getenv(variable.c_str());
    if (!env) return false;

    std::string token;
    std::istringstream iss(env);
    std::vector<std::string> arguments;

    arguments.push_back("-");
    while (wsgetline(iss, token, " ") != EOF) {
	arguments.push_back(token);
	iss >> std::ws;
	if (iss.get() == '"') {
	    if ((wsgetline(iss, token, "\"")) != EOF) {
		arguments.push_back(token);
	    }
	} else iss.unget();
    }

    // parse options exactly like an additional command line
    setFromCommandline(arguments, db_->addSource("environment", variable));

    return true;
}

/*****************************************************************************/
bool Configuration::setFromFile(const std::string &filename)
/*****************************************************************************/
{
    return setFromFile(filename, resource_wildcard_string);
}

/*****************************************************************************/
bool Configuration::setFromFile(const std::string &filename,
				const std::string &currentSelection)
/*****************************************************************************/
{
    int linenr;
    std::ifstream ifs(filename.c_str());
    std::string line, warningDescription;
    std::string includeFilename;
    std::string selection(currentSelection), parameter, value;
    // check if input stream has been succesfully opened
    if (!ifs) {
	std::cerr << "configuration error: failed to open file \""
		  << filename << "\" for reading.";
	if (errno) {
	    std::cerr << " (" << strerror(errno) << ")";
	    errno = 0;
	}
	std::cerr << std::endl;
	return false;
    }

    SourceDescriptor *sd = db_->addSource("file", filename);

    // read configuration file from input stream
    linenr = 0;
    while (getline(ifs, line) != EOF) {
	++ linenr;
	warningDescription = "";
	if (line.size() == 0) continue;

	if (parseInclude(line, includeFilename, warningDescription)) {
	    if (!warningDescription.empty()) warning(filename, linenr, warningDescription);
	    std::string f(resolve(includeFilename));
	    if (!isRegularFile(f))
		f = joinPaths(directoryName(filename), f);
	    if (isRegularFile(f))
		setFromFile(f, selection);
	    else
		error(filename, linenr, "Include file '" + f + "' does not exist.");
	} else if (parseSelection(line, selection, warningDescription)) {
	    if (!warningDescription.empty()) warning(filename, linenr, warningDescription);
	} else if (parseComment(line, warningDescription)) {
	    if (!warningDescription.empty()) warning(filename, linenr, warningDescription);
	} else if (parseResource(line, parameter, value, warningDescription)) {
	    if (!warningDescription.empty()) warning(filename, linenr, warningDescription);
	    if (!isWellFormedResourceName(parameter))
		warning(filename, linenr, "bad resource name: '" + parameter + "'");
	    else {
		std::string resource = prepareResourceName(selection, parameter);
		set(resource, value, sd);
	    }
	} else {
	    error(filename, linenr, "syntax error");
	}
    }
    return true;
}

/*****************************************************************************/
bool Configuration::parseInclude(const std::string &line,
				 std::string &includeFilename,
				 std::string &warningDescription)
/*****************************************************************************/
{
    std::istringstream iss(line);
    std::string token;
    iss >> token;
    if (token == "include") {
	token.clear(); iss >> token;
	if (!token.empty() && token[0] != '=') {
	    includeFilename = token;
	    token.clear(); iss >> token;
	    if (!token.empty() &&
		Application::us()->getCommentCharacters().find(token[0]) == std::string::npos) {
		warningDescription = "unknown include argument '" + token + "'";
	    }
	    return true;
	}
    }
    return false;
}

/*****************************************************************************/
bool Configuration::parseSelection(const std::string &line,
				   std::string &selection,
				   std::string &warningDescription)
/*****************************************************************************/
{
    std::istringstream iss(line);
    std::string token;
    if (iss.get() != '[')
	return false;

    // new selection
    int status = getline(iss, token, "]");
    if (status > 0) {
	if (token.empty() || isWellFormedResourceName(token)) {
	    selection = resolve(token);
	} else {
	    warningDescription = "malformed selection name";
	}
    } else {
	verify(status == 0);
	return false;
    }
    return true;
}

/*****************************************************************************/
bool Configuration::parseComment(const std::string &line,
				 std::string &warningDescription)
/*****************************************************************************/
{
    std::istringstream iss(line);
    std::string token;
    int status = wsgetline(iss, token, " \t\n" + Application::us()->getCommentCharacters());
    return (status == EOF) || (status != 0 && token.empty());
}

/*****************************************************************************/
bool Configuration::parseResource(const std::string &line,
				  std::string &parameter, std::string &value,
				  std::string &warningDescription)
/*****************************************************************************/
{
    std::istringstream iss(line);
    int status = wsgetline(iss, parameter, "= \t\n" +
			   Application::us()->getCommentCharacters());
    if ((status != EOF) && (!parameter.empty())) {
	parameter = resolve(parameter);
	// check wether we have an argument
	if (status != 0) {
	    // Comment, whitespace or '=' found and parameter is not empty...
	    iss.unget(); // put the last character back in order to parse the remainder correctly.
	    iss >> std::ws; // Seek to a non-whitespace character.
	    int peek = iss.get();
	    if (peek == '=') {
		// ... It was '=', so try to parse the value until a comment or new line.
		if (wsgetline(iss, value, "\n\"" +
			      Application::us()->getCommentCharacters())
		    != EOF) {
		    // Value found, let's strip it.
		    std::string::size_type i = value.find_last_not_of(" \t") ;
		    if (i != std::string::npos) value.erase(i + 1) ;
		} else {
		    // Line was empty after the '='. It is an unfinished line.
		    return false;
		}
	    } else if (Application::us()->getCommentCharacters().find(peek) == std::string::npos &&
		       peek != EOF) {
		// ... It was, whitespace followed by something but not a comment.
		return false;
	    } else {
		// ... It was comment or trailing whitespace.
		// Let's handle the parameter as flag type, and simply set it to 'true'.
		value = "true";
	    }
	} else {
	    // Neither comment nor whitespace nor '=' found but parameter is not empty.
	    // Thus, it must be a flag type. Simply set the parameter to 'true'.
	    value = "true";
	}
    } else {
	// Line was empty or comment, whitespace or '=' found but parameter was empty.
	return false;
    }
    return true;
}

/*****************************************************************************/
void Configuration::warning(const std::string &filename, int lineNumber,
			  const std::string &description)
/*****************************************************************************/
{
    std::cerr << "Configuration warning in "
	      << filename << " line " << lineNumber
	      << ": " << description
	      << ". Skipping."
	      << std::endl;
}

/*****************************************************************************/
void Configuration::error(const std::string &filename, int lineNumber,
			  const std::string &description)
/*****************************************************************************/
{
    std::cerr << "Configuration error in "
	      << filename << " line " << lineNumber
	      << ": " << description
	      << "."
	      << std::endl;
}

/*****************************************************************************/
std::vector<std::string> Configuration::setFromCommandline(
    const std::vector<std::string> &arguments)
/*****************************************************************************/
{
    std::string line;
    for (std::vector<std::string>::const_iterator arg = arguments.begin() + 1; arg != arguments.end(); ++arg) {
	if (line.size()) line += " ";
	line += *arg;
    }
    return setFromCommandline(
	arguments,
	db_->addSource("command line", line));
}

/*****************************************************************************/
std::vector<std::string> Configuration::setFromCommandline(
    const std::vector<std::string> &arguments,
    const SourceDescriptor *source)
/*****************************************************************************/
{
    std::string option;
    std::vector<std::string> unparsed;
    enum { Option, Argument } state = Option;

    if (arguments.size() == 0) return unparsed;
    for (std::vector<std::string>::const_iterator arg = arguments.begin() + 1; arg != arguments.end(); ++arg) {
	if (((*arg)[0] == '-') && ((*arg)[1] == '-') && (state == Argument)) {
	    tryToSet(option, "true", source);
	    state = Option;
	}
	switch(state) {
	case Option:
	    if (((*arg)[0] == '-') && ((*arg)[1] == '-')) {
		if ((*arg)[1] == '\0') unparsed.push_back(*arg);
		else {
		    std::string::size_type equalSignPos = arg->find("=");
		    if (equalSignPos == std::string::npos)
			equalSignPos = arg->size();
		    option = arg->substr(2, equalSignPos - 2);
		    if (option[0] == resource_separation_char)
			option = option.substr(1);
		    else
			option = Application::us()->name() + resource_separation_string + option;
		    if (equalSignPos != arg->size()) {
			std::string argument;
			argument = arg->substr(equalSignPos + 1, std::string::npos);
			tryToSet(option, argument, source);
		    } else state = Argument;
		}
	    } else unparsed.push_back(*arg);
	    break;
	case Argument:
	    tryToSet(option, *arg, source);
	    state = Option;
	    break;
	}
    }

    if (state == Argument) {
	// assume open last option to be a flag
	tryToSet(option, "true", source);
    }

    return unparsed;
}

/*****************************************************************************/
const Configuration::Resource* Configuration::find(
  const std::string &parameter) const
/*****************************************************************************/
{
    const Resource *res = db_->find(parameter);
    return res;
}

/*****************************************************************************/
std::string Configuration::getResolvedValue(const Resource *res) const
/*****************************************************************************/
{
    std::string result;

    if (res->isBeingResolved()) {
	std::cerr << "configuration error: "
	     << "circular references encountered in resource \""
	     << res->getName() << "\"" << std::endl;
	result = "(" + res->getName() + ")";
    } else {
	res->beginResolution(); // protect against infinite recursion
	result = resolve(res->getValue());
	res->endResolution();
    }

    return result;
}

/*****************************************************************************/
std::string Configuration::resolve(const std::string &value) const
/*****************************************************************************/
{
    std::string result;
    result = resolveReferences(value);
    result = resolveArithmeticExpressions(result);
    return result;
}

/*****************************************************************************/
std::string Configuration::resolveReferences(const std::string &value) const
/*****************************************************************************/
{
    std::string result;
    std::string::size_type begin, pos = 0;

    while ((begin = value.find("$(", pos)) != std::string::npos) {
	result += value.substr(pos, begin - pos);
	pos = begin + 2;
	std::string::size_type end = value.find(")", pos);
	if (end != std::string::npos) {
	    const Resource *resource = 0;
	    std::string query(value.substr(pos, end - pos));

	    if (isWellFormedParameterName(query)) {
		// search for reference local to global
		for (std::string scope = getSelection();;) {
		    resource = find(prepareResourceName(scope, query));
		    if (resource) break;
		    if (scope.size() == 0) break;
		    std::string::size_type sp = scope.find_last_of(resource_separation_char);
		    if (sp == std::string::npos) sp = 0;
		    scope.erase(sp);
		}
	    } else {
		std::cerr << "configuration error: malformed reference \"" << query << "\"" << std::endl;
	    }

	    if (resource) {
		result += getResolvedValue(resource);
	    } else {
		// if not still not resolved -> warn
		std::cerr << "configuration error: could not resolve reference \""
		     << query << "\"" << std::endl;
		result += "(" + query + ")";
	    }

	    pos = end + 1;
	} else {
	    std::cerr << "configuration error: unclosed bracket in value \""
		 << value << "\"" << std::endl;
	    return std::string("");
	}
    }
    result += value.substr(pos, std::string::npos);

    return result;
}

/*****************************************************************************/
std::string Configuration::resolveArithmeticExpressions(const std::string &value) const
/*****************************************************************************/
{
    std::string result;
    std::string::size_type begin, pos = 0;
    ArithmeticExpressionParserDriver parser;


    while ((begin = value.find("$[", pos)) != std::string::npos) {
	bool error = false;
	std::string errMsg = "";
	result += value.substr(pos, begin - pos);
	pos = begin + 2;
	std::string::size_type end = value.find("]", pos);
	if (end != std::string::npos) {
	    std::string query(value.substr(pos, end - pos));
	    std::string format = "float";
	    if ((begin = query.find(",")) != std::string::npos) {
		format = query.substr(begin+1);
		stripWhitespace(format);
		query = query.substr(0, begin);
	    }
	    double dValue = 0;
	    if (!parser.parse(query, dValue)) {
		error = true;
		errMsg = "parse error";
		errMsg += parser.getLastError();
	    }
	    if (format == "int" || format == "i")
		result += form("%d", int(dValue));
	    else {
		if (format != "float" && format != "f") {
		    error = true;
		    errMsg = "unknown format: \"";
		    errMsg += format;
		    errMsg += "\"";
		}
		result += form("%f", dValue);
	    }
	    pos = end + 1;
	} else {
	    error = true;
	    errMsg = "unclosed bracket";
	}
	if (error) {
	    std::cerr << "configuration error: " << errMsg << " in value \""
		      << value << "\"" << std::endl;
	    return std::string("");
	}
    }
    result += value.substr(pos, std::string::npos);

    return result;
}

/*****************************************************************************/
bool Configuration::get(const std::string &parameter, std::string &value) const
/*****************************************************************************/
{

    require(parameter.find(resource_separation_char) == std::string::npos);
    require(parameter.find(resource_wildcard_char)   == std::string::npos);

    // extend parameter name by current selection
    std::string query = prepareResourceName(getSelection(), parameter);

    // get value for parameter
    const Resource *resource = find(query);
    if (resource) {
	value = getResolvedValue(resource);
	resource->registerUsage(query, 0, value);
	return true;
    } else {
	if (db_) db_->noResource()->registerUsage(query, 0, "?");
	return false;
    }
}


/*****************************************************************************/
void Configuration::enableLogging()
/*****************************************************************************/
{
    db_->enableLogging();
}

/*****************************************************************************/
void Configuration::writeSources(XmlWriter &os) const
/*****************************************************************************/
{
    db_->writeSources(os);
}

/*****************************************************************************/
void Configuration::writeResources(XmlWriter &os) const
/*****************************************************************************/
{
    os << XmlOpen("resources");
    db_->write(os);
    os << XmlClose("resources");
    os << XmlFull("selection", getSelection());
}

/*****************************************************************************/
void Configuration::writeResolvedResources(XmlWriter &os) const
/*****************************************************************************/
{
    os << XmlOpen("resolved-resources");
	for (std::set<Resource>::const_iterator itResource = db_->getResources().begin();
		itResource != db_->getResources().end();
		++itResource) {
		std::string value = getResolvedValue(&*itResource);
		os << itResource->getName() << " = " << value
		   << "\n";
	}
    os << XmlClose("resolved-resources");
    os << XmlFull("selection", getSelection());
}

/*****************************************************************************/
void Configuration::writeUsage(XmlWriter &os) const
/*****************************************************************************/
{
    db_->writeUsage(os);
}
