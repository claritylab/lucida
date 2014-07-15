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
// $Id: EditDistance.cc 5584 2006-02-13 21:36:40Z hoffmeister $

#include "EditDistance.hh"
#include "Fsa.hh"
#include <Core/Assertions.hh>
#include <Core/Hash.hh>
#include <Core/PriorityQueue.hh>
#include <Core/StringUtilities.hh>

#include <string>

using namespace Bliss;

// ===========================================================================
// class EditDistance::AlignmentItem

void EditDistance::AlignmentItem::write(std::ostream &os) const {
    switch (op) {
    case Deletion:
	os << Core::form("%-20s  }  %-20s", a->symbol().str(), "---");
	break;
    case Insertion:
	os << Core::form("%-20s  {  %-20s", "---",             b->symbol().str());
	break;
    case Substitution:
	os << Core::form("%-20s  #  %-20s", a->symbol().str(), b->symbol().str());
	break;
    case Correct:
	os << Core::form("%-20s  =  %-20s", a->symbol().str(), b->symbol().str());
	break;
    case Empty:
	os << Core::form("%-20s  =  %-20s", "---",             "---");
	break;
    }
}

// ===========================================================================
// class EditDistance::Alignment
void EditDistance::Alignment::clear() {
    Precursor::clear();
    score = Core::Type<Score>::max;
    cost  = Core::Type<Cost>::max;
}

void EditDistance::Alignment::write(Core::XmlWriter &xml) const {
    switch (format_) {
    case FormatBliss:
	xml << Core::XmlOpen("alignment")
	    + Core::XmlAttribute("type", "edit-distance")
	    + Core::XmlAttribute("format", "bliss");
	for (const_iterator ai = begin(); ai != end(); ++ai)
	    if (ai->a || ai->b)
		xml << *ai << std::endl;
	xml << Core::XmlClose("alignment");
	break;
    case FormatNist:
	std::ostringstream ossRef, ossHyp, ossEdt;
	u32 c = 0, s = 0, d = 0, i = 0;
	for (const_iterator it = begin(); it != end(); ++it) {
	    if (it->op == Empty)
		continue;
	    std::string strRef, strHyp, strEdt;
	    switch (it->op) {
	    case Deletion:
		strRef= Core::convertToUpperCase(it->a->symbol().str());
		strHyp.assign(strRef.size(), '*');
		strEdt = "D";
		++d;
		break;
	    case Insertion:
		strHyp= Core::convertToUpperCase(it->b->symbol().str());
		strRef.assign(strHyp.size(), '*');
		strEdt = "I";
		++i;
		break;
	    case Substitution:
		strRef= Core::convertToUpperCase(it->a->symbol().str());
		strHyp= Core::convertToUpperCase(it->b->symbol().str());
		strEdt = "S";
		++s;
		break;
	    case Correct:
		strRef= Core::convertToLowerCase(it->a->symbol().str());
		strHyp= Core::convertToLowerCase(it->b->symbol().str());
		strEdt = "";
		++c;
		break;
	    case Empty:
		break;
	    }
	    size_t length = std::max(strRef.size(), strHyp.size()) + 1;
	    ossRef << std::setw(length) << std::left << strRef;
	    ossHyp << std::setw(length) << std::left << strHyp;
	    ossEdt << std::setw(length) << std::left << strEdt;
	}
	xml << Core::XmlOpen("alignment")
	    + Core::XmlAttribute("type", "edit-distance")
	    + Core::XmlAttribute("format", "nist/sclite");
	xml << "Scores: (#C #S #D #I) "<< c << " " << s << " " << d << " " << i << "\n"
	    << "REF:  " << ossRef.str() << "\n"
	    << "HYP:  " << ossHyp.str() << "\n"
	    << "Eval: " << ossEdt.str() << "\n";
	xml << Core::XmlClose("alignment");
	break;
    }
}

// ===========================================================================
struct EditDistance::Trace :
    public Core::ReferenceCounted,
    public AlignmentItem
{
    Core::Ref<Trace> back;
    Trace(const Core::Ref<Trace> &_back,
	  const Bliss::Token *_a, const Bliss::Token *_b, EditOperation _op):
	AlignmentItem(_a, _b, _op), back(_back) {}
};

struct EditDistance::State {
    Fsa::StateId a, b;
    bool operator== (const State &rhs) const { return a == rhs.a && b == rhs.b; }

    struct Hash {
	size_t operator() (const EditDistance::State &p) const {
	    return (size_t(p.a) << 16) ^ size_t(p.b);
	}
    };
};

struct EditDistance::Hyp {
    State pos;
    Score score;
    Cost cost;
    u32 nRef;
    Core::Ref<Trace> back;

    struct KeyFunction {
	const State &operator() (const Hyp &h) const { return h.pos; }
    };

    struct PriorityFunction {
	bool operator() (const Hyp &lhs, const Hyp &rhs) const {
	    /*
	      If hypothesis with equal cost exist,
	      prefer that one yielding the longest reference
	      and thus minimizing the WER;
	      just heuristic, it is not guaranteed that the alignemnt
	      having the lowest WER is chosen, only that from all
	      hypothesis having minimal cost, that one yielding the
	      minimal WER is choosen.
	    */
	    return (lhs.cost == rhs.cost) ?
		(lhs.nRef > rhs.nRef) :
		(lhs.cost < rhs.cost);
	}
    };
};

const Core::Choice  EditDistance::AlignmentFormatChoice(
    "bliss", EditDistance::FormatBliss,
    "nist",  EditDistance::FormatNist,
    Core::Choice::endMark());
const Core::ParameterChoice  EditDistance::paramAlignmentFormat(
    "format",
    &AlignmentFormatChoice,
    "format of alignment output",
    FormatBliss);
const Core::ParameterInt EditDistance::paramInsertionCost(
    "ins-cost", "alignnent cost for insertions", 1, 0);
const Core::ParameterInt EditDistance::paramDeletionCost(
    "del-cost", "alignnent cost for deletions", 1, 0);
const Core::ParameterInt EditDistance::paramSubstitutionCost(
    "sub-cost", "alignnent cost for substitutions", 1, 0);
const Core::ParameterBool EditDistance::paramAllowBrokenWords(
    "allow-broken-words",
    "do edit distance calculation with respect to broken words; "
    "use the NIST syntax, i.e. pre- and postceeding hyphens",
    false);


EditDistance::EditDistance(const Core::Configuration &c) :
    Core::Component(c),
    statSearchSpace_ ("potential search space"),
    statMaxStackSize_("maximum stack size    "),
    statNExpansions_ ("number of expansions  "),
    debugChannel_(c, "debug")
{
    format_           = AlignmentFormat(paramAlignmentFormat(config));
    insertionCost_    = paramInsertionCost(config);
    deletionCost_     = paramDeletionCost(config);
    substitutionCost_ = paramSubstitutionCost(config);
    allowBrokenWords_ = paramAllowBrokenWords(config);
}

/** Cost function for Levenshtein alignment. */

/*
  broken word match;
  a (=ref) may contain preceeding and/or postceeding wildcard (="-")
*/
bool match(const char *ca, const char *cb) {
    if (*ca == '-') {
	size_t la = ::strlen(ca);
	size_t lb = ::strlen(cb);
	if ((ca[la-1] == '-') && (lb >= la - 2)) {
	    const char *eb = cb + lb - 1 - (la - 2);
	    for (const char *pb = cb; pb != eb; ++pb)
		if (::memcmp(ca+1, pb, la-2) == 0) return true;
	} else if (lb >= la - 1) {
	    if (::memcmp(ca+1, cb + lb - la + 1, la - 1) == 0)
		return true;
	}
	return false;
    } else {
	for (; (*ca == *cb) && (*ca != '\0') && (*cb != '\0'); ++ca, ++cb);
	return ((*ca == *cb)
		|| ((*ca == '-') && (*(++ca) == '\0'))) ? true : false;
    }
}

EditDistance::EditCost EditDistance::subCost(const Token *a, const Token *b) const {
    require_(a && b && (a != b));
    if (allowBrokenWords_ && match(a->symbol().str(), b->symbol().str()))
	return std::make_pair(Correct, 0);
    return std::make_pair(Substitution, substitutionCost_);
}

EditDistance::EditCost EditDistance::cost(const Token *a, const Token *b) const {
    if (a) {
	if (b) return (a == b) ? std::make_pair(Correct, Cost(0)) : subCost(a, b);
	else return delCost(a);
    } else if (b) {
	return insCost(b);
    } else
	return std::make_pair(Empty, Cost(0));
}

/**
 * Find best Levenshtein alignment.
 *
 * Implementation notes:
 *
 * In practice it would be very memory consuming to construct either
 * the Levenshtein transducer L or the composition \f$ A \odot L \odot B \f$
 * explicitly.  Since \f$ L \f$ has exactly one state we can
 * represent the vertices of \f$ A \odot L \odot B \f$ by a pairs
 * \f$(a, b)\f$ where \f$a\f$ / \f$b\f$ is a Node in \f$A\f$ / \f$B\f$.
 * The transitions are coded explicitly.
 *
 * The optimal alignment is determined by Dijkstra's shortest-path
 * algorithm.  The search is admissible (i.e. no search errors are
 * possible).  In the low-error regime it is far more efficient than
 * the simple dynamic-programming algorithm, even for linear chains.
 * All costs must be non-negative (which is the case).  Traceback
 * information is managed via reference counting.
 *
 * Subtle issues: This algorithm appears not to handle epsilon arcs.
 * Nevertheless the result is correct.  The point is that the true
 * distinction between insertions, deletions and substitutions is done
 * in the cost() function.  So "substituting" epsilon for A is counted
 * as a deletion; "substituting" epsilon for epsilon is counted as
 * correct; "deleting" epsilon is counted as correct; and so forth.
 *
 * Suggestions for the future:
 * We could convert this routine to a lazy Fsa::Automaton and have the
 * search done by Fsa's SSSP implmentation.  But currently Fsa
 * implements only depth-first search which is probably not efficent
 * in this case.
 * A note for the case that efficency problems (for large graphs)
 * should arise: It should be possible to implement admissible
 * rest-costs (i.e. A*-search) by counting the minimum number of
 * insertions/deletions required from any given point.  Currently this
 * seems not worth the effort.
 */
void EditDistance::align(Fsa::ConstAutomatonRef a, Fsa::ConstAutomatonRef b, Alignment &out) const {
    require(a->type() == Fsa::TypeAcceptor);
    require(b->type() == Fsa::TypeAcceptor);
    require(a->initialStateId() != Fsa::InvalidStateId);
    require(b->initialStateId() != Fsa::InvalidStateId);
    out.clear();

    Core::TracedPriorityQueue<Hyp, State, Hyp::KeyFunction, Hyp::PriorityFunction, State::Hash> stack;
    Core::hash_map<State, Cost, State::Hash> closed;
    Hyp nh;

    nh.pos.a = a->initialStateId();
    nh.pos.b = b->initialStateId();
    nh.score = 0.0;
    nh.cost = 0;
    nh.nRef = 0;

    stack.insert(nh);

    u32 nExpansions = 0, maxStackSize = 0;
    Fsa::ConstStateRef va, vb;
    Fsa::State::const_iterator ea, eb;
    const TokenAlphabet *aa = dynamic_cast<const TokenAlphabet*>(a->getOutputAlphabet().get());
    const TokenAlphabet *ba = dynamic_cast<const TokenAlphabet*>(b->getInputAlphabet().get());
    require(aa);
    require(ba);
    require(aa == ba);

    while (!stack.empty()) {
	const Hyp current(stack.top());

	verify(closed.find(current.pos) == closed.end());
	closed[current.pos] = current.cost;

	va = a->getState(current.pos.a);
	vb = b->getState(current.pos.b);
	if (va->isFinal() && vb->isFinal()) {
	    const_cast<Hyp&>(stack.top()).score += Score(vb->weight());
	    break;
	}

	stack.pop();

	++nExpansions;
	if (maxStackSize < stack.size())
	    maxStackSize = stack.size();

	// deletion (in b wrt a)
	nh.pos.b = current.pos.b;
	for (ea = va->begin(); ea != va->end(); ++ea) {
	    nh.pos.a = ea->target();
	    nh.score = current.score;
	    EditCost opCost = cost(aa->token(ea->output()), 0);
	    nh.cost = current.cost + opCost.second;
	    if (ea->output() != Fsa::Epsilon)
		nh.nRef = current.nRef + 1;
	    else
		nh.nRef = current.nRef;
	    if (closed.find(nh.pos) != closed.end()) {
		verify(nh.cost >= closed[nh.pos]);
		continue;
	    }
	    nh.back = Core::ref(new Trace(
				    current.back,
				    aa->token(ea->output()), 0,
				    opCost.first));
	    stack.insertOrRelax(nh);
	}

	// insertion (in b wrt a)
	nh.pos.a = current.pos.a;
	for (eb = vb->begin(); eb != vb->end(); ++eb) {
	    nh.pos.b = eb->target();
	    nh.score = current.score + Score(eb->weight());
	    EditCost opCost = cost(0, ba->token(eb->input()));
	    nh.cost = current.cost + opCost.second;
	    nh.nRef = current.nRef;
	    if (closed.find(nh.pos) != closed.end()) {
		verify(nh.cost >= closed[nh.pos]);
		continue;
	    }
	    nh.back = Core::ref(new Trace(
				    current.back,
				    0, ba->token(eb->input()),
				    opCost.first));
	    stack.insertOrRelax(nh);
	}

	// substitutions
	for (ea = va->begin(); ea != va->end(); ++ea) {
	    nh.pos.a = ea->target();
	    for (eb = vb->begin(); eb != vb->end(); ++eb) {
		nh.pos.b = eb->target();
		nh.score = current.score + Score(eb->weight());
		EditCost opCost = cost(aa->token(ea->output()), ba->token(eb->input()));
		nh.cost = current.cost + opCost.second;
		if (ea->output() != Fsa::Epsilon)
		    nh.nRef = current.nRef + 1;
		else
		    nh.nRef = current.nRef;
		if (closed.find(nh.pos) != closed.end()) {
		    verify(nh.cost >= closed[nh.pos]);
		    continue;
		}
		nh.back = Core::ref(new Trace(
					current.back,
					aa->token(ea->output()),
					ba->token(eb->input()),
					opCost.first));
		stack.insertOrRelax(nh);
	    }
	}
    }

    //statSearchSpace_  += a->nStates() * b->nStates();
    statMaxStackSize_ += maxStackSize;
    statNExpansions_  += nExpansions;

    if (stack.empty()) {
	error("No final state found.  Apparently one automaton has no final states.");
	Hyp empty;
	empty.score = 0.0;
	empty.cost = 0;
	empty.nRef = 0;
	return;
    }
    const Hyp result(stack.top());
    stack.clear(); closed.clear();

    out.score = result.score;
    out.cost  = result.cost;
    for (Core::Ref<Trace> trace = result.back; trace; trace = trace->back)
	out.push_back(AlignmentItem(*trace));
    std::reverse(out.begin(), out.end());

    if (debugChannel_.isOpen()) {
	debugChannel_ << Core::XmlFull("cost", out.cost);
	debugChannel_ << Core::XmlFull("score", out.score);
	out.write(debugChannel_);
    }
}

EditDistance::~EditDistance() {
    log() << statSearchSpace_;
    log() << statMaxStackSize_;
    log() << statNExpansions_;
}

// ===========================================================================
// class ErrorStatistic

void ErrorStatistic::clear() {
    nLeftTokens_ = nRightTokens_ = nInsertions_ = nDeletions_ = nSubstitutions_ = 0;
}

ErrorStatistic::ErrorStatistic() :
    nLeftTokens_(0), nRightTokens_(0), nInsertions_(0), nDeletions_(0), nSubstitutions_(0)
{}

ErrorStatistic::ErrorStatistic(const EditDistance::Alignment &a) :
    nLeftTokens_(0), nRightTokens_(0), nInsertions_(0), nDeletions_(0), nSubstitutions_(0)
{
    *this += a;
}

void ErrorStatistic::operator+=(const EditDistance::Alignment &a) {
    for (EditDistance::Alignment::const_iterator i = a.begin(); i != a.end(); ++i) {
	switch (i->op) {
	case EditDistance::Deletion:
	    ++nDeletions_;
	    ++nLeftTokens_;
	    verify(i->a && !i->b);
	    break;
	case EditDistance::Insertion:
	    ++nInsertions_;
	    ++nRightTokens_;
	    verify(!i->a && i->b);
	    break;
	case EditDistance::Substitution:
	    ++nSubstitutions_;
	case EditDistance::Correct:
	    verify(i->a && i->b);
	    ++nLeftTokens_;
	    ++nRightTokens_;
	    break;
	case EditDistance::Empty:
	    verify(!i->a && !i->b);
	    break;
	}
    }
    verify(nLeftTokens_ + nInsertions_ == nRightTokens_ + nDeletions_);
}

void ErrorStatistic::operator+=(const ErrorStatistic &e) {
    nLeftTokens_    += e.nLeftTokens_;
    nRightTokens_   += e.nRightTokens_;
    nInsertions_    += e.nInsertions_;
    nDeletions_     += e.nDeletions_;
    nSubstitutions_ += e.nSubstitutions_;
}

void ErrorStatistic::write(Core::XmlWriter &os) const {
    os << Core::XmlOpen("statistic")
	+ Core::XmlAttribute("type", "edit-distance");
    os << Core::XmlOpen("count") + Core::XmlAttribute("event", "token")
       << nLeftTokens_
       << Core::XmlClose("count");
    os << Core::XmlOpen("count") + Core::XmlAttribute("event", "deletion")
       << nDeletions_
       << Core::XmlClose("count");
    os << Core::XmlOpen("count") + Core::XmlAttribute("event", "insertion")
       << nInsertions_
       << Core::XmlClose("count");
    os << Core::XmlOpen("count") + Core::XmlAttribute("event", "substitution")
       << nSubstitutions_
       << Core::XmlClose("count");
    os << Core::XmlClose("statistic");
}
