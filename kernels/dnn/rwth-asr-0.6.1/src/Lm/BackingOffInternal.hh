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
// $Id: BackingOffInternal.hh 8635 2012-04-20 13:04:19Z rybach $

#ifndef _LM_BACKINGOFF_INTERNAL_HH
#define _LM_BACKINGOFF_INTERNAL_HH

#include "BackingOff.hh"
#include "HistoryManager.hh"


/** Internal data structures of BackingOffLm */

namespace BackingOffPrivate {
    typedef u32 NodeIndex;
    typedef u32 WordScoreIndex;
    typedef Bliss::Token::Id TokenIndex;

    template <class Node>
    const Node *binarySearch(const Node *l, const Node *r, TokenIndex t) {
	while (l <= r) {
	    const Node *m = l + (r - l) / 2;
	    if (t < m->token()) {
		r = m - 1;
	    } else if (t > m->token()) {
		l = m + 1;
	    } else /* t == m->token() */ {
		return m;
	    }
	}
	return 0;
    }
}
using BackingOffPrivate::TokenIndex;
using BackingOffPrivate::NodeIndex;
typedef Lm::BackingOffLm::WordScore WordScore;
using BackingOffPrivate::WordScoreIndex;


class Lm::BackingOffLm::Node {
public:
    typedef u16 Depth;

private: // internal data
    friend class BackingOffInternalFriend;

    TokenIndex token_;  /**< least recent word in history */
    Depth      depth_;  /**< number of words in history */
    NodeIndex  parent_;
    Score      backOffScore_;
    NodeIndex firstChild_;
    WordScoreIndex firstWordScore_;

    const Node *childrenBegin() const { return this   +           firstChild_; }
    const Node *childrenEnd  () const { return this+1 + (this+1)->firstChild_; }

public:
    TokenIndex token() const { return token_; }
    Score backOffScore() const { return backOffScore_; }
    Depth depth() const { return depth_; }
    const Node *parent() const { return (parent_) ? (this - parent_) : 0; }
    const Node *findChild(TokenIndex) const;
    const WordScore *scoresBegin(const WordScore *base) const { return base +           firstWordScore_; }
    const WordScore *scoresEnd  (const WordScore *base) const { return base + (this+1)->firstWordScore_; }
    const WordScore *findWordScore(const WordScore *base, TokenIndex) const;

    struct Ordering {
	bool operator() (const Node &a, const Node &b) const {
	    if (a.parent_ != b.parent_)
		return (a.parent_ < b.parent_);
	    return (a.token_ < b.token_);
	}
    };
};


class Lm::BackingOffLm::Internal :
    public Core::ReferenceCounted,
    public StaticHistoryManager
{
public:
    typedef const Bliss::Token *Token;
protected:
    typedef std::vector<Token> Tokens;
    Tokens tokens_;

    Node *nodes_, *nodesTail_, *nodesEnd_;
    void changeNodeCapacity(NodeIndex nNodes);
    Node *newNode();

    WordScore *wordScores_, *wordScoresTail_, *wordScoresEnd_;
    void changeWordScoreCapacity(WordScoreIndex nWordScores);
    WordScore *newWordScore();

    struct InitItemOrdering;
    struct InitItemRange {
	InitItem *begin, *end;
    };
    std::vector<InitItemRange> *init_;
    void buildNode(NodeIndex);
    void makeRoot();
    void finalize();

    char *mmap_;
    size_t mmapSize_;
    bool writeImageTokenTable(int fd) const;

public:
    Internal();
    ~Internal();
    void mapToken(TokenIndex, Token);
    void reserve(NodeIndex nNodes, WordScoreIndex nWordScores);
    void build(InitItem*, InitItem*);
    bool writeImage(int fd, const std::string &info) const;
    bool mountImage(int fd,       std::string &info, const Bliss::TokenInventory&);
    bool isMapped() const { return (mmap_ != NULL); }

    u32 nNodes() const { return nodesTail_ - nodes_; }
    u32 nWordScores() const { return wordScoresTail_ - wordScores_; }
    void draw(std::ostream&, const std::string &title) const;

    const BackingOffLm::Node *root() const { return nodes_; }
    const Node *startHistory(TokenIndex w) const;
    const Node *extendedHistory(const Node *old, TokenIndex w) const;
    std::string formatHistory(const Node*);

    const WordScore *scoresBegin(const Node *node) const { return node->scoresBegin(wordScores_); }
    const WordScore *scoresEnd  (const Node *node) const { return node->scoresEnd  (wordScores_); }
    Score score(const Node*, TokenIndex) const;

    Token token(TokenIndex ti) const {
	require_(-1 <= ti && ti < TokenIndex(tokens_.size()));
	return (ti >= 0) ? tokens_[ti] : 0;
    }

    NodeIndex nodeIndex(const Node *node) const {
	require_(nodes_ <= node && node < nodesTail_);
	return node - nodes_;
    }
    const Node *node(NodeIndex index) const {
	require_(index < nNodes());
	return &nodes_[index];
    }

    static Core::Ref<const Internal> peek(Core::Ref<const BackingOffLm> lm) {
	return lm->internal_;
    }
};


#endif // _LM_BACKINGOFF_INTERNAL_HH
