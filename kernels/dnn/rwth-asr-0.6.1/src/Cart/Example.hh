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
#ifndef _CART_EXAMPLE_HH
#define _CART_EXAMPLE_HH

#include <Core/Assertions.hh>
#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>
#include <Core/Vector.hh>
#include <Core/XmlStream.hh>

#include "Properties.hh"


namespace Cart {

    // ============================================================================
    /**
       An efficient implementation of a 2-dim matrix.
       The class supports masquerading: it can present
       itself as a scalar, a vector, or a 2-dim. matrix.
    */
    class FloatBox {
    public:
	/*
	  vector
	*/
	typedef f64 * vector_iterator;

	typedef const f64 * const_vector_iterator;


	/*
	  matrix -> vector of row vectors
	*/
	typedef f64 * row_iterator;

	typedef const f64 * const_row_iterator;


	class row_vector_iterator {
	private:
	    typedef row_iterator Self;

	private:
	    f64 * ptr_;
	    size_t size_;

	public:
	    row_vector_iterator(f64 * begin, size_t size) :
		ptr_(begin),size_(size) {}

	    row_iterator begin() { return ptr_; }
	    row_iterator end()   { return ptr_ + size_;   }

	    const_row_iterator begin() const { return ptr_; }
	    const_row_iterator end() const   { return ptr_ + size_;   }

	    row_iterator       operator*()       { return ptr_; }
	    const_row_iterator operator*() const { return ptr_; }
	    void operator++() { ptr_ += size_; }
	    void operator--() { ptr_ -= size_; }
	    bool operator!=(const row_vector_iterator & it) const { return ptr_ != it.ptr_; }
	};

	class const_row_vector_iterator {
	private:
	    f64 * ptr_;
	    size_t size_;

	public:
	    const_row_vector_iterator(f64 * begin, size_t size) :
		ptr_(begin),size_(size) {}

	    const_row_iterator begin() const { return ptr_; }
	    const_row_iterator end() const   { return ptr_ + size_;   }

	    const_row_iterator operator*() const { return ptr_; }
	    void operator++() { ptr_ += size_; }
	    void operator--() { ptr_ -= size_; }
	    bool operator!=(const const_row_vector_iterator & it) const { return ptr_ != it.ptr_; }
	};


	/*
	  matrix -> vector of column vectors
	*/
	class column_iterator {
	private:
	    f64 * ptr_;
	    const size_t offset_;

	public:
	    column_iterator(f64 * begin, size_t offset) :
		ptr_(begin), offset_(offset) {}

	    f64 & operator*() { return *ptr_; }
	    void operator++() { ptr_ += offset_; }
	    void operator--() { ptr_ -= offset_; }
	    bool operator!=(const column_iterator & it) const { return ptr_ != it.ptr_; }
	};

	class const_column_iterator {
	private:
	    f64 * ptr_;
	    const size_t offset_;

	public:
	    const_column_iterator(f64 * begin, size_t offset) :
		ptr_(begin), offset_(offset) {}

	    f64  operator*() const { return *ptr_; }
	    void operator++() { ptr_ += offset_; }
	    void operator--() { ptr_ -= offset_; }
	    bool operator!=(const const_column_iterator & it) const { return ptr_ != it.ptr_; }
	};


	class column_vector_iterator {
	private:
	    f64 * ptr_;
	    size_t size_;
	    size_t overall_size_;

	public:
	    column_vector_iterator(f64 * begin, size_t size, size_t overall_size) :
		ptr_(begin), size_(size), overall_size_(overall_size) {}

	    column_iterator begin() { return column_iterator(ptr_, size_); }
	    column_iterator end()   { return column_iterator(ptr_ + overall_size_, 0); }

	    const_column_iterator begin() const { return const_column_iterator(ptr_, size_); }
	    const_column_iterator end() const   { return const_column_iterator(ptr_ + overall_size_, 0); }

	    column_iterator       operator*()       { return column_iterator(ptr_, size_); }
	    const_column_iterator operator*() const { return const_column_iterator(ptr_, size_); }
	    void operator++() { ++ptr_; }
	    void operator--() { --ptr_; }
	    bool operator!=(const column_vector_iterator & it) const { return ptr_ != it.ptr_; }
	};

	class const_column_vector_iterator {
	private:
	    f64 * ptr_;
	    size_t size_;
	    size_t overall_size_;

	public:
	    const_column_vector_iterator(f64 * begin, size_t size, size_t overall_size) :
		ptr_(begin), size_(size), overall_size_(overall_size) {}

	    const_column_iterator begin() const { return const_column_iterator(ptr_, size_); }
	    const_column_iterator end() const   { return const_column_iterator(ptr_ + overall_size_, 0); }

	    const_column_iterator operator*() const { return const_column_iterator(ptr_, size_); }
	    void operator++() { ++ptr_; }
	    void operator--() { --ptr_; }
	    bool operator!=(const const_column_vector_iterator & it) const { return ptr_ != it.ptr_; }
	};


	/*
	  hash function
	*/
	struct HashFcn {
	    size_t operator()(const FloatBox & val) const;
	};

	/*
	  pointer less function
	*/
	struct PtrLessFcn {
	    size_t operator()(const FloatBox * & val1, const FloatBox * & val2) const {
		return (*val1 < *val2);
	    }
	    size_t operator()(const FloatBox * const & val1, const FloatBox * const & val2) const {
		return (*val1 < *val2);
	    }
	};

	/*
	  pointer equality function
	*/
	struct PtrEqualFcn {
	    size_t operator()(const FloatBox * const & val1, const FloatBox * const & val2) const {
		return (*val1 == *val2);
	    }
	};

	/*
	  pointer hash function
	*/
	struct PtrHashFcn {
	    HashFcn hasher;

	    size_t operator()(const FloatBox * const & val) const {
		return hasher(*val);
	    }
	};

    public:
	size_t rows_;
	size_t columns_;
	size_t size_;
	f64 * begin_, * end_;

    private:
	void init() {
	    begin_ = new f64[size_];
	    end_ = begin_ + size_;
	}

    public:
	FloatBox() :
	    rows_(1),
	    columns_(1),
	    size_(1) {
	    init();
	    *begin_ = 0.0;
	}
	FloatBox(size_t size) :
	    rows_(1),
	    columns_(size),
	    size_(size) {
	    init();
	    ::memset((void *)begin_, '\0', size_ * sizeof(f64));
	}
	FloatBox(size_t rows, size_t columns) :
	    rows_(rows),
	    columns_(columns),
	    size_(rows * columns) {
	    init();
	    ::memset((void *)begin_, '\0', size_ * sizeof(f64));
	}
	FloatBox(const FloatBox & floatBox) :
	    rows_(floatBox.rows_),
	    columns_(floatBox.columns_),
	    size_(floatBox.size_) {
	    init();
	    ::memcpy((void *)begin_, (const void *)floatBox.begin_, size_ * sizeof(f64));
	}
	~FloatBox() {
	    delete[] begin_;
	}

	// as scalar
	f64 get() const { return begin_[0]; }
	void set(f64 value) { begin_[0] = value; }


	// as vector
	size_t size() const { return size_; }

	f64 get(size_t i) const { return begin_[i]; }
	void set(size_t i, f64 value) { begin_[i] = value; }

	f64 & operator()(size_t i) { return begin_[i]; }

	vector_iterator begin() { return begin_; }
	vector_iterator end() { return end_; }

	const_vector_iterator begin() const { return begin_; }
	const_vector_iterator end() const { return end_; }


	// as 2-dim. values
	size_t rows() const { return rows_; }
	size_t columns() const { return columns_; }

	f64 get(size_t i, size_t j) const { return begin_[i * columns_ + j]; }
	void set(size_t i, size_t j, f64 value) { begin_[i * columns_ + j] = value; }

	f64 & operator()(size_t i, size_t j) { return begin_[i * columns_ + j]; }

	row_vector_iterator row(size_t i)
	    { return row_vector_iterator(begin_ + (i * columns_), columns_); }
	const_row_vector_iterator row(size_t i) const
	    { return const_row_vector_iterator(begin_ + (i * columns_), columns_); }

	row_vector_iterator begin_row()
	    { return row_vector_iterator(begin_, columns_); }
	row_vector_iterator end_row()
	    { return row_vector_iterator(end_, 0); }

	const_row_vector_iterator begin_row() const
	    { return const_row_vector_iterator(begin_, columns_); }
	const_row_vector_iterator end_row()   const
	    { return const_row_vector_iterator(end_, 0); }


	column_vector_iterator column(size_t i)
	    { return column_vector_iterator(begin_ + i, columns_, rows_ * columns_); }
	const_column_vector_iterator column(size_t i) const
	    { return const_column_vector_iterator(begin_ + i, columns_, rows_ * columns_); }

	column_vector_iterator begin_column()
	    { return column_vector_iterator(begin_, columns_, rows_ * columns_); }
	column_vector_iterator end_column()
	    { return column_vector_iterator(begin_ + columns_, 0, 0); }

	const_column_vector_iterator begin_column() const
	    { return const_column_vector_iterator(begin_, columns_, rows_ * columns_); }
	const_column_vector_iterator end_column()   const
	    { return const_column_vector_iterator(begin_ + columns_, 0, 0); }


	// some operators
	FloatBox & operator+=(const FloatBox & values);
	bool operator==(const FloatBox & values) const;
	bool operator<(const FloatBox & values) const;


	// self-representation (as values)
	void writeValues(std::ostream & os) const;

	void write(std::ostream & os) const;
	void writeXml(Core::XmlWriter & xml) const;
    };



    // ============================================================================
    /**
       An example is the basic unit to be clustered;
       it contains of a properties object at which
       questions can be asked and values used
       to compute the score of an example/ a collection
       of examples.
    */
    class Example :
	public Core::ReferenceCounted {

    public:
	Properties * properties;
	FloatBox * values;
	f64 nObs;

	Example() :
	    properties(0),
	    values(0),
	    nObs(0) {}
	Example(
	    Properties * properties,
	    FloatBox * values
	    ) :
	    properties(properties),
	    values(values),
	    nObs(0) {}
	~Example() {
	    delete properties;
	    delete values;
	}

	void write(std::ostream & os) const;
	void writeXml(Core::XmlWriter & os) const;
    };
    typedef Core::Ref<Example> ExampleRef;
    typedef Core::Ref<const Example> ConstExampleRef;
    typedef std::vector<ConstExampleRef> ConstExampleRefList;


    // ============================================================================


    class ExampleList :
	Core::Component {
    protected:
	typedef Core::Component Precursor;

    public:
	typedef Core::Vector<ExampleRef> ExampleRefVector;
	typedef ExampleRefVector::iterator iterator;
	typedef ExampleRefVector::const_iterator const_iterator;

	static const Core::ParameterString paramExampleFilename;
	static const Core::ParameterStringVector paramExampleFilenamesToMerge;

    private:
	PropertyMapRef map_;
	ExampleRefVector exampleRefs_;

    public:
	ExampleList(
	    const Core::Configuration & config,
	    PropertyMapRef map = PropertyMapRef(new PropertyMap)) :
	    Precursor(config),
	    map_() {
	    setMap(map);
	}

	void setMap(PropertyMapRef map) { require(map); map_ = map; }
	bool hasMap() { return !map_->empty(); }
	PropertyMapRef getMap() const { return map_; }
	const PropertyMap & map() const { return *map_; }

	void add(Example * example) { add(ExampleRef(example)); }
	void add(ExampleRef exampleRef) { exampleRefs_.push_back(exampleRef); }
	void set(size_t id, Example * example) { set(id, ExampleRef(example)); }
	void set(size_t id, ExampleRef exampleRef) { exampleRefs_.set(id, exampleRef); }

	size_t size() const { return exampleRefs_.size(); }
	ExampleRef operator[](size_t i) { return  exampleRefs_[i]; }
	iterator begin() { return exampleRefs_.begin(); }
	iterator end() { return exampleRefs_.end(); }
	ConstExampleRef operator[](size_t i) const { return  exampleRefs_[i]; }
	const_iterator begin() const { return exampleRefs_.begin(); }
	const_iterator end() const { return exampleRefs_.end(); }

	bool loadFromString(const std::string & str);
	bool loadFromStream(std::istream & i);
	bool loadFromFile(std::string filename = "");

	bool mergeFromFiles(std::vector<std::string> filenames = std::vector<std::string>());

	void write(std::ostream & os) const;
	void writeXml(Core::XmlWriter & xml) const;
	void writeToFile() const;
    };
} // namespace Cart

inline std::ostream & operator<<(std::ostream & out, const Cart::Example & e) {
    e.write(out);
    return out;
}

inline std::ostream & operator<<(std::ostream & out, const Cart::ExampleList & e) {
    e.write(out);
    return out;
}

#endif // _CART_EXAMPLE_HH
