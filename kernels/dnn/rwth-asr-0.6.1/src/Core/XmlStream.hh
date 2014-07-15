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
// $Id: XmlStream.hh 7308 2009-07-29 09:49:00Z rybach $

#ifndef _CORE_XML_STREAM_HH
#define _CORE_XML_STREAM_HH

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <Core/Assertions.hh>
#include <Core/TextStream.hh>
#include <Core/Types.hh>

namespace Core {

	class XmlWriter;

	/**
	 * Streaming objects for XML output.
	 *
	 * Use of these classes is obligatory.  Correct XML output cannot
	 * be produced by using string constants and the standard
	 * streaming operators. (I.e.: os << "<tag/>"; does NOT work.)
	 */

	class XmlAttribute {
	protected:
		friend class XmlWriter;
		std::string name_, value_;
	public:
		XmlAttribute() {};
		template <typename V> XmlAttribute(const std::string &name, const V &value) {
			name_ = name;
			std::ostringstream ss; ss << value; value_ = ss.str();
		}
	};

	class XmlOpen {
	typedef XmlOpen Self;
	protected:
		friend class XmlWriter;
		const std::string element_;
		std::vector<XmlAttribute> attributes_;
	public:
		explicit XmlOpen(const std::string &element) : element_(element) {}
		const std::string &element() const { return element_; }
		Self& operator+(const XmlAttribute &attribute) {
			attributes_.push_back(attribute);
			return *this;
		}
	};

	class XmlClose {
	protected:
		friend class XmlWriter;
		const std::string element_;
	public:
		explicit XmlClose(const std::string &element) : element_(element) {}
		XmlClose(const XmlOpen &open) : element_(open.element()) {}
	};

	class XmlEmpty :
		public XmlOpen
	{
	typedef XmlEmpty Self;
	protected:
		friend class XmlWriter;
	public:
		explicit XmlEmpty(const std::string &element) : XmlOpen(element) {}
		Self& operator+(const XmlAttribute &attribute) {
			attributes_.push_back(attribute);
			return *this;
		}
		Self& operator+=(const XmlAttribute &attribute) {
			return operator+(attribute);
		}
	};

	class XmlFull :
		public XmlOpen
	{
		typedef XmlFull Self;
	protected:
		friend class XmlWriter;
		std::string content_;
	public:
		template <typename V>
			XmlFull(const std::string &element, const V &value) :
				XmlOpen(element)
		{
			std::ostringstream ss; ss << value; content_ = ss.str();
		}

		Self& operator+(const XmlAttribute &attribute) {
			attributes_.push_back(attribute);
			return *this;
		}
	};

	class XmlOpenComment {};
	class XmlCloseComment {};

	class XmlComment {
	protected:
		friend class XmlWriter;
		const std::string text_;
	public:
		explicit XmlComment(const std::string &text) : text_(text) {}
	};

	/** A blank character.  Cosecutive blanks are merged. */
	class XmlBlank { };

	/**
	 * Stream adaptor for producing XML output.
	 */

	class XmlWriter :
		public std::ostream
	{
		typedef XmlWriter Self;
	protected:
		class Buffer;

		std::ostream &os_;

		u8 inComment_;
		bool formattingHints_;
		bool shouldGenerateFormattingHints() const { return formattingHints_; }

		void formattingHint(TextOutputStream::FormattingHint);
		/**
		 * Put string but replace special characters.
		 * The given string is output, but all characters listed in @c
		 * escape are replaced by XML character entities (e.g. &lt;).
		 */
		void putEscaped(const std::string&, const char *escape);
		void putTag(const XmlOpen&, bool isEmpty);
		void putClosingTag(const XmlClose&);
		XmlWriter(const XmlWriter&);
	public:
		explicit XmlWriter(std::ostream&);
		~XmlWriter();

		/**
		 * Enable formatting hints.
		 * If formatting hints are enabled the XmlWriter will send
		 * control characters to the underlying stream, which enable
		 * nice structural formatting of the XML document.
		 * TextOutputStream is able to correctly interpret these
		 * control characters, so you should generate formatting hints
		 * only if the output stream is eventually processed by a
		 * TextOutputStream.
		 */
		void generateFormattingHints(bool enable = true) {
			formattingHints_ = enable;
		}
		void put(const std::string&);
		/**
		 * Generate the XML declaration that usually starts an XML
		 * document.  If formatting hints are enabdled, the underlying
		 * text output stream is instructed to insert its actual
		 * character encoding in the header declaration.  Otherwise
		 * the argument @c encoding is used.  You should specify @c
		 * encoding only if you cannot use formatting hints and if you
		 * know for sure which encoding will be used.
		 */
		void putDeclaration(const std::string &encoding = "");
		void put(const XmlOpen&);
		void put(const XmlClose&);
		void put(const XmlEmpty&);
		void put(const XmlFull&);
		void put(const XmlComment&);
		void put(const XmlOpenComment&);
		void put(const XmlCloseComment&);
		void put(const XmlBlank&);
#if 1
		//	Self &operator<<(std::omanip  t) { std::ostream::operator<<(t); return *this; }
		//	Self &operator<<(std::manip   t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(s8           t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(u8           t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(s16          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(u16          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(s32          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(u32          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(s64          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(u64          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(f32          t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(f64          t) { std::ostream::operator<<(t); return *this; }
#ifdef OS_darwin
		Self &operator<<(size_t       t) { std::ostream::operator<<(t); return *this; }
#endif
#ifndef __SUNPRO_CC
		Self &operator<<(char         t) { std::ostream::operator<<(t); return *this; }
#endif
		Self &operator<<(bool         t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(void        *t) { std::ostream::operator<<(t); return *this; }
		Self &operator<<(const  char *t) { return operator<<(std::string(t)); }
		Self &operator<<(const std::string &s) { std::ostream &os(*this); os << s; return *this; }
#endif
		Self &operator<<(const XmlOpen         &t) { put(t); return *this; }
		Self &operator<<(const XmlClose        &t) { put(t); return *this; }
		Self &operator<<(const XmlEmpty        &t) { put(t); return *this; }
		Self &operator<<(const XmlFull         &t) { put(t); return *this; }
		Self &operator<<(const XmlComment      &t) { put(t); return *this; }
		Self &operator<<(const XmlOpenComment  &t) { put(t); return *this; }
		Self &operator<<(const XmlCloseComment &t) { put(t); return *this; }
		Self &operator<<(const XmlBlank        &t) { put(t); return *this; }
	};


	/**
	 * Stream for writing XML files.
	 *
	 * Models TextOutputStream, see there for description of
	 * formatting features.
	 *
	 * Implementation note: Multiple inheritance from XmlWriter and
	 * TextOutputStream is not possible due to virtual base class of
	 * std::ostream
	 */

	class XmlOutputStream :
		public XmlWriter
	{
		typedef XmlOutputStream Self;
	private:
		TextOutputStream textOutputStream_;
	public:
		XmlOutputStream();
		XmlOutputStream(std::ostream*);
		XmlOutputStream(const std::string &filename, int mode = 0);
		void open(const std::string &filename, int mode = 0);
		void close() { textOutputStream_.close(); }
		virtual ~XmlOutputStream();

		// export TextOutputStream methods
		const std::string &encoding() const { return textOutputStream_.encoding(); }
		void setEncoding(const std::string &encoding = UnicodeOutputConverter::defaultEncoding) {
			textOutputStream_.setEncoding(encoding);
		}
		void setIndentation(u32 n) { textOutputStream_.setIndentation(n); }
		void setMargin(u32 m) { textOutputStream_.setMargin(m); }
		void putDeclaration();
	};


	// #if 0
	// 	using XmlWriter::operator<<;
	// #elif 0
	// 	template <typename T> Self &operator<<(const T &t) {
	// 	    XmlWriter &xw(*this); xw << t; return *this;
	// 	}
	// #else
	//       //	Self &operator<<(std::__omanip t) { XmlWriter::operator<<(t); return *this; }
	//       //	Self &operator<<(std::__manip  t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(s8           t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(u8           t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(s16          t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(u16          t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(s32          t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(u32          t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(f32          t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(f64          t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(bool         t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(void        *t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const  char *t) { XmlWriter::operator<<(t); return *this; }
	// 	//      	Self &operator<<(const uchar *t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const std::string &s) { XmlWriter &os(*this); os << s; return *this; }
	//       //	Self &operator<<(const std::string &s) { XmlWriter &os(*this); os << s; return *this; }
	// 	Self &operator<<(const XmlOpen         &t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const XmlClose        &t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const XmlEmpty        &t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const XmlFull         &t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const XmlComment      &t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const XmlOpenComment  &t) { XmlWriter::operator<<(t); return *this; }
	// 	Self &operator<<(const XmlCloseComment &t) { XmlWriter::operator<<(t); return *this; }
	// #endif
	//     };


} // namespace Core

#endif // _CORE_XML_STREAM_HH
