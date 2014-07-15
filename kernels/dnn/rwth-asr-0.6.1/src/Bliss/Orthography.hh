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
// $Id: Orthography.hh 8249 2011-05-06 11:57:02Z rybach $

#ifndef _BLISS_CLASS_HH
#define _BLISS_CLASS_HH

#include "Fsa.hh"
#include "Lexicon.hh"
#include "PrefixTree.hh"
#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>

namespace Bliss {

    class OrthographicPrefixTree {
    private:
	typedef std::pair<const OrthographicForm::Char*, const Lemma*> ListItem;
	typedef std::vector<ListItem> List;
	typedef PrefixTree< const char*, ListItem, Core::select1st<ListItem> > Tree;

	List list_;
	Tree tree_;
    public:
	typedef Tree::ItemRange ItemRange;
	typedef Tree::ItemLocation ItemLocation;

	void build(LexiconRef lexicon);
/*
	ItemRange lookup(const std::string &seq, int &maxLen) const {
	    return tree_.lookup(seq.c_str(), maxLen);
	}
*/
	ItemRange lookup(const char *seq, int &maxLen) const {
	    return tree_.lookup(seq, maxLen);
	}
    };

    /** Parses an orthographic sentence into lemmas. */

    class OrthographicParser :
	public Core::ReferenceCounted, Core::Component
    {
    private:
	Core::Ref<const Lexicon> lexicon_;
	const Lemma *unknownLemma_;
	OrthographicPrefixTree prefixTree_;
	bool allowForSilenceRepetitions_;

    public:
	Core::Ref<const Lexicon> lexicon() const { return lexicon_; }
	const Lemma *unknownLemma() const { return unknownLemma_; }

	typedef void *Node;

	class Handler {
	protected:
	    const OrthographicParser *parser_;
	public:
	    virtual ~Handler() {};
	    virtual void initialize(const OrthographicParser*);
	    virtual Node newNode() = 0;
	    virtual void newEdge(Node from, Node to, const Lemma *lemma) = 0;
	    virtual void newSilenceEdge(Node from, Node to, const Lemma *silenceLemma, bool isFinal) {
		newEdge(from, to, silenceLemma);
	    }
	    virtual void newUnmatchableEdge(Node from, Node to, const std::string &orth);
	    virtual void finalize(Node intial, Node final) = 0;
	};

    public:
	OrthographicParser(
	    const Core::Configuration&,
	    Core::Ref<const Lexicon>);

	~OrthographicParser();

	void suppressSilenceRepetitions() {
	    allowForSilenceRepetitions_ = false;
	}

	/**
	 * Parse an orthographic string and make appropriate calls the
	 * the given handler.
	 */
	void parse(
	    const std::string &orth,
	    Handler &handler) const;

	/**
	 * Parse a string and return two iterators, the begin and the end of the matching lemmas.
	 * This way it can be used to resolve orthographic symbols.
	 */
	class LemmaIterator {
	    typedef OrthographicPrefixTree::ItemLocation ItemIterator;
	    ItemIterator itemIt_;
	public:
	    LemmaIterator() : itemIt_(0) {}
	    LemmaIterator(ItemIterator itemIt) : itemIt_(itemIt) {}
	    const Lemma * operator* () { return itemIt_->second; }
	    const Lemma * operator->() { return itemIt_->second; }
	    void operator++ () { ++itemIt_; }
	    bool operator== (const LemmaIterator & l) const { return itemIt_ == l.itemIt_; }
	    bool operator!= (const LemmaIterator & l) const { return itemIt_ != l.itemIt_; }
	};
	typedef std::pair<LemmaIterator, LemmaIterator> LemmaRange;
	LemmaRange lemmas(std::string &orth) const;

	Core::Ref<LemmaAcceptor> createLemmaAcceptor(const std::string &orth) const;
    };

    class LemmaAcceptorBuilder : public OrthographicParser::Handler {
	typedef OrthographicParser::Handler Precursor;
    private:
	Core::Ref<LemmaAcceptor> result_;
    public:
	virtual void initialize(const OrthographicParser*);
	virtual OrthographicParser::Node newNode();
	virtual void newEdge(OrthographicParser::Node from, OrthographicParser::Node to, const Lemma *lemma);
	virtual void newSilenceEdge(OrthographicParser::Node from, OrthographicParser::Node to, const Lemma *silenceLemma, bool isFinal);
	virtual void finalize(OrthographicParser::Node intial, OrthographicParser::Node final);

	LemmaAcceptorBuilder();
	~LemmaAcceptorBuilder();
	Core::Ref<LemmaAcceptor> product() const { return result_; }
    };

} // namespace Bliss

#endif // _BLISS_CLASS_HH
