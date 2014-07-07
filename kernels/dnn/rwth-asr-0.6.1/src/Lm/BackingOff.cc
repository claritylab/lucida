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
#define BackingOffInternalFriend BackingOffLm::Internal;

#include "BackingOff.hh"
#include "BackingOffInternal.hh"
#include <Bliss/SyntacticTokenMap.hh>
#include <Core/Directory.hh>
#include <Core/IoUtilities.hh>
#include <Fsa/Sort.hh>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

using namespace Lm;

// ===========================================================================
// internal data structures

/*
 * Subtle issue in this code:
 *
 * After finalize() has been called, an extra sentinel node is
 * placed at the end of the node array.  It is placed *at* (not
 * before) nodesTail_, and it is not counted in nNodes().
 *
 * After finalize() has been called, parent_ and firstChild_ are
 * relative to the current node index.  Node::parent() and
 * Node::fildChild() may only be called after finalize().
 *
 * When Node::parent_ points to the node itself, this indicates that
 * is has no parent.  This is the case for only two nodes: the root
 * node and the sentinel node.  Since parent() is never called for the
 * sentinel node, we test for base == this instead.
 *
 * After finalize() has been called, an extra sentinel word score is
 * placed at the end of the word score array.  It is placed *at* (not
 * before) wordScoresTail_, and it is not counted in nWordScores().
 *
 *
 */

using namespace BackingOffPrivate;

const BackingOffLm::Node *BackingOffLm::Node::findChild(TokenIndex t) const {
    return binarySearch(childrenBegin(), childrenEnd() - 1, t);
}

const BackingOffLm::WordScore *BackingOffLm::Node::findWordScore(
    const WordScore *base, TokenIndex t) const
{
    return binarySearch(scoresBegin(base), scoresEnd(base) - 1, t);
}

struct BackingOffLm::Internal::InitItemOrdering {
    bool operator() (const InitItem &a, const InitItem &b) const {
	if (a.history[0] && b.history[0])
	    return (a.history[0]->id() < b.history[0]->id());
	if (!a.history[0] && !b.history[0]) {
	    if (a.token && b.token)
		return (a.token->id() < b.token->id());
	    return (!a.token);
	}
	return (!a.history[0]);
    }
};

BackingOffLm::Internal::Internal() {
    nodes_ = nodesTail_ = nodesEnd_ = NULL;
    wordScores_ = wordScoresTail_ = wordScoresEnd_ = NULL;
    mmap_ = NULL;
}

void BackingOffLm::Internal::changeNodeCapacity(NodeIndex newCapacity) {
    require(!isMapped());
    require(newCapacity >= nNodes());
    size_t nNodes = nodesTail_ - nodes_;
    Node *newNodes =  (Node*) realloc(nodes_, newCapacity * sizeof(Node));
    hope(newNodes != NULL /* memory allocation failed */);
    nodes_     = newNodes;
    nodesTail_ = nodes_ + nNodes;
    nodesEnd_  = nodes_ + newCapacity;
}

BackingOffLm::Node *BackingOffLm::Internal::newNode() {
    size_t spareCapacity = nodesEnd_ - nodesTail_;
    if (spareCapacity < 1) {
	spareCapacity = nNodes();
	if (spareCapacity < 1<<12) spareCapacity = 1<<12;
	if (spareCapacity > 1<<22) spareCapacity = 1<<22;
	changeNodeCapacity(nNodes() + spareCapacity);
    }
    ensure_(nodesTail_ < nodesEnd_);
    return nodesTail_++;
}

void BackingOffLm::Internal::changeWordScoreCapacity(
    WordScoreIndex newCapacity)
{
    require(!isMapped());
    require(newCapacity >= nWordScores());
    size_t nWordScores = wordScoresTail_ - wordScores_;
    WordScore *newWordScores =  (WordScore*) realloc(
	wordScores_, newCapacity * sizeof(WordScore));
    hope(newWordScores != NULL /* memory allocation failed */);
    wordScores_     = newWordScores;
    wordScoresTail_ = wordScores_ + nWordScores;
    wordScoresEnd_  = wordScores_ + newCapacity;
}

BackingOffLm::WordScore *BackingOffLm::Internal::newWordScore() {
    size_t spareCapacity = wordScoresEnd_ - wordScoresTail_;
    if (spareCapacity < 1) {
	spareCapacity = nWordScores();
	if (spareCapacity < 1<<14) spareCapacity = 1<<14;
	if (spareCapacity > 1<<24) spareCapacity = 1<<24;
	changeWordScoreCapacity(nWordScores() + spareCapacity);
    }
    ensure_(wordScoresTail_ < wordScoresEnd_);
    return wordScoresTail_++;
}

void BackingOffLm::Internal::reserve(
    NodeIndex nNodesNeeded,
    WordScoreIndex nWordScoresNeeded)
{
    require(nNodesNeeded >= nNodes());
    require(nWordScoresNeeded >= nWordScores());
    changeNodeCapacity(nNodesNeeded);
    changeWordScoreCapacity(nWordScoresNeeded);
};

BackingOffLm::Internal::~Internal() {
    if (isMapped()) {
	munmap(mmap_, mmapSize_);
    } else {
	::free(nodes_);
	::free(wordScores_);
    }
}

void BackingOffLm::Internal::mapToken(TokenIndex ti, Token t) {
    if (ti >= TokenIndex(tokens_.size())) tokens_.resize(ti+1, 0);
    tokens_[ti] = t;
}

void BackingOffLm::Internal::makeRoot() {
    Node *root = newNode();
    root->token_        = -1;
    root->backOffScore_ = Core::Type<Score>::max;
    root->depth_        = 0;
    root->parent_       = 0;
}

void BackingOffLm::Internal::build(InitItem *begin, InitItem *end) {
    init_ = new std::vector<InitItemRange>;
    makeRoot();
    InitItemRange init;
    init.begin = begin;
    init.end   = end;
    init_->push_back(init);
    for (NodeIndex n = 0; n < nNodes(); ++n)
	buildNode(n);
    finalize();
    delete init_; init_ = 0;
}

void BackingOffLm::Internal::finalize() {
    // create sentinel node
    size_t spareCapacity = nodesEnd_ - nodesTail_;
    if ((spareCapacity < 1) || (spareCapacity > nNodes() / 16))
	changeNodeCapacity(nNodes() + 1);
    Node *sentinel = nodesTail_;
    verify(sentinel < nodesEnd_);
    sentinel->firstChild_     = nNodes();
    sentinel->firstWordScore_ = nWordScores();
    sentinel->token_        = -1;  // phony
    sentinel->backOffScore_ = 0.0; // phony
    sentinel->depth_        = 0;   // phony
    sentinel->parent_       = nNodes(); // phony

    // create sentinel word score
    spareCapacity = wordScoresEnd_ - wordScoresTail_;
    if ((spareCapacity < 1) || (spareCapacity > nWordScores() / 16))
	changeWordScoreCapacity(nWordScores() + 1);
    WordScore *wsSentinel = wordScoresTail_;
    verify(wsSentinel < wordScoresEnd_);
    wsSentinel->token_ = -1;  // phony
    wsSentinel->score_ = 0.0; // phony

    // make parent and child numbers relative
    for (NodeIndex ni = 0; ni <= nNodes(); ++ni) {
	nodes_[ni].parent_      = ni - nodes_[ni].parent_;
	nodes_[ni].firstChild_ -= ni;
    }
}

void BackingOffLm::Internal::buildNode(NodeIndex ni) {
    Node *n = &nodes_[ni];
    InitItem *i = (*init_)[ni].begin, *end = (*init_)[ni].end;

    std::sort(i, end, InitItemOrdering());

    n->firstWordScore_ = nWordScores();
    for (; i < end && i->history[0] == 0; ++i) {
	if (i->token) {
	    WordScore *ws = newWordScore();
	    ws->token_ = i->token->id();
	    ws->score_ = i->score;
	} else {
	    n->backOffScore_ = i->score;
	}
    }

    n->firstChild_ = nNodes();
    Node::Depth d = n->depth_ + 1;
    InitItemRange init;
    for (; i < end ;) {
	verify(i->history[0]);
	Node *nn = newNode(); // CAVEAT: invalidates n
	nn->parent_       = ni;
	nn->depth_        = d;
	nn->token_        = (*i->history++)->id();
	nn->backOffScore_ = 0.0;
	init.begin        = i++;
	while (i < end && (*i->history)->id() == nn->token_) { i->history++; ++i; }
	init.end          = i;
	verify(nodeIndex(nn) == init_->size());
	init_->push_back(init);
    }
}

/**
 * Write back-off tree as DOT file for diagnostic purposes.
 */
void BackingOffLm::Internal::draw(std::ostream &os, const std::string &title) const {
    os << "digraph \"" << title << "\" {" << std::endl
       << "ranksep = 1.5" << std::endl
       << "rankdir = LR" << std::endl
       << "node [fontname=\"Helvetica\"]" << std::endl
       << "edge [fontname=\"Helvetica\"]" << std::endl;

    for (const Node *n = nodes_; n != nodesTail_; ++n) {
	Token tok = token(n->token());
	os << Core::form("n%p [shape=square label=\"%d\\n%s\\nbo=%f",
			 (void*) n,
			 nodeIndex(n),
			 (tok) ? tok->symbol().str() : "(null)",
			 n->backOffScore());
	for (const WordScore *ws = scoresBegin(n); ws != scoresEnd(n); ++ws)
	    os << Core::form("\\n%s: %f", token(ws->token())->symbol().str(), ws->score());
	os << "\"]\n";
	if (n->parent())
	    os << Core::form("n%p -> n%p\n", (void*) n, (void*) n->parent());
    }
    os << "}" << std::endl;
}

// ---------------------------------------------------------------------------
// Memory-mapped image

/*
  Image file structure

    0            ... HS-1  header
    HS           ...       info string (zero-terminated)
    tokensOffset ...       token string table
    nodesOffset  ...	   nodes       (including sentinel)
    scoresOffset ...	   word scores (including sentinel)
    end

  HS = sizeof(header)

*/

namespace BackingOffPrivate {

class ImageHeader {
protected:
    struct Prefix {
	char magicWord[8];
	u32  endianessMark;
	u32  versionMark;
	static const char *magic;
	static const u32 endianess;
    };
    ImageHeader() : data_(0) {}
private:
    ImageHeader* createData(int version) const;
    ImageHeader *data_;

public:
    explicit ImageHeader(int version) : data_(createData(version)) {}
    virtual ~ImageHeader() { delete data_; }
    virtual u64 nTokens() const { return data_->nTokens(); }
    virtual u64 nNodes() const { return data_->nNodes(); }
    virtual u64 nWordScores() const { return data_->nWordScores(); }
    virtual u64 tokensOffset() const { return data_->tokensOffset(); }
    virtual u64 nodesOffset() const { return data_->nodesOffset(); }
    virtual u64 scoresOffset() const { return data_->scoresOffset(); }
    virtual u64 end() const { return data_->end(); }
    virtual void setTokens(u64 n) { data_->setTokens(n); }
    virtual void setNodes(u64 n) { data_->setNodes(n); }
    virtual void setWordScores(u64 n) { data_->setWordScores(n); }
    virtual void setTokensOffset(u64 p) { data_->setTokensOffset(p); }
    virtual void setNodesOffset(u64 p) { data_->setNodesOffset(p); }
    virtual void setScoresOffset(u64 p) { data_->setScoresOffset(p); }
    virtual void setEnd(u64 p) { data_->setEnd(p); }
    virtual bool read(int fd, std::string &info);
    virtual bool write(int fd) const;
    virtual size_t size() const { return sizeof(Prefix) + data_->size(); }
    virtual u32 version() const { return data_->version(); }
};

template<class T, int v>
class ImageHeaderImpl : public ImageHeader {
private:
    typedef T CountType;
    typedef T PositionType;
    struct Values {
	CountType nTokens;
	CountType nNodes;          // excluding sentinel
	CountType nWordScores;     // excluding sentinel
	PositionType tokensOffset;
	PositionType nodesOffset;
	PositionType scoresOffset;
	PositionType end;
    };
    Values values_;
    enum { formatVersion = v };
public:
    virtual u64 nTokens() const { return values_.nTokens; }
    virtual u64 nNodes() const { return values_.nNodes; }
    virtual u64 nWordScores() const { return values_.nWordScores; }
    virtual u64 tokensOffset() const { return values_.tokensOffset; }
    virtual u64 nodesOffset() const { return values_.nodesOffset; }
    virtual u64 scoresOffset() const { return values_.scoresOffset; }
    virtual u64 end() const { return values_.end; }
    virtual void setTokens(u64 n) { values_.nTokens = n; }
    virtual void setNodes(u64 n) { values_.nNodes = n; }
    virtual void setWordScores(u64 n) { values_.nWordScores = n; }
    virtual void setTokensOffset(u64 p) { values_.tokensOffset = p; }
    virtual void setNodesOffset(u64 p) { values_.nodesOffset = p; }
    virtual void setScoresOffset(u64 p) { values_.scoresOffset = p; }
    virtual void setEnd(u64 p) { values_.end = p; }
    virtual bool read(int fd, std::string &info);
    virtual bool write(int fd) const;
    virtual size_t size() const { return sizeof(values_); }
    virtual u32 version() const { return formatVersion; }
};

const char *ImageHeader::Prefix::magic = "MB020205";
const u32 ImageHeader::Prefix::endianess = 0x11223344;

bool ImageHeader::read(int fd, std::string &info)
{
    Prefix prefix;
    ssize_t nBytes = sizeof(prefix);
    if (::read(fd, &prefix, nBytes) != nBytes) {
	info = "failed to read image header";
	return false;
    }
    if (memcmp(prefix.magicWord, Prefix::magic, 8)) {
	info = "bad magic word in image header";
	return false;
    }
    if (prefix.endianessMark != Prefix::endianess) {
	info = "image has incompatible byte order";
	return false;
    }
    delete data_;
    data_ = createData(prefix.versionMark);
    if (!data_) {
	info = Core::form("image has unknown version %d", prefix.versionMark);
	return false;
    }
    return data_->read(fd, info);
}

bool ImageHeader::write(int fd) const
{
    Prefix prefix;
    ::memcpy(prefix.magicWord, Prefix::magic, 8);
    prefix.endianessMark = Prefix::endianess;
    prefix.versionMark = version();
    ssize_t nBytes = sizeof(prefix);
    if (::write(fd, &prefix, nBytes) != nBytes)
	return false;
    return data_->write(fd);
}

template<class T, int v>
bool ImageHeaderImpl<T, v>::read(int fd, std::string &info)
{
    ssize_t nBytes = sizeof(values_);
    if (::read(fd, &values_, nBytes) != nBytes) {
	info = Core::form("failed to read image header (version=%d)",
		version());
	return false;
    }
    if (values_.tokensOffset == 0) {
	info = "image seems to be incomplete";
	return false;
    }
    return true;
}

template<class T, int v>
bool ImageHeaderImpl<T, v>::write(int fd) const
{
    ssize_t nBytes = sizeof(values_);
    return (::write(fd, &values_, nBytes) == nBytes);
}

ImageHeader* ImageHeader::createData(int version) const
{
    // When changing the file format, increment the version number!
    // Backward compatibility of image files is typically not needed.
    // The next unused number is: 4

    typedef ImageHeaderImpl<u32, 1> ImageHeaderV1;
    typedef ImageHeaderImpl<u64, 3> ImageHeaderV3;
    ImageHeader *data = 0;
    switch(version) {
    case 1:
	data = new ImageHeaderV1();
	break;
    case 3:
	data = new ImageHeaderV3();
	break;
    }
    return data;
}


} // namespace BackingOffPrivate


bool BackingOffLm::Internal::writeImageTokenTable(int fd) const {
    char *text, *textEnd, *textTail;
    const char *str;
    ssize_t len = 0;
    str = text = textEnd = 0;
    for (u32 i = 0; i < tokens_.size();) {
	if (!str) {
	    str = (tokens_[i]) ? tokens_[i]->symbol().str() : "";
	    len = strlen(str);
	}
	ssize_t bufferSize = (len > 1<<12) ? len : 1<<12;
	if (textEnd - text < bufferSize) {
	    text = (char*) realloc(text, bufferSize);
	    textEnd = text + bufferSize;
	}
	textTail = text;

	while ((textEnd - textTail) >= (len + 1)) {
	    strcpy(textTail, str);
	    textTail += len + 1;
	    if (++i >= tokens_.size()) break;
	    str = (tokens_[i]) ? tokens_[i]->symbol().str() : "";
	    len = strlen(str);
	}

	if (write(fd, text, textTail - text) != textTail - text) {
	    ::free(text);
	    return false;
	}
    }
    ::free(text);
    return true;
}

/**
 *
 * @return true if successful
 */
bool BackingOffLm::Internal::writeImage(int fd, const std::string &info) const {
    off_t position, pad;
    ssize_t nBytes;
    const u32 fileFormatVersion = 3;
    ImageHeader header(fileFormatVersion);
    header.setTokensOffset(0); // phony
    header.setTokens(tokens_.size());
    header.setNodesOffset(0); // phony
    header.setNodes(nNodes());
    header.setScoresOffset(0); // phony
    header.setWordScores(nWordScores());
    header.setEnd(0); // phony

    // write phony header
    if (!header.write(fd))
	return false;

    // write info
    nBytes = info.size() + 1;
    if (write(fd, info.c_str(), nBytes) != nBytes)
	return false;

    // write token strings
    if ((position = lseek(fd, 0, SEEK_CUR)) == (off_t) -1)
	return false;
    header.setTokensOffset(position);
    if (!writeImageTokenTable(fd)) return false;

    // write nodes
    if ((position = lseek(fd, 0, SEEK_CUR)) == (off_t) -1)
	return false;

    pad = (position + 7) % 8;
    if ((position = lseek(fd, pad, SEEK_CUR)) == (off_t) -1)
	return false;
    header.setNodesOffset(position);
    u64 nNodeBytes = (nodesTail_ - nodes_ + 1) * sizeof(Node);
    if (!Core::writeLargeBlock(fd, reinterpret_cast<const char*>(nodes_), nNodeBytes))
	return false;

    // write word scores
    if ((position = lseek(fd, 0, SEEK_CUR)) == (off_t) -1)
	return false;
    pad = (position + 7) % 8;
    if ((position = lseek(fd, pad, SEEK_CUR)) == (off_t) -1)
	return false;
    header.setScoresOffset(position);
    u64 nScoreBytes = (wordScoresTail_ - wordScores_ + 1) * sizeof(WordScore);
    if (!Core::writeLargeBlock(fd, reinterpret_cast<const char*>(wordScores_), nScoreBytes))
	return false;

    // determine file size
    if ((position = lseek(fd, 0, SEEK_CUR)) == (off_t) -1)
	return false;
    header.setEnd(position);

    // write header
    if ((position = lseek(fd, 0, SEEK_SET)) == (off_t) -1)
	return false;
    return header.write(fd);
}

/**
 *
 * @param info on success the info string from the header is returned
 * in @c info.  On failure a failure message is stored in info
 * @return true if successful
 */

bool BackingOffLm::Internal::mountImage(
    int fd, std::string &info,
    const Bliss::TokenInventory &inventory)
{
    ImageHeader header(0);
    if (!header.read(fd, info))
	return false;
    ::free(nodes_);
    ::free(wordScores_);

    mmap_ = (char*) mmap(0, mmapSize_ = header.end(), PROT_READ, MAP_SHARED, fd, 0);
    if (mmap_ == MAP_FAILED) {
	info = "mapping of image failed";
	return false;
    }

    info = std::string(mmap_ + header.size());

    tokens_.resize(header.nTokens());
    char *str = mmap_ + header.tokensOffset();
    for (TokenIndex ti = 0; ti < TokenIndex(header.nTokens()); ++ti) {
	const Bliss::Token *token = 0;
	if (*str) {
	    token = inventory[str];
	    if (!token) {
		info = Core::form("undefined token \"%s\"", str);
		munmap(mmap_, mmapSize_);
		return false;
	    }
	    if (token->id() != ti) {
		info = Core::form("wrong token index: \"%s\" has index %d, should be %d",
				  str, token->id(), ti);
		munmap(mmap_, mmapSize_);
		return false;
	    }
	    while (*++str);
	}
	tokens_[ti] = token;
	++str;
    }

    nodes_ = (Node*) (mmap_ + header.nodesOffset());
    nodesTail_ = nodesEnd_ = nodes_ + header.nNodes();
    verify(nNodes() == header.nNodes());

    wordScores_ = (WordScore*) (mmap_ + header.scoresOffset());
    wordScoresTail_ = wordScoresEnd_ = wordScores_ + header.nWordScores();
    verify(nWordScores() == header.nWordScores());

    ensure(isMapped());
    return true;
}

// ===========================================================================
// decendants interface

void BackingOffLm::initialize(InitItem *begin, InitItem *end) {
    internal_ = Core::ref(new Internal);
    historyManager_ = internal_.get();

    for (const InitItem *i = begin; i != end; ++i) {
	if (i->token)
	    internal_->mapToken(i->token->id(), i->token);
	for (Token *h = i->history; *h; ++h)
	    internal_->mapToken((*h)->id(), *h);
    }

    u32 nNodes = 0, nWordScores = 0;
    for (const InitItem *i = begin; i != end; ++i) {
	if (i->token)
	    ++nWordScores;
	else
	    ++nNodes;
    }
    nNodes += 2; // nNodes is just an educated guess, not a constraint
    internal_->reserve(nNodes, nWordScores);
    internal_->build(begin, end);

    logInitialization();
}

void BackingOffLm::initialize(Core::Ref<Internal> i) {
    internal_ = i;
    historyManager_ = internal_.get();
    logInitialization();
}

void BackingOffLm::logInitialization() const {
    log("size: %d n-grams in %d histories",
	internal_->nWordScores(), internal_->nNodes());
    log("dependency value: ") << dependency_.value();
    Core::Channel dc(config, "dot");
    if (dc.isOpen())
	internal_->draw(dc, fullName());
}

// ===========================================================================
// language model interface

BackingOffLm::BackingOffLm(const Core::Configuration &c, Bliss::LexiconRef l) :
    Core::Component(c),
    LanguageModel(c, l)
{}

BackingOffLm::~BackingOffLm() {}

const Core::ParameterString BackingOffLm::paramImage(
    "image",
    "create and/or use language model binary image file");

void BackingOffLm::load() {

    std::string image = paramImage(config);
    if (!image.size()) {
	read();
	return;
    }

    if (!Core::isRegularFile(image)) {
	read();
	if (hasFatalErrors()) return;
	log("writing image file to \"%s\" ...", image.c_str());
	int fd = open(image.c_str(),
		      O_CREAT|O_WRONLY|O_TRUNC,
		      S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
	if (fd == -1) {
	    error("Failed to open image file \"%s\" for writing", image.c_str());
	    return;
	}
	if (!internal_->writeImage(fd, dependency_.value())) {
	    error("failed to write image file");
	    close(fd);
	    return;
	}
	if (close(fd)) {
	    error("failed to write image file");
	    return;
	}
    } else {
	internal_ = Core::ref(new Internal);
    }

    log("mounting image file \"%s\" ...", image.c_str());
    int fd = open(image.c_str(), O_RDONLY);
    if (fd == -1) {
	error("Failed to open image file \"%s\" for reading", image.c_str());
	return;
    }
    std::string info;
    if (!internal_->mountImage(fd, info, tokenInventory())) {
	error("failed to mount image file: ") << info;
	return;
    }
    dependency_.setValue(info);
    initialize(internal_);
}

Score BackingOffLm::sentenceBeginScore() const
{
    return score(history(internal_->root()), sentenceBeginToken());
}

const BackingOffLm::Node *BackingOffLm::Internal::startHistory(TokenIndex w) const {
    const Node *n = root()->findChild(w);
    if (!n) n = root();
    return n;
}

History BackingOffLm::startHistory() const {
    return history(internal_->startHistory(sentenceBeginToken()->id()));
}

const BackingOffLm::Node *BackingOffLm::Internal::extendedHistory(
    const Node *old, TokenIndex w) const
{
    TokenIndex hist[old->depth() + 1];
    for (const Node *n = old; n; n = n->parent())
	hist[n->depth()] = n->token();
    verify_(hist[0] == -1);
    hist[0] = w;

    const Node *result = root();
    for (Node::Depth d = 0; d <= old->depth(); ++d) {
	const Node *n = result->findChild(hist[d]);
	if (!n) break;
	result = n;
    }
    return result;
}

void BackingOffLm::historyTokens(const History& h, const Bliss::Token** target, u32& size, u32 arraySize) const {
    size = 0;
    for (const Node* n = descriptor<Self>(h); n;  (n = n->parent()) && size <   arraySize) {
	Token tok = internal_->token(n->token());
	if (tok) {
	    target[size] = tok;
	    ++size;
	}
    }
}

u32 BackingOffLm::historyLenght(const Lm::History& h) const
{
    const Node *n = descriptor<Self>(h);
    if(!n)
      return 0;
    return n->depth();
}

History BackingOffLm::extendedHistory(const History &h, Token w) const {
    return history(internal_->extendedHistory(descriptor<Self>(h), w->id()));
}

History BackingOffLm::reducedHistory(const History &h, u32 limit) const {
    const Node *n = descriptor<Self>(h);
    while (n->depth() > limit)
	n = n->parent();
    return history(n);
}

std::string BackingOffLm::Internal::formatHistory(const Node *h) {
    std::string result;
    for (const Node *n = h; n;  n = n->parent()) {
	Token tok = token(n->token());
	if (tok) {
	    if (n != h) result += utf8::blank;
	    result += std::string(tok->symbol());
	}
    }
    return result;
}

std::string BackingOffLm::formatHistory(const History &h) const {
    return internal_->formatHistory(descriptor<Self>(h));
}

Lm::Score BackingOffLm::Internal::score(const Node *h, TokenIndex w) const {
    Score backOffScore = 0.0;
    for (const Node *n = h; n;  n = n->parent()) {
	const WordScore *ws = n->findWordScore(wordScores_, w);
	if (ws)
	    return backOffScore + ws->score();
	backOffScore += n->backOffScore();
    }
    return backOffScore;
}

Lm::Score BackingOffLm::score(const History &h, Token w) const {
    return internal_->score(descriptor<Self>(h), w->id());
}

Score BackingOffLm::getAccumulatedBackOffScore(const History &history, int limit) const
{
    const Node *hn = descriptor<Self>(history);
  Score ret = 0;
  for (const Node *n = hn; n; n = n->parent())
    if( n->depth() >= limit )
      ret += n->backOffScore();
  return ret;
}

BackingOffLm::BackOffScores BackingOffLm::getBackOffScores(const Lm::History& history, int depth) const
{
    const Node *hn = descriptor<Self>(history);
    const Node *nodes[hn->depth()+1];
    for (const Node *n = hn; n; n = n->parent()) {
	Node::Depth d = n->depth();
	nodes[d] = n;
    }

    int nodeDepth = ((int)hn->depth()) - depth;

    BackOffScores ret;

    // Node-depth 0 is the zerogram back-off
    verify(nodeDepth >= 0 && nodeDepth <= hn->depth());

    ret.backOffScore = nodes[nodeDepth]->backOffScore();
    verify(ret.backOffScore != Core::Type<Score>::max);
    ret.start = internal_->scoresBegin(nodes[nodeDepth]);
    ret.end = internal_->scoresEnd(nodes[nodeDepth]);

    return ret;
}

void BackingOffLm::getBatch(
    const History &history,
    const CompiledBatchRequest *cbr,
    std::vector<f32> &result) const
{
    const Node *hn = descriptor<Self>(history);
    const Node *nodes[hn->depth()+1];
    Score backOffScore[hn->depth()+2];
    backOffScore[hn->depth()+1] = 0.0;
    for (const Node *n = hn; n; n = n->parent()) {
	Node::Depth d = n->depth();
	nodes[d] = n;
	backOffScore[d] = n->backOffScore() + backOffScore[d+1];
    }

    Bliss::SyntacticTokenMap<Score> scores(lexicon());
    scores.fill(backOffScore[0]);
    for (Node::Depth d = 0; d <= hn->depth(); ++d) {
	const Node *n = nodes[d];
	for (const WordScore *ws = internal_->scoresBegin(n); ws != internal_->scoresEnd(n); ++ws){
	    const Bliss::SyntacticToken *syntacticToken =
		static_cast<const Bliss::SyntacticToken*>(lexicon()->syntacticTokenInventory()[ws->token()]);
	    scores[syntacticToken] = ws->score() + backOffScore[d+1];
	}
    }

    const NonCompiledBatchRequest *ncbr = required_cast(const NonCompiledBatchRequest*, cbr);
    const BatchRequest &request(ncbr->request);
    for (BatchRequest::const_iterator r = request.begin(); r != request.end(); ++r) {
	Score score = 0.0;
	if (r->tokens.length() >= 1) {
	    score += scores[r->tokens[0]];
	    if (r->tokens.length() > 1) {
		const Node *h = internal_->extendedHistory(hn, r->tokens[0]->id());
		for (u32 ti = 1; ; ++ti) {
		    const Bliss::SyntacticToken *st = r->tokens[ti];
		    score += internal_->score(h, st->id());
		    if (ti+1 >= r->tokens.length()) break;
		    h = internal_->extendedHistory(h, st->id());
		}
	    }
	}

	score *= ncbr->scale();
	score += r->offset;

	if (result[r->target] > score)
	    result[r->target] = score;
    }
}

// ===========================================================================
// FSA interface

class BackingOffLm::Automaton : public LanguageModelAutomaton {
private:
    Core::Ref<Internal> internal_;
    const Node *initial_;
    TokenIndex sentenceEndToken_;
public:
    Automaton(Core::Ref<const BackingOffLm> lm) :
	LanguageModelAutomaton(static_cast<Core::Ref<const LanguageModel> >(lm)),
	internal_(lm->internal_)
    {
	addProperties(Fsa::PropertySortedByInput);
	setProperties(Fsa::PropertyLinear, Fsa::PropertyNone);
	setProperties(Fsa::PropertyAcyclic, Fsa::PropertyNone);
	initial_ = internal_->startHistory(lm->sentenceBeginToken()->id());
	sentenceEndToken_ = lm->sentenceEndToken()->id();
    }
    virtual ~Automaton() {}

    virtual Fsa::StateId initialStateId() const {
	return internal_->nodeIndex(initial_);
    }

    virtual Fsa::ConstStateRef getState(Fsa::StateId s) const {
	const Node *node = internal_->node(s);
	Fsa::State *state = new Fsa::State(s);

	// back-off
	if (node->backOffScore() < Core::Type<Score>::max) {
	    hope(node->parent()); // zero-gram backing-off not supported
	    state->newArc(
		internal_->nodeIndex(node->parent()),
		Fsa::Weight(node->backOffScore()),
		backOffLabel_);
	}

	for (const WordScore *ws = internal_->scoresBegin(node); ws != internal_->scoresEnd(node); ++ws) {
	    if (ws->score() >= Core::Type<Score>::max) continue;
	    if (ws->token() == sentenceEndToken_) {
		state->setFinal(Fsa::Weight(ws->score()));
	    } else {
		state->newArc(
		    internal_->nodeIndex(internal_->extendedHistory(node, ws->token())),
		    Fsa::Weight(ws->score()),
		    internal_->token(ws->token())->id());
	    }
	}

	state->sort(Fsa::byInput());

	return Fsa::ConstStateRef(state);
    }
};

Fsa::ConstAutomatonRef BackingOffLm::getFsa() const {
    return Fsa::ConstAutomatonRef(new Automaton(Core::ref(this)));
}
