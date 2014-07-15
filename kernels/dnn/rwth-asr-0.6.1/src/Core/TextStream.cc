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
// $Id: TextStream.cc 5439 2005-11-09 11:05:06Z bisani $

#include <fstream>
#include <Core/Assertions.hh>
#include <Core/Types.hh>
#include <Core/Unicode.hh>
#include "TextStream.hh"
#include "FileStream.hh"
#include "Utility.hh"

using namespace Core;

// ===========================================================================
class TextOutputStream::Buffer :
    public std::streambuf
{
private:
    static const u32 defaultBufferThreshold = 512;

    TextOutputStream *master_;
    std::streambuf *output_;
    std::string encoding_;
    u32 indentation_, indentationLevel_, margin_;
    bool isLineBuffered_;
    u32 bufferThreshold_;
    bool isFastPathActive_;
    Core::UnicodeOutputConverter charsetConversion_;
    bool hasFailed_;
    void chooseFastPath();
    std::string pre_;  /**< unformatted */
    std::string pend_; /**< formatted */
    enum { startOfLine, leftMarginOfLine, withinLine } lineState_;
    u32 protection_; /**< number of times "protect" has been seen */
    u32 position_, length_;
    void startNewLine();
    void indentLine();
    void startNewWord();
    void process(char);
    void processFastPath(char);
public:
    Buffer(TextOutputStream *master);
    virtual ~Buffer();
    const std::string &encoding() const { return encoding_; }
    void setOutput(std::streambuf*);
    void setLineBuffered(bool);
    void setOutputBufferSize(u32);
    void setEncoding(const std::string&);
    void setIndentation(u32);
    void setMargin(u32);
    virtual int overflow(int c);
    virtual std::streamsize xsputn(const char*, std::streamsize);
    virtual int sync();
};

TextOutputStream::Buffer::Buffer(TextOutputStream *m) :
    master_(m),
    output_(0),
    indentation_(0), indentationLevel_(0), margin_(0),
    isLineBuffered_(false), bufferThreshold_(defaultBufferThreshold),
    isFastPathActive_(true), hasFailed_(false),
    lineState_(startOfLine), protection_(0), position_(0), length_(0)
{}

TextOutputStream::Buffer::~Buffer() {
    pend_ += pre_; sync();
    if (charsetConversion_.nErrors())
	std::cerr << "warning: Text stream encountered character set conversion errors."
		  << std::endl;
}

void TextOutputStream::Buffer::chooseFastPath() {
    bool canUseFastPath = (indentation_ == 0) && (margin_ == 0);
    if (canUseFastPath && !isFastPathActive_)
	startNewWord();
    isFastPathActive_ = canUseFastPath;
}

void TextOutputStream::Buffer::setOutput(std::streambuf *o) {
    output_ = o;
    hasFailed_ = false;
}

void TextOutputStream::Buffer::setLineBuffered(bool lb) {
    isLineBuffered_ = lb;
}

void TextOutputStream::Buffer::setOutputBufferSize(u32 s) {
    bufferThreshold_ = (s) ? s : defaultBufferThreshold;
}

void TextOutputStream::Buffer::setEncoding(const std::string &encoding) {
    if (encoding == encoding_) return;
    encoding_ = encoding;
    charsetConversion_.setOutputEncoding(encoding_.c_str());
}

void TextOutputStream::Buffer::setIndentation(u32 d) {
    indentation_ = d;
    chooseFastPath();
}

void TextOutputStream::Buffer::setMargin(u32 m) {
    margin_ = m;
    chooseFastPath();
}

int TextOutputStream::Buffer::sync() {
    const char *pend = pend_.data(), *pend_end = pend + pend_.size();
    std::ostreambuf_iterator<char> oi(output_);
    oi = charsetConversion_.convert(pend, pend_end, oi);
    pend_.erase();
    hasFailed_ = hasFailed_ || oi.failed();
    return 0;
}

void TextOutputStream::Buffer::startNewLine() {
    pend_.push_back('\n');
    if (isLineBuffered_) master_->flush();
    lineState_ = startOfLine;
    position_ = 0;
}

void TextOutputStream::Buffer::indentLine() {
    require_(lineState_ == startOfLine);
    pend_.append(indentation_ * indentationLevel_, utf8::blank);
    lineState_ = leftMarginOfLine;
    position_ += indentation_ * indentationLevel_;
}

void TextOutputStream::Buffer::startNewWord() {
    if (pre_.size() == 0) return;
    if (margin_ && (position_ + length_ > margin_)) {
	switch (lineState_) {
	case withinLine: startNewLine();
	case startOfLine: indentLine();
	case leftMarginOfLine: ;
	}
	std::string::size_type i = pre_.find_first_not_of(utf8::whitespace);
	if (i != std::string::npos) {
	    pend_ += pre_.substr(i);
	    lineState_ = withinLine;
	    position_ += length_ - i;
	}
    } else {
	switch (lineState_) {
	case startOfLine: indentLine();
	case leftMarginOfLine: ;
	case withinLine: ;
	}
	pend_ += pre_;
	lineState_ = withinLine;
	position_ += length_;
    }
    pre_.erase();
    length_ = 0;
}

inline void TextOutputStream::Buffer::process(char ch) {
    switch (ch) {
    case indentPlus: {
	++indentationLevel_;
    } break;
    case indentMinus: {
	if (indentationLevel_ > 0) --indentationLevel_;
    } break;
    case describeEncoding: {
	pre_ += std::string(encoding_);
	length_ += encoding_.size();
    } break;
    case protect: {
	++protection_;
    } break;
    case unprotect: {
	if (protection_ > 0) --protection_;
    } break;
    case newWord: {
	startNewWord();
    } break;
    case softBlank:
	if (pre_.size() == 0) break;
	if (pre_[pre_.size()-1] == utf8::blank) break;
	ch = utf8::blank;
	// fall through
    case utf8::blank: {
	if (!protection_) startNewWord();
	pre_.push_back(ch); length_ += 1;
    } break;
    case lineBreak: {
	startNewWord();
	startNewLine();
    } break;
    case newLine: {
	startNewWord();
	if (lineState_ == withinLine) startNewLine();
    } break;
    default: {
	if (lineState_ == startOfLine) indentLine();
	pre_.push_back(ch);
	utf8::ByteType bt = utf8::byteType(ch);
	if (bt == utf8::singleByte || bt == utf8::multiByteHead)
	    ++length_;
    } break;
    }
}

inline void TextOutputStream::Buffer::processFastPath(char ch) {
    switch (ch) {
    case indentPlus:
    case indentMinus:
    case protect:
    case unprotect:
    case newWord:
	break;
    case describeEncoding: {
	pend_ += encoding_;
    } break;
    case lineBreak: {
	startNewLine();
    } break;
    case newLine: {
	if (lineState_ == withinLine) startNewLine();
    } break;
    case softBlank:
	ch = utf8::blank;
	// fall through
    default: {
	pend_.push_back(ch);
	lineState_ = withinLine;
    } break;
    }
}

int TextOutputStream::Buffer::overflow(int c) {
    int result = c;
    if (c != EOF) {
	if (isFastPathActive_)
	    processFastPath(c);
	else
	    process(c);
	if (pend_.size() >= bufferThreshold_) sync();
	if (hasFailed_) result = EOF;
    }
    return result;
}

std::streamsize TextOutputStream::Buffer::xsputn(const char *s, std::streamsize n) {
    if (isFastPathActive_) {
	for (const char *c = s; c < s + n; ++c)
	    processFastPath(*c);
    } else {
	for (const char *c = s; c < s + n; ++c)
	    process(*c);
    }
    if (pend_.size() >= bufferThreshold_) sync();
    return (hasFailed_) ? 0 : n;
}

// ===========================================================================
TextOutputStream::TextOutputStream() :
    std::ostream(new Buffer(this)), output_(0)
{
    buffer_ = dynamic_cast<Buffer*>(rdbuf());
    setEncoding();
}

TextOutputStream::TextOutputStream(std::ostream *s) :
    std::ostream(new Buffer(this)), output_(s)
{
    buffer_ = dynamic_cast<Buffer*>(rdbuf());
    buffer_->setOutput(output_->rdbuf());
    setEncoding();
}

TextOutputStream::TextOutputStream(const std::string &filename, int mode /*= 0*/) :
    std::ostream(new Buffer(this))
{
    buffer_ = dynamic_cast<Buffer*>(rdbuf());
    output_ = new std::fstream(filename.c_str(),(std::ios_base::openmode) mode | std::ios::out);
    buffer_->setOutput(output_->rdbuf());
    setEncoding();
}

void TextOutputStream::adopt(std::ostream *o) {
    delete output_;
    output_ = o;
    buffer_->setOutput(output_->rdbuf());
}

void TextOutputStream::open(const std::string &filename, int mode /* = 0*/) {
    delete output_;
    output_ = new std::fstream(filename.c_str(),(std::ios_base::openmode) mode | std::ios::out);
    buffer_->setOutput(output_->rdbuf());
}

void TextOutputStream::close() {
    buffer_->sync();
    buffer_->setOutput(0);
    delete output_; output_ = 0;
}

TextOutputStream::~TextOutputStream() {
    delete buffer_;
    delete output_;
}

void TextOutputStream::flush() {
    buffer_->sync();
    if (output_) output_->flush();
}

void TextOutputStream::setLineBuffered(bool lb) {
    buffer_->setLineBuffered(lb);
}

void TextOutputStream::setBufferSize(size_t s) {
    buffer_->setOutputBufferSize(s);
}

const std::string &TextOutputStream::encoding() const {
    return buffer_->encoding();
}

void TextOutputStream::setEncoding(const std::string &encoding) {
    buffer_->setEncoding(encoding);
}

void TextOutputStream::setIndentation(u32 d) {
    buffer_->setIndentation(d);
}

void TextOutputStream::setMargin(u32 m) {
    buffer_->setMargin(m);
}

// ===========================================================================
class TextInputStream::Buffer :
    public std::streambuf
{
private:
    static const u32 defaultBufferThreshold = 512;
    static const u32 defaultPutBackThreshold = 4;

    TextInputStream *master_;
    std::streambuf *input_;
    Core::UnicodeInputConverter charsetConversion_;
    std::string encoding_;
    bool hasFailed_;
    std::string formatted_;
    size_t bufferThreshold_;
    size_t putBackThreshold_;
    char *buffer_;
public:
    Buffer(TextInputStream *master);
    virtual ~Buffer();
    const std::string &encoding() const { return encoding_; }
    void setEncoding(const std::string&);
    void setBufferSize(size_t s = 0);
    void setPutBackBufferSize(size_t s = 0);
    void setInput(std::streambuf*);
    virtual int underflow();
    //  virtual int uflow();
    //  virtual std::streamsize xsgetn(char*, std::streamsize);
};

TextInputStream::Buffer::Buffer(TextInputStream *m) :
    master_(m),
    input_(0),
    buffer_(0)
{
    setBufferSize(defaultBufferThreshold);
    setPutBackBufferSize(defaultPutBackThreshold);
}

TextInputStream::Buffer::~Buffer() {
    delete [] buffer_; buffer_ = 0;
    if (charsetConversion_.nErrors())
	std::cerr << "warning: Text stream encountered character set conversion errors."
		  << std::endl;
}

void TextInputStream::Buffer::setInput(std::streambuf *i) {
    input_ = i;
    hasFailed_ = false;
}

void TextInputStream::Buffer::setEncoding(const std::string &encoding) {
    if (encoding == encoding_) return;
    encoding_ = encoding;
    charsetConversion_.setInputEncoding(encoding_.c_str());
}

void TextInputStream::Buffer::setBufferSize(size_t s) {
    bufferThreshold_ = (s) ? s : defaultBufferThreshold;
    delete [] buffer_;
    buffer_ = new char[bufferThreshold_];
}

void TextInputStream::Buffer::setPutBackBufferSize(size_t s) {
    putBackThreshold_ = (s) ? s : defaultPutBackThreshold;
    require(putBackThreshold_ < bufferThreshold_);
}

int TextInputStream::Buffer::underflow() {
    if (gptr() < egptr()) return *gptr();

    // save putback buffer
    size_t putBackBufferSize = gptr() - eback();
    putBackBufferSize = std::min(putBackBufferSize, putBackThreshold_);
    std::copy(gptr() - putBackBufferSize, gptr(), buffer_);

    // refill formatted_, if necessary
    if (formatted_.empty()) {
	std::string unformatted;
	if (master_->getInputLine(unformatted) != EOF)
	    unformatted.push_back('\n');
	else return EOF;
	const char *unformatted_beg = unformatted.data();
	const char *unformatted_end = unformatted_beg + unformatted.size();
	std::back_insert_iterator<std::string> formatted_beg(formatted_);
	charsetConversion_.convert(unformatted_beg, unformatted_end, formatted_beg);
    }

    // refill buffer
    size_t bufferSize = std::min(bufferThreshold_ - putBackBufferSize, formatted_.size());
    formatted_.copy(buffer_ + putBackBufferSize, bufferSize);
    formatted_.erase(formatted_.begin(), formatted_.begin() + bufferSize);
    setg(buffer_, buffer_ + putBackBufferSize, buffer_ + putBackBufferSize + bufferSize);

    return *gptr();
}

//std::streamsize TextInputStream::Buffer::xsgetn(char *s, std::streamsize n) {
//    std::streamsize todo = n;
//    while (todo > 0) {
//	underflow();
//	std::streamsize bufferSize = egptr() - gptr();
//	size_t tocopy = std::min(todo, bufferSize);
//	s = std::copy(gptr(), gptr() + tocopy, s);
//	setg(eback(), gptr() + tocopy, egptr());
//	todo -= tocopy;
//    }
//
//    return n - todo;
//}

// ===========================================================================
TextInputStream::TextInputStream() :
    std::istream(new Buffer(this)), input_(0)
{
    buffer_ = dynamic_cast<Buffer*>(rdbuf());
    setEncoding();
}

TextInputStream::TextInputStream(std::istream *s) :
    std::istream(new Buffer(this)), input_(s)
{
    buffer_ = dynamic_cast<Buffer*>(rdbuf());
    buffer_->setInput(input_->rdbuf());
    setEncoding();
}

TextInputStream::TextInputStream(const std::string &filename, int mode /*= 0*/) :
    std::istream(new Buffer(this))
{
    buffer_ = dynamic_cast<Buffer*>(rdbuf());
    input_ = new std::fstream(filename.c_str(),(std::ios_base::openmode) mode | std::ios::in);
    buffer_->setInput(input_->rdbuf());
    setEncoding();
}

void TextInputStream::adopt(std::istream *i) {
    delete input_;
    input_ = i;
    buffer_->setInput(input_->rdbuf());
}

void TextInputStream::open(const std::string &filename, int mode /* = 0*/) {
    delete input_;
    input_ = new std::fstream(filename.c_str(),(std::ios_base::openmode) mode | std::ios::in);
    buffer_->setInput(input_->rdbuf());
}

void TextInputStream::close() {
    buffer_->setInput(0);
    delete input_; input_ = 0;
}

void TextInputStream::setBufferSize(u32 s) {
    buffer_->setBufferSize(s);
}

void TextInputStream::setPutBackBufferSize(u32 s) {
    buffer_->setPutBackBufferSize(s);
}

TextInputStream::~TextInputStream() {
    delete buffer_;
    delete input_;
}

const std::string &TextInputStream::encoding() const {
    return buffer_->encoding();
}

void TextInputStream::setEncoding(const std::string &encoding) {
    buffer_->setEncoding(encoding);
}

int TextInputStream::getInputLine(std::string &str) const {
    return Core::getline(*input_, str);
}
