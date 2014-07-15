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
#ifndef _FLOW_ATTRIBUTES_HH
#define _FLOW_ATTRIBUTES_HH

#include <algorithm>
#include <functional>
#include <list>
#include <string>
#include <sstream>

#include <Core/Types.hh>
#include <Core/Utility.hh>
#include <Core/XmlBuilder.hh>
#include <Core/XmlStream.hh>
#include <Core/ReferenceCounting.hh>

/** @page Flow::Attributes Flow attributes
 *
 * Commonly used Flow attributes
 * - datatype
 * - filename
 * - id
 * - sample-rate
 * - track-count
 */

namespace Flow {

    /** Set of Flow network attributes.  */
    class Attributes : public Core::ReferenceCounted {

	class Attribute {
	private:
	    std::string name_;
	    std::string value_;

	public:
	    Attribute(const std::string &name, const std::string &value) { name_ = name; value_ = value; }

	    const std::string& getName() const { return name_; }
	    const std::string& getValue() const { return value_; }

	    bool operator< (const Attribute &a) const { return (name_ < a.name_); }
	    bool operator== (const Attribute &a) const { return (name_ == a.name_); }
	    friend Core::XmlWriter &operator<< (Core::XmlWriter &o, const Attribute &a) {
		o  << Core::XmlEmpty("flow-attribute")
		    + Core::XmlAttribute("name", a.name_)
		    + Core::XmlAttribute("value", a.value_);
		return o;
	    }
	};

    private:
	std::list<Attribute> list;

    public:
	Attributes() {}

	void merge(const Attributes &a) {
	    std::list<Attribute>::const_iterator it;
	    for (it = a.list.begin(); it != a.list.end(); it++)
		set((*it).getName().c_str(), (*it).getValue().c_str());
	}
	void set(const std::string &name, const std::string &value) {
	    std::list<Attribute>::iterator it;
	    for (it = list.begin(); it != list.end(); it++)
		if (it->getName() >= name) {
		    if (it->getName() == name) (*it) = Attribute(name, value);
		    else list.insert(it, Attribute(name, value));
		    return;
		}
	    list.insert(it, Attribute(name, value));
	}
	void set(const std::string &name, u32 value) {
	    std::string v;
	    set(name, Core::itoa(v, value));
	}
	void set(const std::string &name, s32 value) {
	    std::string v;
	    set(name, Core::itoa(v, value));
	}
	void set(const std::string &name, f32 value) {
	    std::ostringstream v;
	    v << value;
	    set(name, v.str());
	}
	void set(const std::string &name, f64 value) {
	    std::ostringstream v;
	    v << value;
	    set(name, v.str());
	}
	void remove(const std::string &name) {
	    std::list<Attribute>::iterator it;
	    for (it = list.begin(); it != list.end(); it++)
		if ((*it).getName() >= name) {
		    if ((*it).getName() == name) list.erase(it);
		    return;
		}
	}
	std::string get(const std::string &name) const {
	    std::list<Attribute>::const_iterator it;
	    for (it = list.begin(); it != list.end(); it++)
		if ((*it).getName() >= name)
		    if ((*it).getName() == name) return (*it).getValue();
	    return "";
	}

	friend Core::XmlWriter &operator<< (Core::XmlWriter &o, const Attributes &a) {
	    o << Core::XmlOpen("flow-attributes");
	    for (std::list<Attribute>::const_iterator i = a.list.begin(); i != a.list.end(); ++i)
		o << *i;
	    o << Core::XmlClose("flow-attributes");
	    return o;
	}

	class Parser : public Core::XmlSchemaParser {
	private:
	    typedef Parser Self;
	    Attributes *attribs_;
	    void startAttribute(const Core::XmlAttributes);
	public:
	    Parser(const Core::Configuration&);
	    bool buildFromString(Attributes&, const std::string &str);
	    bool buildFromStream(Attributes&, std::istream &i);
	    bool buildFromFile  (Attributes&, const std::string &filename);
	};
    };

} // namespace Flow

#endif // _FLOW_ATTRIBUTES_HH
