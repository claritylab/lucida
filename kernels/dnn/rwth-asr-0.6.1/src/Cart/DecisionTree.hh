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
#ifndef _CART_DECISION_TREE_HH
#define _CART_DECISION_TREE_HH

#include <Core/Assertions.hh>
#include <Core/Component.hh>
#include <Core/ReferenceCounting.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

#include "BinaryTree.hh"
#include "Properties.hh"

namespace Cart {
    /**
       Answers; tenary logic.
    */
    typedef s8 Answer;
    const Answer FALSE =  0;
    const Answer TRUE  =  1;
    const Answer UNDEF = -1;

    typedef std::vector<Answer> AnswerList;


    // ============================================================================
    /**
       Questions

       <question index="0" description="hmm-state">
       <key> state </key> <values> 1, 2 </values>
       </question>
       <question description="sampling rate">
       <key> 16kHz_sampling_rate </key>
       </question>
       <for-each-key keys="history[1], history[0], central, future[0], future[1]">
       <question description="vowels">
       <values> a, e, i, o, u </values>
       </question>
       </for-each-key>
    */
    class Question :
	public Core::ReferenceCounted {
    public:
	typedef PropertyMap::Index KeyIndex;
	typedef PropertyMap::Index ValueIndex;
	typedef size_t QuestionIndex;

	static const QuestionIndex InvalidQuestionIndex;

    protected:
	PropertyMapRef map_;
	const std::string desc_;

    public:
	mutable QuestionIndex index;

    public:
	Question(
	    PropertyMapRef map,
	    const std::string & desc = ""
	    ) :
	    map_(map), desc_(desc), index(InvalidQuestionIndex) {}
	virtual ~Question() {}

	virtual Answer operator()(const Properties & props) const = 0;

	virtual void write(std::ostream & out) const = 0;

	virtual void writeXml(Core::XmlWriter & xml) const = 0;

	void write(std::ostream & out, const Answer & a) const;
    };

    typedef Core::Ref<const Question> QuestionRef;
    typedef std::vector<QuestionRef> QuestionRefList;

    void write(std::ostream & out, const QuestionRefList & questionRefs);
    void writeXml(Core::XmlWriter & xml, const QuestionRefList & questionRefs);


    class ScalarQuestion :
	public Question {
    private:
	const KeyIndex keyIndex_;
	ValueIndex valueIndex_;

    public:
	ScalarQuestion(
	    PropertyMapRef map,
	    const std::string & key,
	    const std::string & value,
	    const std::string & desc);

	Answer operator()(const Properties & props) const {
	    ValueIndex valueIndex = props[keyIndex_];
	    if (valueIndex == map_->undefinedIndex)
		return UNDEF;
	    else if (valueIndex == valueIndex_)
		return TRUE;
	    else
		return FALSE;
	}

	void write(std::ostream & out) const;

	void writeXml(Core::XmlWriter & xml) const;
    };


    class SetQuestion :
	public Question {
    private:
	const KeyIndex keyIndex_;
	ValueIndex * begin_, * end_;

    protected:
	std::string values2str() const;

    public:
	SetQuestion(
	    PropertyMapRef map,
	    const std::string & key,
	    const std::vector<std::string> & values,
	    const std::string & desc
	    );
	~SetQuestion() {
	    delete[] begin_;
	    begin_ = 0; end_ = 0;
	}

	Answer operator()(const Properties & props) const {
	    ValueIndex valueIndex = props[keyIndex_];
	    if (valueIndex == map_->undefinedIndex)
		return UNDEF;
	    else if (std::binary_search(begin_, end_, valueIndex))
		return TRUE;
	    else
		return FALSE;
	}

	void write(std::ostream & out) const;

	void writeXml(Core::XmlWriter & xml) const;
    };



    // ============================================================================
    /**
       Information expected to be produced
       by generic cart trainer.

       <information>
       <order> ... </order>
       <size>  ... </size>
       <score> ... </score>
       </information>
    */
    struct TrainingInformation :
	public Information {
	static const u32 InvalidOrder;
	static const u32 InvalidSize;
	static const f32 InvalidScore;

	u32 order;
	u32 size;
	f64 score;

	TrainingInformation() :
	    order(InvalidOrder),
	    size(InvalidSize),
	    score(InvalidScore) {}
	TrainingInformation(
	    u32 order,
	    u32 size,
	    f64 score
	    ) :
	    order(order),
	    size(size),
	    score(score) {}

	void writeXml(Core::XmlWriter & xml) const;
    };

    /**
       Tenary decision tree;
       supports classification and multification.

       <decision-tree>
       <properties-definition>
       ...
       </properties-definition>
       <questions>
       <question name="..." id="..."> ... </question>
       ...
       </questions>
       <binary-tree>
       <!-- a "true" node, i.e. a node with childs -->
	<node id="%question-id">
	    <information>
		<order> %node-order             </order>
		<size>  %number_of_observations </size>
		<score> %log-likelihood         </score>
	    </information>
	    <!-- a leaf, i.e. a node without childs -->
	    <node id="%class-id">
		<information>
		    <order> %node-order             </order>
		    <size>  %number_of_observations </size>
		    <score> %log-likelihood         </score>
		</information>
	    </node>
	    <node ...

       </node>
       </binary-tree>
       </decision-tree>

       Property map and questions (not question list) are
       shareable, i.e. reference counted.
    */
    class DecisionTree :
	public BinaryTree {

    protected:
	typedef DecisionTree Self;
	typedef BinaryTree Precursor;

    public:
	typedef Node::Id QuestionId;
	typedef Node::Id ClassId;

	typedef std::vector<const Node *> NodePtrList;
	typedef std::stack<Node *>        NodePtrStack;

	static const Core::ParameterString paramCartFilename;

	// path of a classification
	struct Path {
	    QuestionRefList questionRefs;
	    AnswerList answers;
	    const Node * leaf;

	    void write(std::ostream & out) const;
	};

    protected:
	PropertyMapRef map_;
	QuestionRefList * questionRefs_;

    protected:
	virtual void checkNode(const Node & node) {
	    if (!node.isLeaf())
		require((0 <= node.id()) && (node.id() < questionRefs_->size()));
	}

    public:
	DecisionTree(
	    const Core::Configuration & config,
	    PropertyMapRef map = PropertyMapRef(new PropertyMap),
	    QuestionRefList * questionRefs = 0,
	    Node * root = 0);
	virtual ~DecisionTree() {
	    delete questionRefs_;
	    questionRefs_ = 0;
	}

	virtual void setMap(PropertyMapRef map) { require(map); map_ = map; }

	bool hasMap() { return !map_->empty(); }
	PropertyMapRef getMap() const { return map_; }
	const PropertyMap & map() const { return *map_; }

	virtual void setQuestions(QuestionRefList * questionRefs) {
	    require(!map_->empty());
	    delete questionRefs_;
	    questionRefs_ = questionRefs;
	}

	const QuestionRefList & questions() const {
	    require(questionRefs_);
	    return *questionRefs_;
	}

	const Question & question(QuestionId id) const {
	    require_(questionRefs_);
	    require_((0 <= id) & (id < questionRefs_->size()));
	    return *((*questionRefs_)[id]);
	}

	QuestionRef getQuestion(QuestionId id) const {
	    require_(questionRefs_);
	    require_((0 <= id) & (id < questionRefs_->size()));
	    return (*questionRefs_)[id];
	}

	virtual void setRoot(Node * root) {
	    require(questionRefs_);
	    Precursor::setRoot(root);
	}

	const Node * find(const Properties & props) const;

	NodePtrList findAll(const Properties & props) const;

	Path findPath(const Properties & props) const;

	ClassId classify(const Properties & props) const { return find(props)->id(); }

	virtual bool loadFromString(const std::string & str);

	virtual bool loadFromStream(std::istream & i);

	virtual bool loadFromFile(std::string filename = "");

	virtual void draw(std::ostream & out) const;

	virtual void write(std::ostream & out) const;

	virtual void writeXml(Core::XmlWriter & xml, const std::string & name = "decision-tree") const;

	void writeToFile() const;
    };


} // namespace Cart

inline std::ostream & operator<<(std::ostream & out, const Cart::Question & q) {
    q.write(out);
    return out;
}

inline std::ostream & operator<<(std::ostream & out, const Cart::QuestionRefList & q) {
    Cart::write(out, q);
    return out;
}

inline std::ostream & operator<<(std::ostream & out, const Cart::DecisionTree::Path & p) {
    p.write(out);
    return out;
}

#endif // _CART_DECISION_TREE_HH
