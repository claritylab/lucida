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
#ifndef _FLOW_DATA_ADAPTOR_HH
#define _FLOW_DATA_ADAPTOR_HH

#include <Core/ReferenceCounting.hh>
#include "Timestamp.hh"

namespace Flow {

    /** Predefinition of DataAdaptor
     */
    template<class Type, class NetworkData = Timestamp>  class DataAdaptor;


    /** Network timestamp adaptor for type f32
     */
    typedef DataAdaptor<f32> Float32;
    /** Network timestamp adaptor for type f64
     */
    typedef DataAdaptor<f64> Float64;

    /** Network string class with timestamp
     */
    typedef DataAdaptor<std::string> String;


    /** DataAdaptor adapts the type Type to a network data.
     *  NetworkData can be the class Data or its derivaties.
     */
    template<class Type, class NetworkData>
    class DataAdaptor : public NetworkData {
	typedef DataAdaptor<Type, NetworkData> Self;
	typedef NetworkData Predecessor;
    private:
	Type data_;
    public:
	DataAdaptor() : NetworkData(type()) {};
	explicit DataAdaptor(const Type &d) : NetworkData(type()), data_(d) {}
	virtual ~DataAdaptor() {}

	static const Datatype* type() {
	    static Core::NameHelper<Type> name;
	    static DatatypeTemplate<Self> dt(name);
	    return &dt;
	};

	virtual Data* clone() const { return new Self(*this); }

	Type& operator()() { return data_; }
	Type& data() { return data_; }
	const Type& operator()() const { return data_; }
	const Type& data() const { return data_; }

	virtual bool operator==(const Data &other) const;

	virtual bool read(Core::BinaryInputStream &i);
	virtual bool write(Core::BinaryOutputStream &o) const;

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const;
    private:
	template<class T>
	void readType(Core::BinaryInputStream &i, T &t) const {
	    i >> t;
	}
	template<class T>
	void readType(Core::BinaryInputStream &i, Core::Ref<T> &t) const {
	    // reference types are not readable
	    defect();
	}
	template<class T>
	void writeType(Core::BinaryOutputStream &o, const T &t) const {
	    o << t;
	}
	template<class T>
	void writeType(Core::BinaryOutputStream &o, Core::Ref<const T> &t) const {
	    // reference types are not writable
	    defect();
	}
    };


    template<class Type, class NetworkData>
    bool DataAdaptor<Type, NetworkData>::operator==(const Data &other) const {
	// workaround for missing double dispatch
	const Self *pOther = dynamic_cast<const Self*>(&other);
	verify(pOther);
	return data_ == pOther->data();
    }


    template<class Type, class NetworkData>
    bool DataAdaptor<Type, NetworkData>::read(Core::BinaryInputStream &i) {
	// i >> data_;
	readType(i, data_);
	return Predecessor::read(i);
    }


    template<class Type, class NetworkData>
    bool DataAdaptor<Type, NetworkData>::write(Core::BinaryOutputStream &o) const {
	writeType(o, data_);
	return Predecessor::write(o);
    }


    template<class Type, class NetworkData>
    Core::XmlWriter& DataAdaptor<Type, NetworkData>::dump(Core::XmlWriter &o) const {
	o << this->xmlOpen();
	o << data_;
	o << this->xmlClose();
	return o;
    }


    template<class Type, class NetworkData>
    Core::XmlWriter& operator<< (Core::XmlWriter& o,
				 const DataAdaptor<Type, NetworkData> &v) {
	v.dump(o); return o;
    }


    template<class Type, class NetworkData>
    Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream& o,
					 const DataAdaptor<Type, NetworkData> &v) {
	v.write(o); return o;
    }


    template<class Type, class NetworkData>
    Core::BinaryInputStream& operator>>(Core::BinaryInputStream& i,
					DataAdaptor<Type, NetworkData> &v) {
	v.read(i); return i;
    }

} //namespace Flow

#endif // _FLOW_DATA_ADAPTOR_HH
