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
// $Id: Unicode.cc 9227 2013-11-29 14:47:07Z golik $

#include "Assertions.hh"
#include "Unicode.hh"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "TemporaryBuffer.hh"
#include <iterator>

using namespace Core;


size_t utf8::length(const char *uu) {
    size_t result = 0;
    for (; *uu; ++uu) {
	utf8::ByteType bt = utf8::byteType(*uu);
	if (bt == utf8::singleByte || bt == utf8::multiByteHead)
	    ++result;
    }
    return result;
}

std::wstring widen(const std::string &in) {
    std::wstring result;
    for (std::string::size_type i = 0 ; i < in.length() ;) {
	unsigned char u = in[i];
	wchar_t w;
	if ((u & 0x80) == 0) {
	    w = wchar_t(in[i++]);
	} else if ((u & 0xE0) == 0xC0) {
	    w  = wchar_t(in[i++] & 0x1F) << 6;
	    w |= wchar_t(in[i++] & 0x3F);
	} else if ((u & 0xF0) == 0xE0) {
	    w  = wchar_t(in[i++] & 0x0F) << 12;
	    w |= wchar_t(in[i++] & 0x3F) << 6;
	    w |= wchar_t(in[i++] & 0x3F);
	} else if ((u & 0xF8) == 0xF0) {
	    w  = wchar_t(in[i++] & 0x07) << 18;
	    w |= wchar_t(in[i++] & 0x3F) << 12;
	    w |= wchar_t(in[i++] & 0x3F) << 6;
	    w |= wchar_t(in[i++] & 0x3F);
	} else if ((u & 0xFC) == 0xF8) {
	    w  = wchar_t(in[i++] & 0x03) << 24;
	    w |= wchar_t(in[i++] & 0x3F) << 18;
	    w |= wchar_t(in[i++] & 0x3F) << 12;
	    w |= wchar_t(in[i++] & 0x3F) << 6;
	    w |= wchar_t(in[i++] & 0x3F);
	} else if ((u & 0xFE) == 0xFC) {
	    w  = wchar_t(in[i++] & 0x01) << 30;
	    w |= wchar_t(in[i++] & 0x3F) << 24;
	    w |= wchar_t(in[i++] & 0x3F) << 18;
	    w |= wchar_t(in[i++] & 0x3F) << 12;
	    w |= wchar_t(in[i++] & 0x3F) << 6;
	    w |= wchar_t(in[i++] & 0x3F);
	} else {
	    require(false); // not a valid UTF-8 string
	    w = 0; // avoid warning in release build
	}
	result += w;
    }
    return result;
}

std::string narrow(const std::wstring &in) {
    std::string result;
    for (std::wstring::size_type i = 0 ; i < in.length() ; ++i) {
	wchar_t w = in[i];
	require(w >= 0);
	if (w < 0x80) {
	    result += char(w);
	} else if (w < 0x800) {
	    result += char(0xC0 | w>>6);
	    result += char(0x80 | (w     & 0x3F));
	} else if (w < 0x10000) {
	    result += char(0xE0 | w>>12);
	    result += char(0x80 | (w>>6  & 0x3F));
	    result += char(0x80 | (w     & 0x3F));
	} else if (w < 0x200000) {
	    result += char(0xF0 | w>>18);
	    result += char(0x80 | (w>>12 & 0x3F));
	    result += char(0x80 | (w>>6  & 0x3F));
	    result += char(0x80 | (w     & 0x3F));
	} else if (w < 0x4000000) {
	    result += char(0xF8 | w>>24);
	    result += char(0x80 | (w>>18 & 0x3F));
	    result += char(0x80 | (w>>12 & 0x3F));
	    result += char(0x80 | (w>>6  & 0x3F));
	    result += char(0x80 | (w     & 0x3F));
	} else /* if (w < 0x80000000) */ {
	    result += char(0xFC | w>>30);
	    result += char(0x80 | (w>>24 & 0x3F));
	    result += char(0x80 | (w>>18 & 0x3F));
	    result += char(0x80 | (w>>12 & 0x3F));
	    result += char(0x80 | (w>>6  & 0x3F));
	    result += char(0x80 | (w     & 0x3F));
	}
    }
    return result;
}

const char *const UnicodeOutputConverter::defaultEncoding = "ISO-8859-1";
const char *const UnicodeInputConverter ::defaultEncoding = "ISO-8859-1";

void CharsetConverter::deactivate() {
    if (isConversionActive()) {
	if (iconv_close(iconvHandle_)) {
	    perror("iconv_close");
	    errno = 0;
	}
	iconvHandle_ = (iconv_t) -1;
    }
    ensure(!isConversionActive());
}

void UnicodeInputConverter::setInputEncoding(const char *inputEncoding) {
    deactivate();
    if (!strcmp(inputEncoding, "UTF-8")) return;

    iconvHandle_ = iconv_open("UTF-8", inputEncoding);

    if (iconvHandle_ == (iconv_t) -1 && errno == EINVAL) {
	std::cerr << "Conversion from \"" << inputEncoding << "\" not available. "
	     << "Using default encoding \"" << defaultEncoding << "\" instead." << std::endl;
	iconvHandle_ = iconv_open(defaultEncoding, "UTF-8");
    }
    if (iconvHandle_ == (iconv_t) -1) {
	perror("iconv_open");
	errno = 0;
	std::cerr << "Failed to set up input conversion. Input will treated as UTF-8 encoded." << std::endl;
    }
}

void UnicodeOutputConverter::setOutputEncoding(const char *outputEncoding) {
    deactivate();
    if (!strcmp(outputEncoding, "UTF-8")) return;

    iconvHandle_ = iconv_open(outputEncoding, "UTF-8");

    if (iconvHandle_ == (iconv_t) -1 && errno == EINVAL) {
	std::cerr << "Conversion to \"" << outputEncoding << "\" not available. "
	     << "Using default encoding \"" << defaultEncoding << "\" instead." << std::endl;
	iconvHandle_ = iconv_open(defaultEncoding, "UTF-8");
    }

    if (iconvHandle_ == (iconv_t) -1) {
	perror("iconv_open");
	errno = 0;
	std::cerr << "Failed to set up output conversion. Output will be UTF-8 encoded." << std::endl;
    }
}

template <typename InIterator, typename OutIterator>
OutIterator UnicodeInputConverter::convert(
    const InIterator &begin, const InIterator &end, OutIterator out) {
    if (!isConversionActive()) {
	return std::copy(begin, end, out);
    }

    int errnoBackup = errno;

    size_t todo = end - begin;
    temporary_buffer<const char*, char> buffer(&*begin, &*end);
    for (const char *in = &*begin; in < &*end ;) {
	char *buf = buffer.begin();
	size_t avail = buffer.size();
	/*! \todo Efence finds a buffer overflow here.  Not sure it that's real. */
	size_t nconv = iconv(iconvHandle_, const_cast<char**>(&in), &todo, &buf, &avail);
	out = std::copy(buffer.begin(), buf, out);
	if (nconv == (size_t) -1) {
	    switch (errno) {
	    case E2BIG:
		/* Not an error: The conversion stopped because it ran out
		 * of space in the output buffer.  */
		break;
	    case EINVAL:
		/* The conversion stopped because of an incomplete byte
		 * sequence at the end of the input buffer.
		 *
		 * FIXME: Currently we do not handle this case, since
		 * it requires buffer to be a non-static member.
		 * It is also unclear if this ever occurs. */
		defect();
		break;
	    case EILSEQ:
		/* The conversion stopped because of an invalid byte
		 * sequence in the input.  Non-representable
		 * characters are replaced by question marks. */
		in += 1;
		todo -= 1;
		*out++ = '?';
		nErrors_ += 1;
		break;
	    default:
		perror("iconv");
	    }
	    errno = 0;
	}
    }
    verify(todo == 0);
    errno = errnoBackup;
    return out;
}

template std::back_insert_iterator<std::vector<char> > UnicodeInputConverter::convert(
    const char *const &begin,
    const char *const &end,
    std::back_insert_iterator<std::vector<char> > out);

template std::back_insert_iterator<std::string> UnicodeInputConverter::convert(
    const char *const &begin,
    const char *const &end,
    std::back_insert_iterator<std::string> out);

template <typename InIterator, typename OutIterator>
OutIterator UnicodeOutputConverter::convert(
    const InIterator &begin, const InIterator &end, OutIterator out)
{
    if (!isConversionActive()) {
	return std::copy(begin, end, out);
    }

    int errnoBackup = errno;

    size_t converted = 0;
    size_t todo = end - begin;
    temporary_buffer<const char*, char> buffer(&*begin, &*end);
    for (const char *in = &*begin; in < &*end ;) {
	char *buf = buffer.begin();
	size_t avail = buffer.size();
	/*! \todo Efence finds a buffer overflow here.  Not sure it that's real. */
	size_t nconv = iconv(iconvHandle_, const_cast<char**>(&in), &todo, &buf, &avail);
	out = std::copy(buffer.begin(), buf, out);
	if (nconv != (size_t) -1) {
	    converted += nconv;
	} else {
	    switch (errno) {
	    case E2BIG:
		/* Not an error: The conversion stopped because it ran out
		 * of space in the output buffer.  */
		break;
	    case EINVAL:
		/* The conversion stopped because of an incomplete byte
		 * sequence at the end of the input buffer.
		 *
		 * FIXME: Currently we do not handle this case, since
		 * it requires buffer to be a non-static member.
		 * It is also unclear if this ever occurs. */
		defect();
		break;
	    case EILSEQ:
		/* The conversion stopped because of an invalid byte
		 * sequence in the input.  Non-representable
		 * characters are replaced by question marks. */
		in += 1;
		todo -= 1;
		converted += 1;
		*out++ = '?';
		nErrors_ += 1;
		break;
	    default:
		perror("iconv");
	    }
	    errno = 0;
	}
    }
    verify(todo == 0);
    errno = errnoBackup;
    return out;
}

template std::back_insert_iterator<std::vector<char> > UnicodeOutputConverter::convert(
    const char *const &begin,
    const char *const &end,
    std::back_insert_iterator<std::vector<char> > out);

template std::back_insert_iterator<std::string> UnicodeOutputConverter::convert(
    const char *const &begin,
    const char *const &end,
    std::back_insert_iterator<std::string> out);

template std::ostream_iterator<char> UnicodeOutputConverter::convert(
    const std::string::const_iterator &begin,
    const std::string::const_iterator &end,
    std::ostream_iterator<char> out);

template std::ostream_iterator<char> UnicodeOutputConverter::convert(
    const char *const &begin,
    const char *const &end,
    std::ostream_iterator<char> out);

template std::ostreambuf_iterator<char> UnicodeOutputConverter::convert(
    const char *const &begin,
    const char *const &end,
    std::ostreambuf_iterator<char> out);
