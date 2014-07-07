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
#ifndef _FLOW_QUANTIZED_VECTOR_HH
#define _FLOW_QUANTIZED_VECTOR_HH

#include <algorithm>
#include <string>
#include <typeinfo>
#include <vector>

#include <Core/Utility.hh>
#include <Core/XmlStream.hh>
#include <Core/BinaryStream.hh>

#include "Timestamp.hh"

namespace Flow {

    template<class T> class Vector : public virtual Timestamp, public std::vector<T> {
    private:
	typedef Vector<T> Self;

    public:
	Vector() : Data(&Datatype<Self>::type) {};
	Vector(int size) : Data(&Datatype<Self>::type), std::vector<T>(size) {}
	Vector(const std::vector<T> &v) : Data(&Datatype<Self>::type), std::vector<T>(v) {}
	virtual ~Vector() { }

	virtual Data* clone() { return new Self(*this); }

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const {
	    o  << Core::XmlOpen open(datatype()->name())
		+ Core::XmlAttribute("size", size())
		+ Core::XmlAttribute("start", startTime())
		+ Core::XmlAttribute("end", endTime());
	    if (!empty()) {
		for (const_iterator i = begin(); i != end() - 1; ++i)
		    o << *i << " ";
		o << back();
	    }
	    o << Core::XmlClose(datatype()->name());
	    return o;
	}
	bool read(Core::BinaryInputStream &i) {
	    u32 s; i >> s; resize(s);
	    for(iterator it = begin(); it != end(); it ++) i >> (*it);
	    return Timestamp::read(i);
	}
	bool write(Core::BinaryOutputStream &o) const {
	    o << (u32)size();
	    std::copy(begin(), end(), Core::BinaryOutputStream::Iterator<T>(o));
	    return Timestamp::write(o);
	}
    };
    template<class T> Core::XmlWriter& operator<< (Core::XmlWriter& o, const Vector<T> &v) {
	v.dump(o); return o;
    }
    template<class T> Core::BinaryOutputStream& operator<< (Core::BinaryOutputStream& o, const Vector<T> &v) {
	v.write(o); return o;
    }
    template<class T> Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, Vector<T> &v) {
	v.read(i); return i;
    }


    /**
     * Flow::Vector extended by an identifier
     */
    template<class T> class NamedVector : public Vector<T> {
    private:
	typedef NamedVector<T> Self;
	std::string name_;

    public:
	NamedVector(const std::string &name = "") : Data(&Datatype<Self>::type), name_(name) {}
	NamedVector(const std::string &name, int size) : Data(&Datatype<Self>::type), Vector<T>(size), name_(name) {}
	virtual ~NamedVector() {}

	const std::string name() const { return name_; }
	void setName(const std::string &name) { name_ = name; }

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const {
	    o  << Core::XmlOpen open(datatype()->name())
		+ Core::XmlAttribute("name", name())
		+ Core::XmlAttribute("size", size())
		+ Core::XmlAttribute("start", startTime())
		+ Core::XmlAttribute("end", endTime());
	    if (!empty()) {
		for (const_iterator i = begin(); i != end() - 1; ++i)
		    o << *i << " ";
		o << back();
	    }
	    o << Core::XmlClose(datatype()->name());
	    return o;
	}
	bool read(Core::BinaryInputStream &i) {
	    i >> name_;
	    return Vector<T>::read(i);
	}
	bool write(Core::BinaryOutputStream &o) const {
	    o << name_;
	    return Vector<T>::write(o);
	}
    };
    template<class T> Core::XmlWriter& operator<<(Core::XmlWriter& o, const NamedVector<T> &v) {
	v.dump(o); return o;
    }
    template<class T> Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream& o, const NamedVector<T> &v) {
	v.write(o); return o;
    }
    template<class T>  Core::BinaryInputStream& operator>>(Core::BinaryInputStream& i, NamedVector<T> &v) {
	v.read(i); return i;
    }

} // namespace Flow

#endif // _FLOW_QUANTIZED_VECTOR_HH
