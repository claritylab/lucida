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
#include <time.h>
#include <fstream>
#include <sstream>

#include "Application.hh"
#include "Dependency.hh"
#include "XmlBuilder.hh"
#include "XmlParser.hh"
#include "Types.hh"
#include <string>

using namespace Core;

//-----------------------------------------------------------------------------
// Parser

namespace {
class Parser : public XmlSchemaParser {
    typedef Parser Self;
private:
    DependencySet &dependencySet_;
    std::string name_;
    Dependency dependency_;

private:
    void startDependency(const XmlAttributes atts) {
	const char *name = atts["name"];
	if (!name) {
	    error("no name specified for dependency");
	    name_ = std::string();
	} else name_ = std::string(name);
	dependency_ = Dependency();
    }
    void setDependencyValue(const std::string &v) { dependency_.setValue(v); }
    void setDependencyDate (const std::string &d) { dependency_.setDate(d); }
    void endDependency() {
	if (!name_.empty())
	    dependencySet_.add(name_, dependency_);
    }

public:
    Parser(const Configuration &c, DependencySet &dependencySet) :
	XmlSchemaParser(c), dependencySet_(dependencySet)
    {
	XmlRegularElement *dependency = new XmlRegularElementRelay(
	    "dependency", this,
	    XmlRegularElementRelay::startHandler(&Self::startDependency),
	    XmlRegularElementRelay::endHandler(&Self::endDependency));
	collect(dependency);
	dependency->addTransition(
	    0, 1, collect(new XmlStringBuilderElement(
			      "value", this,
			      XmlStringBuilderElement::handler(&Self::setDependencyValue))));
	dependency->addTransition(
	    1, 2, collect(new XmlStringBuilderElement(
			      "date", this,
			      XmlStringBuilderElement::handler(&Self::setDependencyDate))));
	dependency->addFinalState(2);

	XmlRegularElement *newDependencySet = new XmlRegularElementRelay(
	    "dependency-set", this, 0, 0);
	collect(newDependencySet);
	newDependencySet->addTransition(0, 0, dependency);
	newDependencySet->addFinalState(0);

	setRoot(newDependencySet);
    }

    bool buildFromFile(const std::string &filename) { return (parseFile(filename.c_str()) == 0); }
    bool buildFromStream(std::istream &is) { return (parseStream(is) == 0); }
};
}

//-----------------------------------------------------------------------------
// Dependency

/*****************************************************************************/
void Dependency::setValue(const std::string &value)
/*****************************************************************************/
{
    value_ = value;
    time_t currentTime;
    time(&currentTime);
    date_ = ctime(&currentTime);
    std::string::size_type pos = date_.rfind('\n');
    if (pos != std::string::npos) date_.erase(pos);
}

//-----------------------------------------------------------------------------
// DependencySet

const std::string DependencySet::nameSeparator_(".");

/*****************************************************************************/
void DependencySet::add(const std::string &name, const DependencySet &dependencySet)
/*****************************************************************************/
{
    require(&dependencySet != this);

    for(Dependencies::const_iterator d = dependencySet.dependencies_.begin();
	d != dependencySet.dependencies_.end(); ++ d) {
	std::string n = name + nameSeparator_ + d->first;
	add(name + "." + d->first, d->second);
    }
}

/*****************************************************************************/
void DependencySet::add(const std::string &name, const Dependency &d)
/*****************************************************************************/
{
    if (dependencies_.find(name) != dependencies_.end())
	Application::us()->criticalError("duplicate entry '%s'", name.c_str());
    dependencies_[name] = d;
}

/*****************************************************************************/
void DependencySet::add(const std::string &name, const bool value)
/*****************************************************************************/
{
    if (value) add(name, Dependency("true"));
    else add(name, Dependency("false"));
}

/*****************************************************************************/
void DependencySet::add(const std::string &name, const std::string &value)
/*****************************************************************************/
{
    add(name, Dependency(value));
}

/*****************************************************************************/
void DependencySet::add(const std::string &name, const int value)
/*****************************************************************************/
{
    std::string tmp;
    add(name, itoa(tmp, value));
}

/*****************************************************************************/
void DependencySet::add(const std::string &name, const double value)
/*****************************************************************************/
{
    std::ostringstream tmp;
    tmp << value;
    add(name, tmp.str());
}

/*****************************************************************************/
bool DependencySet::operator== (const DependencySet &s) const
/*****************************************************************************/
{
    bool isEqual = true;
    if (dependencies_.size() != s.dependencies_.size()) {
	Application::us()->warning("dependency sets differ in size");
	isEqual = false;
    }
    for (Dependencies::const_iterator d = dependencies_.begin(); d != dependencies_.end(); ++d) {
	Dependencies::const_iterator found = s.dependencies_.find(d->first);
	if (found == s.dependencies_.end()) {
	    Application::us()->warning
		("dependency named '%s' with value '%s' generated on %s not in second set",
		 d->first.c_str(), d->second.value().c_str(),
		 d->second.date().c_str());
	    isEqual = false;
	} else {
	    if (d->second.value() != found->second.value()) {
		Application::us()->warning
		    ("dependency named '%s' with value '%s' generated on %s differs from value '%s' generated on %s",
		     d->first.c_str(), d->second.value().c_str(),
		     d->second.date().c_str(), found->second.value().c_str(),
		     found->second.date().c_str());
		isEqual = false;
	    }
	}
    }
    return isEqual;
}

/*****************************************************************************/
bool DependencySet::satisfies(const DependencySet &required) const
/*****************************************************************************/
{
    bool isEqual = true;
    Dependencies::const_iterator d;
    for (d = required.dependencies_.begin(); d != required.dependencies_.end(); ++ d) {
	Dependencies::const_iterator found = dependencies_.find(d->first);
	if (found == dependencies_.end()) {
	    Application::us()->warning
		("dependency named '%s' with value '%s' generated on %s not found",
		 d->first.c_str(), d->second.value().c_str(),
		 d->second.date().c_str());
	    isEqual = false;
	} else {
	    if (d->second.value() != found->second.value()) {
		Application::us()->warning
		    ("dependency named '%s' with value '%s' generated on %s differs from value '%s' generated on %s",
		     d->first.c_str(), d->second.value().c_str(),
		     d->second.date().c_str(), found->second.value().c_str(),
		     found->second.date().c_str());
		isEqual = false;
	    }
	}
    }
    return isEqual;
}

/*****************************************************************************/
bool DependencySet::weak_satisfies(const DependencySet& required, bool warn) const
/*****************************************************************************/
{
	bool isEqual = true;
    Dependencies::const_iterator d;
    for (d = required.dependencies_.begin(); d != required.dependencies_.end(); ++ d) {
	Dependencies::const_iterator found = dependencies_.find(d->first);
	if (found == dependencies_.end()) {
	    Application::us()->warning
		("dependency named '%s' with value '%s' generated on %s not found",
		 d->first.c_str(), d->second.value().c_str(),
		 d->second.date().c_str());
	    isEqual = false;
	} else {
	    if (d->second.value() != found->second.value()) {
			if( d->first == "acoustic-model.stream-extraction"){
				// convert strings to comparable values
				std::stringstream convert;
				u32 dValue;
				u32 foundValue;
				u32 help = d->second.value().rfind("=");
				convert << d->second.value().substr(help + 1);
				convert >> dValue;
				convert.str("");
				help = found->second.value().rfind("=");
				convert << found->second.value().substr(help +1);
				convert >> foundValue;
				convert.str("");

				if(dValue < foundValue){
					Application::us()->warning
					("dependency named '%s' with feature value '%s' generated on %s is smaller than acoustic model value '%s' generated on %s; should be greater equal.",
					 d->first.c_str(), d->second.value().c_str(),
					 d->second.date().c_str(), found->second.value().c_str(),
					 found->second.date().c_str());
					isEqual = false;
				}
			} else {
				Application::us()->warning
				("dependency named '%s' with feature value '%s' generated on %s differs from acoustic model value '%s' generated on %s",
				 d->first.c_str(), d->second.value().c_str(),
				 d->second.date().c_str(), found->second.value().c_str(),
				 found->second.date().c_str());
				isEqual = false;
			}
	    }
	}
    }
    return isEqual;
}
/*****************************************************************************/
bool DependencySet::read(const Configuration &config, const std::string &filename)
/*****************************************************************************/
{
    clear();
    Parser parser(config, *this);
    return parser.buildFromFile(filename);
}

/*****************************************************************************/
bool DependencySet::read(const Configuration &config, std::istream &is)
/*****************************************************************************/
{
    clear();
    Parser parser(config, *this);
    return parser.buildFromStream(is);
}

/*****************************************************************************/
bool DependencySet::write(Core::XmlWriter &str) const
/*****************************************************************************/
{
    if (!str) return false;
    str << Core::XmlOpen("dependency-set");
    for (Dependencies::const_iterator d = dependencies_.begin(); d != dependencies_.end(); ++d) {
	str << Core::XmlOpen("dependency") + Core::XmlAttribute("name", d->first)
	    << Core::XmlFull("value", d->second.value())
	    << Core::XmlFull("date",  d->second.date())
	    << Core::XmlClose("dependency");
    }
    str << Core::XmlClose("dependency-set");
    return str;
}

/*****************************************************************************/
bool DependencySet::write(const std::string &filename) const
/*****************************************************************************/
{
    Core::XmlOutputStream o(filename);
    return write(o);
}



/*****************************************************************************/
void Dependable::setDependencyValue(const std::string &value)
/*****************************************************************************/
{
    dependency_.setValue(value);
}
