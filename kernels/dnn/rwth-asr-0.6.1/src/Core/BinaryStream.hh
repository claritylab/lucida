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
#ifndef _CORE_BINARY_STREAM_HH
#define _CORE_BINARY_STREAM_HH

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <iterator>

#include "Types.hh"

/**
 * @page BinaryStreams
 *
 * BinaryStream classes: BinaryStream, BinaryOutputStream, BinaryInputStream
 *
 * Classes support binary output using streaming operators.
 * read and write functions support the io of blocks of data.
 *
 * BinaryStreams support endianess swapping.  The byte order of the
 * file can be set in the constructor.  Swapping is carried out if the
 * native byte order of the architecture is different from one in the
 * file.  By convention Sprint stores binary data in little endian
 * files.  Therefore you should specify the byte order in the
 * constructor only when dealing with non-Sprint file formats.
 *
 * Classes for which binary persistence is required should implement
 * methods void read() and write() methods accepting BinaryInputStream
 * or BinaryOutputStream respecitvely, or global operators for
 * BinaryStreams:
 * BinaryOutputStream& operator<<(BinaryOutputStream& o, const Class &c);
 * BinaryInputStream& operator>>(BinaryInputStream& i, Class &c);
 **/

/**
 * @file
 * @todo CodeSmell: wrapper and file stream roles should be separated: BinaryWriter/Reader vs. BinaryStream
 */


namespace Core {

    class BinaryStreamIos {
    public:
	typedef std::ios_base::iostate iostate;
	enum Endianess { bigEndian, littleEndian, nativeByteOrder };
	static const Endianess defaultEndianess = littleEndian;

    protected:
	std::ios *ios_;
	std::fstream *fstream_;
	bool swap_;

	BinaryStreamIos(Endianess endianess = defaultEndianess);
	BinaryStreamIos(std::ios&, Endianess endianess = defaultEndianess);
	BinaryStreamIos(const std::string&, std::ios_base::openmode, Endianess endianess = defaultEndianess);
    public:
	~BinaryStreamIos();

	// fstream functions
	void open(const std::string &name, std::ios_base::openmode mode);
	void close();
	bool isOpen() const;

	void setEndianess(Endianess endianess);

	// state functions
	bool good() const { return ios_->good(); }
	bool eof() const { return ios_->eof(); }
	bool fail() const { return ios_->fail(); }
	bool bad() const { return ios_->bad(); }
	void clear() { ios_->clear(); }
	iostate state() const { return ios_->rdstate(); }
	void setState(iostate s) { ios_->clear(s); }
	void addState(iostate s) { ios_->setstate(s); }

	operator bool() const { return !fail(); }
	bool operator!() const { return fail(); }
    };

    /**
     * BinaryOutputStream
     */

    class BinaryOutputStream :
	virtual public BinaryStreamIos
    {
    private:
	std::ostream *os_;
    public:
	template <class Tp> class Iterator : public std::iterator<std::output_iterator_tag, Tp > {
	protected:
	    BinaryOutputStream* stream_;
	public:
	    Iterator(BinaryOutputStream &os) : stream_(&os) {}
	    Iterator<Tp>& operator=(const Tp &value) { (*stream_) << value; return *this; }
	    Iterator<Tp>& operator*() { return *this; }
	    Iterator<Tp>& operator++() { return *this; }
	    Iterator<Tp>& operator++(int) { return *this; }
	};

    public:
	BinaryOutputStream(Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(endianess), os_(fstream_) {}
	explicit BinaryOutputStream(std::ostream &stream, Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(stream, endianess), os_(&stream) {}
	explicit BinaryOutputStream(
	    const std::string &fileName,
	    std::ios_base::openmode mode = std::ios::out,
	    Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(fileName, mode | std::ios::out, endianess),
	    os_(fstream_) {}

	void open(
	    const std::string &name,
	    std::ios_base::openmode mode = std::ios::out)
	{
	    BinaryStreamIos::open(name, mode);
	}

	std::streampos position() const { return os_->tellp(); }
	BinaryOutputStream& seek(std::streampos position) {
	    os_->seekp(position); return *this;
	}
	BinaryOutputStream& seek(std::streamoff offset, std::ios::seekdir direction) {
	    os_->seekp(offset, direction); return *this;
	}

	bool synchronizeBuffer() {
	    os_->flush();
	    return (!os_->bad());
	}

	template <typename T> bool write(const T *v, std::streamsize n = 1);

	BinaryOutputStream& operator<<(u8);
	BinaryOutputStream& operator<<(s8);
	BinaryOutputStream& operator<<(s16);
	BinaryOutputStream& operator<<(u16);
	BinaryOutputStream& operator<<(s32);
	BinaryOutputStream& operator<<(u32);
	BinaryOutputStream& operator<<(u64);
	BinaryOutputStream& operator<<(s64);
#ifdef OS_darwin
	BinaryOutputStream& operator<<(size_t);
#endif
	BinaryOutputStream& operator<<(const char *s);
	BinaryOutputStream& operator<<(const std::string &);
	BinaryOutputStream& operator<<(bool);
	BinaryOutputStream& operator<<(f64);
	BinaryOutputStream& operator<<(f32);
    };

    template<class T>
    BinaryOutputStream&  operator<< (BinaryOutputStream& o, const std::vector<T>& vec){
	o << (u32)vec.size();
	for(typename std::vector<T>::const_iterator p= vec.begin(); p != vec.end(); ++p)
	    o << *p;
	return o;
    }

    template<class T1,class T2>
    BinaryOutputStream&  operator<< (BinaryOutputStream& o, const std::map<T1,T2>& m){
	o << (u32)m.size();
	for(typename std::map<T1, T2>::const_iterator p= m.begin(); p!= m.end(); ++p)
	    o << p->first << p->second;
	return o;
    }

    template<class T>
    BinaryOutputStream&  operator<< (BinaryOutputStream& o, const std::set<T>& s){
	o << (u32)s.size();
	for(typename std::set<T>::const_iterator p= s.begin(); p!= s.end(); ++p)
	    o << *p;
	return o;
    }


    /**
     * BinaryInputStream
     */

    class BinaryInputStream : virtual public BinaryStreamIos {
    private:
	std::istream *is_;
    public:
	template <class Tp> class Iterator : public std::iterator<std::input_iterator_tag, Tp> {
	protected:
	    BinaryInputStream *stream_;
	    Tp value_;

	    void read() { if (!stream_->eof()) (*stream_) >> value_; }
	public:
	    Iterator(BinaryInputStream &is) : stream_(&is) { read(); }
	    const Tp& operator*() const { return value_; }
	    const Tp* operator->() const { return &(operator*()); }
	    Iterator<Tp>& operator++() { read(); return *this; }
	    Iterator<Tp> operator++(int) {
		Iterator<Tp> tmp = *this; read(); return tmp;
	    }
	};

    public:
	BinaryInputStream(
	    Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(endianess), is_(fstream_) {}
	BinaryInputStream(
	    std::istream &stream, Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(stream, endianess), is_(&stream) {}
	BinaryInputStream(
	    const std::string &fileName,
	    std::ios_base::openmode mode = std::ios::in,
	    Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(fileName, mode | std::ios::in, endianess),
	    is_(fstream_) {}

	void open(
	    const std::string &name,
	    std::ios_base::openmode mode = std::ios::in)
	{
	    BinaryStreamIos::open(name, mode);
	}

	std::streampos position() const { return is_->tellg(); }
	BinaryInputStream& seek(std::streampos position) {
	    is_->seekg(position); return *this;
	}
	BinaryInputStream& seek(std::streamoff offset, std::ios::seekdir direction) {
	    is_->seekg(offset, direction); return *this;
	}

	bool synchronizeBuffer() {
	    is_->sync();
	    return (!is_->bad());
	}

	template <typename T> bool read(T *v, std::streamsize n = 1);
	template <typename T> std::streamsize readSome(T *v, std::streamsize n = 1);

	BinaryInputStream& operator>>(u8&);
	BinaryInputStream& operator>>(s8&);
	BinaryInputStream& operator>>(s16&);
	BinaryInputStream& operator>>(u16&);
	BinaryInputStream& operator>>(s32&);
	BinaryInputStream& operator>>(u32&);
	BinaryInputStream& operator>>(s64&);
	BinaryInputStream& operator>>(u64&);
	BinaryInputStream& operator>>(std::string &);
	BinaryInputStream& operator>>(bool&);
	BinaryInputStream& operator>>(f64&);
	BinaryInputStream& operator>>(f32&);
    };

    template<class T>
    BinaryInputStream& operator>> (BinaryInputStream& is, std::vector<T>& vec){
	u32 size;
	is >> size;
	vec.resize(size);
	for(typename std::vector<T>::iterator iter= vec.begin(); iter != vec.end(); ++iter) {
	    is >> (*iter);
	}
	return is;
    }

    template<class T1,class T2>
    BinaryInputStream& operator>> (BinaryInputStream& is, std::map<T1,T2>& m){
	u32 size;
	is >> size;
	for(u32 j=0; j< size; ++j){
	    T1 key; T2 value;
	    is >> key >> value;
	    m[key]=value;
	}
	return is;
    }

    template<class T>
    BinaryInputStream& operator>> (BinaryInputStream& is, std::set<T>& s){
	u32 size;
	is >> size;
	for(u32 j=0; j< size; ++j){
	    T value;
	    is >> value;
	    s.insert(value);
	}
	return is;
    }


    class BinaryStream : public BinaryInputStream, public BinaryOutputStream {
    public:
	BinaryStream(
	    const std::string &fileName = "",
	    std::ios_base::openmode mode = std::ios::in | std::ios::out,
	    Endianess endianess = defaultEndianess) :
	    BinaryStreamIos(fileName, mode, endianess),
	    BinaryInputStream(fileName, mode, endianess),
	    BinaryOutputStream(fileName, mode, endianess) {}

	bool synchronizeBuffer() {
	    BinaryOutputStream::synchronizeBuffer();
	    BinaryInputStream::synchronizeBuffer();
	    return (BinaryInputStream::good() && BinaryOutputStream::good());
	}
    };



    class BinaryInputStreams : public std::vector<BinaryInputStream*> {
	typedef std::vector<BinaryInputStream*> Precursor;
    public:
    BinaryInputStreams();

	BinaryInputStreams(
	    const std::vector<std::string> &filenames,
	    std::ios_base::openmode mode = std::ios::in,
	    BinaryStreamIos::Endianess endianess = BinaryStreamIos::defaultEndianess);
	BinaryInputStreams(
		    std::vector<std::istream*> *istreams,
		    BinaryStreamIos::Endianess endianess = BinaryStreamIos::defaultEndianess);

	~BinaryInputStreams();

	BinaryInputStream& front() { return *Precursor::front(); }
	BinaryInputStream& operator[](u32 n) { return *Precursor::operator[](n); }
	const BinaryInputStream& operator[](u32 n) const { return *Precursor::operator[](n); }
	bool areOpen() const;
    };


} // namespace Core

#endif // _CORE_BINARY_STREAM_HH
