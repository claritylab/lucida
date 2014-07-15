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
#ifndef _BLISS_PREFIXTREE_HH
#define _BLISS_PREFIXTREE_HH

#include <utility>
#include <string>
#include <Core/Obstack.hh>
#include <Core/Hash.hh>
#include <Core/Extensions.hh>

namespace Bliss {

    template <class Symbol> struct PrefixTreeSymbolTraits;

    template <>
    struct PrefixTreeSymbolTraits<std::string> {
	typedef std::string::value_type Symbol;
	typedef std::string::difference_type Index;
	static const int term = 0 ;
	static const int first = 0 ;
	static const int last = 0xfd ; // 0xfe and 0xff are illegal in UTF-8
	static const int maximumLength = 100 ;
	static Symbol sentinel[maximumLength] ;
    };

    template <>
    struct PrefixTreeSymbolTraits<const char*> {
	typedef char Symbol;
	typedef int Index;
	static const int term = 0 ;
	static const int first = 0 ;
	static const int last = 0xfd ; // 0xfe and 0xff are illegal in UTF-8
	static const int maximumLength = 100 ;
	static Symbol sentinel[maximumLength];
    };

    /**
     * Prefix tree class template.
     */

    template <
	class S,
	class I,
	class K = typename Core::identity<I>,
	class T = PrefixTreeSymbolTraits<S>
    >
    class PrefixTree {
    public:
	typedef T Traits;
	typedef S SymbolString;
	typedef typename Traits::Symbol Symbol;
	typedef typename Traits::Index SymbolIndex;
	typedef I Item;
	typedef Item *ItemLocation;
	typedef std::pair<ItemLocation, ItemLocation> ItemRange;
	typedef K KeyFunction;

    private:
	KeyFunction key;

	ItemLocation sentinelLocation_;
	SymbolString sentinelBackup_;
	void putSentinel(ItemLocation l, SymbolIndex d) {
	    Traits::sentinel[d] = Traits::last + 1 ;
	    sentinelLocation_ = l;
	    sentinelBackup_ = key(*sentinelLocation_);
	    // This cast shouldn't be necessary.
	    // I just don't know where the const qualifier gets introduced.
	    const_cast<SymbolString&>(key(*sentinelLocation_)) = Traits::sentinel;
	}
	void removeSentinel() {
	    // This cast shouldn't be necessary.
	    // I just don't know where the const qualifier gets introduced.
	    const_cast<SymbolString&>(key(*sentinelLocation_)) = sentinelBackup_;
	}

	template <typename B> struct Buckets {
	    B data[Traits::last - Traits::first + 3];
	    B &operator[](int i) {
		require(Traits::first-1 <= i && i <= Traits::last+1);
		return data[i - Traits::first + 1];
	    }
	};

	struct TreeNode {
	    Symbol symbol;
	    bool expanded;
	    SymbolIndex depth;
	    union {
		struct {
		    Item *begin, *end ;
		} entries ;
		struct {
		    TreeNode *begin, *end ;
		} subtrees ;
	    };
	};
	Core::Obstack<TreeNode> tree ;
	TreeNode *root ;

	int bucketSortThreshold_;

	void buildNode(TreeNode*) ;
	void buildNodeBucketSort(TreeNode*) ;
	void buildNodeInsertionSort(TreeNode*) ;

	TreeNode *findSuccessor(const TreeNode *n, Symbol c) const ;
	TreeNode *findTermSuccessor(const TreeNode *n) const {
	    // This is an optimization for finding terminal successors.
	    // Will be optimized out by the compiler if it's not
	    // applicable.
	    if (Traits::term == Traits::first) {
		if (n->subtrees.begin->symbol == Traits::term)
		    return n->subtrees.begin ;
		else
		    return 0 ;
	    } else {
		return findSuccessor(n, Traits::term) ;
	    }
	}

	void drawTree(std::ostream&, const TreeNode*) const;

    public:
	PrefixTree();

	/**
	 * Construct a prefix tree.
	 *
	 * All sequences in the range [@c begin, @c end) are included
	 * in the prefix tree.  The element at position @c end MUST
	 * exists although it is not used, since a sentinel is stored
	 * there during the construction phase.
	 */
	void build(Item *begin, Item *end);

	void clear();

	/**
	 * Search prefixes of a string
	 *
	 * lookup() finds the longest sequence contained in the tree
	 * which is a prefix of @c seq and is not longer than @c
	 * maxLen or symbols.  lookup() returns the range of items
	 * with this sequence.  On return @c maxLen is adjusted to the
	 * length of this found sequence.
	 *
	 * If no matching sequence is found, an empty range is
	 * returned and @c maxLen is set to -1.  Note that this is
	 * different from the case of a zero length match, when the
	 * empty sequence is in the tree.
	 *
	 * @param seq sequence to search for
	 * @param maxLen maximum length of matching sequence
	 * @return a pair of iterators designating the range of items
	 * having the longest prefixes
	 *
	 * To find all prefixes of @seq with descending length do
	 * something like:
	 * for (int maxLen = INT_MAX ; maxLen >= 0 ; --maxLen) {
	 *     PFTree::ItemRange m = tree.lookup(seq, maxLen);
	 *     copy(m.first, m.second, ostream_iterator<string>(clog(), " "));
	 */
	// lookup, set length of longest match, return longest match
	ItemRange lookup(const SymbolString &seq,
			 SymbolIndex &maxLen) const;
	// lookup, set all matches, return length of longest match
	SymbolIndex lookup(const SymbolString &seq,
		    std::vector<ItemRange> &matches,
		    SymbolIndex maxLen) const;

	void drawTree(std::ostream&) const;
    };

} // namespace Bliss

#include "PrefixTree.tcc"

#endif // _BLISS_PREFIXTREE_HH
