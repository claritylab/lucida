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
#include <iostream>
#include <sstream>
#include <cstdio>
#include "PrefixTree.hh"
#include <Core/Application.hh>

namespace Bliss {

template <class S, class L, class K, class T>
PrefixTree<S, L, K, T>::PrefixTree() {
    bucketSortThreshold_ = (Traits::last - Traits::first + 1) / 2;
    root = 0;
}


template <class S, class L, class K, class T>
void PrefixTree<S, L, K, T>::build(
    ItemLocation begin,
    ItemLocation end)
{
    require(!root);

    TreeNode m;
    m.symbol        = Traits::last+1;
    m.expanded      = false;
    m.depth         = -1;
    m.entries.begin = begin;
    m.entries.end   = end;
    root = tree.add(m);

//    list.append(Item(Traits::sentinel, Value())); // provide space for sentinel
    buildNode(root);
//    list.truncate(list.size()-1); // remove sentinel
}

// While building the prefix tree the list of entries is effectively
// sorted via Radix Sort.  I.e. the sequences are first sorted by
// their first symbol, then all sequences starting with "A" are
// sorted by their second symbol, and so on.  For these subordinate
// sorting processes we use Bucket Sort (AKA Histogram or Counting
// Sort) which has a time complexity of O(N + K), where N is the
// number of sequences in the subtree and K the cardinality of the set
// of symbols.  We note that Bucket Sort gets rather inefficient when
// we get down to the small subtrees which have fewer sequences than
// there are symbols.  For this reason we switch to Insertion Sort
// which is efficient on small sets, when the size of the subtree gets
// smaller than some empirically adjusted threshold called
// @c bucketSortThreshold_

template <class S, class L, class K, class T>
void PrefixTree<S, L, K, T>::buildNode(TreeNode *n) {
    require(!n->expanded);
    if (n->depth >= Traits::maximumLength){
	std::stringstream ss;
	ss << "assertion failed: depth of prefix tree is " << n->depth
		<< ", but maximal length is " << Traits::maximumLength << "\n";
	ss << "labels of current node:\n";
	for (ItemLocation m = n->entries.begin ; m != n->entries.end ; ++m) {
	    ss << "l" << m << " [shape=\"box\" label=\"l" << key(*m) << "\"]\n";
	}
	Core::Application::us()->criticalError("%s", ss.str().c_str());
    }

    if (n->entries.end - n->entries.begin > bucketSortThreshold_)
	buildNodeBucketSort(n);
    else
	buildNodeInsertionSort(n);
    n->expanded = true;

    for (TreeNode* m = n->subtrees.begin ; m < n->subtrees.end ; ++m) {
	if (m->symbol != Traits::term) {
	    buildNode(m);
	}
    }
//  drawTree(cout); // DEBUG
}


template <class S, class L, class K, class T>
void PrefixTree<S, L, K, T>::buildNodeInsertionSort(TreeNode *n) {
    SymbolIndex d = n->depth + 1;
    ItemLocation i, j;
    Item v;

    putSentinel(n->entries.end, d);

    // insertion sort (backwards)
    for (i = n->entries.end - 2 ; i >= n->entries.begin ; --i) {
	v = *i;
	for (j = i ; key(*(j+1))[d] < key(v)[d] ; ++j)
	    *j = *(j+1);
	*j = v;
    }

    // create tree nodes
    TreeNode m;
    tree.start();
    for (i = n->entries.begin ; i < n->entries.end ;) {
	m.symbol        = key(*i)[d];
	m.expanded      = false;
	m.depth         = d;
	m.entries.begin = i;
	while (key(*(++i))[d] == m.symbol);
	m.entries.end   = i;
	tree.grow(m);
    }
    n->subtrees.begin = tree.currentBegin();
    n->subtrees.end   = tree.currentEnd();
    tree.finish();

    removeSentinel();
}


template <class S, class L, class K, class T>
void PrefixTree<S, L, K, T>::buildNodeBucketSort(TreeNode *n) {
    SymbolIndex d = n->depth + 1;

    putSentinel(n->entries.end, d);

    // create counts
    static Buckets<int> count;
    for (Symbol k = Traits::first ; k <= Traits::last ; ++k)
	count[k] = 0;
    for (ItemLocation s = n->entries.begin ; s != n->entries.end ; ++s)
	++count[key(*s)[d]];

    // create iterators by summing counts
    static Buckets<ItemLocation> loc;
    loc[Traits::last+1] = n->entries.end;
    for (int k = Traits::last ; k >= Traits::first ; --k)
	loc[k] = loc[k+1] - count[k];
    verify(loc[Traits::first] == n->entries.begin);

    // sort in-situ by circular exchange
    Item u, v;
    for (int k = Traits::last ; k >= Traits::first ; --k) {
	while (key(v = *loc[k])[d] <= k) {
	    do {
		u = v;
		v = *loc[key(u)[d]];
		*loc[key(u)[d]] = u;
		++loc[key(u)[d]];
	    } while (key(u)[d] != k);
	}
    }
    loc[Traits::first-1] = n->entries.begin;
    verify(loc[Traits::last] == n->entries.end);

    removeSentinel();

    // create tree nodes
    TreeNode m;
    tree.start();
    for (int k = Traits::first ; k <= Traits::last ; ++k) {
	if (loc[k-1] < loc[k]) {
	    m.symbol        = k;
	    m.expanded      = false;
	    m.depth         = d;
	    m.entries.begin = loc[k-1];
	    m.entries.end   = loc[k];
	    tree.grow(m);
	}
    }
    n->subtrees.begin = tree.currentBegin();
    n->subtrees.end   = tree.currentEnd();
    tree.finish();
}


template <class S, class L, class K, class T>
typename PrefixTree<S, L, K, T>::TreeNode*
PrefixTree<S, L, K, T>::findSuccessor(
    const TreeNode *n, Symbol c) const
{
    TreeNode *l, *r, *m ;

    l = n->subtrees.begin ; r = n->subtrees.end - 1 ;
    while (l <= r) {
	m = l + (r - l) / 2 ;
	if (c < m->symbol) {
	    r = m - 1 ;
	} else if (c > m->symbol) {
	    l = m + 1 ;
	} else /* c == m->symbol */ {
	    return m ;
	}
    }
    return 0 ;
}


template <class S, class L, class K, class T>
typename PrefixTree<S, L, K, T>::ItemRange PrefixTree<S, L, K, T>::lookup(
    const SymbolString &seq, SymbolIndex &maxLen) const
{
    require(root);

    TreeNode *n, *m, *longest_match;

    longest_match = 0;
    n = root;
    while (n->depth < maxLen) {
	if ((m = findTermSuccessor(n)) != 0)
	    longest_match = m;
	if ((seq[n->depth+1] == Traits::term) ||
	    (n = findSuccessor(n, seq[n->depth+1])) == 0)
	    break;
    }

    if (longest_match != 0) {
	maxLen = longest_match->depth;
	return ItemRange(longest_match->entries.begin,
			 longest_match->entries.end);
    } else {
	maxLen = -1;
	return ItemRange(0, 0);
    }
}

template <class S, class L, class K, class T>
typename PrefixTree<S, L, K, T>::SymbolIndex PrefixTree<S, L, K, T>::lookup(
    const SymbolString &seq, std::vector<ItemRange> &matches, SymbolIndex maxLen) const
{
    require(root);

    TreeNode *n, *m;

    matches.clear();
    n = root;
    while (n->depth < maxLen) {
	if ((m = findTermSuccessor(n)) != 0)
	    matches.push_back(ItemRange(m->entries.begin, m->entries.end));
	else
	    matches.push_back(ItemRange(0,0));
	if ((seq[n->depth+1] == Traits::term) ||
	    ((n = findSuccessor(n, seq[n->depth+1])) == 0))
	    break;
    }
    return matches.size();
}

template <class S, class L, class K, class T>
void PrefixTree<S, L, K, T>::drawTree(std::ostream &os, const TreeNode *n) const {
    //os.form("n%xd [label=\"%c\"]\n", n, (n->symbol != Traits::term) ? n->symbol : '$'); // _GNU_ extension which should not be used
    //std::fprintf(os, "n%xd [label=\"%c\"]\n", n, (n->symbol != Traits::term) ? n->symbol : '$');
    os << "n" << n << " [label=\"" << ((n->symbol != Traits::term) ? n->symbol : '$') << "\"]" << std::endl;

    if (n->expanded) {
	for (const TreeNode *m = n->subtrees.begin ; m != n->subtrees.end ; ++m) {
	    //os.form("n%xd -> n%xd\n", n, m); // _GNU_ extension
	    //std::fprintf(os, "n%xd -> n%xd\n", n, m);
	    os << "n" << n << " -> n" << m << std::endl;
	    drawTree(os, m);
	}
    } else {
	for (ItemLocation m = n->entries.begin ; m != n->entries.end ; ++m) {
	    //os.form("n%xd -> l%d\n", n, m); // _GNU_ extension
	    //std::fprintf(os, "n%xd -> l%d\n", n, m);
	    os << "n" << n << " -> l" << m << std::endl;
	    //os.form("l%d [shape=\"box\" label=\"", m) << key(*m) << "\"]" << std::endl; // _GNU_ extension
	    //std::fprintf(os, "l%d [shape=\"box\" label=\"l%d\"]\n", m, key(*m)); // _GNU_ extension
	    os << "l" << m << " [shape=\"box\" label=\"l" << key(*m) << "\"]" << std::endl;
	}
    }
}

template <class S, class L, class K, class T>
void PrefixTree<S, L, K, T>::drawTree(std::ostream &os) const {
    os << "digraph G {" << std::endl
       << "ranksep = 1.5" << std::endl
       << "rankdir = LR" << std::endl
       << "node [fontname=\"Helvetica\"]" << std::endl
       << "edge [fontname=\"Helvetica\"]" << std::endl;
    drawTree(os, root);
    os << "}" << std::endl;
}

} // namespace Bliss

