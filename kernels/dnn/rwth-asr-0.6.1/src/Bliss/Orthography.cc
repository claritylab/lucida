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
// $Id: Orthography.cc 8249 2011-05-06 11:57:02Z rybach $

#include "Orthography.hh"
#include <Core/StringUtilities.hh>
#include <Core/Utility.hh>

using namespace Bliss;


void OrthographicPrefixTree::build(LexiconRef lexicon) {
    Core::Obstack<OrthographicForm::Char> strings;
    Lexicon::LemmaIterator l, l_end;
    for (Core::tie(l, l_end) = lexicon->lemmas() ; l != l_end ; ++l) {
	const OrthographicFormList &ol((*l)->orthographicForms());
	for (OrthographicFormList::Iterator o = ol.begin() ; o != ol.end() ; ++o) {
	    std::string str(o->str());
	    Core::enforceTrailingBlank(str);
	    list_.push_back(
		ListItem(strings.add0(&(*str.begin()), &(*str.end())), *l));
	}
    }
    list_.push_back(ListItem("__SENTINEL__", 0)); // sentinel
    tree_.build(&(*list_.begin()), &(*(list_.end()-1)));
}

OrthographicParser::OrthographicParser(
    const Core::Configuration &c,
    LexiconRef _lexicon) :
    Component(c),
    lexicon_(_lexicon),
    allowForSilenceRepetitions_(true)
{
    unknownLemma_ = lexicon_->specialLemma("unknown");
    prefixTree_.build(lexicon_);
}

OrthographicParser::~OrthographicParser() {}

void OrthographicParser::Handler::initialize(const OrthographicParser *parser) {
    parser_ = parser;
}

void OrthographicParser::Handler::newUnmatchableEdge(
    Node from, Node to, const std::string &orth)
{
    const Lemma *unk = parser_->unknownLemma();
    if (unk) {
	parser_->warning("substituting unknown word")
	    << " \"" << orth
	    << "\" with \""
	    << unk->preferredOrthographicForm() << "\"";
    } else {
	parser_->warning("skipping unknown word")
	    << " \"" << orth << "\"";
    }
    newEdge(from, to, unk);
}

void OrthographicParser::parse(
    const std::string &orth,
    Handler &handler) const
{
    require(Core::isWhitespaceNormalized(orth, Core::requireTrailingBlank));

    handler.initialize(this);

    std::string::size_type i, j, right_most;
    std::string::size_type length = orth.length();
    Bliss::OrthographicPrefixTree::ItemRange matchingLemmas;

    std::vector<Node> b_nodes(length + 1, 0);

    b_nodes[right_most = 0] = handler.newNode();
    for (i = 0 ; i <= length ; ++i) {
	verify(i <= right_most);

	if (!b_nodes[i]) {
	    verify(i < right_most);
	    continue;
	}

	for (int l = length - i ; l >= 0 ; --l) {
	    matchingLemmas = prefixTree_.lookup(orth.c_str() + i, l);
	    if (l < 0) continue;
	    j = i + l;
	    if (!b_nodes[j])
		b_nodes[j] = handler.newNode();
	    if (right_most < j)
		right_most = j;
	    Bliss::OrthographicPrefixTree::ItemLocation lemma;
	    for (lemma = matchingLemmas.first ; lemma != matchingLemmas.second ; ++lemma) {
		const bool isSilenceLemma = (lemma->second == lexicon_->specialLemma("silence"));
		if (allowForSilenceRepetitions_ or !isSilenceLemma) {
		    handler.newEdge(
			b_nodes[i], b_nodes[j],
			lemma->second);
		} else {
		    handler.newSilenceEdge(
			b_nodes[i],
			handler.newNode(),
			lemma->second,
			i == length);
		}
	    }
	}

	if (i == right_most && i < length) {
	    for (j = i + 1 ; j < length && orth[j] != utf8::blank ; ++j); ++j;
	    verify(j <= length);
	    verify(!b_nodes[j]) ;
	    b_nodes[j] = handler.newNode();
	    verify(right_most < j);
	    right_most = j;
	    handler.newUnmatchableEdge(
		b_nodes[i], b_nodes[j],
		orth.substr(i, j-i));
	}
    }

    handler.finalize(b_nodes[0], b_nodes[length]);
}

// ===========================================================================
OrthographicParser::LemmaRange OrthographicParser::lemmas(std::string &orth) const {
    Core::enforceTrailingBlank(orth);
    int maxId(orth.size());
    OrthographicPrefixTree::ItemRange range = prefixTree_.lookup(orth.c_str(), maxId);
    orth.resize(orth.size() - 1);
    if (maxId < int(orth.size() + 1))
	return LemmaRange(LemmaIterator(), LemmaIterator());
    else
	return LemmaRange(LemmaIterator(range.first), LemmaIterator(range.second));
}

// ===========================================================================
LemmaAcceptorBuilder::LemmaAcceptorBuilder() {}
LemmaAcceptorBuilder::~LemmaAcceptorBuilder() {}

void LemmaAcceptorBuilder::initialize(const OrthographicParser *parser) {
    Precursor::initialize(parser);
    result_ = ref(new LemmaAcceptor(parser->lexicon()));
}

OrthographicParser::Node LemmaAcceptorBuilder::newNode() {
    return result_->newState();
}

void LemmaAcceptorBuilder::newEdge(
    OrthographicParser::Node from,
    OrthographicParser::Node to,
    const Lemma *lemma)
{
    Fsa::State *source = static_cast<Fsa::State*>(from);
    Fsa::State *target = static_cast<Fsa::State*>(to);
    source->newArc(
	target->id(),
	result_->semiring()->one(),
	(lemma) ? lemma->id() : Fsa::Epsilon);
}

void LemmaAcceptorBuilder::newSilenceEdge(
    OrthographicParser::Node from,
    OrthographicParser::Node to,
    const Lemma *silenceLemma,
    bool isFinal)
{
    Fsa::State *source = static_cast<Fsa::State*>(from);
    Fsa::State *target = static_cast<Fsa::State*>(to);
    require(target->nArcs() == 0);
    for (Fsa::State::const_iterator a = source->begin(); a != source->end(); ++ a) {
	target->insert(target->end(), *a);
    }
    if (isFinal) {
	target->setFinal(result_->semiring()->one());
    }
    source->newArc(
	target->id(),
	result_->semiring()->one(),
	(silenceLemma) ? silenceLemma->id() : Fsa::Epsilon);
}

void LemmaAcceptorBuilder::finalize(
    OrthographicParser::Node initial,
    OrthographicParser::Node final)
{
    result_->setInitialStateId(static_cast<Fsa::State*>(initial)->id());
    result_->setStateFinal(static_cast<Fsa::State*>(final));
    result_->normalize();
}

// ===========================================================================
Core::Ref<LemmaAcceptor> OrthographicParser::createLemmaAcceptor(
    const std::string &orth) const
{
    LemmaAcceptorBuilder lab;
    parse(orth, lab);
    return lab.product();
}
