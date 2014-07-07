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
// $Id: Channel.cc 8249 2011-05-06 11:57:02Z rybach $

#include "Channel.hh"

#include <fstream>
#include <iostream>
#include <cerrno>
#include <unistd.h>
#include <Core/Parameter.hh>
#include <Core/TextStream.hh>
#include <Core/FileStream.hh>
#include <Core/CompressedStream.hh>

using namespace Core;

// ===========================================================================
class Channel::Target :
	public Core::TextOutputStream,
	public Core::Configurable,
	public Core::Mutex
{
	public:
		static const Core::ParameterString paramFilename;
		static const Core::ParameterBool   paramBuffering;
		static const Core::ParameterBool   paramAppend;
		static const Core::ParameterBool   paramShouldPutDeclaration;
		static const Core::ParameterString paramEncoding;
		static const Core::ParameterInt    paramMargin;
		static const Core::ParameterInt    paramIndentation;
		static const Core::ParameterBool   paramCompressed;
		static const Core::ParameterBool   paramAddSprintTags;
	private:
		void open(const std::string&);
		void setup();
		bool isTty_, shouldBeLineBuffered_;

		XmlWriter xml_;
		bool isXmlDocument_;
		std::streambuf *defaultStreamBuf_;

	public:
		Target(const Core::Configuration&, bool isXmlDocument, std::ostream *os = 0);
		Target(const Core::Configuration&, bool isXmlDocument, const std::string &defaultFilename, std::streambuf *defaultStreamBuf = 0);
		virtual ~Target();
		bool isTty() const { return isTty_; }
		void block(u32 bufferLimit);
		void unblock();
};

const Core::ParameterString Channel::Target::paramFilename(
		"file",
		"name of output file");
const Core::ParameterBool Channel::Target::paramBuffering(
		"unbuffered",
		"switch to line-buffered mode (useful for watching with tail -f)",
		false /* For TTY devices the default is true. */);
const Core::ParameterBool Channel::Target::paramAppend(
		"append",
		"append to file instead of overwriting",
		false);
const Core::ParameterBool Channel::Target::paramShouldPutDeclaration(
		"put-declaration",
		"should put declaration to the beginning of the document.",
		true);
const Core::ParameterString Channel::Target::paramEncoding(
		"encoding",
		"output character set encoding",
		Core::defaultEncoding,
		"Specify the output character set encoding of the channel. "
		"(Type 'iconv --list' to get a list of available encodings.)");
const Core::ParameterInt Core::Channel::Target::paramMargin(
		"margin",
		"maximum line length",
		0, 0);
const Core::ParameterInt Core::Channel::Target::paramIndentation(
		"indentation",
		"number of blanks for indenting output",
		2, 0);
const Core::ParameterBool Channel::Target::paramCompressed(
		"compressed",
		"write to gzip compressed file",
		false);
const Core::ParameterBool Channel::Target::paramAddSprintTags(
		"add-sprint-tags",
		"write <sprint> tags into channel",
		true);

Channel::Target::Target(const Core::Configuration &c, bool isXmlDocument, const std::string &defaultFilename, std::streambuf *defaultStreamBuf) :
	Core::Configurable(c), isTty_(false),
	xml_(*this), isXmlDocument_(isXmlDocument),
	defaultStreamBuf_(defaultStreamBuf)
{
	open(paramFilename(config, defaultFilename));
	setup();
}

Channel::Target::Target(const Core::Configuration &c, bool isXmlDocument, std::ostream *defaultStream) :
	Core::Configurable(c), isTty_(false),
	xml_(*this), isXmlDocument_(isXmlDocument),
	defaultStreamBuf_(defaultStream->rdbuf())
{
	require(defaultStream);
	std::string filename = paramFilename(config);
	if (filename.size()) {
		delete defaultStream;
		open(filename);
	} else {
		adopt(defaultStream);
		std::filebuf *fb = dynamic_cast<std::filebuf*>(defaultStream->rdbuf());
		if (fb) {
			FileStreamBuffer *fsb = static_cast<FileStreamBuffer*>(fb);
			if (isatty(fsb->fd())) {
				isTty_ = true;
			}
		}
	}
	setup();
}

void Channel::Target::open(const std::string &filename) {
	int mode = (paramAppend(config)) ? std::ios::app : std::ios::out;
	if (paramCompressed(config)) {
		std::string gzFilename = filename;
		if (gzFilename.rfind(".gz") != gzFilename.length() - 3)
			gzFilename += ".gz";

		if (paramAppend(config)) {
			std::cerr << "channel warning: Target \"" << fullName()
				<< "\" cannot append to compressed file" << std::endl;
		}
		adopt(new CompressedOutputStream(gzFilename));
	} else {
		TextOutputStream::open(filename, mode);
	}
	if (!good()) {
		std::cerr << "channel error: Target \"" << fullName()
			<< "\" failed to open file \"" << filename << "\" for writing.";
		if (errno) {
			std::cerr <<  " (" << strerror(errno) << ")";
			errno = 0;
		}
		std::cerr << std::endl;
		//adopt(new std::ostream(std::cerr.rdbuf()));
		if(defaultStreamBuf_){
			adopt(new std::ostream(defaultStreamBuf_));
		} else {
			std::cerr << "Target " << fullName() << " could not be redirected: no default stream buffer given." << std::endl;
			defect();
		}
	}
}

void Channel::Target::setup() {
	shouldBeLineBuffered_ = paramBuffering(config, isTty());
	setLineBuffered(shouldBeLineBuffered_);
	setEncoding(paramEncoding(config).c_str());
	setMargin(paramMargin(config));
	setIndentation(paramIndentation(config));

	if (isTty()) isXmlDocument_ = false;
	if (isXmlDocument_) {
		xml_.generateFormattingHints();
		if (paramShouldPutDeclaration(config))
			xml_.putDeclaration();
		if (paramAddSprintTags(config))
			xml_ << XmlOpen("sprint");
	}
}

Channel::Target::~Target() {
	if (isXmlDocument_ && paramAddSprintTags(config)) {
		xml_ << XmlClose("sprint");
	}
}

void Channel::Target::block(u32 bufferLimit) {
	setLineBuffered(false);
	setBufferSize(bufferLimit);
}

void Channel::Target::unblock() {
	setLineBuffered(shouldBeLineBuffered_);
	setBufferSize(0);
}

// ===========================================================================
class Channel::Dispatcher :
	public std::streambuf
{
	private:
		std::vector<Channel::Target*> targets_;
	protected:
		virtual int overflow(int c);
		virtual std::streamsize xsputn(const char *s, std::streamsize num);
	public:
		Dispatcher() {}
		virtual ~Dispatcher();
		void addTarget(Target *t) { targets_.push_back(t); }
};

Channel::Dispatcher::~Dispatcher() {

}

int Channel::Dispatcher::overflow(int c) {
	int result = c;
	if (c != EOF) {
		for (std::vector<Target*>::iterator it = targets_.begin(); it != targets_.end(); it++) {
			Target *t(*it);
			t->lock();
			int w = t->rdbuf()->sputc(c);
			if (w != c) result = w;
			t->release();
		}
	}
	return result;
}

std::streamsize Channel::Dispatcher::xsputn(const char *s, std::streamsize num) {
	std::streamsize min = num;
	for (std::vector<Target*>::iterator it = targets_.begin(); it != targets_.end(); it++) {
		Target *t(*it);
		t->lock();
		std::streamsize n = t->rdbuf()->sputn(s, num);
		if (n < min) min = n;
		t->release();
	}
	return min;
}

// ===========================================================================
const Core::ParameterInt Channel::Manager::paramBlockedTargetBufferLimit(
		"blocked-buffer-limit",
		"maximum number of bytes to be held back in blocked channel targets",
		64*1024, 0);

Channel::Manager *Channel::Manager::singleton_ = 0;

Channel::Manager::Manager(const Core::Configuration &c, bool outputXmlHeader) :
	Core::Configurable(c)
{
	require(!singleton_);
	lock();
	singleton_ = this;

	blockedTargetBufferLimit_ = paramBlockedTargetBufferLimit(config);

	// create default channels
	Target *stdoutTarget = new Target(select("stdout"), outputXmlHeader, new std::ostream(std::cout.rdbuf()));
	Target *stderrTarget = new Target(select("stderr"), outputXmlHeader, new std::ostream(std::cerr.rdbuf()));
	targets_["stdout"] = stdoutTarget;
	targets_["stderr"] = stderrTarget;
	targets_["nil"] = 0;

	if (stdoutTarget->isTty()) ttyTargets_.push_back(stdoutTarget);
	if (stderrTarget->isTty()) ttyTargets_.push_back(stderrTarget);

	// redirect cout and cerr
	// Hope this won't encourage people to use cout/cerr where they shouldn't
	originalStreamBuffers[0] = std::cout.rdbuf(stdoutTarget->rdbuf());
	originalStreamBuffers[1] = std::cerr.rdbuf(stderrTarget->rdbuf());
	originalStreamBuffers[2] = std::clog.rdbuf(stderrTarget->rdbuf());

	release();
}

Channel::Manager *Channel::Manager::us() {
	return singleton_;
}

Channel::Manager::~Manager() {
	lock();

	// restore cout and cerr
	std::cout.rdbuf(originalStreamBuffers[0]);
	std::cerr.rdbuf(originalStreamBuffers[1]);
	std::clog.rdbuf(originalStreamBuffers[2]);

	flushAll();
	for (TargetMap::iterator t = targets_.begin() ; t != targets_.end() ; ++t)
		delete t->second;

	singleton_ = 0;
	release();
}

void Channel::Manager::flushAll() {
	lock();
	for (TargetMap::const_iterator t = targets_.begin() ; t != targets_.end() ; ++t)
		if (t->second) t->second->flush();
	release();
}

void Channel::Manager::flushTty() {
	lock();
	for (TargetList::const_iterator t = ttyTargets_.begin() ; t != ttyTargets_.end() ; ++t)
		(*t)->flush();
	release();
}

void Channel::Manager::blockTty() {
	lock();
	for (TargetList::const_iterator t = ttyTargets_.begin() ; t != ttyTargets_.end() ; ++t) {
		(*t)->flush();
		(*t)->block(blockedTargetBufferLimit_);
	}
	release();
}

void Channel::Manager::unblockTty() {
	lock();
	for (TargetList::const_iterator t = ttyTargets_.begin() ; t != ttyTargets_.end() ; ++t)
		(*t)->unblock();
	release();
}

Channel::Target *Channel::Manager::createTarget(
		TargetType type, const Core::Configuration &c, const std::string &fn)
{
	Target *result = 0;
	lock();
	switch (type) {
		case plainTarget:
			result = new Target(c, false, fn, originalStreamBuffers[1]);
			break;
		case xmlTarget:
			result = new Target(c, true, fn, originalStreamBuffers[1]);
			break;
		default: defect();
	}
	if (result->isTty()) ttyTargets_.push_back(result);
	release();
	return result;
}

Channel::Target *Channel::Manager::get(TargetType type, const std::string &name) {
	Target *result = 0;
	lock();
	TargetMap::iterator found = targets_.find(name);
	if (found == targets_.end()) {
		std::string::size_type i = name.find_last_of(".");
		std::string configName;
		if (i == std::string::npos)
			configName = name;
		else
			configName = name.substr(i+1);
		targets_[name] = result = createTarget(type, select(configName), name);
	} else {
		result = found->second;
	}
	release();
	return result;
}

// ===========================================================================
const Core::ParameterString Core::Channel::paramTargets(
		"channel",
		"comma separated list of channel targets (\"stdout\", \"stderr\", \"nil\" or filename)",
		"stdout");

Core::Channel::Channel(
		const Configuration &c,
		const std::string &name,
		Default defaultTarget,
		TargetType type) :
	std::ostream(0),
	isOpen_(false)
{
	Configuration config(c, name); // <- Do NOT take this as an example!

	if (!Manager::us()) { // fallback when Channel::Manager is not present
		/*! \todo should prevent deletion in destructor. */
		std::cerr << "channel warning: Creating Channel "
			<< config.getSelection()
			<< " before Manager is active." << std::endl;
		std::streambuf *buffer = 0;
		switch (defaultTarget) {
			case standard: buffer = std::cout.rdbuf(); break;
			case error:    buffer = std::cerr.rdbuf(); break;
			default:       buffer = 0;
		}
		if (buffer) {
			isOpen_ = true;
			delete rdbuf(buffer);
		}
		return;
	}

	const char *defaultTargetSpec = 0;
	switch (defaultTarget) {
		case disabled: defaultTargetSpec = "nil";    break;
		case standard: defaultTargetSpec = "stdout"; break;
		case error:    defaultTargetSpec = "stderr"; break;
		default: defect();
	}
	std::string targetSpec = paramTargets(config, defaultTargetSpec);

	Dispatcher *buffer = new Dispatcher();
	std::set<std::string> added;
	std::string token;
	std::istringstream iss(targetSpec);
	while ((wsgetline(iss, token, ", ") != EOF)) {
		if (token != "nil" && added.find(token) == added.end()) {
			Target *os = Manager::us()->get(type, token);
			if (os) {
				buffer->addTarget(os);
				isOpen_ = true;
			}
			added.insert(token);
		}
	}
	delete rdbuf(buffer);
}

Channel::~Channel() {
	delete rdbuf(0);
}

Core::XmlChannel::XmlChannel(
		const Configuration &c,
		const std::string &name,
		Channel::Default defaultTarget) :
	XmlWriter(channel_),
	channel_(c, name, defaultTarget, Channel::xmlTarget)
{
	generateFormattingHints();
}

Core::XmlChannel::~XmlChannel() {}
