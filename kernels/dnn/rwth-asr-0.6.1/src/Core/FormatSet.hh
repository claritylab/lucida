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
#ifndef _CORE_FORMAT_SET_HH
#define _CORE_FORMAT_SET_HH

#include "BinaryStream.hh"
#include "CompressedStream.hh"
#include "Hash.hh"
#include "XmlStream.hh"
#include "Component.hh"
#include "ReferenceCounting.hh"
#include "Application.hh"
#include <fstream>

namespace Core {

    /**
     *  Set of format-qualifiers.
     *  Supports filenames with format qualifiers, e.g. bin:foo.matrix, xml:foo.vector, etc.
     *  Usage:
     *    1) Instantiate this class in every library, tipically in  Application classes.
     *    2) For your class Foo, derive from the Format<Foo> class your own formats, e.g.
     *      FooFormat : FormatSet::Format<Foo> {..}.
     *    3) Implement the required read/write methods in FooFormat class.R
     *    4) Register the your format at your library FormatSet object.
     *    5) To read or store foo objects call the I/O function of the fomat set,
     *       e.g. Application::us()->formatSet()->read("xml:foo.xml", foo);
     *  Note:
     *    -You can have different types of format objects in one format set.
     */
    class FormatSet : public Component {
    private:
	static const std::string defaultQualifier;
	static const std::string qualifierDelimiter;
    public:
	enum AccessMode {
	    modeRead,
	    modeWrite
	};
	/**
	 *  Interface class for type specific formats.
	 *  ReferenceCounting is used to automatize the destruction.
	 */
	template<class T>
	struct Format : ReferenceCounted {
	    virtual bool read(const std::string &filename, T &o) const = 0;
	    virtual bool write(const std::string &filename, const T &o) const = 0;
	    virtual bool write(const std::string &filename, const T &o, int precision) const {
		return write(filename, o);
	    }
	};

	/**
	 *  String map of format object references for the type T.
	 *  ReferenceCounting is used to automatize the destruction.
	 */
	template<class T>
	class TypeSpecificFormats :
	    public StringHashMap<Ref<const Format<T> > >,
	    public ReferenceCounted
	{};
	/**
	 *   String map of type specific format maps.
	 *   Since the different type specific formats maps have only one
	 *   common precursor, this map stores the list by referencing
	 *   ReferenceCounted derived object.
	 *   The type specific maps are casted back to their oroginal type
	 *   TypeSpecificFormats<T> on demand when the type T becomes known
	 *   via a call to read or write methods.
	 *   @see getTypeSpecificFormats.
	 */
	typedef StringHashMap<Ref<ReferenceCounted> > Formats;
    private:
	Formats formats_;
    private:
	static std::string getQualifier(const std::string &filename);
	static std::string stripQualifier(const std::string &filename);
	template<class T>
	void getTypeSpecificFormats(Ref<TypeSpecificFormats<T> > &);
	template<class T>
	void getTypeSpecificFormats(Ref<TypeSpecificFormats<T> > &) const;
	template<class T>
	bool getFormat(const std::string &qualifier,
		       Ref<const Format<T> > &format) const;
	bool checkFile(const std::string &filename, AccessMode mode) const;
    public:
	FormatSet(const Configuration &c) : Component(c) {}
	~FormatSet() {}

	/**
	 *  Registers a format in the format set.
	 *  @c defines the string constant which chooses identifies
	 *  the Format<T> object per type T, e.g.: bin, xml, ascii, dot, ...
	 *  @c is pointer to a class derived from Format<T>. This class has to
	 *  implement the abstract virtual I/O functions declared in Format class.
	 *  Type T defines the object the can be read or written. Implement these
	 *  classes next to your class T, and register it in the format set of the
	 *  you library.
	 *  If $s isDefault is true, the format $c format will be used for filenames
	 *  without a type qualifier. Note, that there is only one default format
	 *  per type T.
	 */
	template<class T>
	void registerFormat(const std::string &qualifier,
			    const Format<T> *format,
			    bool isDefault = false);

	/**
	 *  Reads the object @c o of type T from the file @c filename.
	 *  @c filename can contain a format qualifier, e.g. xml:foo.matrix, bin:foo.cart.
	 *  @return is false if the specified format is not registered for the type T, or
	 *  if the I/O operation was not successful.
	 *  In case of an error error messages are generated.
	 */
	template<class T>
	bool read(const std::string &filename, T &o) const;
	/**
	 *  Stores the object @c o of type T in the file @c filename.
	 *  @c filename can contain a format qualifier, e.g. xml:foo.matrix, bin:foo.cart.
	 *  @return is false if the specified format is not registered for the type T, or
	 *  if the I/O operation was not successful.
	 *  In case of an error error messages are generated.
	 */
	template<class T>
	bool write(const std::string &filename, const T &o) const;
	/**
	 *  Stores the object @c o of type T in the file @c filename.
	 *  @c precision specifies the precision with which numbers are represented.
	 *  This functionality is mainly meant for text formats thus the default behaviour
	 *  is ignoring this parameter.
	 *  @c filename can contain a format qualifier, e.g. xml:foo.matrix, bin:foo.cart.
	 *  @return is false if the specified format is not registered for the type T, or
	 *  if the I/O operation was not successful.
	 *  In case of an error error messages are generated.
	 */
	template<class T>
	bool write(const std::string &filename, const T &o, int precision) const;
    };

    template<class T>
    void FormatSet::getTypeSpecificFormats(Ref<TypeSpecificFormats<T> > &typeSpecificFormats)
    {
	NameHelper<T>  nameHelper;
	Formats::iterator f = formats_.find(nameHelper);
	if (f == formats_.end())
	    f = formats_.insert(std::make_pair(nameHelper, ref(new TypeSpecificFormats<T>()))).first;
	typeSpecificFormats = ref(dynamic_cast<TypeSpecificFormats<T>*>(f->second.get()));
	require(typeSpecificFormats);
    }

    template<class T>
    void FormatSet::getTypeSpecificFormats(Ref<TypeSpecificFormats<T> > &typeSpecificFormats) const
    {
	NameHelper<T>  nameHelper;
	Formats::const_iterator f = formats_.find(nameHelper);
	verify(f != formats_.end());
	typeSpecificFormats = ref(dynamic_cast<TypeSpecificFormats<T>*>(f->second.get()));
	require(typeSpecificFormats);
    }

    template<class T>
    void FormatSet::registerFormat(const std::string &qualifier,
				   const Format<T> *format,
				   bool isDefault)
    {
	Ref<TypeSpecificFormats<T> > formats; getTypeSpecificFormats(formats);
	require(formats->find(qualifier) == formats->end());
	(*formats)[qualifier] = ref(format);
	if (isDefault)
	    registerFormat(defaultQualifier, format, false);
    }

    template<class T>
    bool FormatSet::getFormat(const std::string &qualifier,
			      Ref<const Format<T> > &format) const
    {
	Ref<TypeSpecificFormats<T> > formats; getTypeSpecificFormats(formats);
	std::string q(qualifier);
	if (q.empty()) q = defaultQualifier;

	typename TypeSpecificFormats<T>::const_iterator found = formats->find(q);
	if (found == formats->end()) {
	    NameHelper<T> nameHelper;
	    error("Format '%s' is not supported by type '%s'.", q.c_str(), nameHelper.c_str());
	    return false;
	}
	format = found->second;
	return true;
    }

    template<class T>
    bool FormatSet::read(const std::string &filename, T &o) const
    {
	std::string name(stripQualifier(filename));
	Ref<const Format<T> > format;
	if (checkFile(name, modeRead) && getFormat(getQualifier(filename), format))
	    return format->read(name, o);
	return false;
    }

    template<class T>
    bool FormatSet::write(const std::string &filename, const T &o) const
    {
	std::string name(stripQualifier(filename));
	Ref<const Format<T> > format;
	if (checkFile(name, modeWrite) && getFormat(getQualifier(filename), format))
	    return format->write(name, o);
	return false;
    }

    template<class T>
    bool FormatSet::write(const std::string &filename, const T &o, int precision) const
    {
	std::string name(stripQualifier(filename));
	Ref<const Format<T> > format;
	if (checkFile(name, modeWrite) && getFormat(getQualifier(filename), format))
	    return format->write(name, o, precision);
	return false;
    }

    /**
     *  Template helper class for registering binary formats.
     *  To register binary format for class Foo in a format set call the function
     *  Formatset::registerFormat("bin", new Core::BinaryFormat<Foo>());
     */
    template<class T>
    struct BinaryFormat : public FormatSet::Format<T> {
	virtual bool read(const std::string &filename, T &o) const {
	    BinaryInputStream is(filename); is >> o; return is.good();
	}
	virtual bool write(const std::string &filename, const T &o) const {
	    BinaryOutputStream os(filename); os << o; return os.good();
	}
    };

    /**
      *  Template helper class for registering compressed binary formats.
      *  To register compressed binary format for class Foo in a format set call the function
      *  Formatset::registerFormat("bin", new Core::BinaryFormat<Foo>());
      */
    template<class T>
    struct CompressedBinaryFormat : public FormatSet::Format<T> {
    virtual bool read(const std::string &filename, T &o) const {
	CompressedInputStream cis(filename);
	BinaryInputStream is(cis); is >> o; return is.good();
    }
    virtual bool write(const std::string &filename, const T &o) const {
	CompressedOutputStream cos(filename);
	BinaryOutputStream os(cos); os << o; return os.good();
    }
    };

    /**
     *  Template helper class for registering plain text formats.
     *  To register plain text format for class Foo in a format set call the function
     *  Formatset::registerFormat("ascii", new Core::PlainTextFormat<Foo>());
     */
    template<class T>
    struct PlainTextFormat : public FormatSet::Format<T> {
	virtual bool read(const std::string &filename, T &o) const {
		std::ifstream is(filename.c_str()); is >> o; return is.good();
	}
	virtual bool write(const std::string &filename, const T &o) const {
	    std::ofstream os(filename.c_str());
	    os << o; return os.good();
	}
	virtual bool write(const std::string &filename, const T &o, int precision) const {
	    std::ofstream os(filename.c_str());
	    os.precision(precision); os << o; return os.good();
	}
    };

    /**
     *  Template helper class for registering plain text formats.
     *  To register plain text format for class Foo in a format set call the function
     *  Formatset::registerFormat("ascii", new Core::PlainTextFormat<Foo>());
     */
    template<class T>
    struct CompressedPlainTextFormat : public FormatSet::Format<T> {
	virtual bool read(const std::string &filename, T &o) const {
	    Core::CompressedInputStream is(filename.c_str());
	    is >> o; return is.good();
	}
	virtual bool write(const std::string &filename, const T &o) const {
	    Core::CompressedOutputStream os(filename.c_str());
	    os << o; return os.good();
	}
	virtual bool write(const std::string &filename, const T &o, int precision) const {
	    Core::CompressedOutputStream os(filename.c_str());
	    os.precision(precision); os << o; return os.good();
	}
    };

    /**
     *  Template helper class for registering xml formats.
     *  To register xml format for class Foo in a format set call the function
     *  Formatset::registerFormat("xml", new Core::XmlFormat<Foo, FooParser>());
     *
     *  Template parameter Parser is a class, which can parse an XML document and
     *  initialize an object of type T.
     */
    template<class T, class Parser>
    struct XmlFormat : public FormatSet::Format<T> {
	virtual bool read(const std::string &filename, T &o) const {
	    Parser parser(Application::us()->getConfiguration(), o);
	    return parser.parseFile(filename.c_str()) == 0;
	}
	virtual bool write(const std::string &filename, const T &o) const {
	    Core::XmlOutputStream os(filename);
	    os << o; return os.good();
	}
	virtual bool write(const std::string &filename, const T &o, int precision) const {
	    Core::XmlOutputStream os(filename);
	    os.precision(precision); os << o; return os.good();
	}
    };

} // namespace Core

#endif // _CORE_FORMAT_SET_HH
