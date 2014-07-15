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
#include "DecisionTreeTrainer.hh"
#include "Parser.hh"

#include <ctime>

#include <Core/CompressedStream.hh>
#include <Core/PriorityQueue.hh>
#include <Core/Parameter.hh>
#include <Core/XmlStream.hh>


//define dbg(message) std::cerr << __FILE__ << ":" <<__LINE__ << "\t" << message << std::endl

using namespace Cart;


// ============================================================================
void ID3::write(std::ostream &os) const {
    os << "ID3\n";
}

Score ID3::entropy(ExamplePtrList::const_iterator begin, ExamplePtrList::const_iterator end) const {
    Score entropy = 0.0;
    u32 nObs = 0;
    for (ExamplePtrList::const_iterator it = begin; it != end; ++it) {
	const Example & example = **it;
	CountMap::iterator itt = map_.find(example.values);
	if (itt == map_.end())
	    map_.insert(CountMap::value_type(example.values, example.nObs));
	else
	    itt->second += example.nObs;
	nObs += example.nObs;
    }
    if (nObs == 0)
	return 0.0;

    Score denominator = nObs;
    for (CountMap::const_iterator it = map_.begin(); it != map_.end(); ++it) {
	Score p = it->second;
	if (p > 0.0) {
	    p /= denominator;
	    entropy -= p * ::log10(p);
	}
    }
    entropy /= ::log10(2.0); // -> log2
    map_.clear();
    return entropy;
}

void ID3::operator()(const ExamplePtrRange &examples, Score& score) const {
    score = entropy(examples.begin, examples.end);
}

Score ID3::operator()(
    const ExamplePtrRange &leftExamples, const ExamplePtrRange &rightExamples,
    const Score fatherEntropy,
    Score &leftChildEntropy, Score &rightChildEntropy) const {
    leftChildEntropy = entropy(leftExamples.begin, leftExamples.end);
    rightChildEntropy = entropy(rightExamples.begin, rightExamples.end);
    Score
	nObs = leftExamples.size() + rightExamples.size(),
	leftNObs = leftExamples.size(),
	rightNObs = rightExamples.size();
    return fatherEntropy - (leftNObs / nObs) * leftChildEntropy - (rightNObs / nObs) * rightChildEntropy;
}
// ============================================================================


// ============================================================================
void DecisionTreeTrainer::TrainingPlan::Step::write(std::ostream & os) const {
    os << "step \"" << name << "\":" << std::endl
       << "action    : " << action  << std::endl
       << "min-obs   : " << minObs  << std::endl
       << "min-gain  : " << minGain << std::endl;
    if (nRandomQuestion > 1)
	os << "randomize : " << nRandomQuestion << std::endl;
    Cart::write(os, *(questionRefs));
}

void DecisionTreeTrainer::TrainingPlan::Step::writeXml(Core::XmlWriter & xml) const {
    xml << (Core::XmlOpen("step")
	    + Core::XmlAttribute("name", name)
	    + Core::XmlAttribute("action", action))
	<< Core::XmlFull("min-obs", minObs)
	<< Core::XmlFull("min-gain", minGain);
    if (nRandomQuestion > 1)
	xml << (Core::XmlEmpty("randomize")
		+ Core::XmlAttribute("nQuestion", nRandomQuestion));
    Cart::writeXml(xml, *(questionRefs));
    xml << Core::XmlClose("step");
}

void DecisionTreeTrainer::TrainingPlan::write(std::ostream & os) const {
    os << "max-leaves: " << maxLeaves << std::endl;
    map->write(os);
    for (TrainingPlan::StepPtrList::const_iterator it = steps.begin();
	 it != steps.end(); ++it) (*it)->write(os);
}

void DecisionTreeTrainer::TrainingPlan::writeXml(Core::XmlWriter & xml) const {
    xml << Core::XmlOpen("decision-tree-training");
    map->writeXml(xml);
    if (maxLeaves != Core::Type<u32>::max)
	xml << Core::XmlFull("max-leaves", maxLeaves);
    for (TrainingPlan::StepPtrList::const_iterator it = steps.begin();
	 it != steps.end(); ++it) (*it)->writeXml(xml);
    xml << Core::XmlClose("decision-tree-training");
}
// ============================================================================


// ============================================================================
namespace Cart {
    class Training :
	public Core::Component {
    private:
	typedef DecisionTree::QuestionId QuestionId;
	typedef std::vector<QuestionId> QuestionIdList;

	typedef DecisionTree::ClassId ClassId;

	typedef DecisionTreeTrainer::Information Information;
	typedef DecisionTreeTrainer::TrainingPlan TrainingPlan;


	/**
	   Collection of some information about
	   the usage of a specific question in
	   the decision tree.
	*/
	struct QuestionStatistic {
	    u32 count;
	    Score minGain;
	    Score sumGain;
	    Score maxGain;
	    u32 minDepth;
	    u32 sumDepth;
	    u32 maxDepth;

	    QuestionStatistic() :
		count(0),
		minGain(Core::Type<Score>::max),
		sumGain(Score(0)),
		maxGain(Core::Type<Score>::min),
		minDepth(Core::Type<u32>::max),
		sumDepth(0),
		maxDepth(Core::Type<u32>::min) {}
	};
	typedef std::vector<QuestionStatistic> QuestionStatisticList;

	/**
	   A node together with associated examples.
	*/
	struct Node {
	    u32 depth;
	    Score score;
	    u32 nObs;
	    ExamplePtrList * examplePtrs;

	    QuestionId questionId;
	    u32 order;
	    Node * left, * right;

	    Node(
		u32 depth,
		Score score,
		u32 nObs,
		ExamplePtrList * examplePtrs
		) :
		depth(depth),
		score(score),
		nObs(nObs),
		examplePtrs(examplePtrs),
		questionId(DecisionTree::Node::InvalidId),
		order(DecisionTreeTrainer::Information::InvalidOrder),
		left(0), right(0) {}
	};
	typedef std::deque<Node *> NodePtrQueue;

	/**
	   Split hypothesis; used for finding the best split for a node.
	   Stores (up to) the N -best hypotheses and returns either the best or a randomly choosen hypothesis
	*/
	struct SplitHypothesis {
	    size_t questionIdId;
	    Score gain;
	    u32 nLeftExamples, nRightExamples;
	    u32 nLeftObs, nRightObs;
	    Score leftScore, rightScore;
	};
	typedef std::vector<SplitHypothesis *> SplitHypothesisPtrList;

	/**
	   Manages split hypotheses,
	   supports randomization
	*/
	class SplitHypothesesManager {
	private:
	    SplitHypothesisPtrList hyps_;
	    u32 nHyps_;
	private:
	    void clear() {
		for (SplitHypothesisPtrList::iterator it = hyps_.begin(); it != hyps_.end(); ++it)
		    delete *it;
		hyps_.clear();
	    }
	public:
	    SplitHypothesesManager() {
		::srand(::time(0));
		init();
	    }
	    ~SplitHypothesesManager() {
		clear();
	    }

	    void init(u32 size = 1) {
		verify(size >= 1);
		clear();
		hyps_.resize(size);
		verify(hyps_.size() == size);
		for (SplitHypothesisPtrList::iterator it = hyps_.begin(); it != hyps_.end(); ++it)
		    *it = new SplitHypothesis;
		nHyps_ = 0;
	    }

	    bool empty() const
		{ return (nHyps_ == 0) ? true : false; }

	    void reset()
		{ nHyps_ = 0; }

	    /*
	      Keep the N-best hypotheses,
	      use a simple bubble-sort to insert new hypothesis.
	      For small N this will be very efficient.
	    */
	    bool push(const SplitHypothesis &hyp) {
		if (nHyps_ == 0) {
		    nHyps_ = 1;
		    *hyps_.front() = hyp;
		} else if (hyps_.size() == 1) {
		    if (hyp.gain <= hyps_.front()->gain)
			return false;
		    *hyps_.front() = hyp;
		} else {
		    SplitHypothesisPtrList::iterator itHyp = hyps_.begin() + (nHyps_ - 1);
		    if (nHyps_ == hyps_.size()) {
			if (hyp.gain <= (*itHyp)->gain)
			    return false;
		    } else
			{ ++nHyps_; ++itHyp; }
		    **itHyp = hyp;
		    for (SplitHypothesisPtrList::iterator itPrevHyp = itHyp - 1;
			 (itHyp != hyps_.begin()) && ((*itPrevHyp)->gain < (*itHyp)->gain);
			 --itPrevHyp, --itHyp) std::swap(*itPrevHyp, *itHyp);
		}
		return true;
	    }

	    /*
	      return best hypotheis
	    */
	    const SplitHypothesis & best() {
		verify(nHyps_ > 0);
		return *hyps_.front();
	    }

	    /*
	      choose randomly from the (up to) N best hypotheses
	    */
	    const SplitHypothesis & get() {
		verify(nHyps_ > 0);
		return (nHyps_ == 1) ? *hyps_.front() : **(hyps_.begin() + ::rand() % nHyps_);
	    }
	};

	/**
	   (Possible) split of a given node.
	*/
	struct Split {
	    Node * node;
	    QuestionIdList * questionIds;
	    size_t questionIdId;
	    Score gain;

	    QuestionId questionId() const {
		return (*questionIds)[questionIdId];
	    }

	    Split(
		Node * node,
		QuestionIdList * questionIds,
		size_t questionIdId,
		Score gain
		):
		node(node),
		questionIds(questionIds),
		questionIdId(questionIdId),
		gain(gain) { require(questionIds); }
	};
	struct SplitPtrGreaterThan {
	    bool operator()(Split * split1, Split * split2) const {
		return (split1->gain > split2->gain);
	    }
	};
	typedef Core::PriorityQueue<Split *, SplitPtrGreaterThan> SplitPtrQueue;


	typedef std::vector<Cluster *> ClusterPtrList;


    private:
	const Scorer & scorer_;
	const TrainingPlan & plan_;
	const ExampleList & examples_;
	DecisionTree * tree_;

	Score score_;
	u32 nObs_;
	u32 nSplit_;
	u32 nNumber_;
	u32 nLeaf_;
	u32 nCluster_;
	Score gain_;
	const TrainingPlan::Step * step_;

	SplitHypothesesManager splitHyps_;
	SplitPtrQueue splits_;
	NodePtrQueue nodes_;
	QuestionRefList questionRefs_;
	QuestionStatisticList questionStats_;
	QuestionIdList questionIdMap_;
	ClusterPtrList clusters_;


    private:
	Node * init() {
	    ExamplePtrList * examplePtrs = new ExamplePtrList(examples_.size());
	    ExamplePtrList::iterator itExamplePtr = examplePtrs->begin();
	    nObs_ = 0;
	    for (ExampleList::const_iterator itExample = examples_.begin();
		 itExample != examples_.end(); ++itExample, ++itExamplePtr) {
		*itExamplePtr = itExample->get();
		nObs_ += (*itExamplePtr)->nObs;
	    }
	    scorer_(ExamplePtrRange(examplePtrs->begin(), examplePtrs->end()), score_);
	    {
		log() << Core::XmlFull("initial-score", score_ - gain_);
	    }
	    Node *root = new Node(0, score_, nObs_, examplePtrs);
	    root->order = nNumber_++;
	    return root;
	}

	void writeXml(Core::XmlWriter & xml, const Split & split) {
	    xml << Core::XmlOpen("split")
		<< Core::XmlFull("depth", split.node->depth)
		<< Core::XmlFull("gain", split.gain)
		<< Core::XmlOpen("question");
	    questionRefs_[split.questionId()]->write(xml);
	    xml << Core::XmlClose("question")
		<< Core::XmlFull("nQuestion", split.questionIds->size())
		<< (Core::XmlOpen("node")
		    + Core::XmlAttribute("order", split.node->order))
		<< Core::XmlFull("score", split.node->score)
		<< Core::XmlFull("nObs", split.node->nObs)
		<< (Core::XmlOpen("left-child")
		    + Core::XmlAttribute("order", split.node->left->order))
		<< Core::XmlFull("score", split.node->left->score)
		<< Core::XmlFull("nObs", split.node->left->nObs)
		<< Core::XmlClose("left-child")
		<< (Core::XmlOpen("right-child")
		    + Core::XmlAttribute("order", split.node->right->order))
		<< Core::XmlFull("score", split.node->right->score)
		<< Core::XmlFull("nObs", split.node->right->nObs)
		<< Core::XmlClose("right-child")
		<< Core::XmlClose("node")
		<< Core::XmlClose("split");
	}

	/*
	  finds best split of a node, if any
	*/
	Split * splitNode(Node * node, QuestionIdList * questionIds, bool strict = false) {
	    if (node->nObs < 2 * step_->minObs)
		return 0;

	    ExamplePtrList * examplePtrs = node->examplePtrs;

	    ExamplePtrList leftExamplePtrs(examplePtrs->size(), 0);
	    ExamplePtrList rightExamplePtrs(examplePtrs->size(), 0);
	    const ExamplePtrList::iterator beginLeftExamplePtr = leftExamplePtrs.begin();
	    const ExamplePtrList::iterator beginRightExamplePtr = rightExamplePtrs.begin();
	    SplitHypothesis splitHyp;
	    verify(splitHyps_.empty());
	    for (splitHyp.questionIdId = 0; splitHyp.questionIdId < questionIds->size(); ++splitHyp.questionIdId) {
		const Question & question = *(questionRefs_[(*questionIds)[splitHyp.questionIdId]]);
		ExamplePtrList::iterator endLeftExamplePtr = beginLeftExamplePtr;
		ExamplePtrList::iterator endRightExamplePtr = beginRightExamplePtr;
		splitHyp.nLeftObs = 0;
		splitHyp.nRightObs = 0;
		for (ExamplePtrList::const_iterator itExamplePtr = examplePtrs->begin(), endExamplePtr = examplePtrs->end();
		     itExamplePtr != endExamplePtr; ++itExamplePtr) {
		    if (question(*((*itExamplePtr)->properties)) == TRUE) {
			*(endLeftExamplePtr++) = *itExamplePtr;
			splitHyp.nLeftObs += (*itExamplePtr)->nObs;
		    } else {
			*(endRightExamplePtr++) = *itExamplePtr;
			splitHyp.nRightObs += (*itExamplePtr)->nObs;
		    }
		}
		splitHyp.nLeftExamples = u32(endLeftExamplePtr - beginLeftExamplePtr);
		splitHyp.nRightExamples = u32(endRightExamplePtr - beginRightExamplePtr);
		verify(splitHyp.nLeftExamples + splitHyp.nRightExamples == node->examplePtrs->size());
		if ((splitHyp.nLeftObs < step_->minObs) || (splitHyp.nRightObs < step_->minObs))
		    continue;
		if (strict && ((splitHyp.nLeftObs == 0) || (splitHyp.nRightObs == 0)))
		    continue;
		splitHyp.gain = scorer_(
		    ExamplePtrRange(beginLeftExamplePtr, endLeftExamplePtr),
		    ExamplePtrRange(beginRightExamplePtr, endRightExamplePtr),
		    node->score,
		    splitHyp.leftScore, splitHyp.rightScore);
		if (splitHyp.gain < 0.0) {
		    if (!Core::isAlmostEqualUlp(f32(splitHyp.gain), f32(0.0), 20))
			error() << "negative split gain of " << splitHyp.gain <<"; gain must be positive";
		    continue;
		}
		if (splitHyp.gain < step_->minGain)
		    continue;
		if (strict && (splitHyp.gain == 0.0))
		    continue;
		splitHyps_.push(splitHyp);
	    }
	    Split *split = 0;
	    if (!splitHyps_.empty()) {
		const SplitHypothesis &bestSplitHyp = splitHyps_.get();
		const Question & question = *(questionRefs_[(*questionIds)[bestSplitHyp.questionIdId]]);
		ExamplePtrList *leftExamplePtrs = new ExamplePtrList(bestSplitHyp.nLeftExamples);
		ExamplePtrList *rightExamplePtrs = new ExamplePtrList(bestSplitHyp.nRightExamples);
		ExamplePtrList::iterator itLeftExamplePtr = leftExamplePtrs->begin();
		ExamplePtrList::iterator itRightExamplePtr = rightExamplePtrs->begin();
		for (ExamplePtrList::const_iterator itExamplePtr = node->examplePtrs->begin(), endExamplePtr = node->examplePtrs->end();
		     itExamplePtr != endExamplePtr; ++itExamplePtr) {
		    if (question(*((*itExamplePtr)->properties)) == TRUE)
			*(itLeftExamplePtr++) = *itExamplePtr;
		    else
			*(itRightExamplePtr++) = *itExamplePtr;
		}
		verify(itLeftExamplePtr == leftExamplePtrs->end());
		verify(itRightExamplePtr == rightExamplePtrs->end());
		delete node->examplePtrs;
		node->examplePtrs = 0;
		node->left = new Node(
		    node->depth + 1,
		    bestSplitHyp.leftScore,
		    bestSplitHyp.nLeftObs,
		    leftExamplePtrs);
		node->right = new Node(
		    node->depth + 1,
		    bestSplitHyp.rightScore,
		    bestSplitHyp.nRightObs,
		    rightExamplePtrs);
		split = new Split(
		    node,
		    questionIds,
		    bestSplitHyp.questionIdId,
		    bestSplitHyp.gain);
	    }
	    splitHyps_.reset();
	    return split;
	}

	/*
	  insert split into tree
	*/
	void insertSplit(Split * split) {
	    nSplit_++;
	    gain_ += split->gain;
	    Node * node = split->node;
	    node->questionId = split->questionId();
	    node->left->order = nNumber_++;
	    node->right->order = nNumber_++;
	    {
		Core::Component::Message msg = log();
		writeXml(msg, *split);
		msg << Core::XmlFull("total-gain", gain_)
		    << Core::XmlFull("total-score", score_ - gain_);
	    }
	    // collect question statistic
	    QuestionStatistic & questionStat = questionStats_[node->questionId];
	    ++(questionStat.count);
	    questionStat.sumGain += split->gain;
	    questionStat.minGain = std::min(questionStat.minGain, split->gain);
	    questionStat.maxGain = std::max(questionStat.maxGain, split->gain);
	    questionStat.sumDepth += node->depth;
	    questionStat.minDepth = std::min(questionStat.minDepth, node->depth);
	    questionStat.maxDepth = std::max(questionStat.maxDepth, node->depth);
	}


	/*
	  queues the best split of the node
	*/
	void suggestSplit(Node * node, QuestionIdList * questionIds) {
	    Split * split = splitNode(node, questionIds, true);
	    if (split) {
		splits_.insert(split);
	    } else {
		delete questionIds;
		nodes_.push_back(node);
	    }
	}

	/*
	  commits a split,
	  suggests new splits for left and right child
	*/
	void commitSplit(Split * split) {
	    insertSplit(split);
	    Node * node = split->node;
	    QuestionIdList * questionIds = split->questionIds;
	    (*questionIds)[split->questionIdId] = questionIds->back();
	    questionIds->pop_back();
	    suggestSplit(node->left, new QuestionIdList(*questionIds));
	    suggestSplit(node->right, questionIds);
	}

	/*
	  commits a split,
	  suggests new split for the right child (i.e. question answer=NO) and
	  returns the left child (i.e. question answer=YES)
	*/
	Node * commitHalfSplit(Split * split) {
	    insertSplit(split);
	    Node * node = split->node;
	    QuestionIdList * questionIds = split->questionIds;
	    (*questionIds)[split->questionIdId] = questionIds->back();
	    questionIds->pop_back();
	    suggestSplit(node->right, questionIds);
	    return node->left;
	}

	/*
	  rollback split
	*/
	void rollbackSplit(Split * split) {
	    Node * node = split->node;
	    node->examplePtrs = node->left->examplePtrs;
	    node->examplePtrs->insert(
		node->examplePtrs->end(),
		node->right->examplePtrs->begin(),
		node->right->examplePtrs->end());
	    delete node->right->examplePtrs;
	    delete node->left;
	    delete node->right;
	    node->left = 0;
	    node->right = 0;
	    nodes_.push_back(node);

	    delete split->questionIds;
	    delete split;
	}

	/*
	  action: split
	  find tuple of node and question causing best split,
	  split using best split,
	  repeat procedure until no splits left or max. leaf number is reached
	*/
	void split() {
	    QuestionId questionId = questionRefs_.size();
	    QuestionIdList questionIds(step_->questionRefs->size());
	    questionRefs_.resize(questionRefs_.size() + step_->questionRefs->size());
	    questionStats_.resize(questionRefs_.size());
	    for (size_t i = 0; i < step_->questionRefs->size(); ++i, ++questionId) {
		questionRefs_[questionId] = (*(step_->questionRefs))[i];
		questionIds[i] = questionId;
	    }

	    for (size_t i = nodes_.size(); i > 0; nodes_.pop_front(), --i)
		suggestSplit(nodes_.front(), new QuestionIdList(questionIds));

	    while (!splits_.empty() && (nLeaf_ + nodes_.size() + splits_.size() < plan_.maxLeaves)) {
		Split * split = splits_.top(); splits_.pop();
		commitSplit(split);
	    }
	}

	/*
	  action: partition(->finalizeLeftNodes=false) or cluster(->finalizeLeftNodes=true)
	  same as split, but put only right node back into queue for further splitting;
	  if finalizeLeftNodes=false, then put left nodes back into queue for the next step,
	  otherwise each left node will become a leaf
	*/
	void halfSplit(bool finalizeLeftNodes = false) {
	    QuestionId questionId = questionRefs_.size();
	    QuestionIdList questionIds(step_->questionRefs->size());
	    questionRefs_.resize(questionRefs_.size() + step_->questionRefs->size());
	    questionStats_.resize(questionRefs_.size());
	    for (size_t i = 0; i < step_->questionRefs->size(); ++i, ++questionId) {
		questionRefs_[questionId] = (*(step_->questionRefs))[i];
		questionIds[i] = questionId;
	    }

	    for (size_t i = nodes_.size(); i > 0; nodes_.pop_front(), --i)
		suggestSplit(nodes_.front(), new QuestionIdList(questionIds));

	    if (finalizeLeftNodes) {
		while (!splits_.empty() && (nLeaf_ + nodes_.size() + splits_.size() < plan_.maxLeaves)) {
		    Split * split = splits_.top(); splits_.pop();
		    commitHalfSplit(split);
		    ++nLeaf_;
		}
	    } else {
		while (!splits_.empty() && (nLeaf_ + nodes_.size() + splits_.size() < plan_.maxLeaves)) {
		    Split * split = splits_.top(); splits_.pop();
		    nodes_.push_back(commitHalfSplit(split));
		}
	    }
	}

	void start(Node * root) {
	    nodes_.push_back(root);
	    for (TrainingPlan::StepPtrList::const_iterator it = plan_.steps.begin();
		 (it != plan_.steps.end()) && (nLeaf_ + nodes_.size() < plan_.maxLeaves); ++it) {
		step_ = *it;
		step_->write(log());
		splitHyps_.init(step_->nRandomQuestion);
		if (step_->action == "split")
		    split();
		else if (step_->action == "partition")
		    halfSplit(false);
		else if (step_->action == "cluster")
		    halfSplit(true);
		else
		    error("Unknown action \"%s\"", step_->action.c_str());

		while (!splits_.empty()) {
		    Split * split = splits_.top(); splits_.pop();
		    rollbackSplit(split);
		}
		writeXml(log());
	    }
	    nLeaf_ += nodes_.size();
	    nodes_.clear();
	    step_ = 0;
	}

	DecisionTree::Node * commitNode(Node * node) {
	    verify(node->order != DecisionTreeTrainer::Information::InvalidOrder);
	    DecisionTree::Node * _node = new DecisionTree::Node(
		0, 0, new DecisionTreeTrainer::Information(
		    node->order,
		    node->nObs,
		    node->score));
	    if ((node->left) && (node->right)) {
		_node->id_ = questionIdMap_[node->questionId];
		_node->setChilds(
		    commitNode(node->left),
		    commitNode(node->right));
	    } else {
		verify(!(node->left) && !(node->right));
		_node->id_ = ClassId(nCluster_++);
		ConstExampleRefList * exampleRefs = new ConstExampleRefList();
		exampleRefs->reserve(node->examplePtrs->size());
		clusters_.push_back(new Cluster(_node, exampleRefs));
	    }
	    delete node->examplePtrs;
	    delete node;
	    return _node;
	}

	ClusterList * finish(Node * root) {
	    tree_->setMap(plan_.map);

	    // reduce list of questions to used ones
	    QuestionRefList * usedQuestionRefs = new QuestionRefList();
	    questionIdMap_.resize(questionRefs_.size(), DecisionTree::Node::InvalidId);
	    QuestionId questionId = 0;
	    QuestionStatisticList::iterator questionStatIt  = questionStats_.begin();
	    QuestionRefList      ::iterator questionRefIt   = questionRefs_.begin();
	    QuestionIdList       ::iterator questionIdMapIt = questionIdMap_.begin();
	    for (; questionStatIt != questionStats_.end();
		 ++questionStatIt, ++questionRefIt, ++questionIdMapIt) {
		if (questionStatIt->count > 0) {
		    usedQuestionRefs->push_back(*questionRefIt);
		    *questionIdMapIt = questionId++;
		}
	    }
	    tree_->setQuestions(usedQuestionRefs);

	    // create decision tree
	    clusters_.reserve(nLeaf_);
	    nCluster_ = 0;
	    if (root->order == DecisionTreeTrainer::Information::InvalidOrder)
		root->order = 0;
	    tree_->setRoot(commitNode(root));
	    verify(nLeaf_ == nCluster_);

	    // create cluster
	    for (ExampleList::const_iterator it = examples_.begin();
		 it != examples_.end(); ++it)
		clusters_[tree_->classify(*((*it)->properties))]->exampleRefs->push_back(*it);
	    ClusterList * clusters = new ClusterList(config, plan_.map);
	    for (ClusterPtrList::const_iterator it = clusters_.begin();
		 it != clusters_.end(); ++it)
		clusters->add(*it);

	    clusters_.clear();
	    return clusters;
	}


    public:
	Training(
	    const Core::Configuration & config,
	    const Scorer & scorer,
	    const TrainingPlan & plan,
	    const ExampleList & examples,
	    DecisionTree * tree) :
	    Core::Component(config),
	    scorer_(scorer),
	    plan_(plan),
	    examples_(examples),
	    tree_(tree),
	    score_(0.0),
	    nObs_(0),
	    nSplit_(0),
	    nNumber_(0),
	    nLeaf_(0),
	    nCluster_(0),
	    gain_(0.0),
	    step_(0) {
	    if (plan_.map.get() != examples_.getMap().get()) {
		PropertyMapDiff diff(config, *plan.map, examples_.map(), false);
		if (diff.hasDifferences())
		    criticalError("differences in property maps of training and example list");
	    }
	}

	ClusterList * train() {
	    Node * root = init();
	    start(root);
	    return finish(root);
	}

	void write(std::ostream & os) const {}

	void writeXml(Core::XmlWriter & xml) const {
	    xml << Core::XmlFull("total-gain", gain_)
		<< Core::XmlFull("total-score", score_ - gain_)
		<< Core::XmlFull("nObs", nObs_);
	    xml << Core::XmlOpen("tree-statistic")
		<< Core::XmlFull("nNode", nSplit_)
		<< ( (plan_.maxLeaves < Core::Type<u32>::max) ?
		     Core::XmlFull("nLeaf", nLeaf_) + Core::XmlAttribute("max", plan_.maxLeaves) :
		     Core::XmlFull("nLeaf", nLeaf_) )
		<< Core::XmlFull("nUnknown", nodes_.size())
		<< Core::XmlClose("tree-statistic");
	    xml << Core::XmlOpen("question-statistics")
		+ Core::XmlAttribute("count", questionRefs_.size());
	    size_t i = 0;
	    QuestionRefList::const_iterator questionRefIt = questionRefs_.begin();
	    QuestionStatisticList::const_iterator questionStatIt = questionStats_.begin();
	    for (; questionRefIt != questionRefs_.end(); ++questionRefIt, ++questionStatIt, ++i) {
		xml << Core::XmlOpen("question-statistic")
		    + Core::XmlAttribute("index", i)
		    << Core::XmlOpen("question");
		(*questionRefIt)->write(xml);
		xml << Core::XmlClose("question")
		    << Core::XmlFull("count", questionStatIt->count);
		if (questionStatIt->count > 0)
		    xml << Core::XmlFull("total-gain", questionStatIt->sumGain)
			<< Core::XmlFull("min-gain", questionStatIt->minGain)
			<< Core::XmlFull("avg-gain", questionStatIt->sumGain / Score(questionStatIt->count))
			<< Core::XmlFull("max-gain", questionStatIt->maxGain)
			<< Core::XmlFull("min-depth", questionStatIt->minDepth)
			<< Core::XmlFull("avg-depth", f32(questionStatIt->sumDepth) / f32(questionStatIt->count))
			<< Core::XmlFull("max-depth", questionStatIt->maxDepth);
		xml << Core::XmlClose("question-statistic");
	    }
	    xml << Core::XmlClose("question-statistics");
	}
    };
} // namespace Cart
// ============================================================================


// ============================================================================
const Core::ParameterString DecisionTreeTrainer::paramTrainingFilename(
    "training-file",
    "name of training file");

const Core::ParameterString DecisionTreeTrainer::paramClusterFilename(
    "cluster-file",
    "name of cluster file");

bool DecisionTreeTrainer::loadFromString(const std::string & str) {
    verify(!plan_);
    plan_ = new TrainingPlan(map_);
    XmlTrainingPlanParser parser(config);
    return parser.parseString(str, plan_);
}

bool DecisionTreeTrainer::loadFromStream(std::istream & i) {
    verify(!plan_);
    plan_ = new TrainingPlan(map_);
    XmlTrainingPlanParser parser(config);
    return parser.parseStream(i, plan_);
}

bool DecisionTreeTrainer::loadFromFile(std::string filename) {
    if (filename.empty()) {
	filename = paramTrainingFilename(config);
	verify(!filename.empty());
    }
    log() << "load training plan from \"" << filename << "\"";
    verify(!plan_);
    plan_ = new TrainingPlan(map_);
    XmlTrainingPlanParser parser(config);
    return parser.parseFile(filename, plan_);
}

ClusterList * DecisionTreeTrainer::train(DecisionTree * tree, const ExampleList & examples) {
    require(plan_ && scorer_);
    Training training(config, *scorer_, *plan_, examples, tree);
    return training.train();
}

ClusterList * DecisionTreeTrainer::train(DecisionTree * tree, const std::string & filename) {
    require(plan_ && scorer_);
    ExampleList examples(config, plan_->map);
    if (examples.loadFromFile(filename))
	return train(tree, examples);
    else
	return 0;
}

void DecisionTreeTrainer::write(std::ostream & os) const {
    require(plan_ && scorer_);
    os << "scorer:\n";
    scorer_->write(os);
    os << "training:" << std::endl;
    plan_->write(os);
}

void DecisionTreeTrainer::writeXml(Core::XmlWriter & xml) const {
    require(plan_ && scorer_);
    xml << Core::XmlOpen("scorer");
    scorer_->write(xml);
    xml << Core::XmlClose("scorer");
    plan_->writeXml(xml);
}

void DecisionTreeTrainer::writeToFile() const {
    std::string filename(paramTrainingFilename(config));
    if (!filename.empty()) {
	log() << "write training plan to \"" << filename << "\"";
	Core::XmlOutputStream xml(new Core::CompressedOutputStream(filename));
	xml.generateFormattingHints(true);
	xml.setIndentation(4);
	xml.setMargin(78);
	writeXml(xml);
    } else {
	warning("cannot store training plan, because no filename is given");
    }
}
// ============================================================================
