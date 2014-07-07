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
#ifndef _CORE_UNICODE_HH
#define _CORE_UNICODE_HH

#include <functional>
#include <string>
#include <iconv.h>
#include "TemporaryBuffer.hh"
#include "Assertions.hh"

/**
 * @page Unicode Unicode
 *
 * Internally all text is processed in Unicode.  This allows Sprint to
 * work gracefully with any language, even multiple languages
 * simultaneously.
 *
 * @subsection about About Unicode
 *
 * Unicode (http://www.unicode.org) provides a unique number for every
 * character ever used in human writing.  Therefore we can consider
 * all character sets as representations (encodings) of Unicode.
 * Internally we use either the UTF-8 or the UCS-4 encoding.  UTF-8 is
 * more compact but uses multibyte characters.  UCS-4, using 4 bytes
 * for each character, is rather wasteful on memory, but easier to use
 * for string processing.  Conversion between UTF-8 and UCS-4 is
 * always possible using the widen() and narrow() functions.
 *
 * @subsection internal Internal character set
 *
 * Rule: Inside Sprint any text data must be either UTF-8 (string) or
 * UCS-4 (wstring).  If your code reads text data from an external
 * source (e.g. a file) you must ensure proper conversion.  Yet,
 * special measures need to be taken only rarely (see below).
 *
 * @subsection external External character sets
 *
 * Externally various character sets can be used.  Most popular ones
 * are ASCII (7 bit characters, onyl standard Latin alphapet) and
 * IS0-8859-1 (8 bit characters including accented characters for
 * Western European languages).  Conversion can be done using
 * UnicodeInputConverter and UnicodeOutputConverter, but the
 * recommended approach is:
 * - Input of text data should be in XML.  These are processed by
 *   XmlParser which automatically detects the encoding of the XML file
 *   and converts all data to UTF-8.  (XML files should have a header of
 *   the form <?xml version="1.0" encoding="ISO-8859-1"?> to ensure that
 *   the character encoding is always detected correctly.)
 * - Text output is most of the time done via Channels, which
 *   automatically convert to a run-time configurable encoding.
 * - Text embedded in binary files should be UTF-8 encoded.
 * - If non of the above applies, the classes TextOutputStream and
 *   TextInputStream can be used.
 * - As a last resort convert files externally to UTF-8 using the
 *   iconv(1) tool.
 */

/**
 * @file Unicode.hh Unicode support functions
 * @see @ref Unicode
 */

namespace utf8 {
    const char blank = ' ' ;
    const char whitespace[] = " \t\n\r\f\v" ;
    /**
     * UTF-8 is based on bytes (8 bit).  Codepoints below 128
     * (i.e. ASCII characters) are represented as a singleByte.  Other
     * codepoints are encoded as multi-byte sequence, which consists
     * of a multiByteHead followed by one up to five multiByteTail
     * bytes.  The two byte values 0xfe and 0xff are illegal, they can
     * never occur in a UTF-8 string.
     */
    enum ByteType { singleByte, multiByteHead, multiByteTail, illegal };
    inline ByteType byteType(char u) {
	if ((u & 0x80) == 0x00) return singleByte;
	if ((u & 0xC0) == 0x80) return multiByteTail;
	if ((u & 0xFE) == 0xFE) return illegal;
	return multiByteHead;
    }

    /**
     * Count unicode characters in a UTF-8 string.
     * Unlike normal strlen(), this function handles multi-byte
     * characters correctly.
     */
    size_t length(const char*);

}

/** Convert UTF-8 Unicode string to wide character string (UCS-4) */
std::wstring widen(const std::string&);

/** Convert wide character (UCS-4) Unicode string to UTF-8. */
std::string narrow(const std::wstring&);


namespace Core {

    const char *const defaultEncoding = "ISO-8859-1";


class CharsetConverter {
protected:
    iconv_t iconvHandle_;
    size_t nErrors_;
    void deactivate();
public:
    CharsetConverter() {
	iconvHandle_ = (iconv_t) -1;
	nErrors_ = 0;
    }

    ~CharsetConverter() {
	deactivate();
    }

    bool isConversionActive() const { return iconvHandle_ != (iconv_t) -1; }
    size_t nErrors() const { return nErrors_; }
};

/**
 * Converter for reading text data.
 * When reading text from a source which is not encoded in UTF-8, this
 * class should be used to do the conversion. (Remember that ASCII
 * (7bit) is a subset of UTF-8.)  You don't need this if you use
 * XmlParser!
 * @see @ref Unicode
 */

class UnicodeInputConverter :
    public CharsetConverter
{
public:
    static const char *const defaultEncoding;

    void setInputEncoding(const char *outputEncoding);

    /** Convert a range of UTF-8 encoded characters of specified
     * input encoding.
     * @param begin characters from the range [begin .. end) are converted
     * @param end see @c begin
     * @param out ouput iterator to which converted character are stored
     * @return @c out plus number of generated characters. */
    template <typename InIterator, typename OutIterator>
    //    size_t convert(const InIterator &begin, const InIterator &end, OutIterator &out);
    OutIterator convert(const InIterator &begin, const InIterator &end, OutIterator out);
};

/**
 * Converter for writing text data.
 * When writing text data in an encoding other than UTF-8, this class
 * can be used to do the conversion.  (The user might want to choose
 * the output encoding at run time.)  You don't need this if you use a
 * Channel!
 * @see @ref Unicode
 */

class UnicodeOutputConverter :
    public CharsetConverter
{
public:
    static const char *const defaultEncoding;

    void setOutputEncoding(const char *outputEncoding);

    /** Convert a range of UTF-8 encoded characters of specified
     * output encoding.
     * @param begin characters from the range [begin .. end) are converted
     * @param end see @c begin
     * @param out ouput iterator to which converted character are stored
     * @return @c out plus number of generated characters. */
    template <typename InIterator, typename OutIterator>
    OutIterator convert(const InIterator &begin, const InIterator &end, OutIterator out);
};

} // namespace Core

#endif // _CORE_UNICODE_HH
