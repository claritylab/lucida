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
#include <Core/Application.hh>
#include <Core/Choice.hh>
#include <Core/Parameter.hh>
#include <Core/XmlStream.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Utility.hh>

#include "FlfCore/Basic.hh"
#include "FlfCore/Ftl.hh"
#include "Best.hh"
#include "Cache.hh"
#include "Compose.hh"
#include "Copy.hh"
#include "Determinize.hh"
#include "EpsilonRemoval.hh"
#include "Lexicon.hh"
#include "Map.hh"
#include "NBest.hh"


namespace Flf {

    // -------------------------------------------------------------------------
    void dumpNBest(ConstLatticeRef l, std::ostream &os) {
	if (!l)
	    return;
	for (u32 i = 1, n = l->getState(l->initialStateId())->nArcs(); i <= n; ++n) {
	    os << Core::form("%5d. ", i);
	    dumpBest(partial(l, i), os);
	}
    }
    // -------------------------------------------------------------------------


    /**
     *
     * Collection of fast hash functions taken from http://www.concentric.net/~Ttwang/tech/inthash.htm.
     *

     32 bit Mix Functions

     Based on an original suggestion on Robert Jenkin's part in 1997, I have done some research for a version of the integer hash function. This is my latest version as of January 2007. The specific value of the bit shifts are obtained from running the accompanied search program.

     public int hash32shift(int key)
     {
     key = ~key + (key << 15); // key = (key << 15) - key - 1;
     key = key ^ (key >>> 12);
     key = key + (key << 2);
     key = key ^ (key >>> 4);
     key = key * 2057; // key = (key + (key << 3)) + (key << 11);
     key = key ^ (key >>> 16);
     return key;
     }

     (~x) + y is equivalent to y - x - 1 in two's complement representation.

     By taking advantages of the native instructions such as 'add complement', and 'shift & add', the above hash function runs in 11 machine cycles on HP 9000 workstations.

     Having more rounds will strengthen the hash function by making the result more random looking, but performance will be slowed down accordingly. Simulation seems to prefer small shift amounts for inner rounds, and large shift amounts for outer rounds.
     Robert Jenkins' 32 bit integer hash function

     uint32_t hash( uint32_t a)
     {
     a = (a+0x7ed55d16) + (a<<12);
     a = (a^0xc761c23c) ^ (a>>19);
     a = (a+0x165667b1) + (a<<5);
     a = (a+0xd3a2646c) ^ (a<<9);
     a = (a+0xfd7046c5) + (a<<3);
     a = (a^0xb55a4f09) ^ (a>>16);
     return a;
     }

     This version of integer hash function uses operations with integer constants to help producing a hash value. I suspect the actual values of the magic constants are not very important. Even using 16 bit constants may still work pretty well.

     These magic constants open up the construction of perfect integer hash functions. A test program can vary the magic constants until a set of perfect hashes are found.
     Using Multiplication for Hashing

     Using multiplication requires a mechanism to transport changes from high bit positions to low bit positions. Bit reversal is best, but is slow to implement. A viable alternative is left shifts.

     Using multiplication presents some sort of dilemma. Certain machine platforms supports integer multiplication in hardware, and an integer multiplication can be completed in 4 or less cycles. But on some other platforms an integer multiplication could take 8 or more cycles to complete. On the other hand, integer hash functions implemented with bit shifts perform equally well on all platforms.

     A compromise is to multiply the key with a 'sparse' bit pattern, where on machines without fast integer multiplier they can be replaced with a 'shift & add' sequence. An example is to multiply the key with (4096 + 8 + 1), with an equivalent expression of (key + (key << 3)) + (key << 12).

     On most machines a bit shift of 3 bits or less, following by an addition can be performed in one cycle. For example, Pentium's 'lea' instruction can be used to good effect to compute a 'shift & add' in one cycle.

     Function hash32shiftmult() uses a combination of bit shifts and integer multiplication to hash the input key.

     public int hash32shiftmult(int key)
     {
     int c2=0x27d4eb2d; // a prime or an odd constant
     key = (key ^ 61) ^ (key >>> 16);
     key = key + (key << 3);
     key = key ^ (key >>> 4);
     key = key * c2;
     key = key ^ (key >>> 15);
     return key;
     }

     64 bit Mix Functions

     public long hash64shift(long key)
     {
     key = (~key) + (key << 21); // key = (key << 21) - key - 1;
     key = key ^ (key >>> 24);
     key = (key + (key << 3)) + (key << 8); // key * 265
     key = key ^ (key >>> 14);
     key = (key + (key << 2)) + (key << 4); // key * 21
     key = key ^ (key >>> 28);
     key = key + (key << 31);
     return key;
     }

     The longer width of 64 bits require more mixing than the 32 bit version.
     64 bit to 32 bit Hash Functions

     One such use for this kind of hash function is to hash a 64 bit virtual address to a hash table index. Because the output of the hash function is narrower than the input, the result is no longer one-to-one.

     Another usage is to hash two 32 bit integers into one hash value.

     public int hash6432shift(long key)
     {
     key = (~key) + (key << 18); // key = (key << 18) - key - 1;
     key = key ^ (key >>> 31);
     key = key * 21; // key = (key + (key << 2)) + (key << 4);
     key = key ^ (key >>> 11);
     key = key + (key << 6);
     key = key ^ (key >>> 22);
     return (int) key;
    */


    // -------------------------------------------------------------------------
    /**
     * Taken from Core::PriorityQueue, modified:
     * - allow some insito manipulation of the heap
     *
     * ToTry: Replace Binary heaps by Fibonacci Heaps, see Cormen pp. 476
     *
     **/
    template<class T, class WeakOrder>
    class BinaryPriorityQueue {
    public:
	typedef std::vector<T> Heap;
    private:
	WeakOrder lessThan_;
	Heap heap_; // for the sake of simple index arithmetic the first element in the heap must never be used (aka pseudo sentinel), i.e. heap_[0] == T()
    public:
	BinaryPriorityQueue(const WeakOrder &lessThan = WeakOrder()) :
	    lessThan_(lessThan), heap_(1, T()) {}

	BinaryPriorityQueue(const Heap &heap, const WeakOrder &lessThan = WeakOrder()) :
	    lessThan_(lessThan), heap_(heap) {}

	Heap & heap() { return  heap_; }

	bool empty() const { return heap_.size() == 1; }

	u32 size() const { return heap_.size() - 1; }

	void clear() { heap_.resize(1); }

	bool invariant() const {
	    for (u32 i = 2 ; i < size() ; ++i)
		if (lessThan_(heap_[i], heap_[i / 2]))
		    return false;
	    return true;
	}

	T& top() {
	    require(!empty());
	    verify_(invariant());
	    return heap_[1];
	}

	void pop() {
	    require(!empty());
	    heap_[1] = heap_.back();
	    heap_.pop_back();
	    if (!empty())
		downHeap(1);
	}

	void changeTop(const T &element) {
	    require(!empty());
	    heap_[1] = element;
	    downHeap(1);
	}

	void append(const T &element) {
	    heap_.push_back(element);
	    upHeap(heap_.size() - 1);
	}

	void downHeap(u32 i) {
	    require((1 <= i) && (i < heap_.size()));
	    const T element = heap_[i];
	    u32 j;
	    while (i <= size() / 2) {
		j = 2 * i;
		if ((j < size()) && lessThan_(heap_[j + 1], heap_[j]))
		    j = j + 1;
		if (!lessThan_(heap_[j], element))
		    break;
		heap_[i] = heap_[j];
		i = j;
	    }
	    heap_[i] = element;
	    verify_(invariant());
	}

	void upHeap(u32 i) {
	    require((1 <= i) && (i < heap_.size()));
	    const T element = heap_[i];
	    while ((i > 1) && !lessThan_(heap_[i / 2], element)) {
		heap_[i] = heap_[i / 2];
		i /= 2;
	    }
	    heap_[i] = element;
	    verify_(invariant());
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    /**
     * A reduced binary map (like std::map):
     * - fixed size, all elements specified on construction
     *
     * ToTry:
     * - Replace binary look ups by hash maps (in PrefixTree::Node::arcs, PrefixTree::Node::suffixes, Detour::forks)
     *
     **/
    template<class Key, class Data>
    class BinaryLookUp : protected std::vector<std::pair<Key, Data> > {
	typedef std::vector<std::pair<Key, Data> > Precursor;
    public:
	typedef std::pair<Key, Data> Value;
	typedef std::vector<Value> ValueList;
    protected:
	struct ValueWeakOrder {
	    bool operator() (const Value &v1, const Value &v2) const
		{ return v1.first < v2.first; }
	};
    public:
	BinaryLookUp(const ValueList &values) : Precursor(values) {
	    std::sort(Precursor::begin(), Precursor::end(), ValueWeakOrder());
	}
	const Data & get(const Key &key, const Data &def = Data()) const {
	    if (Precursor::size() <= 8) {
		typename Precursor::const_iterator it = Precursor::begin(), it_end = Precursor::end();
		for (; (it != it_end) && (it->first < key); ++it);
		return ((it == it_end) || (it->first != key)) ? def : it->second;
	    } else {
		s32 l = 0, r = Precursor::end() - Precursor::begin() - 1, m;
		while (l <= r) {
		    m = (s32)((u32)(l + r) >> 1); // = (l + r) / 2;
		    verify_((l <= r) && (0 <= m) && (m < s32(Precursor::size())));
		    const Key &tmp = (Precursor::begin() + m)->first;
		    if (key > tmp)
			l = m + 1;
		    else if (key < tmp)
			r = m - 1;
		    else
			return (Precursor::begin() + m)->second;
		}
		return def;
	    }
	}
    };
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class NBestBuilder {
    public:
	class PrefixTree {
	public:
	    struct Node;
	    struct Arc {
		Node *to;
		Fsa::LabelId label;
		Arc(Fsa::LabelId label) : to(0), label(label) {}
		Arc(Node *to, Fsa::LabelId label) : to(to), label(label) {}
		bool operator< (const Arc &a) const { return label < a.label; }
	    };
	    struct Node {
		typedef std::vector<Arc> ArcList;
		ArcList arcs;
		StateIdList suffixes;
		std::pair<Node*, bool> add(Fsa::LabelId label) {
		    if (label == Fsa::Epsilon)
			return std::make_pair(this, false);
		    ArcList::iterator itArc = std::lower_bound(arcs.begin(), arcs.end(), Arc(label));
		    if ((itArc == arcs.end()) || (itArc->label != label)) {
			Node *to = new Node;
			arcs.insert(itArc, Arc(to, label));
			return std::make_pair(to, true);
		    } else
			return std::make_pair(itArc->to, false);
		}
		bool addSuffix(Fsa::StateId suffixSid) {
		    StateIdList::iterator itSuffix = std::lower_bound(suffixes.begin(), suffixes.end(), suffixSid);
		    if ((itSuffix == suffixes.end()) || (*itSuffix != suffixSid)) {
			suffixes.insert(itSuffix, suffixSid);
			return true;
		    } else
			return false;
		}
	    };
	private:
	    Node *root_;
	public:
	    PrefixTree() :
		root_(new Node) {}
	    ~PrefixTree() {
		std::vector<Node*> S(1, root_);
		while (!S.empty()) {
		    Node *node = S.back(); S.pop_back();
		    for (Node::ArcList::iterator a = node->arcs.begin(), a_end = node->arcs.end(); a != a_end; ++a)
			S.push_back(a->to);
		    delete node;
		}
		root_ = 0;
	    }
	    Node * root() { return root_; }
	};


	class Arc;
	class Node;
	class Path;
	class DissenterList;
	class Detour;


	struct Arc {
	    u32 id;
	    Score score, dissScore;
	    Fsa::LabelId label;
	    Fsa::StateId from, to;
	};
	typedef const Arc * ConstArcPtr;
	typedef std::vector<ConstArcPtr> ConstArcPtrList;

	struct Node {
	    Arc *begin, *end;
	    Score fwdScore, bwdScore;
	    DissenterList *dissenters;
	    Node() :
		begin(0), end(0),
		fwdScore(Core::Type<Score>::max), bwdScore(Core::Type<Score>::max),
		dissenters(0) {}
	    ~Node() { delete dissenters; }
	    bool hasArcs() const { return begin != end; }
	    u32 nArcs() const { return end - begin; }
	};


	class Path {
	public:
	    typedef u32 Number;
	    static const Number InvalidNumber;
	public:
	    Number n;
	    u32 fatherId;
	    Score detourScore;
	    const Arc *dissenter;
	    Path() :
		n(0), fatherId(0), detourScore(0.0), dissenter(0) {}
	    Path(Number n, u32 fatherId, Score detourScore, const Arc *dissenter, bool isDuplicate) :
		n(n), fatherId(fatherId), detourScore(detourScore), dissenter(dissenter) {}
	};
	typedef std::vector<Path> PathList;
	typedef std::vector<u32> PathIdList;


	class DissenterList {
	public:
	    struct WeakOrder {
		bool operator() (const Arc &a1, const Arc &a2) const
		    { return a1.dissScore < a2.dissScore; }
	    };
	private:
	    typedef std::pair<const Arc*, const Arc*> Range;
	    struct RangeWeakOrder {
		bool operator() (const Range &r1, const Range &r2) const
		    { return r1.first->dissScore < r2.first->dissScore; }
	    };
	private:
	    typedef BinaryPriorityQueue<Range, RangeWeakOrder> PriorityQueue;
	    mutable PriorityQueue Q_;
	    mutable ConstArcPtrList list_;
	    PriorityQueue::Heap initialHeap_;
	public:
	    DissenterList() :
		Q_() { initialHeap_ = Q_.heap(); }
	    DissenterList(const Node &node, const DissenterList &next) :
		Q_(next.initialHeap_) {
		verify(node.begin != node.end);
		if (node.begin + 1 != node.end)
		    Q_.append(std::make_pair(node.begin + 1, node.end));
		initialHeap_ = Q_.heap();
	    }
	    const Arc * operator[] (u32 i) const {
		while (i >= list_.size()) {
		    if (!Q_.empty()) {
			Range &r = Q_.top();
			list_.push_back(r.first);
			if (++r.first == r.second)
			    Q_.pop();
			else
			    Q_.downHeap(1);
		    } else
			list_.push_back(0);
		}
		return list_[i];
	    }
	};

	class Detour {
	public:
	    typedef BinaryLookUp<Fsa::StateId, PrefixTree::Node*> ForkLookUp;
	    typedef ForkLookUp::Value Fork;
	    typedef ForkLookUp::ValueList ForkList;
	private:
	    const Fsa::StateId initialNid_;
	    PrefixTree::Node * const initialPrefix_;
	    const u32 pathId_;
	    const Score pathScore_;
	    const DissenterList * const dissenters_;
	    const ForkLookUp forks_;

	    u32 i_;
	    Score score_;
	    PrefixTree::Node *prefix_;
	    const Arc *dissenter_;
	protected:
	    void update() {
		for (dissenter_ = (*dissenters_)[i_]; dissenter_ != 0; dissenter_ = (*dissenters_)[++i_]) {
		    prefix_ = forks_.get(dissenter_->from, 0);
		    if (prefix_) {
			score_ = pathScore_ + dissenter_->dissScore;
			break;
		    }
		}
	    }
	public:
	    Detour(Fsa::StateId initialNid, PrefixTree::Node *initialPrefix, u32 pathId, Score pathScore, const DissenterList *dissenters, const ForkList &forks) :
		initialNid_(initialNid), initialPrefix_(initialPrefix),
		pathId_(pathId), pathScore_(pathScore),
		dissenters_(dissenters), forks_(forks),
		i_(0) { update(); }
	    // const member access
	    Fsa::StateId initialNid() const { return initialNid_; }
	    PrefixTree::Node * initialPrefix() const { return initialPrefix_; }
	    u32 pathId() const { return pathId_; }
	    Score pathScore() const { return pathScore_; }
	    // dynamic, next "detour"
	    bool empty() const { return dissenter_ == 0; }
	    const Arc* dissenter() const { return dissenter_; }
	    Score score() const { return score_; }
	    PrefixTree::Node * prefix() const { return prefix_; }
	    bool next() { verify(!empty()); ++i_; update(); return !empty(); }
	};
	typedef Detour * DetourPtr;
	struct DetourPtrWeakOrder {
	    bool operator() (const DetourPtr d1, const DetourPtr d2) const
		{ return d1->score() < d2->score(); }
	};
	typedef BinaryPriorityQueue<DetourPtr, DetourPtrWeakOrder> DetourPtrQueue;

    private:
	ConstLatticeRef l_;
	bool removeDuplicates_;
	u32 nUniquePaths_;
	PathList paths_;
	PathIdList validPathIds_;

	Arc *arcs_;
	Node *nodes_;
	u32 nodesSize_;
	Score bestScore_;
	PrefixTree prefixTree_;

    private:
	void initialize(bool ignoreNonWords);
	void find(u32 n);
    public:
	NBestBuilder(ConstLatticeRef l, u32 n, bool removeDuplicates = true, bool ignoreNonWords = true) :
	    l_(), removeDuplicates_(removeDuplicates), nUniquePaths_(0) {
	    ensure(l);
	    l_ = persistent(l);
	    nodes_ = 0; arcs_ = 0;
	    nodesSize_ = 0;
	    bestScore_ = Core::Type<Score>::max;
	    initialize(ignoreNonWords);
	    find(n);
	}
	~NBestBuilder() {
	    delete [] arcs_;
	    delete [] nodes_;
	}
	void dumpStatistics(Core::XmlWriter &xml);
	StaticLatticeRef lattice(ScoreId scoreId = Semiring::InvalidId);
    };
    const NBestBuilder::Path::Number NBestBuilder::Path::InvalidNumber = Core::Type<Number>::max;

    /**
     * Calculate
     * - state fwd./bwd. scores
     * - arc dissenter score: additional cost of using arc instead of (locally) best path
     **/
    void NBestBuilder::initialize(bool ignoreNonWords) {
	const Lattice &l = *l_;
	const ScoreList &scales = l.semiring()->scales();
	LabelMapRef nonWordMapRef = ignoreNonWords ?
	    LabelMap::createNonWordToEpsilonMap(Lexicon::us()->alphabetId(l.getInputAlphabet())) :
	    LabelMapRef();
	ConstStateMapRef topSort = sortTopologically(l_);
	nodesSize_ = topSort->maxSid + 1;
	// count arcs
	u32 nArcs = 0;
	for (StateMap::const_iterator itSid = topSort->begin(), endSid = topSort->end(); itSid != endSid; ++itSid)
	    nArcs += l.getState(*itSid)->nArcs();
	// arc scores and labels, fwd. scores
	nodes_ = new Node[nodesSize_];
	Arc *nextArcPtr = arcs_ = new Arc[nArcs];
	nodes_[topSort->front()].fwdScore = 0.0;
	for (StateMap::const_iterator itSid = topSort->begin(), endSid = topSort->end(); itSid != endSid; ++itSid) {
	    Fsa::StateId sid = *itSid;
	    ConstStateRef sr = l.getState(sid);
	    Node &node = nodes_[sid];
	    node.begin = node.end = nextArcPtr;
	    nextArcPtr += sr->nArcs();
	    if (sr->isFinal()) {
		Score score = sr->weight()->project(scales);
		node.fwdScore += score;
		node.bwdScore = score;
		if (node.fwdScore < bestScore_)
		    bestScore_ = node.fwdScore;
	    } else if (!sr->hasArcs())
		Core::Application::us()->criticalError(
		    "N-best: lattice \"%s\" is not trim", l.describe().c_str());
	    u32 arcId = 0;
	    for (State::const_iterator a = sr->begin(), a_end = sr->end(); a != a_end; ++a, ++node.end, ++arcId) {
		Arc &arc = *node.end;
		arc.id = arcId;
		arc.from = sid;
		arc.to = a->target();
		arc.label = nonWordMapRef ? ((*nonWordMapRef)[a->input()].empty() ? a->input() : Fsa::Epsilon) : a->input();
		arc.score = a->weight()->project(scales);
		Score fwdScore = node.fwdScore + arc.score;
		Node &targetNode = nodes_[arc.to];
		if (fwdScore < targetNode.fwdScore)
		    targetNode.fwdScore = fwdScore;
	    }
	    verify(node.end == nextArcPtr);
	}
	verify(u32(nextArcPtr - arcs_) == nArcs);
	// bwd. scores and arc diss. scores
	for (StateMap::const_reverse_iterator itSid = topSort->rbegin(), endSid = topSort->rend(); itSid != endSid; ++itSid) {
	    Fsa::StateId sid = *itSid;
	    Node &node = nodes_[sid];
	    for (Arc *a = node.begin; a != node.end; ++a) {
		Arc &arc = *a;
		Score bwdScore = arc.dissScore = arc.score + nodes_[arc.to].bwdScore;
		if (bwdScore < node.bwdScore)
		    node.bwdScore = bwdScore;
	    }
	    for (Arc *a = node.begin; a != node.end; ++a)
		a->dissScore -= node.bwdScore;
	}
	verify(Core::isAlmostEqualUlp(f32(bestScore_), f32(nodes_[topSort->front()].bwdScore), 100));
    }

    void NBestBuilder::find(u32 n) {
	Fsa::StateId initialSid = l_->initialStateId();
	if ((initialSid == Fsa::InvalidStateId) || (!nodes_[initialSid].hasArcs()))
	    return;

	//u32 maxPaths = std::max(u32(n * 100), u32(10000));
	u32 maxPaths = 1000000;

	StateIdList S;
	Detour::ForkList F;
	DetourPtrQueue Q;
	paths_.reserve(n);
	validPathIds_.reserve(n);
	// add first best, i.e. initialize
	{
	    paths_.push_back(Path(1, Core::Type<u32>::max, 0.0, 0, false));
	    validPathIds_.push_back(0);
	    nUniquePaths_ = 1;
	    S.push_back(initialSid);
	    PrefixTree::Node *prefix = prefixTree_.root(), *initialPrefix = prefixTree_.root();
	    DissenterList *dissenters = 0;
	    for (;;) {
		Fsa::StateId nid = S.back();
		Node &node = nodes_[nid];
		if (!node.hasArcs()) {
		    dissenters = node.dissenters = new DissenterList;
		    break;
		}
		prefix->addSuffix(nid);
		F.push_back(Detour::Fork(nid, prefix));
		std::sort(node.begin, node.end, DissenterList::WeakOrder());
		const Arc &arc = *node.begin;
		verify(arc.dissScore == 0.0);
		S.push_back(arc.to);
		prefix = prefix->add(arc.label).first;
	    }
	    for (StateIdList::const_reverse_iterator itNid = S.rbegin() + 1, endNid = S.rend(); itNid != endNid; ++itNid) {
		Node &node = nodes_[*itNid];
		dissenters = node.dissenters = new DissenterList(node, *dissenters);
	    }
	    Detour *detour = new Detour(initialSid, initialPrefix, 0, 0.0, dissenters, F);
	    S.clear(); F.clear();
	    if (!detour->empty())
		Q.append(detour);
	    else
		delete detour;
	}
	// add remaining n-1 best
	Score lastPathScore = 0.0;
	while ((validPathIds_.size() < n) && !Q.empty() && (paths_.size() < maxPaths)) {
	    Detour *detour = Q.top();
	    verify(!detour->empty());
	    const Score pathScore = detour->score();
	    const Arc *dissenter = detour->dissenter();
	    std::pair<PrefixTree::Node*, bool> prefix = std::make_pair(detour->prefix(), false);
	    bool isUnique = false, sharesSuffix = false;
	    // leave father path
	    prefix = prefix.first->add(dissenter->label);
	    if (prefix.second) isUnique = true;
	    // add rest of new path, i.e. best path starting from dissenter
	    PrefixTree::Node *newPrefix = prefix.first;
	    DissenterList *newDissenters = 0;
	    S.push_back(dissenter->to);
	    for (;;) {
		Fsa::StateId nid = S.back();
		Node &node = nodes_[nid];
		if (node.dissenters) {
		    newDissenters = node.dissenters;
		    break;
		}
		if (!node.hasArcs()) {
		    newDissenters = node.dissenters = new DissenterList;
		    break;
		}
		if (!sharesSuffix)
		    sharesSuffix = !prefix.first->addSuffix(nid);
		else
		    verify_(!isUnique);
		if (!sharesSuffix || !removeDuplicates_)
		    F.push_back(Detour::Fork(nid, prefix.first));
		std::sort(node.begin, node.end, DissenterList::WeakOrder());
		const Arc &arc = *node.begin;
		verify(arc.dissScore == 0.0);
		S.push_back(arc.to);
		prefix = prefix.first->add(arc.label);
		if (prefix.second) isUnique = true;
	    }
	    for (StateIdList::const_reverse_iterator itNid = S.rbegin() + 1, endNid = S.rend(); itNid != endNid; ++itNid) {
		Node &node = nodes_[*itNid];
		newDissenters = node.dissenters = new DissenterList(node, *newDissenters);
	    }
	    if (!sharesSuffix || !removeDuplicates_) {
		for (Fsa::StateId nid = S.back();;) {
		    Node &node = nodes_[nid];
		    if (!node.hasArcs())
			break;
		    if (!prefix.first->addSuffix(nid)) {
			verify(!isUnique);
			sharesSuffix = true;
			if (removeDuplicates_) break;
		    }
		    F.push_back(Detour::Fork(nid, prefix.first));
		    const Arc &arc = *node.begin;
		    verify(arc.dissScore == 0.0);
		    prefix = prefix.first->add(arc.label);
		    if (prefix.second) isUnique = true;
		    nid = arc.to;
		}
	    } else
		verify_(!isUnique);
	    // determine path number and uniqueness; update counts
	    Path::Number n = Path::InvalidNumber;
	    u32 pathId = paths_.size();
	    if (isUnique || !removeDuplicates_ ) {
		validPathIds_.push_back(pathId);
		n = validPathIds_.size();
		if (isUnique) ++nUniquePaths_;
	    } else
		verify_(!isUnique);
	    paths_.push_back(Path(n, detour->pathId(), pathScore, dissenter, !isUnique));
	    Detour *newDetour = new Detour(dissenter->to, newPrefix, pathId, pathScore, newDissenters, F);
	    S.clear(); F.clear();
	    detour->next();
	    if (!detour->empty() && !newDetour->empty()) {
		Q.downHeap(1);
		Q.append(newDetour);
	    } else if (!detour->empty()) {
		Q.downHeap(1);
		delete newDetour;
	    } else if (!newDetour->empty()) {
		Q.changeTop(newDetour);
		delete detour;
	    } else {
		Q.pop();
		delete detour;
		delete newDetour;
	    }
	    verify(lastPathScore <= pathScore);
	    lastPathScore = pathScore;

	}
	if (paths_.size() == maxPaths)
	    Core::Application::us()->warning(
		"Did not find %d valid paths after searching %d paths; give up.", n, maxPaths);
	for (DetourPtrQueue::Heap::iterator itDetourPtr = Q.heap().begin() + 1, endDetourPtr = Q.heap().end(); itDetourPtr != endDetourPtr; ++itDetourPtr)
	    delete *itDetourPtr;
	Q.clear();
    }

    StaticLatticeRef NBestBuilder::lattice(ScoreId scoreId) {

	// dbg(3);

	u32 n = validPathIds_.size();
	const Lattice &l = *l_;
	const Semiring &semiring = *l_->semiring();
	const Boundaries &b = *l_->getBoundaries();
	StaticLattice *s = new StaticLattice;
	s->setDescription(Core::form("nbest(%s,%d)", l.describe().c_str(), n));
	s->setType(l.type());
	s->addProperties(Fsa::PropertyAcyclic | Fsa::TypeTransducer);
	s->setInputAlphabet(l.getInputAlphabet());
	if (l.type() != Fsa::TypeAcceptor)
	    s->setOutputAlphabet(l.getOutputAlphabet());
	s->setSemiring(l.semiring());
	StaticBoundaries *sb = 0;
	if (b.valid()) {
	    sb = new StaticBoundaries;
	    s->setBoundaries(ConstBoundariesRef(sb));
	}
	Fsa::StateId internalInitialSid = l.initialStateId();
	State *root = new State(0);
	s->setState(root);
	s->setInitialStateId(0);
	if (sb)
	    sb->set(0, Boundary(0));
	// empty
	if (n == 0) {
	    root->setFinal(semiring.one());
	    return StaticLatticeRef(s);
	}
	Fsa::StateId nextSid = n + 1;
	std::vector<Fsa::StateId> lattice2internal(n + 1, Fsa::InvalidStateId);
	std::vector<std::pair<Fsa::StateId, ScoresRef> > suffix2lattice(nodesSize_, std::make_pair(Fsa::InvalidStateId, ScoresRef()));
	// add best; prepare suffix representation
	{
	    State *sp = new State(1);
	    lattice2internal[1] = internalInitialSid;
	    s->setState(sp);
	    if (sb)
		sb->set(sp->id(), b.get(internalInitialSid));
	    Fsa::StateId firstSuffixSid = nextSid;
	    ScoresRef suffixSum;
	    for (Fsa::StateId internalSid = internalInitialSid, internalTargetSid;; internalSid = internalTargetSid) {
		const Node &node = nodes_[internalSid];
		internalTargetSid = node.begin->to;
		ConstStateRef sr = l.getState(internalSid);
		verify_(lattice2internal[sp->id()] == sr->id());
		const Flf::Arc &a = *(sr->begin() + node.begin->id);
		verify(a.target() == internalTargetSid);
		suffix2lattice[sr->id()] = std::make_pair(sp->id(), a.weight());
		sp->newArc(nextSid, a.weight(), a.input(), a.output());
		sp = new State(nextSid++);
		s->setState(sp);
		if (sb)
		    sb->set(sp->id(), b.get(internalTargetSid));
		lattice2internal.push_back(internalTargetSid);
		if (!nodes_[internalTargetSid].hasArcs()) {
		    ConstStateRef sr = l.getState(internalTargetSid);
		    verify_(lattice2internal[sp->id()] == sr->id());
		    sp->setFinal(sr->weight());
		    suffix2lattice[sr->id()] = std::make_pair(sp->id(), sr->weight());
		    suffixSum = sr->weight();
		    break;
		}
	    }
	    for (u32 suffixSid = nextSid - 2; suffixSid >= firstSuffixSid; --suffixSid) {
		std::pair<Fsa::StateId, ScoresRef> &suffix2latticeElement = suffix2lattice[lattice2internal[suffixSid]];
		suffixSum = suffix2latticeElement.second = semiring.extend(suffix2latticeElement.second, suffixSum);
	    }
	    std::pair<Fsa::StateId, ScoresRef> &suffix2latticeElement = suffix2lattice[lattice2internal[1]];
	    suffixSum = suffix2latticeElement.second = semiring.extend(suffix2latticeElement.second, suffixSum);
	    if (scoreId != Semiring::InvalidId) {
		suffixSum = semiring.clone(suffixSum);
		suffixSum->set(scoreId, bestScore_);
	    }
	    root->newArc(1, suffixSum, Fsa::Epsilon, Fsa::Epsilon);
	}
	// add the n-1 "detours"
	// copy father prefix, add all disssenters, suffix - join asap
	ConstArcPtrList dissenters;
	for (PathIdList::const_iterator itPathId = validPathIds_.begin() + 1; itPathId != validPathIds_.end(); ++itPathId) {
	    const Path &path = paths_[*itPathId];
	    verify(path.n != Path::InvalidNumber);
	    dissenters.push_back(path.dissenter);
	    u32 fatherPathId = path.fatherId;
	    for (;;) {
		const Path &fatherPath = paths_[fatherPathId];
		if (fatherPath.n == Path::InvalidNumber) {
		    dissenters.push_back(fatherPath.dissenter);
		    fatherPathId = fatherPath.fatherId;
		} else
		    break;
	    }
	    // copy common prefix: father*
	    ScoresRef sum = semiring.one();
	    const State *fatherSp = s->fastState(paths_[fatherPathId].n);
	    Fsa::StateId fatherInternalSid = lattice2internal[fatherSp->id()];
	    State *sp = new State(path.n);
	    s->setState(sp);
	    if (sb) {
		Boundary boundary = sb->get(fatherSp->id());
		sb->set(sp->id(), boundary);
	    }
	    lattice2internal[path.n] = fatherInternalSid;
	    Fsa::StateId prefixEndSid = dissenters.back()->from;
	    while (fatherInternalSid != prefixEndSid) {
		const Flf::Arc &a = *fatherSp->begin();
		sp->newArc(nextSid, a.weight(), a.input(), a.output());
		sum = semiring.extend(sum, a.weight());
		fatherSp = s->fastState(a.target());
		fatherInternalSid = lattice2internal[fatherSp->id()];
		sp = new State(nextSid++);
		s->setState(sp);
		if (sb) {
		    Boundary boundary = sb->get(fatherSp->id());
		    sb->set(sp->id(), boundary);
		}
		lattice2internal.push_back(fatherInternalSid);
		verify_(lattice2internal[sp->id()] == fatherInternalSid);
	    }
	    // add middle part: dissenter, (best*, dissenter)*
	    verify_(lattice2internal[sp->id()] == dissenters.back()->from);
	    for (ConstArcPtrList::const_reverse_iterator itDissenter = dissenters.rbegin(), endDissenter = dissenters.rend();;) {
		const Arc &dissenter = **itDissenter; ++itDissenter;
		ConstStateRef sr = l.getState(dissenter.from);
		const Flf::Arc &a = *(sr->begin() + dissenter.id);
		verify_(a.target() == dissenter.to);
		if ((itDissenter == endDissenter) && (suffix2lattice[dissenter.to].first != Fsa::InvalidStateId)) {
		    std::pair<Fsa::StateId, ScoresRef> &suffix2latticeElement = suffix2lattice[dissenter.to];
		    sp->newArc(suffix2latticeElement.first, a.weight(), a.input(), a.output());
		    sum = semiring.extend(semiring.extend(sum, a.weight()), suffix2latticeElement.second);
		    sp = 0;
		    break;
		}
		sp->newArc(nextSid, a.weight(), a.input(), a.output());
		sum = semiring.extend(sum, a.weight());
		sp = new State(nextSid++);
		s->setState(sp);
		if (sb)
		    sb->set(sp->id(), b.get(dissenter.to));
		lattice2internal.push_back(dissenter.to);
		verify_(lattice2internal[sp->id()] == dissenter.to);
		if (itDissenter == endDissenter)
		    break;
		for (Fsa::StateId internalSid = dissenter.to, internalTargetSid, endSid = (*itDissenter)->from;
		     internalSid != endSid; internalSid = internalTargetSid) {
		    const Node &node = nodes_[internalSid];
		    internalTargetSid = node.begin->to;
		    ConstStateRef sr = l.getState(internalSid);
		    verify_(lattice2internal[sp->id()] == sr->id());
		    const Flf::Arc &a = *(sr->begin() + node.begin->id);
		    verify_(a.target() == internalTargetSid);
		    sp->newArc(nextSid, a.weight(), a.input(), a.output());
		    sum = semiring.extend(sum, a.weight());
		    sp = new State(nextSid++);
		    s->setState(sp);
		    if (sb)
			sb->set(sp->id(), b.get(internalTargetSid));
		    lattice2internal.push_back(internalTargetSid);
		    verify_(lattice2internal[sp->id()] == internalTargetSid);
		}
	    }
	    // add suffix part/ join suffix: best+
	    if (sp) {
		verify_(lattice2internal[sp->id()] == dissenters.front()->to);
		Fsa::StateId firstSuffixSid = sp->id(), lastSuffixSid = 0;
		ScoresRef suffixSum;
		for (Fsa::StateId internalSid = dissenters.front()->to, internalTargetSid;; internalSid = internalTargetSid) {
		    const Node &node = nodes_[internalSid];
		    internalTargetSid = node.begin->to;
		    ConstStateRef sr = l.getState(internalSid);
		    verify_(lattice2internal[sp->id()] == sr->id());
		    const Flf::Arc &a = *(sr->begin() + node.begin->id);
		    verify_(a.target() == internalTargetSid);
		    suffix2lattice[internalSid] = std::make_pair(sp->id(), a.weight());
		    std::pair<Fsa::StateId, ScoresRef> &suffix2latticeTarget = suffix2lattice[internalTargetSid];
		    if (suffix2latticeTarget.first != Fsa::InvalidStateId) {
			sp->newArc(suffix2latticeTarget.first, a.weight(), a.input(), a.output());
			suffixSum = suffix2latticeTarget.second;
			lastSuffixSid = nextSid - 1;
			break;
		    }
		    sp->newArc(nextSid, a.weight(), a.input(), a.output());
		    sp = new State(nextSid++);
		    s->setState(sp);
		    if (sb)
			sb->set(sp->id(), b.get(internalTargetSid));
		    lattice2internal.push_back(internalTargetSid);
		    verify_(lattice2internal[sp->id()] == internalTargetSid);
		    if (!nodes_[internalTargetSid].hasArcs()) {
			ConstStateRef sr = l.getState(internalTargetSid);
			sp->setFinal(sr->weight());
			suffix2lattice[internalTargetSid] = std::make_pair(sp->id(), sr->weight());
			suffixSum = sr->weight();
			lastSuffixSid = nextSid - 2;
			break;
		    }
		}
		for (u32 suffixSid = lastSuffixSid; suffixSid >= firstSuffixSid; --suffixSid) {
		    std::pair<Fsa::StateId, ScoresRef> &suffix2latticeElement = suffix2lattice[lattice2internal[suffixSid]];
		    suffixSum = suffix2latticeElement.second = semiring.extend(suffix2latticeElement.second, suffixSum);
		}
		sum = semiring.extend(sum, suffixSum);
	    }
	    if (scoreId != Semiring::InvalidId) {
		sum = semiring.clone(sum);
		sum->set(scoreId, bestScore_ + path.detourScore);
	    }
	    root->newArc(path.n, sum, Fsa::Epsilon, Fsa::Epsilon);
	    // clean up
	    dissenters.clear();
	}
	return StaticLatticeRef(s);
    }

    void NBestBuilder::dumpStatistics(Core::XmlWriter &xml) {
	xml << Core::XmlFull("best-paths", validPathIds_.size())
	    << Core::XmlFull("searched-paths", paths_.size())
	    << Core::XmlFull("unique-paths", nUniquePaths_)
	    << Core::XmlFull("ratio", f32(paths_.size()) / f32(nUniquePaths_));
    }
    /*
    ConstLatticeRef nbest(ConstLatticeRef l, u32 n, bool removeDuplicates, bool ignoreNonWords, ScoreId scoreId) {
	NBestBuilder builder(l, n, removeDuplicates, ignoreNonWords);
	return builder.lattice(scoreId);
    }
    */
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    ConstLatticeRef nbest(ConstLatticeRef l, u32 n, bool removeDuplicates, NBestAlgorithm algo) {
	bool ignoreNonWords = false;
	if (removeDuplicates)
	    switch (Lexicon::us()->alphabetId(l->getInputAlphabet())) {
	    case Lexicon::LemmaAlphabetId:
	    case Lexicon::LemmaPronunciationAlphabetId:
		ignoreNonWords = true;
	    default: ;
	    }
	switch (algo) {
	case Eppstein: {
	    NBestBuilder builder(l, n, removeDuplicates, ignoreNonWords);
	    return builder.lattice();
	}
	case Mori: {
	    if (removeDuplicates) {
		if (ignoreNonWords)
		    l = applyOneToOneLabelMap(
			l, LabelMap::createNonWordToEpsilonMap(
			    Lexicon::us()->alphabetId(l->getInputAlphabet())));
		l = fastRemoveEpsilons(projectOutput(l));
		l = cache(l, 75000);
		l = determinize(l);
		l = cache(l, 75000);
	    }
	    return Flf::nbest(l, n);
	}
	default:
	    defect();
	    return ConstLatticeRef();
	}
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    namespace {
	const Core::Choice choiceNBestAlgorithm(
	    "eppstein", Eppstein,
	    "more",     Mori,
	    Core::Choice::endMark());
	const Core::ParameterString paramNBestAlgorithm(
	    "algorithm",
	    "n-best algorithm",
	    "eppstein");
	NBestAlgorithm getNBestAlgorithm(const Core::Configuration &config) {
	    Core::Choice::Value nBestChoice = choiceNBestAlgorithm[paramNBestAlgorithm(config)];
	    if (nBestChoice == Core::Choice::IllegalValue)
		Core::Application::us()->criticalError(
		    "NBestAlgorithm: Unknown algorithm \"%s\"",
		    paramNBestAlgorithm(config).c_str());
	    return NBestAlgorithm(nBestChoice);
	}
    } // namespace

    class NBestNode : public FilterNode {
	typedef FilterNode Precursor;
    public:
	static const Core::ParameterInt paramN;
	static const Core::ParameterBool paramRemoveDuplicates;
	static const Core::ParameterBool paramIgnoreNonWords;
	static const Core::ParameterString paramScoreKey;

    private:
	Core::XmlChannel statisticsChannel_;
	u32 n_;
	bool removeDuplicates_;
	bool ignoreNonWords_;
	Key scoreKey_;

	ConstLatticeRef nBestL_;
	bool isValid_;

    protected:
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (!isValid_) {
		ScoreId scoreId = Semiring::InvalidId;
		if (!scoreKey_.empty()) {
		    scoreId = l->semiring()->id(scoreKey_);
		    if (scoreId == Semiring::InvalidId)
			criticalError(
			    "Semiring \"%s\" has no dimension \"%s\".",
			    l->semiring()->name().c_str(), scoreKey_.c_str());
		}
		NBestBuilder builder(l, n_, removeDuplicates_, ignoreNonWords_);
		nBestL_ = builder.lattice(scoreId);
		statisticsChannel_ << Core::XmlOpen("statistics")
		    + Core::XmlAttribute("component", name);
		builder.dumpStatistics(statisticsChannel_);
		statisticsChannel_ << Core::XmlClose("statistics");
		isValid_ = true;
	    }
	    return nBestL_;
	}

    public:
	NBestNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), statisticsChannel_(config, "statistics") {}
	virtual ~NBestNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    n_ = paramN(config);
	    verify(n_ >= 1);
	    if (n_ == 1)
		warning("N-Best list generation is called with n=1; did you forget to specify n?");
	    removeDuplicates_ = paramRemoveDuplicates(config);
	    ignoreNonWords_ = paramIgnoreNonWords(config);
	    scoreKey_ = paramScoreKey(config);
	    Core::Component::Message msg = log();
	    msg << "NBest configuration:\n"
		<< "- n=" << n_ << "\n";
	    if (removeDuplicates_)
		msg << "- Remove duplicates from n-best list.\n";
	    if (ignoreNonWords_)
		msg << "- Ignore non words, i.e. treat two hypothesis as duplicates, if they only differ in non words.\n";
	    if (!scoreKey_.empty())
		msg << "- Store hypotheses score in dimension \"" << scoreKey_ << "\".\n";
	    isValid_ = false;
	}
	virtual void sync() {
	    nBestL_.reset();
	    isValid_ = false;
	}
    };
    const Core::ParameterInt NBestNode::paramN(
	"n",
	"(up to) n best hypothesis",
	1);
    const Core::ParameterBool NBestNode::paramRemoveDuplicates(
	"remove-duplicates",
	"remove duplicate pathes from the n-best list",
	true);
    const Core::ParameterBool NBestNode::paramIgnoreNonWords(
	"ignore-non-words",
	"ignore non words, i.e. treat two pathes as duplicates, if they only differ in non words",
	true);
    const Core::ParameterString NBestNode::paramScoreKey(
	"score-key",
	"store path score in this dimension",
	"");

    NodeRef createNBestNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new NBestNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class SelectNBestNode : public Node {
    private:
	ConstLatticeRef nBest_;
	ConstStateRef initial_;
	u32 n_;
    public:
	SelectNBestNode(const std::string &name, const Core::Configuration &config) :
	    Node(name, config) {}
	virtual ~SelectNBestNode() {}

	virtual void init(const std::vector<std::string> &arguments) {
	    if (!connected(0))
		criticalError("N-best lattice at port 0 required.");
	    n_ = 0;
	}

	virtual ConstLatticeRef sendLattice(Port to) {
	    if (!nBest_) {
		nBest_ = requestLattice(0);
		if (nBest_) {
		    initial_ = nBest_->getState(nBest_->initialStateId());
		    n_ = initial_->nArcs();
		} else
		    n_ = 0;
	    }
	    if (to < n_) {
		ConstLatticeRef l = partial(nBest_, (initial_->begin() + to)->target());
		l->addProperties(Fsa::PropertyAcyclic | Fsa::PropertyLinear);
		return l;
	    } else
		return ConstLatticeRef();
	}

	virtual void sync() {
	    nBest_.reset();
	    initial_.reset();
	    n_ = 0;
	}
    };
    NodeRef createSelectNBestNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new SelectNBestNode(name, config));
    }
    // -------------------------------------------------------------------------


    // -------------------------------------------------------------------------
    class DumpNBestNode : public FilterNode {
	typedef FilterNode Precursor;
    public:
	static const Core::ParameterStringVector paramScoreKeys;
    private:
	Core::Channel dump_;
	KeyList scoreKeys_;
    protected:
	void dumpNBest(ConstLatticeRef l, std::ostream &os) {
	    printSegmentHeader(os, connected(1) ? requestSegment(1) : ConstSegmentRef(), "# ");
	    if (!l || (l->initialStateId() == Fsa::InvalidStateId)) {
		os << "# n=0" << std::endl;
		return;
	    }
	    Fsa::ConstAlphabetRef alphabet = l->getInputAlphabet();
	    ConstSemiringRef semiring = l->semiring();
	    ScoreIdList scoreIds;
	    if (scoreKeys_.empty()) {
		scoreIds.resize(semiring->size());
		for (ScoreId scoreId = 0; scoreId < semiring->size(); ++scoreId) scoreIds[scoreId] = scoreId;
	    } else {
		for (KeyList::const_iterator itKey = scoreKeys_.begin(); itKey != scoreKeys_.end(); ++itKey) {
		    scoreIds.push_back(semiring->id(*itKey));
		    if (scoreIds.back() == Semiring::InvalidId)
			criticalError("DumpTracebackNode: Dimension \"%s\" does not exist.",
				      itKey->c_str());
		}
	    }
	    ConstStateRef begin = l->getState(l->initialStateId());
	    os << "# n=" << begin->nArcs() << std::endl;
	    os << "# ";
	    for (ScoreIdList::const_iterator itId = scoreIds.begin(); itId != scoreIds.end(); ++itId)
		os << semiring->key(*itId) << "/" << semiring->scale(*itId) << "\t";
	    os << "N";
	    os << std::endl;
	    std::vector<Fsa::LabelId> labels;
	    for (State::const_iterator a = begin->begin(), a_end = begin->end(); a != a_end; ++a) {
		ScoresRef scores = a->weight();
		State::const_iterator arc = a;
		for (;;) {
		    if (arc->input() != Fsa::Epsilon)
			labels.push_back(arc->input());
		    ConstStateRef sr = l->getState(arc->target());
		    if (sr->hasArcs()) {
			if (sr->nArcs() > 1)
			    criticalError("DumpNBestNode: \"%s\" is not an n-best-list",
					  l->describe().c_str());
			arc = sr->begin();
		    } else
			break;
		}
		for (ScoreIdList::const_iterator itId = scoreIds.begin(); itId != scoreIds.end(); ++itId)
		    os << Core::form(" %.4f", -scores->get(*itId)) << "\t";
		os << labels.size() << "\t";
		os << "<s>";
		for (std::vector<Fsa::LabelId>::const_iterator it = labels.begin(), it_end = labels.end(); it != it_end; ++it)
		    os << " " << alphabet->symbol(*it);
		os << " </s>";
		os << std::endl;
		labels.clear();
	    }
	    os << std::endl;
	}
	virtual ConstLatticeRef filter(ConstLatticeRef l) {
	    if (dump_.isOpen())
		dumpNBest(l, dump_);
	    else
		dumpNBest(l, clog());
	    return l;
	}
    public:
	DumpNBestNode(const std::string &name, const Core::Configuration &config) :
	    Precursor(name, config), dump_(config, "dump") {}
	virtual ~DumpNBestNode() {}
	virtual void init(const std::vector<std::string> &arguments) {
	    scoreKeys_ = paramScoreKeys(config);
	}
    };
    const Core::ParameterStringVector DumpNBestNode::paramScoreKeys(
	"scores",
	"dimension of scores to be dumped; default is all scores",
	"");
    NodeRef createDumpNBestNode(const std::string &name, const Core::Configuration &config) {
	return NodeRef(new DumpNBestNode(name, config));
    }
    // -------------------------------------------------------------------------

} // namespace Flf
