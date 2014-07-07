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
// $Id: TextStream.hh 5439 2005-11-09 11:05:06Z bisani $

#ifndef _CORE_TEXT_STREAM_HH
#define _CORE_TEXT_STREAM_HH

#include <iostream>
#include <fstream>
#include <Core/Unicode.hh>
#include <Core/StringUtilities.hh>
#include <Core/Types.hh>

namespace Core {

    /**
     * Stream for text output.
     *
     * Use this for writing plain text files.  For XML use XmlOutoutStream.
     *
     * Features if TextOutputStream:
     * - word wrap
     * - indentation
     * - character set conversion
     *   Remeber: all internal string are considered UTF-8
     *
     * TextOutputStream redefines some ASCII control characters to
     * control text formatting.  Since these codes are old teletype
     * legacy and have no use in modern computing environments, we
     * should get away with this.
     */

    class TextOutputStream :
	public std::ostream
    {
    public:
	enum FormattingHint {
	    // Note: In XML documents only CR, LF and HT are permitted.
	    BEL = 7,
	    BS  = 8,
	    HT  = 9,
	    LF  = 10,
	    VT  = 11,
	    FF  = 12,
	    CR  = 13,

	    indentPlus       =  1, /**< increase indentation level by one (effective after next newline) */
	    indentMinus      =  2, /**< decrease indentation level by one (effective after next newline) */
	    describeEncoding =  3, /**< output name of output encoding used */
	    protect          =  4, /**< temporarily disable word-wrap (may be nested) */
	    unprotect        =  5, /**< reenable word-wrap */
	    newWord          =  6, /**< optional line break (even when protect is set) */
	    softBlank        = BEL,/**< consecutive soft blanks are replaced by a single normal blank */
	    lineBreak        = LF, /**< start a new line uncoditionally */
	    newLine          = 14  /**< start a new line, ignore if the current position is the beginning of a line */
	};

    private:
	class Buffer;

	Buffer *buffer_;
	std::ostream *output_;

    public:
	TextOutputStream();

	/**
	 * Create TextOutputStream on top of an existing ostream.
	 * TextOutputStream takes ownership of the ostream passed,
	 * and will delete it.
	 */
	explicit TextOutputStream(std::ostream*);

	/**
	 * Open/create named file and construct a TextOutputStream.
	 */
	TextOutputStream(const std::string &filename, int mode = 0);
	virtual ~TextOutputStream();

	/**
	 * Set a new underlying ostream.  TextOutputStream takes
	 * ownership of the ostream passed and will delete it.
	 */
	void adopt(std::ostream*);

	/**
	 * Open/create named file.
	 */
	void open(const std::string &filename, int mode = 0);

	/**
	 * Close and delete underlying file or ostream.
	 */
	void close();

	/**
	 * Restrict internal buffering to a single line.
	 */
	void setLineBuffered(bool);

	/**
	 * Limit amount of data to buffer internally.
	 * An argument of zero restores the default size.
	 */
	void setBufferSize(size_t s);

	/**
	 *  Character set encoding to be used in the external.
	 */
	const std::string &encoding() const;

	/**
	 * Change the character set encoding to be used in the external
	 * file.  Remember, internal strings are expected to be UTF-8.
	 */
	void setEncoding(const std::string &encoding = UnicodeOutputConverter::defaultEncoding);

	/**
	 * Set number of blank characters at the begining of each new
	 * line, PER LEVEL of indentation.  The actual indentation
	 * level is changed using the indentPlus and indentMinus
	 * control characters.
	 */
	void setIndentation(u32);

	/**
	 * Set maximum number of characters per line.  This is not a
	 * hard limit: When a single word is longer than the margin it
	 * will not be broken.  Also using the protect control
	 * character can cause the margin to be exceeded.
	 */
	void setMargin(u32);

	bool good() const { return output_ && output_->good(); }
	operator bool() const { return good(); }

	void flush();

    };

    /**
     * Stream for text input.
     *
     * Use this for reading plain text files.  For XML use XmlInputStream.
     *
     * Features if TextInputStream:
     * - word wrap
     * - indentation
     * - character set conversion
     *   Remember: all internal strings are considered UTF-8
     *
     * TextInputStream redefines some ASCII control characters to
     * control text formatting.  Since these codes are old teletype
     * legacy and have no use in modern computing environments, we
     * should get away with this.
     */

    class TextInputStream :
	public std::istream
    {
    private:
	class Buffer;

	Buffer *buffer_;
	std::istream *input_;

    private:
	int getInputLine(std::string &) const;

    public:
	TextInputStream();

	/**
	 * Create TextInputStream on top of an existing istream.
	 * TextInputStream takes ownership of the istream passed,
	 * and will delete it.
	 */
	explicit TextInputStream(std::istream*);

	/**
	 * Open/create named file and construct a TextInputStream.
	 */
	TextInputStream(const std::string &filename, int mode = 0);
	virtual ~TextInputStream();

	/**
	 * Set a new underlying istream.  TextInputStream takes
	 * ownership of the istream passed and will delete it.
	 */
	void adopt(std::istream*);

	/**
	 * Open/create named file.
	 */
	void open(const std::string &filename, int mode = 0);

	/**
	 * Close and delete underlying file or istream.
	 */
	void close();

	/**
	 * Limit amount of data to buffer internally.
	 * An argument of zero restores the default size.
	 */
	void setBufferSize(u32 s);

	/**
	 * Limit amount of data to put-back buffer internally.
	 * An argument of zero restores the default size.
	 */
	void setPutBackBufferSize(u32 s);

	/**
	 *  Character set encoding used in the external.
	 */
	const std::string &encoding() const;

	/**
	 * Change the character set encoding used in the external
	 * file.  Remember, internal strings are expected to be UTF-8.
	 */
	void setEncoding(const std::string &encoding = UnicodeInputConverter::defaultEncoding);

	bool good() const { return input_ && input_->good(); }
	operator bool() const { return good(); }
    };

} // namespace Core

#endif //_CORE_TEXT_STREAM_HH
