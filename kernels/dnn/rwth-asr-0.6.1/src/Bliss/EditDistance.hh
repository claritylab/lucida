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
// $Id: EditDistance.hh 9621 2014-05-13 17:35:55Z golik $

#ifndef _BLISS_EDIT_DISTANCE
#define _BLISS_EDIT_DISTANCE

#include <Core/Component.hh>
#include <Core/Parameter.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Statistics.hh>
#include <Core/Types.hh>
#include <Fsa/Automaton.hh>
#include "Fsa.hh"
#include "Lexicon.hh"

namespace Bliss {

    /**
     * Calculation of Levenshtein alignment.
     *
     * The Levenshtein distance of two strings \f$A\f$ and \f$B\f$ is the
     * minimum number of edit operations required to transform \f$A\f$
     * into \f$B\f$, where a editing operation is either a deletetion, an
     * insertion or a substitution.  By the Levenshtein alignment we
     * mean the corresponding optimal sequence of pairs \f$(a_i, b_i)\f$,
     * where \f$A = a_1 ... a_n\f$, \f$B = b_1 ... b_n\f$, and \f$a_i = \epsilon\f$
     * represents an insertion, \f$b_i = \epsilon\f$ a deletion, and
     * otherwise \f$a_i \neq b_i\f$ a substitution.
     *
     * Here we generalize this notion from sequences to directed
     * graphs (or equivalently FSAs).  The minimization extends not
     * only over the set of editing operations, but also over all
     * strings represented in either graph.  This has several
     * applications in NLP, e.g. word error rate, word graph error
     * rate, phoneme error rate in the presence of pronunciation
     * variation.
     *
     * In the formalism of FSTs the problem can be stated as follows:
     * The Levenshtein transducer \f$L\f$ is defined as follows: There
     * is a single state with self transitions.  For each \f$a\f$
     * there are transitions \f$a:a\f$ (correct) with zero cost,
     * \f$a:\epsilon\f$ (deletion) with cost 1 and \f$\epsilon:a\f$
     * (insertion) with cost 1.  For each \f$a \neq b \f$ there is
     * transition with cost 1 (substitution).  To determine the
     * optimal Levenshtein alignment of FSAs \f$A\f$ and \f$B\f$ we
     * just form the composition \f$A \odot L \odot B\f$ and determine
     * the best path.
     */

    class EditDistance : public Core::Component {
    public:
	typedef enum {Deletion, Insertion, Substitution, Correct, Empty} EditOperation;
	typedef f32 Score;
	typedef u32 Cost;
	typedef std::pair<EditOperation, Cost> EditCost;

	typedef enum {FormatBliss, FormatNist} AlignmentFormat;

	struct AlignmentItem {
	    const Token *a, *b;
	    EditOperation op;
	    AlignmentItem(const Bliss::Token *_a, const Bliss::Token *_b, EditOperation _op):
		a(_a), b(_b), op(_op) {}

	    void write(std::ostream &o) const;
	    friend std::ostream &operator<<(std::ostream &o, const AlignmentItem &a) {
		a.write(o); return o;
	    }
	};

	class Alignment : public std::vector<AlignmentItem> {
	    typedef std::vector<AlignmentItem> Precursor;
	private:
	    AlignmentFormat format_;
	public:
	    Score score;
	    Cost cost;
	public:
	    Alignment(AlignmentFormat format = FormatBliss) :
		format_(format), score(0), cost(0) {}
	    void clear();
	    void write(Core::XmlWriter &xml) const;
	};

    private:
	struct Hyp;
	struct State;
	struct Trace;

	AlignmentFormat format_;
	bool allowBrokenWords_;
	Cost deletionCost_, insertionCost_, substitutionCost_;

	EditCost delCost(const Token *a) const {
	    require_(a);
	    return std::make_pair(Deletion, deletionCost_);
	}
	EditCost insCost(const Token* b) const {
	    require_(b);
	    return std::make_pair(Insertion, insertionCost_);
	}
	EditCost subCost(const Token *a, const Token *b) const;
	EditCost cost(const Token *a, const Token *b) const;

    private:
	mutable Core::Statistics<u32> statSearchSpace_, statMaxStackSize_, statNExpansions_;
	mutable Core::XmlChannel debugChannel_;

    public:
	static const Core::Choice AlignmentFormatChoice;
	static const Core::ParameterChoice paramAlignmentFormat;
	static const Core::ParameterInt paramInsertionCost;
	static const Core::ParameterInt paramDeletionCost;
	static const Core::ParameterInt paramSubstitutionCost;
	static const Core::ParameterBool paramAllowBrokenWords;

	EditDistance(const Core::Configuration&);
	~EditDistance();

	Alignment newAlignment() const { return Alignment(format_); }
	void align(Fsa::ConstAutomatonRef, Fsa::ConstAutomatonRef, Alignment &out) const;
    };

    class ErrorStatistic {
	u32 nLeftTokens_, nRightTokens_, nInsertions_, nDeletions_, nSubstitutions_;
    public:
	ErrorStatistic();
	ErrorStatistic(const EditDistance::Alignment&);
	void clear();
	void operator+=(const EditDistance::Alignment&);
	void operator+=(const ErrorStatistic&);
	void write(Core::XmlWriter&) const;
	u32 nLeftTokens()    const { return nLeftTokens_;    }
	u32 nRightTokens()   const { return nLeftTokens_ - nDeletions_ + nInsertions_;    }
	u32 nInsertions()    const { return nInsertions_;    }
	u32 nDeletions()     const { return nDeletions_;     }
	u32 nSubstitutions() const { return nSubstitutions_; }
	u32 nErrors()        const { return nInsertions_ + nDeletions_ + nSubstitutions_; }
    };

}; //namespace Bliss

#endif // _BLISS_EDIT_DISTANCE
