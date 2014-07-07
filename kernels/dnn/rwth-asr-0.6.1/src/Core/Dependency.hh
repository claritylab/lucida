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
#ifndef _CORE_DEPENDENCY_HH
#define _CORE_DEPENDENCY_HH

#include <iostream>
#include <map>
#include <string>

#include "Configuration.hh"

namespace Core {

    class Dependency {
    private:
	std::string value_;
	std::string date_;

    public:
	Dependency() {}
	Dependency(const std::string &value) { setValue(value); }
	void setValue(const std::string&);
	std::string value() const { return value_; }
	void setDate(const std::string &date) { date_ = date; }
	std::string date() const { return date_; }

	friend std::ostream& operator<< (std::ostream &o, const Dependency &d) {
	    return o << "value '" << d.value_ << "' generated on " << d.date_;
	}

	unsigned int getChecksum() const {
	    unsigned int ret = 0;
	    for(unsigned int a = 0; a < value_.length(); a++) ret += (unsigned int)value_[a];
	    return ret;
	}
    };

    class DependencySet {
    private:
	static const std::string nameSeparator_;

	typedef std::map<std::string, Dependency> Dependencies;
	Dependencies dependencies_;

    public:
	void clear() { dependencies_.erase(dependencies_.begin(), dependencies_.end()); }
	/**
	 *  Adds a complete dependency set to this one.
	 *  Before appending this set, each name in @param dependencySet is extended by @param name.
	 *
	 *  Use this add function to build a hierarchy of dependencies,
	 *  i.e. to differentiate resources instantiated more than once.
	 */
	void add(const std::string &name, const DependencySet &dependencySet);
	void add(const std::string&, const Dependency&);
	void add(const std::string&, const bool);
	void add(const std::string&, const std::string&);
	void add(const std::string&, const int);
	void add(const std::string&, const double);

	bool operator== (const DependencySet&) const;
	/** @return is true of all dependencies in @param required are equal. */
	bool satisfies(const DependencySet &required) const;

	/** @return is true if all dependencies in @param required are greater or equal.
		you should use this method with care.
	 */
	bool weak_satisfies(const DependencySet& required, bool warn=false) const;

	friend std::ostream& operator<< (std::ostream &o, const DependencySet &s) {
	    for (DependencySet::Dependencies::const_iterator d =
		     s.dependencies_.begin(); d != s.dependencies_.end(); ++d)
		o << "depends on '" << d->first << "' with " << d->second << std::endl;
	    return o;
	}

	unsigned int getChecksum() const {
	   unsigned int ret = 0;

	for (DependencySet::Dependencies::const_iterator d = dependencies_.begin(); d != dependencies_.end(); ++d)
	    ret += d->second.getChecksum();

	    return ret;
	}

	bool read(const Configuration &config, std::istream&);
	bool read(const Configuration &config, const std::string &filename);
	bool write(Core::XmlWriter&) const;
	bool write(const std::string &filename) const;
    };

    class Dependable {
    private:
	Dependency dependency_;

    public:
	Dependable() {}
	void setDependencyValue(const std::string&);
	Dependency getDependency() const { return dependency_; }
    };

} // namespace Core

#endif // _CORE_DEPENDENCY_HH
