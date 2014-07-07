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
#ifndef _FLOW_DATATYPE_HH
#define _FLOW_DATATYPE_HH

#include <iostream>
#include <Core/Assertions.hh>
#include <Core/BinaryStream.hh>
#include <Core/Types.hh>

#include "Data.hh"

namespace Flow {

    class Application;

    /**
     * Flow data type identifier
     *
     * Currently, datatypes in flow are used for:
     *
     * - abstract creation (method newData())
     * - identification (method name())
     * - abstract gathered i/o (methods readGatheredData(), writeGatheredData())
     *
     * generic gathered i/o is simulated through virtual reads/writes delegated
     * to Data.
     **/
    class Datatype {
    private:
	std::string name_;
    protected:
	Datatype(const std::string &_name) { name_ = _name; }
    public:
	virtual ~Datatype() {}
	inline bool operator < (const Datatype &d) const
	    { return name_ < d.name_; }
	friend std::ostream& operator << (std::ostream& o, const Datatype &d)
	    { o << d.name_; return o; }
	friend std::ostream& operator << (std::ostream& o, const Datatype *d)
	    { o << d->name_; return o; }

	const std::string& name() const { return name_; }

	bool readData(Core::BinaryInputStream &, DataPtr<Data> &) const;
	bool writeData(Core::BinaryOutputStream &, const DataPtr<Data> &) const;

	virtual bool readGatheredData(Core::BinaryInputStream &i,
				      std::vector<DataPtr<Data> > &data) const;
	virtual bool writeGatheredData(Core::BinaryOutputStream &o,
				       const std::vector<DataPtr<Data> > &data) const;

    protected:
	void brand(Data *d) const {
	    verify(d->datatype_ == this);
	}
    public:
	virtual Data* newData() const = 0;
    };

    /** Flow data type definition */
    template <class T>
    class DatatypeTemplate :
	public Datatype
    {
    public:
	DatatypeTemplate(const std::string &_name) : Datatype(_name) {}
	virtual Data* newData() const {
	    Data* d = new T();
	    brand(d);
	    return d;
	}
    };

} // namespace Flow

#endif // _FLOW_DATATYPE_HH
