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
// $Id: XmlStream.cc 5439 2005-11-09 11:05:06Z bisani $

#include "XmlStream.hh"

using namespace Core;

class XmlWriter::Buffer :
    public std::streambuf
{
private:
    XmlWriter *output_;
public:
    Buffer(XmlWriter *xw) : output_(xw) {}
    virtual int overflow(int c) {
	if (c != EOF) output_->put(std::string(1, char(c)));
	return c;
    }
    virtual std::streamsize xsputn(const char *u, std::streamsize n) {
	output_->put(std::string(u, n));
	return n;
    }
};

XmlWriter::XmlWriter(std::ostream &os) :
    std::ostream(new Buffer(this)),
    os_(os), inComment_(0), formattingHints_(false)
{}

inline void XmlWriter::formattingHint(TextOutputStream::FormattingHint hint) {
    if (shouldGenerateFormattingHints())
	os_.put(hint);
}

void XmlWriter::putEscaped(const std::string &s, const char *escape) {
    std::string::size_type i, j;
    for (i = 0; i < s.size(); i = j+1) {
	j = s.find_first_of(escape, i);
	if (j == std::string::npos) j = s.size();
	os_.write(s.data() + i, j - i);
	if (j >= s.size()) break;
	switch (s[j]) {
	case '&':  os_ << "&amp;"; break;
	case '<':  os_ << "&lt;";  break;
	case '>':  os_ << "&gt;";  break;
	case '"':  os_ << "&quot;"; break;
	case '\'': os_ << "&apos;"; break;
	default:
	    require(utf8::byteType(s[j]) == utf8::singleByte);
	    os_ << "&#" << u32(s[j]) << ";";
	    break;
	}
    }
}

void XmlWriter::put(const std::string &s) {
    if (inComment_) {
	// comment must not contain double-hyphens
	std::string::size_type i, j;
	for (i = 0; i < s.size(); i = j+2) {
	    j = s.find("--",i);
	    if (j == std::string::npos) j = s.size();
	    os_.write(s.data() + i, j - i);
	    if (j >= s.size()) break;
	    os_ << "=";
	}
    } else {
	putEscaped(s, "&<>");
    }
}

void XmlWriter::putDeclaration(const std::string &encoding) {
    os_ << "<?xml version=\"1.0\"";
    if (shouldGenerateFormattingHints() || encoding.size()) {
	os_ << " encoding=\"";
	if (shouldGenerateFormattingHints())
	    os_.put(TextOutputStream::describeEncoding);
	else if (encoding.size())
	    os_ << encoding;
	os_ << "\"";
    }
    os_ << "?>";
    formattingHint(TextOutputStream::newLine);
}

void XmlWriter::putTag(const XmlOpen &open, bool isEmpty) {
    os_ << "<" << open.element_;
    for (std::vector<XmlAttribute>::const_iterator a = open.attributes_.begin(); a != open.attributes_.end(); ++a) {
	os_ << " ";
	formattingHint(TextOutputStream::protect);
	os_ << a->name_ << "=\"";
	putEscaped(a->value_, "&<>\"'");
	os_ << "\"";
	formattingHint(TextOutputStream::unprotect);
    }
    if (isEmpty) os_ << "/";
    os_ << ">";
}

void XmlWriter::putClosingTag(const XmlClose &close) {
    os_ << "</" << close.element_ << ">";
}

void XmlWriter::put(const XmlOpen &open) {
    formattingHint(TextOutputStream::newLine);
    putTag(open, false);
    formattingHint(TextOutputStream::newWord);
    formattingHint(TextOutputStream::indentPlus);
    formattingHint(TextOutputStream::newLine);
}

void XmlWriter::put(const XmlClose &close) {
    formattingHint(TextOutputStream::newWord);
    formattingHint(TextOutputStream::indentMinus);
    formattingHint(TextOutputStream::newLine);
    putClosingTag(close);
    formattingHint(TextOutputStream::newLine);
}

void XmlWriter::put(const XmlEmpty &empty) {
    formattingHint(TextOutputStream::newLine);
    putTag(empty, true);
    formattingHint(TextOutputStream::newLine);
}

void XmlWriter::put(const XmlFull &element) {
    formattingHint(TextOutputStream::newLine);
    putTag(element, false);
    formattingHint(TextOutputStream::newWord);
    formattingHint(TextOutputStream::indentPlus);
    put(element.content_);
    formattingHint(TextOutputStream::newWord);
    formattingHint(TextOutputStream::indentMinus);
    putClosingTag(element);
    formattingHint(TextOutputStream::newLine);
}

void XmlWriter::put(const XmlComment &comment) {
    if (inComment_) {
	put(comment.text_);
    } else {
	++inComment_;
	os_ << "<!-- ";
	put(comment.text_);
	os_ << " -->";
	--inComment_;
    }
}

void XmlWriter::put(const XmlOpenComment&) {
    if (!inComment_) os_ << "<!-- ";
    ++inComment_;
}

void XmlWriter::put(const XmlCloseComment&) {
    require(inComment_ >= 1);
    --inComment_;
    if (!inComment_) os_ << " -->";
}

void XmlWriter::put(const XmlBlank&) {
    if (shouldGenerateFormattingHints())
	os_.put(TextOutputStream::softBlank);
    else
	os_.put(utf8::blank);
}

XmlWriter::~XmlWriter() {
    delete rdbuf(0);
}

// ===========================================================================
XmlOutputStream::XmlOutputStream() :
    XmlWriter(textOutputStream_)
{
    generateFormattingHints(true);
}

XmlOutputStream::XmlOutputStream(std::ostream *os) :
    XmlWriter(textOutputStream_),
    textOutputStream_(os)
{
    generateFormattingHints(true);
    putDeclaration();
}

XmlOutputStream::XmlOutputStream(const std::string &filename, int mode) :
    XmlWriter(textOutputStream_),
    textOutputStream_(filename, mode)
{
    generateFormattingHints(true);
    putDeclaration();
}

XmlOutputStream::~XmlOutputStream() {}

void XmlOutputStream::open(const std::string &filename, int mode) {
    textOutputStream_.open(filename, mode);
    putDeclaration();
}

void XmlOutputStream::putDeclaration() {
    XmlWriter::putDeclaration(encoding());
}
