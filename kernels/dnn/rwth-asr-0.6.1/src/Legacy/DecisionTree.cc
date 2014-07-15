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
// $Id: DecisionTree.cc 6678 2007-10-30 13:05:28Z rybach $

#include "DecisionTree.hh"

#include <Core/Application.hh>
#include <Core/Assertions.hh>
#include <Core/MD5.hh>
#include <Core/Utility.hh>
#include <algorithm>
#include <list>
#include <stack>

namespace LegacyDecisionTree {
extern "C" {
#define syserr Core::Application::us()->criticalError
#define error Core::Application::us()->error
    enum BoundaryStyle { noPosDep, posDep, superPosDep };
#include "DecisionTree-legacy.c"
#undef syserr
#undef error
} // extern
} // namespace


using namespace Legacy;
using namespace LegacyDecisionTree;
using Core::tie;

// ===========================================================================
const Core::Choice PhoneticDecisionTreeBase::boundaryStyleChoice(
    "pos-indep", noPosDep,
    "pos-dep", posDep,
    "super-pos-dep", superPosDep,
    Core::Choice::endMark());

const Core::ParameterChoice PhoneticDecisionTreeBase::paramBoundaryStyle(
    "boundary-style",
    &PhoneticDecisionTreeBase::boundaryStyleChoice,
    "word boundary indication convention used in the CART file",
    noPosDep);

PhoneticDecisionTreeBase::PhoneticDecisionTreeBase(
    const Core::Configuration &c,
    Core::Ref<const Bliss::PhonemeInventory> pi)
    :
    Core::Component(c),
    pi_(pi)
{
    require(pi);
    boundaryStyle_ = PhoneticDecisionTree::BoundaryStyle(paramBoundaryStyle(config));
}

// ===========================================================================

struct PhoneticDecisionTree::LightTree :
    public ::LightTree
{};


const Core::ParameterString PhoneticDecisionTree::paramFilename(
    "file",
    "name of (legacy) decision tree (aka CART) file to load");

const Core::ParameterBool PhoneticDecisionTree::paramUseCentralStateClassesOnly(
    "use-central-state-classes-only",
    "remove branches to begin and end state classes in the tree");


PhoneticDecisionTree::PhoneticDecisionTree(const Core::Configuration & config,
		     Am::ClassicStateModelRef stateModel) :
    Core::Component(config),
    PhoneticDecisionTreeBase(config, stateModel->getPhonemeInventory()),
    ClassicStateTying(config, stateModel),
    phonemeMap_(pi_),
    tree_(0),
    classifyDumpChannel_(config, "dump-classify")
{
    require(pi_);
    load(paramFilename(config).c_str());

    Core::Channel dc(config, "dot");
    if (dc.isOpen()) draw(dc);
}

PhoneticDecisionTree::~PhoneticDecisionTree() {
    delete tree_;
}

bool PhoneticDecisionTree::load(const char *filename) {
    require(!tree_);

    Core::MD5 md5;
    if (md5.updateFromFile(filename)) dependency_.setValue(md5);
    else warning("could not derive md5 sum from file '%s'", filename);

    log("reading (legacy) decision tree from file \"%s\" ...", filename);
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
	error("failed to open decision tree file \"%s\"", filename);
	return false;
    }
    log("assuming decision tree uses the %s convention",
	boundaryStyleChoice[boundaryStyle_].c_str());

    tree_ = new LightTree;
    ReadDefFiles(file, tree_, LegacyDecisionTree::BoundaryStyle(boundaryStyle_));
    tree_->root = CreateLightNode();
    tree_->n_Clusters = BuildTree(file, tree_->root);

    fclose(file);

    makePhonemeMapping();

    log("%d phonemes, %d questions, %d clusters",
	tree_->n_Phonems, tree_->n_Questions, tree_->n_Clusters+1);

    if (paramUseCentralStateClassesOnly(config)) removeBeginAndEndBranches();

    return true;
}

void PhoneticDecisionTree::makePhonemeMapping() {
    require(pi_);
    require(tree_);

    phonemeMap_.fill(-1);
    inversePhonemeMap_.resize(tree_->n_Phonems);

    // map CART phomemes
    for (int pho = 0; pho < tree_->n_Phonems; ++pho) {
	// tree_->Boundary is implicitly mapped to Bliss::Phoneme::term
	if (pho == tree_->Boundary) continue;

	const char *sym = tree_->Phonems[pho];
	const Bliss::Phoneme *ph = pi_->phoneme(sym);
	if (!ph) {
	    error("unknown phoneme \"%s\"", sym);
	} else if (phonemeMap_[ph] != -1) {
	    warning("decision tree distinguishes different symbols for same phoneme: /%s/ vs. /%s/",
		    sym, ph->symbol().str());
	} else {
	    phonemeMap_[ph] = pho;
	    inversePhonemeMap_[pho]=ph->id();
	}
    }

    // check for unmapped phonemes
    inversePhonemeMap_.resize(tree_->n_Phonems);
    Bliss::PhonemeInventory::PhonemeIterator pho, pho_end;
    for (Core::tie(pho, pho_end) = pi_->phonemes(); pho != pho_end; ++pho) {
	if (phonemeMap_[*pho] == -1) {
	    error("phoneme \"%s\" not known to decision tree", (*pho)->symbol().str());
	} else{
	    verify(0 <= phonemeMap_[*pho] && phonemeMap_[*pho] < tree_->n_Phonems);
	}
    }
}

inline bool PhoneticDecisionTree::answerQuestion(
    short quest, short phonePosition,
    const Am::Allophone &phone,
    short state, short boundary) const
{
    require(0 <= quest && quest < tree_->n_Questions);
    const Question &q(tree_->Questions[quest]);
    Bliss::Phoneme::Id phi;
    signed char pho;
    switch(q.type) {
    case PhonemClass:
	phi = phone.phoneme(phonePosition);
	if (phi == Bliss::Phoneme::term)
	    pho = tree_->Boundary;
	else
	    pho = phonemeMap_[phi];
	if (pho == -1) {
	    error("phoneme \"%s\" cannot be classified",
		  pi_->phoneme(phone.phoneme(phonePosition))->symbol().str());
	    return false;
	}
	return (q.value.set[pho] == USED);
	break;

    case State:
	return (q.value.state == state);
	break;

    case Position:
	return (q.value.boundary == boundary);
	break;

    default:
	defect();
    }

    defect();
}

inline s16 PhoneticDecisionTree::translateBoundaryFlag(u8 newStyleBoundary) const {
    switch (boundaryStyle_) {
    case noPosDep: {
	return 0;
    } break;
    case posDep: {
	if (newStyleBoundary == 0)
	    return 0;
	else
	    return 1;
    } break;
    case superPosDep: {
	if (newStyleBoundary == 0)
	    return 0; // triphone within a word
	else if (newStyleBoundary == Am::Allophone::isInitialPhone)
	    return 2; // triphone at word begin
	else if (newStyleBoundary == Am::Allophone::isFinalPhone)
	    return 3; // triphone at word end
	else if(newStyleBoundary == (Am::Allophone::isInitialPhone | Am::Allophone::isFinalPhone))
	    return 1; // one phoneme word
	else {
	    error("unknown boundary flag %d", newStyleBoundary);
	    return 0;
	}
    } break;
    }
    defect();
}

inline u32 PhoneticDecisionTree::nClasses() const {
    return tree_->n_Clusters + 1;
}

u32 PhoneticDecisionTree::classify(const Am::AllophoneState &phone) const {
    u32 result;
    if (phonemeMap_[phone.allophone()->phoneme(0)] == tree_->Silence) {
	result = tree_->n_Clusters;
    } else {
	s16 boundary = translateBoundaryFlag(phone.allophone()->boundary);
	LightTreeNode *treenode = tree_->root;
	while ((treenode->leftChild != NULL) && (treenode->rightChild != NULL)) {
	    if (answerQuestion(treenode->question, treenode->context, *phone.allophone(), phone.state(), boundary))
		treenode = treenode->leftChild;
	    else
		treenode = treenode->rightChild;
	}
	result = treenode->question - 1;
    }

    if (classifyDumpChannel_.isOpen()) {
	classifyDumpChannel_ << Core::form(
	    "classify(phone: %s, state: %d, boundary: %d): %d\n",
	    alphabetRef_->toString(phone).c_str(),
	    phone.state(), phone.allophone()->boundary,
	    result);
    }

    ensure(result < nClasses());
    return result;
}

void PhoneticDecisionTree::draw(std::ostream &os) const {
    os << "digraph \"" << fullName() << "\" {" << std::endl
       << "node [fontname=\"Helvetica\"]" << std::endl
       << "edge [fontname=\"Helvetica\"]" << std::endl;

    std::stack<const LightTreeNode*> stack;
    stack.push(tree_->root);
    while (!stack.empty()) {
	const LightTreeNode *node = stack.top(); stack.pop();
	if ((node->leftChild != NULL) && (node->rightChild != NULL)) {
	    const Question &quest(tree_->Questions[node->question]);
	    os << Core::form("n%p [label=\"%d\\n%s\\ncontext: %d\"]\n",
			     (void*) node, node->number, quest.name, node->context)
	       << Core::form("n%p -> n%p [label=\"yes\"]\n",
			     (void*) node, (void*) node->leftChild)
	       << Core::form("n%p -> n%p [label=\"no\"]\n",
			     (void*) node, (void*) node->rightChild);
	    stack.push(node->leftChild);
	    stack.push(node->rightChild);
	} else {
	    os << Core::form("n%p [shape=box label=\"%d\\nclass: %d\"]\n",
			     (void*) node, node->number, node->question - 1);
	}
    }

    os << "}" << std::endl;
}

Core::BinaryTree::TreeStructure PhoneticDecisionTree::treeStructure(void) const {
    Core::BinaryTree::TreeStructure structure;
    std::stack<const LightTreeNode*> stack;
    stack.push(tree_->root);
    while (!stack.empty()) {
	const LightTreeNode *node = stack.top(); stack.pop();
	if ((node->leftChild != NULL) && (node->rightChild != NULL)) { // node
	    structure.push_back(Core::BinaryTree::TreeStructureEntry(node->number, Core::BinaryTree::invalidId));
	    stack.push(node->rightChild);
	    stack.push(node->leftChild);
	} else // leaf
	    structure.push_back(Core::BinaryTree::TreeStructureEntry(node->number, node->question-1));
    }
    return structure;
}




// ---------------------------------------------------------------------------
// PhoneticDecisionTree::phonemeToMixtureIndices()
namespace {

typedef std::list<Bliss::Phoneme::Id> PhonemeSet;
typedef std::list<short> StateSet;
typedef std::list<short> BoundarySet;
struct BuildState {
    const LightTree *tree_;
    const Bliss::PhonemeMap<signed char> *phonemeMap_;
    const LightTreeNode *node_;
    PhonemeSet phone_[3];
    StateSet state_;
    BoundarySet boundary_;

    BuildState() {}

    void reduce(const LightTreeNode *node, bool answer) {
	const Question &q(tree_->Questions[node_->question]);
	switch(q.type) {
	case PhonemClass:
	    for (PhonemeSet::iterator i = phone_[1 + node_->context].begin(); i != phone_[1 + node_->context].end();) {
		if ((q.value.set[*i] == USED) != answer) {
		    PhonemeSet::iterator i_ = i++;
		    phone_[1 + node_->context].erase(i_);
		} else ++i;
	    }
	    break;
	case State:
	    for (StateSet::iterator i = state_.begin(); i != state_.end();) {
		if ((*i == q.value.state) != answer) {
		    StateSet::iterator i_ = i++;
		    state_.erase(i_);
		} else ++i;
	    }
	    break;
	case Position:
	    for (BoundarySet::iterator i = boundary_.begin(); i != boundary_.end();) {
		if ((*i == q.value.boundary) != answer) {
		    BoundarySet::iterator i_ = i++;
		    boundary_.erase(i_);
		} else ++i;
	    }
	    break;
	default:
	    defect();
	}
	node_ = node;
    }

    friend std::ostream& operator<< (std::ostream &o, const BuildState &s) {
	std::cout << "question " << s.node_->question << std::endl;
	std::cout << "context " << s.node_->context << std::endl;
	for (u32 ctx = 0; ctx < 3; ctx++) {
	    o << "context " << ctx << " ";
	    for (PhonemeSet::const_iterator i = s.phone_[ctx].begin(); i != s.phone_[ctx].end(); ++i) o << *i << " ";
	    o << std::endl;
	}
	o << "state ";
	for (StateSet::const_iterator i = s.state_.begin(); i != s.state_.end(); ++i) o << *i << " ";
	o << std::endl;
	o << "boundary ";
	for (BoundarySet::const_iterator i = s.boundary_.begin(); i != s.boundary_.end(); ++i) o << *i << " ";
	o << std::endl;
	return o;
    }
};

} // namespace

std::vector<std::set<u32> > PhoneticDecisionTree::phonemeToMixtureIndices() const {
    // initial build state includes all possibilities
    std::stack<BuildState> states;

    {
	BuildState s;
	s.tree_ = tree_;
	s.phonemeMap_ = &phonemeMap_;
	s.node_ = tree_->root;
	for (short ctx = 0; ctx < 3; ++ctx) {
	    for (int pho = 0; pho < tree_->n_Phonems; ++pho) {
		if (pho == tree_->Boundary)  continue;
		if (pho == tree_->Silence) continue;
		s.phone_[ctx].push_front(pho);
	    }
	    s.state_.push_front(ctx);
	}
	s.boundary_.push_front(0);
	s.boundary_.push_front(1);
	states.push(s);
    }

    std::vector<std::set<u32> > phonemeToMixtureIndices(pi_->nPhonemes()+1);

    while (!states.empty()) {
	BuildState s = states.top();
	states.pop();

	bool isLeaf = (s.node_->leftChild == NULL) && (s.node_->rightChild == NULL);
	if (isLeaf) {
	    for(PhonemeSet::const_iterator p=s.phone_[1].begin(); p!=s.phone_[1].end(); ++p){
		phonemeToMixtureIndices[inversePhonemeMap_[*p]].insert(s.node_->question - 1);
	    }

	} else {
	    BuildState ls = s, rs = s;
	    ls.reduce(s.node_->leftChild, true);
	    rs.reduce(s.node_->rightChild, false);
	    states.push(ls);
	    states.push(rs);
	}
    }

    phonemeToMixtureIndices[inversePhonemeMap_[tree_->Silence]].insert(tree_->n_Clusters);
    return phonemeToMixtureIndices;
}




// -----------------------------------------------------------------------------
// PhoneticDecisionTree::addTree

enum Answer { left=1, yes=1, right=0, no=0, notKnown=2 };


/**
 * The Path class stores a sequence of LightTreeNodes from a root node to a leaf node.
 * A path object is initialized with the path from the root to the leftmost node of the tree.
 * The next() function can be used to traverse the tree.
 */
class Path : public std::vector<LightTreeNode*> {
private:
    Answer lastBranch() {
	return (Answer)((*this)[size()-2]->leftChild == back());
    }
    void extendToLeftmostLeaf() {
	LightTreeNode *node = back()->leftChild;
	while(node != 0) {
	    push_back(node);
	    node = node->leftChild;
	}
    }
public:
    explicit Path(LightTreeNode* root) { push_back(root); extendToLeftmostLeaf(); }
    void next() {
	require(!empty());
	// go back to last node with unseen right subtree
	while ((size() >= 2) && (lastBranch() == right)) pop_back();
	pop_back();
	if (empty()) return;
	// branch into the right subtree
	push_back(back()->rightChild);
	extendToLeftmostLeaf();
    }
};


/**
 * An KnownAnswerSet is feeded with one or more known questions and should
 * answer a given question as precise as possible.
 *
 * In the current implementation, the KnownAnswer class can just answer the
 * questions with that it was feeded.
 * The implementation could be extended however, enabling the answer function
 * to answer e.g. the question 'Is the central phoneme a vocal?'
 * with 'yes', if it knows, that the central phoneme is an 'e'.
 */
class KnownAnswerSet {
private:
    struct KnownQuestion {
	short question_;
	short context_;
	Answer answer_;
	KnownQuestion(short question, short context, Answer answer) :
	    question_(question), context_(context), answer_(answer) {};
    };
    typedef std::vector<KnownQuestion> KnownAnswerBase;
    KnownAnswerBase kab_;
public:
     KnownAnswerSet() {};
    KnownAnswerSet(short question, short context, Answer answer) {
	kab_.push_back(KnownQuestion(question, context, answer));
    }
    KnownAnswerSet(LightTreeNode* node, Answer answer) {
	*this = KnownAnswerSet(node->question, node->context, answer);
    }
    KnownAnswerSet(Path path) {
	for(u32 i=0; i<path.size()-1; i++) {
	    *this += KnownAnswerSet(path[i], (Answer)(path[i]->leftChild == path[i+1]));
	}
    }
    Answer answer(short question, short context) const {
	for(KnownAnswerBase::const_iterator i=kab_.begin(); i!= kab_.end(); i++) {
	    if ((i->question_ == question) && (i->context_ == context))
		return i->answer_;
	}
	return notKnown;
    }
    KnownAnswerSet operator+(const KnownAnswerSet &knownAnswers) const {
	KnownAnswerSet result = *this;
	result.kab_.insert(result.kab_.end(), knownAnswers.kab_.begin(), knownAnswers.kab_.end());
	return result;
    }
    KnownAnswerSet& operator+=(const KnownAnswerSet &knownAnswers) {
	kab_.insert(kab_.end(), knownAnswers.kab_.begin(), knownAnswers.kab_.end());
	return *this;
    }
};


u32 getNumberOfLeaves(LightTreeNode *node) {
    u32 result=0;
    for(Path path(node); !path.empty(); path.next()) { result++; }
    return result;
}


LightTreeNode* copySubTree(const LightTreeNode* node) {
    LightTreeNode *copyNode = new LightTreeNode(*node);
    if (node->leftChild != 0) {
	copyNode->leftChild  = copySubTree(node->leftChild);
	copyNode->rightChild = copySubTree(node->rightChild);
    }
    return copyNode;
}


void deleteSubTree(LightTreeNode* node) {
    if (node->leftChild != 0) {
	deleteSubTree(node->leftChild);
	deleteSubTree(node->rightChild);
    }
    delete node;
}


void overwriteNode(LightTreeNode* node, LightTreeNode* withNode) {
    *node = *withNode;
    delete withNode;
}


void removeKnownQuestions(LightTreeNode* node, const KnownAnswerSet &knownAnswers) {
    if (node->leftChild == 0) return;
    Answer answer = knownAnswers.answer(node->question, node->context);
    switch(answer) {
    case notKnown:
	removeKnownQuestions(node->leftChild, knownAnswers + KnownAnswerSet(node, left));
	removeKnownQuestions(node->rightChild, knownAnswers + KnownAnswerSet(node, right));
	break;
    case yes:
	deleteSubTree(node->rightChild);
	overwriteNode(node, node->leftChild);
	removeKnownQuestions(node, knownAnswers);
	break;
    case no:
	deleteSubTree(node->leftChild);
	overwriteNode(node, node->rightChild);
	removeKnownQuestions(node, knownAnswers);
	break;
    }
}


void PhoneticDecisionTree::addTree(const PhoneticDecisionTree& pdt2,
				   PhoneticDecisionTree::ClassPairs& cp)
{
    cp.clear();
    checkCompatibility(pdt2);

    // traverse the 1st tree
    for(Path path1(tree_->root); !path1.empty(); path1.next()) {
	u32 leafIndex1 = path1.back()->question -1;

	LightTreeNode* root2 = copySubTree(pdt2.tree_->root);
	removeKnownQuestions(root2, KnownAnswerSet(path1));

	// traverse the 2nd tree
	for(Path path2(root2); !path2.empty(); path2.next()) {
	    u32 leafIndex2 = path2.back()->question -1;
	    cp.push_back(ClassPair(leafIndex1, leafIndex2));
	    // renumber leaf index
	    path2.back()->question = cp.size();
	}

	// connect the trees
	overwriteNode(path1.back(), root2);
    }

    // keep the strange silence handling
    cp.push_back(ClassPair(tree_->n_Clusters, pdt2.tree_->n_Clusters));
    tree_->n_Clusters = cp.size()-1;

    log("added tree; resulting tree has %d phonemes, %d questions, %zd clusters",
	tree_->n_Phonems, tree_->n_Questions, cp.size());
}


void PhoneticDecisionTree::removeBeginAndEndBranches()
{
    const int centralSegmentIndex = 1;
    KnownAnswerSet knownAnswers;
    for(int quest=0; quest < tree_->n_Questions; quest++) {
	const Question &q(tree_->Questions[quest]);
	if (q.type == State) {
	    // let central state question be true and
	    // let begin / end state question be false
	    knownAnswers += KnownAnswerSet(quest, 0, (Answer)(q.value.state == centralSegmentIndex));
	}
    }
    removeKnownQuestions(tree_->root, knownAnswers);

    log("removed branches to begin and end states; " \
	"%d remaining clusters", getNumberOfLeaves(tree_->root) + 1);
}


void PhoneticDecisionTree::checkCompatibility(const PhoneticDecisionTree& pdt2)
{
    if (pi_ != pdt2.pi_)
	error("CARTs have different phoneme inventories.");
    if (tree_->n_Phonems != pdt2.tree_->n_Phonems)
	error("CARTs have different number of phonemes.");
    for (u32 pho = 1; pho <= pi_->nPhonemes(); ++pho) {
	if (phonemeMap_[pho] != pdt2.phonemeMap_[pho])
	    error("CARTs have different phoneme maps.");
    }
    if (tree_->n_Questions != pdt2.tree_->n_Questions)
	error("CARTs have different questions.");
}
