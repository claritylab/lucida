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
#ifndef _FLOW_VECTOR_HH
#define _FLOW_VECTOR_HH

#include <algorithm>
#include <complex>
#include <string>
#include <vector>
#include <Core/BinaryStream.hh>
#include <Core/XmlStream.hh>

#include "Timestamp.hh"

namespace Flow {

    /**
     * Vector
     */
    template<class T>
    class Vector :
	public Timestamp, public std::vector<T>
    {
    private:
	typedef Vector<T> Self;
    protected:
	Core::XmlOpen xmlOpen() const {
	    return (Timestamp::xmlOpen() + Core::XmlAttribute("size", this->size()));
	}
    public:
	static const Datatype *type() {
	    static Core::NameHelper<Vector<T> > name;
	    static DatatypeTemplate<Self> dt(name);
	    return &dt;
	};
	Vector() : Timestamp(type()) {};
	Vector(size_t size) : Timestamp(type()), std::vector<T>(size) {}
	Vector(size_t n, const T &t) : Timestamp(type()), std::vector<T>(n, t) {}
	Vector(const std::vector<T> &v) : Timestamp(type()), std::vector<T>(v) {}
	template<class InputIterator>
	Vector(InputIterator begin, InputIterator end) : Timestamp(type()), std::vector<T>(begin, end) {}

	virtual ~Vector() { }

	virtual Data* clone() const { return new Self(*this); }

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const;
	virtual bool read(Core::BinaryInputStream &i);
	virtual bool write(Core::BinaryOutputStream &o) const;
    };

    template <typename T>
    Core::XmlWriter& Vector<T>::dump(Core::XmlWriter &o) const {
	o << xmlOpen();
	if (!this->empty()) {
	    for (typename std::vector<T>::const_iterator i = this->begin(); i != this->end() - 1; ++ i)
		o << *i << " ";
	    o << this->back();
	}
	o << xmlClose();
	return o;
    }

    template <typename T>
    bool Vector<T>::read(Core::BinaryInputStream &i) {
	u32 s; i >> s; this->resize(s);
	T e; // Intoduced for T=bool: std::vector<bool> does not meet the requirements for being a container.
	for(typename std::vector<T>::iterator it = this->begin(); it != this->end();  ++ it) { i >> e; (*it) = e;}
	return Timestamp::read(i);
    }

    template <typename T>
    bool Vector<T>::write(Core::BinaryOutputStream &o) const {
	o << (u32)this->size();
	std::copy(this->begin(), this->end(), Core::BinaryOutputStream::Iterator<T>(o));
	return Timestamp::write(o);
    }

    template<class T> Core::XmlWriter& operator<< (Core::XmlWriter& o, const Vector<T> &v) {
	v.dump(o); return o;
    }
    template<class T> Core::BinaryOutputStream& operator<< (Core::BinaryOutputStream& o, const Vector<T> &v) {
	v.write(o); return o;
    }
    template<class T> Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, Vector<T> &v) {
	v.read(i); return i;
    }

} // namespace Flow

namespace Core {
    template <typename T>
    class NameHelper<Flow::Vector<T> > : public std::string {
    public:
	NameHelper() : std::string(Core::NameHelper<std::vector<T> >()) {}
    };
} // namespace Core

#endif // _FLOW_VECTOR_HH
