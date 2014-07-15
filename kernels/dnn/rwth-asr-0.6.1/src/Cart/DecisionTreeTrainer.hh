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
#ifndef _CART_DECISION_TREE_TRAINER_HH
#define _CART_DECISION_TREE_TRAINER_HH

#include <Core/Assertions.hh>
#include <Core/Component.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

#include <map>

#include "Example.hh"
#include "Cluster.hh"
#include "DecisionTree.hh"

namespace Cart {

    // ============================================================================
    /**
       The scorer takes a cluster of elements as input
       and produces a single score as output;
       the trainer aims to train a decision tree such
       that the accumulated score of all clusters is
       minimized (i.e. classical minimum log-likelihood
       approach).
    */
    typedef f64 Score;
    typedef std::vector<const Example *> ExamplePtrList;
    struct ExamplePtrRange {
	ExamplePtrList::const_iterator begin;
	ExamplePtrList::const_iterator end;
	ExamplePtrRange(ExamplePtrList::const_iterator begin, ExamplePtrList::const_iterator end) :
	    begin(begin), end(end) {}
	size_t size() const { return end - begin; }
    };

    class Scorer : public Core::Component, public Core::ReferenceCounted {
    public:
	Scorer(const Core::Configuration &config) :
	    Core::Component(config) {}
	virtual ~Scorer() {}

	/*
	  Describe the scorer (incl. the settings of all parameters)
	*/
	virtual void write(std::ostream &os) const = 0;

	/*
	  Split a node, into a left and a right node.
	  The parameters leftExamples and rightExamples are the result of splitting the father node,
	  fatherScore is the score assigned to the father node.
	  The return value is the gain achieved by the split, i.e. the higher the better, and must be positive.
	  leftScore and rightScore are the scores of the left and right child node after the split of the father node.
	  Remark:
	  The second form is used for initialization.
	*/
	virtual Score operator()(
	    const ExamplePtrRange & leftExamples, const ExamplePtrRange & rightExamples,
	    const Score fatherScore,
	    Score & leftChildScore, Score & rightChildScore) const = 0;
	virtual void operator()(
	    const ExamplePtrRange &examples,
	    Score & score) const
	    { Score dummy; operator()(examples, ExamplePtrRange(ExamplePtrList::const_iterator(), ExamplePtrList::const_iterator()), 0.0, score, dummy); }
    };
    typedef Core::Ref<const Scorer> ConstScorerRef;

    /**
       ID3, entropy gain
    **/
    class ID3 : public Scorer {
	typedef Scorer Precursor;
    private:
	typedef std::map<const FloatBox * const, u32, FloatBox::PtrLessFcn> CountMap;

    private:
	mutable CountMap map_;

    public:
	ID3(const Core::Configuration &config) :
	    Precursor(config),
	    map_() {}

	void write(std::ostream &os) const;

	Score entropy(ExamplePtrList::const_iterator begin, ExamplePtrList::const_iterator end) const;

	Score operator()(
	    const ExamplePtrRange &leftExamples, const ExamplePtrRange &rightExamples,
	    const Score fatherEntropy,
	    Score &leftChildEntropy, Score &rightChildEntropy) const;
	void operator()(const ExamplePtrRange &examples, Score& score) const;
    };



    // ============================================================================
    /**
       behaves like a binary decision tree trainer,
       i.e. UNDEF is treated as FALSE
    */
    class DecisionTreeTrainer :
	public Core::Component {
    protected:
	typedef DecisionTreeTrainer Self;
	typedef Core::Component Precursor;

	static const Core::ParameterString paramFilename;

    public:
	static const Core::ParameterString paramTrainingFilename;
	static const Core::ParameterString paramExampleFilename;
	static const Core::ParameterString paramClusterFilename;

	/**
	   Telling the trainer some constraints about how
	   to do the training.
	*/
	struct TrainingPlan {
	    struct Step {
		std::string name;
		std::string action;
		u32 minObs;
		Score minGain;
		u32 nRandomQuestion; // 1 -> randomization disabled
		QuestionRefList * questionRefs;

		Step() :
		    name(),
		    action("split"),
		    minObs(0),
		    minGain(0.0),
		    nRandomQuestion(1),
		    questionRefs(0) {}
		~Step() {
		    delete questionRefs;
		}

		void write(std::ostream & os) const;
		void writeXml(Core::XmlWriter & xml) const;
	    };
	    typedef std::vector<Step *> StepPtrList;

	    PropertyMapRef map;
	    u32 maxLeaves;
	    StepPtrList steps;

	    TrainingPlan(PropertyMapRef map = PropertyMapRef(new PropertyMap)) :
		map(map),
		maxLeaves(Core::Type<u32>::max),
		steps() {}
	    ~TrainingPlan() {
		for (StepPtrList::iterator it = steps.begin();
		     it != steps.end(); ++it)
		    delete *it;
	    }

	    void write(std::ostream & os) const;
	    void writeXml(Core::XmlWriter & xml) const;
	};


	/**
	   The kind of information stored by that specific
	   trainer at every node, regardless if internal
	   or leaf.
	*/
	typedef TrainingInformation Information;

    private:
	PropertyMapRef map_;
	ConstScorerRef scorer_;
	TrainingPlan * plan_;

    public:
	DecisionTreeTrainer(
	    const Core::Configuration & config,
	    PropertyMapRef map = PropertyMapRef(new PropertyMap),
	    ConstScorerRef scorer = ConstScorerRef(),
	    TrainingPlan * plan = 0) :
	    Precursor(config),
	    map_(),
	    scorer_(scorer),
	    plan_(plan) {
	    setMap(map);
	}
	~DecisionTreeTrainer() {
	    delete plan_;
	}

	void setMap(PropertyMapRef map) { require(map); map_ = map; }
	bool hasMap() { return !map_->empty(); }
	PropertyMapRef getMap() const { return map_; }
	const PropertyMap & map() const { return *map_; }

	void setScorer(ConstScorerRef scorer) { scorer_ = scorer; }
	ConstScorerRef scorer() { return scorer_; }

	const TrainingPlan & plan() { return *plan_; }

	ClusterList * train(DecisionTree * tree, const ExampleList & examples);

	ClusterList * train(DecisionTree * tree, const std::string & examplesFilename = "");

	bool loadFromString(const std::string & str);

	bool loadFromStream(std::istream & i);

	bool loadFromFile(std::string filename = "");

	void write(std::ostream & os) const;

	void writeXml(Core::XmlWriter & xml) const;

	void writeToFile() const;
    };

} // namespace Cart

inline std::ostream & operator<<(std::ostream & out, const Cart::DecisionTreeTrainer & t) {
    t.write(out);
    return out;
}

#endif //_CART_DECISION_TREE_TRAINER_HH
