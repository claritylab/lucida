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
#ifndef _FLOW_AGGREGATE_HH
#define _FLOW_AGGREGATE_HH

#include "Merger.hh"
#include "Vector.hh"

namespace Flow {

    /** Aggregate of Flow objects
     *  Aggregate collects objects of different streams into one object.
     *  It supports polymorfism:
     *    -The base class for the input objects is Timestamp.
     *     Thus, input objects must be instances of the base class or of one of its derived classes.
     *    -I/O is solved using features supported by Data.
     */
    class Aggregate : public Timestamp, public std::vector<DataPtr<Timestamp> >
    {
	typedef Aggregate Self;
	typedef std::vector<DataPtr<Timestamp> > Precursor;
    public:
	typedef Timestamp DataType;
    private:
	bool isSameDataType() const;
	bool readSameType(Core::BinaryInputStream &is);
	bool writeSameType(Core::BinaryOutputStream &os) const;

	bool readDifferentTypes(Core::BinaryInputStream &is);
	bool writeDifferentTypes(Core::BinaryOutputStream &os) const;
    protected:
	Core::XmlOpen xmlOpen() const {
	    return (Timestamp::xmlOpen() + Core::XmlAttribute("size", size()));
	}
    public:
	static const Datatype *type();
	Aggregate() : Timestamp(type()) {}
	template<class InputIterator>
	Aggregate(InputIterator begin, InputIterator end) : Timestamp(type()), Precursor(begin, end) {}
	virtual ~Aggregate() {}

	virtual Data* clone() const { return new Self(*this); }

	/** Retrieves a reference to the i-th element.
	 *  @return is false, if the i-th element is not an instance of class T or
	 *  of one of its descendants.
	 */
	template<class T>
	bool get(size_t i, DataPtr<T> &d) { return (bool)(d.set(operator[](i))); }
	/** Retrieves references to each elements.
	 *  @return is false, if one of the elements is not an instance of class T or
	 *  of one of its descendants.
	 */
	template<class T>
	bool get(std::vector<DataPtr<T> > &d) {
	    std::vector<DataPtr<T> > dd(begin(), end());
	    if (std::find_if(dd.begin(), dd.end(), std::bind2nd(std::equal_to<bool>(), false)) != dd.end())
		return false;
	    d = dd;
	    return true;
	}

	virtual Core::XmlWriter& dump(Core::XmlWriter &o) const;
	virtual bool read(Core::BinaryInputStream &is);
	virtual bool write(Core::BinaryOutputStream &os) const;
    };

    /** Aggregation filter.
     *  All input packets must be derived from type Aggregate::DataType, which will be aggregated,
     *  e.g. their references will be collected in a vector.
     */
    class AggregationNode : public MergerNode<Aggregate::DataType, Aggregate>
    {
	typedef MergerNode<Aggregate::DataType, Aggregate> Precursor;
    public:
	static std::string filterName() {
	    return std::string("generic-aggregation");
	}
	AggregationNode(const Core::Configuration &c) :
	    Core::Component(c), Precursor(c) {}
	virtual ~AggregationNode() {}

	virtual bool configure();

	virtual Aggregate *merge(std::vector<DataPtr<Aggregate::DataType> > &inputData) {
	    return new Aggregate(inputData.begin(), inputData.end());
	}
    };

} // namespace Flow

namespace Core {
    template <>
    class NameHelper<Flow::Aggregate> : public std::string {
    public:
	NameHelper() : std::string("aggregate") {}
    };
} // namespace Core

#endif // _FLOW_AGGREGATE_HH
