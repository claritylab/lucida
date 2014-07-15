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
#ifndef MAPPEDARCHIVE_HH
#define MAPPEDARCHIVE_HH

#include <string>
#include <fstream>
#include "ReferenceCounting.hh"
#include "Application.hh"
#include <tr1/type_traits>

namespace Core
{
class MappedArchiveReader;
class MappedArchiveWriter;
/**
 * This class represents a simple memory-mapped file.
 * The whole contents of the file is mapped upon calling "load",
 * and can be accessed through the data(offset) member function.
 * */
class MMappedFile
{
public:
    MMappedFile();
    ~MMappedFile();

    /// Maps the whole file.
    bool load(std::string file);
    /// Unmaps the file.
    void unload();

    /// Access the mapped data at a given @p offset in bytes
    template <class Type>
    const Type* data(size_t offset = 0) const {
	if(data_ == 0)
	    return 0;
	return reinterpret_cast<const Type*>(data_ + offset);
    }

    /// Returns the size of the mapped file
    size_t size() const;
private:
    char* data_;
    int desc_;
    size_t size_;
};

/**
 * This class represents an archive containing multiple "virtual" files,
 * which can be mapped to memory.
 *
 * The contained files can be accessed and written through the getWriter()
 * and getReader() functions.
 *
 * When a contained file is re-written, it is not really overwritten, but
 * rather the contents of the new version is appended. Old versions
 * are then removed during a cleanup phase when opening the archive the
 * next time.
 * */
class MappedArchive
{
public:
    MappedArchive(std::string archivePath, bool readOnly = false);

    ~MappedArchive();

    /**
     * Returns a writer which can be used to write the contents.
     * Only one writer may be alive at each point in time.
     * The changes are applied upon destruction of the writer.
     * The writer can be safely copied.
     * */
    MappedArchiveWriter getWriter(std::string name);

    /**
     * Returns a reader object for the contained file.
     * The reader can be safely copied.
     * */
    MappedArchiveReader getReader(std::string name);

    struct Writer;
    struct Reader;

private:
    std::string tempFile_;
    std::set<std::string> wroteTempFileItems_;

    void initializeTempFile();

    void releaseWriter(Writer* writer);

    struct MappedItem {
	MappedItem() : data(0), size(0)
	{
	}

	std::string name;
	const char* data;
	u64 size;
    };

    MappedItem getItem(std::string name) const;

    /*
     * Cleans up the current data into a temporary file,
     * and then writes it back into the original file.
     * */
    void finalize();

    bool loadData();

    bool loadFile();

    bool available(size_t size) const {
	return readOffset_ + size <= mapped_->size();
    }

    template <class T>
    T get() {
	T ret = *mapped_->data<T>(readOffset_);
	readOffset_ += sizeof(T);
	return ret;
    }

    void skip(size_t size) {
	readOffset_ += size;
    }

    std::vector<MappedItem> items_;
    MMappedFile* mapped_;
    std::string archivePath_;
    size_t readOffset_;
    Writer* currentWriter_;
    bool readOnly_;
};

/**
 * An optionally non-changeable and mappable vector,
 * which can either have a real vector as back-end storage, or can be memory-mapped.
 * */
template <class T>
class ConstantVector
{
public:
    ConstantVector() : data_(0), size_(0) {}

    ConstantVector(const T* mapped, size_t size) : data_(mapped), size_(size) { }

    ConstantVector(const std::vector<T>& data) :
	editable_(data),
	data_(editable_.data()),
	size_(editable_.size()) {
    }

    inline const T& operator[](size_t index) const {
	return data_[index];
    }

    /// Edit an element of the vector. After editing an element,
    /// the data is not constant any more.
    T& edit(size_t index) {
	makeEditable();
	return editable_[index];
    }

    size_t size() const {
	return size_;
    }

    const T* data() const {
	return data_;
    }

    typedef const T* const_iterator;

    const_iterator begin() const {
	return data_;
    }

    const_iterator end() const {
	return data_ + size_;
    }

    /// Resize the vector. After resizing the internal data
    /// is not constant and shared any more.
    void resize(size_t size, const T& value = T()) {
	if(size_ == size)
	    return;
	makeEditable();
	editable_.resize(size, value);
	size_ = size;
	data_ = editable_.data();
    }

    /// Add an element to the vector. Afterwards,
    /// the data is not constant and shared any more.
    void push_back(const T& value) {
	resize(size_ + 1, value);
    }

    const T& back() const {
	return *(data_ + size_ - 1);
    }

    bool isConstant() const {
	return data_ != editable_.data();
    }

private:
    void makeEditable() {
	if(isConstant())
	{
	    editable_.resize(size_);
	    memcpy(editable_.data(), data_, size_ * sizeof(T));
	    data_ = editable_.data();
	}
    }

    std::vector<T> editable_;
    const T* data_;
    size_t size_;
};

/**
 * The reader object which allows reading file contents from mapped archives
 * */
struct MappedArchive::Reader : public Core::ReferenceCounted
{
    Reader(std::string name, const char* data, size_t offset, size_t size);
    std::string name_;
    const char* data_;
    size_t initialOffset_;
    size_t offset_;
    size_t size_;
};

/**
 * The writer object which allows writing file contents into mapped archives
 * */
struct MappedArchive::Writer : public Core::ReferenceCounted
{
    Writer(std::string name, std::ofstream * out, MappedArchive * archive);
    ~Writer();
    std::string name_;
    std::ofstream* out_;
    MappedArchive* archive_;
    size_t itemOffset_;
    size_t dataOffset_;
};

class MappedArchiveReader : private Core::Ref<MappedArchive::Reader>
{
public:
    MappedArchiveReader(const Core::Ref<MappedArchive::Reader>& rhs = Core::Ref<MappedArchive::Reader>()) : Core::Ref<MappedArchive::Reader>(rhs) {
    }

    template <class T>
    void readValue(T& value) {
	verify(std::tr1::is_pod<T>::value && "must be a pod type");
	if(!good() || get()->offset_ + sizeof(T) > get()->initialOffset_ + get()->size_)
	{
	    invalidate();
	    return;
	}
	value = *reinterpret_cast<const T*>(get()->data_ + get()->offset_);
	get()->offset_ += sizeof(T);
	verify(get()->offset_ <= get()->initialOffset_ + get()->size_);
    }

    template <class T>
    void readVector(std::vector<T>& vec) {
	u64 size;
	readValue<u64>(size);

	if(!good() || get()->offset_ + sizeof(T) * size > get()->initialOffset_ + get()->size_)
	{
	    invalidate();
	    return;
	}

	vec.resize(size);
	memcpy(vec.data(), get()->data_ + get()->offset_, sizeof(T) * size);
	get()->offset_ += sizeof(T) * size;
    }

    template <class T>
    void readVectorVector(std::vector<std::vector<T> >& vectors) {
	u32 nVectors;
	readValue(nVectors);
	vectors.resize(nVectors);

	for(u32 a = 0; a < nVectors; ++a)
	    readVector(vectors[a]);
    }

    template <class T>
    void readVector(ConstantVector<T>& vec) {
	u64 size;
	readValue<u64>(size);

	if(!good() || get()->offset_ + sizeof(T) * size > get()->initialOffset_ + get()->size_)
	{
	    invalidate();
	    return;
	}

	vec = ConstantVector<T>(reinterpret_cast<const T*>(get()->data_ + get()->offset_), size);
	get()->offset_ += sizeof(T) * size;
    }

    std::string name() const {
	return get()->name_;
    }

    /// Reset the reader into the state where it was after retrieving
    void reset() {
	get()->offset_ = get()->initialOffset_;
    }

    void invalidate() {
	Core::Ref<MappedArchive::Reader>::reset();
    }

    /// Returns Whether this object is valid, eg. can be used for reading.
    /// Upon trying to read data over the file boundaries, the reader
    /// becomes invalid, eg. good() returns false.
    /// After you've finished reading, you should always check this to
    /// verify that the reading was successful.
    bool good() const {
	return (bool)get() && get()->data_;
    }

    operator bool() const {
	return good();
    }

    template <class T>
    MappedArchiveReader & operator>>(T& value) {
	readValue(value);
	return *this;
    }

    template <class T>
    MappedArchiveReader & operator>>(std::vector<T>& vector) {
	readVector(vector);
	return *this;
    }

    template <class T>
    MappedArchiveReader & operator>>(ConstantVector<T>& vector) {
	readVector(vector);
	return *this;
    }

    MappedArchiveReader & operator>>(std::string& str) {
	ConstantVector<char> vec;
	readVector(vec);
	if(vec.size() == 0)
	    str = std::string();
	else
	    str = std::string(vec.data());
	return *this;
    }

    template <class T>
    MappedArchiveReader & operator>>(std::vector<std::vector<T> >& vectors) {
	readVectorVector(vectors);
	return *this;
    }

    /// Convenience function for quick checking whether a read value equals the expected one
    /// On mismatch, returns false and logs the mismatch.
    /// @p expected The expected value
    /// @p desc Description of the value for logging the mismatch
    template <class T>
    bool check(T expected, std::string desc) {
	T v;
	(*this) >> v;
	if(v != expected)
	{
	    Core::Application::us()->log() << " in stream " << get()->name_ << ": " << " mismatch in value " << desc << ": got " << v << ", need " << expected;
	    return false;
	}
	return true;
    }
};

// WriterRef which can be forward-declared
class MappedArchiveWriter : private Core::Ref<MappedArchive::Writer>
{
public:
    MappedArchiveWriter(const Core::Ref<MappedArchive::Writer>& rhs = Core::Ref<MappedArchive::Writer>()) : Core::Ref<MappedArchive::Writer>(rhs) {
    }

    template <class T>
    void writeVector(const std::vector<T>& data) {
	writeValue<u64>(data.size());
	get()->out_->write((const char*)data.data(), sizeof(T) * data.size());
    }

    template <class T>
    void writeVectorVector(const std::vector<std::vector<T> >& vectors) {
	u32 nVectors = vectors.size();
	writeValue(nVectors);

	for(u32 a = 0; a < nVectors; ++a)
	    writeVector(vectors[a]);
    }

    template <class T>
    void writeVector(const ConstantVector<T>& data) {
	writeValue<u64>(data.size());
	get()->out_->write((const char*)data.data(), sizeof(T) * data.size());
    }

    template <class Type>
    inline void writeValue(const Type& value) {
	verify(std::tr1::is_pod<Type>::value && "must be a pod type");
	get()->out_->write((const char*)&value, sizeof(Type));
    }

    template <class Type>
    inline void writeValue(const Type* value) {
	verify(0 && "pointers are not allowed");
    }

    template <class T>
    MappedArchiveWriter & operator<<(const T& value) {
	writeValue(value);
	return *this;
    }

    template <class T>
    MappedArchiveWriter & operator<<(const std::vector<T>& vector) {
	writeVector(vector);
	return *this;
    }

    template <class T>
    MappedArchiveWriter & operator<<(const ConstantVector<T>& vector) {
	writeVector(vector);
	return *this;
    }

    template <class T>
    MappedArchiveWriter & operator<<(const std::vector<std::vector<T> >& vectors) {
	writeVectorVector(vectors);
	return *this;
    }

    MappedArchiveWriter & operator<<(const char* str) {
	return operator<<(std::string(str));
    }

    MappedArchiveWriter & operator<<(const std::string& str) {
	std::vector<char> vec(str.begin(), str.end());
	vec.push_back(0);
	writeVector(vec);
	return *this;
    }

    bool good() const {
	return get() && get()->out_->good();
    }

    operator bool() const {
	return good();
    }
};
}

#endif
