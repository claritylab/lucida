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
#ifndef _CART_PARSER_HH
#define _CART_PARSER_HH

#include <Core/Component.hh>
#include <Core/Types.hh>
#include <Core/XmlParser.hh>
#include <Core/XmlBuilder2.hh>

#include "Properties.hh"
#include "BinaryTree.hh"
#include "DecisionTree.hh"
#include "DecisionTreeTrainer.hh"


namespace Cart {
    using Core::XmlSchemaParser;

    using Core::XmlContext;
    using Core::XmlAttributes;
    using Core::XmlElement;
    using Core::XmlEmptyElement;
    using Core::XmlIgnoreElement;
    using Core::XmlMixedElement;
    using Core::XmlRegularElement;

    using Core::XmlEmptyElementRelay;
    using Core::XmlMixedElementRelay;
    using Core::XmlRegularElementRelay;

    using Core::BuildDelegation;
    using XmlBuilder2::XmlStringBuilderElement;
    using XmlBuilder2::XmlSignedBuilderElement;
    using XmlBuilder2::XmlUnsignedBuilderElement;
    using XmlBuilder2::XmlFloatBuilderElement;

    // ============================================================================
    /**
       <properties-definition>
       <key> central </key> <value-map> <value id="-1"> # </value> <value id="0"> a </value> </value-map>
       <!--
       <key> x       </key> <value-range> 0.0 - 1.0 </value-range>
       -->
       ...
       </properties-definition>
    */
    class XmlIndexedStringBuilderElement :
	public XmlEmptyElement,
	public BuildDelegation<PropertyMap::IndexedString> {

    public:
	typedef PropertyMap::Index         Index;
	typedef PropertyMap::IndexedString IndexedString;

    protected:
	typedef XmlEmptyElement Precursor;

    private:
	IndexedString * pair_;
	Index index_;
	bool ok_;

    public:
	XmlIndexedStringBuilderElement(const char * name, XmlContext * context) :
	    Precursor(name, context),
	    pair_(new IndexedString()),
	    index_(0),
	    ok_(true) {}
	~XmlIndexedStringBuilderElement() {
	    delete pair_;
	}

	void reset() {
	    index_ = 0;
	}

	void start(const XmlAttributes atts);

	void end();

	void characters(const char * ch, int len) {
	    pair_->first.append(ch, len);
	}
    };


    class XmlPropertiesDefinitionBuilderElement :
	public XmlRegularElement,
	public BuildDelegation<PropertyMap> {

    protected:
	typedef XmlPropertiesDefinitionBuilderElement Self;
	typedef XmlRegularElement Precursor;
	typedef std::pair<std::string, s32> Pair;

    private:
	PropertyMap * mapPtr_;
	PropertyMap::StringList keys_;
	PropertyMap::ListOfIndexedStringList values_;

	XmlStringBuilderElement * keyParser_;
	XmlIndexedStringBuilderElement * valueParser_;
	XmlMixedElementRelay * valueMapParser_;

    protected:
	void key(const std::string & key);

	void value(const PropertyMap::IndexedString & pair);

    public:
	XmlPropertiesDefinitionBuilderElement(const char * name, Core::XmlContext * context);
	~XmlPropertiesDefinitionBuilderElement();

	void set(PropertyMap * mapPtr);

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    /**
       <questions>
       <question index="0" description="hmm-state">
       <key> state </key> <values> 1, 2 </values>
       </question>
       <question description="sampling rate">
       <key> 16kHz_sampling_rate </key>
       </question>
       ...
       <for-each-key keys="history[1], history[0], central, future[0], future[1]">
       <question description="vowels">
       <values> a, e, i, o, u </values>
       </question>
       </for-each-key>
       ...
       <for-each-key>
       <for-each-value>
       <question/>
       </for-each-value>
       </for-each-key>
       ...
       </questions>
    */
    class XmlQuestionRefListBuilderElement :
	public XmlMixedElement,
	public BuildDelegation<QuestionRefList> {

    protected:
	typedef XmlQuestionRefListBuilderElement Self;
	typedef XmlMixedElement Precursor;

    private:
	PropertyMapRef mapRef_;
	QuestionRefList * questionRefs_;
	Question::QuestionIndex index_;
	std::string desc_;
	std::string key_;
	std::vector<std::string> values_;
	bool isForEachKey_;
	bool isForEachValue_;
	std::vector<std::string> keyList_;
	std::vector<std::string> valueList_;

	XmlStringBuilderElement * keyBuilder_;
	XmlStringBuilderElement * valueBuilder_;
	XmlStringBuilderElement * valuesBuilder_;
	XmlRegularElementRelay * questionElement_;
	XmlMixedElementRelay * forEachKeyElement_;
	XmlMixedElementRelay * forEachValueElement_;

	bool warnOnQuestionIndex_;

    protected:
	void addQuestion(const std::string & key, const std::string & value);

	void addQuestion(const std::string & key, const std::vector<std::string> & values);

	void forEachValue(const std::string & key);

	void forEachKey();

	void key(const std::string & key);

	void values(const std::string & values);

	void startForEachKey(const Core::XmlAttributes atts);

	void endForEachKey();

	void startForEachValue(const Core::XmlAttributes atts);

	void endForEachValue();

	void startQuestion(const XmlAttributes atts);

	void endQuestion();


    public:
	XmlQuestionRefListBuilderElement(const char * name, Core::XmlContext * context);

	~XmlQuestionRefListBuilderElement();

	void set(PropertyMapRef mapRef);

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    /**
       Just ignore any information.

       <information>
       ...
       </information>
    */
    class XmlIgnoreInformationBuilderElement :
	public XmlIgnoreElement,
	public BuildDelegation<Information> {
    protected:
	typedef XmlIgnoreElement Precursor;
    public:
	XmlIgnoreInformationBuilderElement (
	    const char * name,
	    XmlContext * context
	    ) :
	    Precursor(name, context) {}
    };


    /**
       Information produced be cart trainer.

       <information>
       <order> ... </order>
       <size>  ... </size>
       <score> ... </score>
       </information>
    */
    class XmlTrainingInformationBuilderElement :
	public XmlMixedElement,
	public BuildDelegation<Information> {

    protected:
	typedef XmlTrainingInformationBuilderElement Self;
	typedef XmlMixedElement Precursor;
	typedef f64 Score;

    private:
	TrainingInformation * info_;

	XmlUnsignedBuilderElement * orderParser_;
	XmlUnsignedBuilderElement * sizeParser_;
	XmlFloatBuilderElement    * scoreParser_;

    protected:
	void order(const u32 & u) {
	    info_->order = u32(u);
	}

	void size(const u32 & u) {
	    info_->size = u32(u);
	}

	void score(const f64 & f) {
	    info_->score = Score(f);
	}

    public:
	XmlTrainingInformationBuilderElement(
	    const char * name,
	    XmlContext * context
	    );

	~XmlTrainingInformationBuilderElement();

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    /**

    */
    class InformationBuilder {
    public:
	virtual ~InformationBuilder() {}
	virtual void init(const char * name, XmlContext * context) = 0;
	virtual XmlElement * getXmlElement() = 0;
	virtual BuildDelegation<Information> * getBuildDelegation() = 0;
    };


    class IgnoreInformationBuilder :
	public InformationBuilder {
    private:
	XmlIgnoreInformationBuilderElement * infoBuilder_;

    public:
	IgnoreInformationBuilder() : infoBuilder_(0) {}
	~IgnoreInformationBuilder() { delete infoBuilder_; }

	void init(const char * name, XmlContext * context) {
	    infoBuilder_ = new XmlIgnoreInformationBuilderElement(name, context);
	}

	XmlElement * getXmlElement() {
	    return infoBuilder_;
	}

	BuildDelegation<Information> * getBuildDelegation() {
	    return infoBuilder_;
	}
    };


    class TrainingInformationBuilder :
	public InformationBuilder {
    private:
	XmlTrainingInformationBuilderElement * infoBuilder_;

    public:
	TrainingInformationBuilder() : infoBuilder_(0) {}
	~TrainingInformationBuilder() { delete infoBuilder_; }

	void init(const char * name, XmlContext * context) {
	    infoBuilder_ = new XmlTrainingInformationBuilderElement(name, context);
	}

	XmlElement * getXmlElement() {
	    return infoBuilder_;
	}

	BuildDelegation<Information> * getBuildDelegation() {
	    return infoBuilder_;
	}
    };


    // ============================================================================
    /**
       <node>
       <node id="...">
       <information>
       ...
       </information>
       </node>
       <node>
       ...
       </node>
       </node>
    */
    class XmlBinaryTreeRootBuilderElement :
	public XmlMixedElement,
	public BuildDelegation<BinaryTree::Node> {

    protected:
	typedef XmlBinaryTreeRootBuilderElement Self;
	typedef XmlMixedElement Precursor;
	typedef BinaryTree::Node Node;

    private:
	Node * node_;

	InformationBuilder * infoBuilder_;

    protected:
	void info(Information * info) {
	    node_->setInfo(info);
	}

    public:
	XmlBinaryTreeRootBuilderElement (
	    const char * name,
	    XmlContext * context,
	    InformationBuilder * infoBuilder = 0);

	~XmlBinaryTreeRootBuilderElement ();

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    /**
       Tenary decision tree;
       supports classification and multification.

       <decision-tree>
       <properties-definition>
       ...
       </properties-definition>
       <questions>
       <question id="..." description="..."> ... </question>
       ...
       </questions>
       <binary-tree>
       <node>
       ...
       </node>
       </binary-tree>
       </decision-tree>
    */
    class XmlDecisionTreeBuilderElement :
	public XmlRegularElement,
	public BuildDelegation<DecisionTree> {

    protected:
	typedef XmlDecisionTreeBuilderElement Self;
	typedef XmlRegularElement Precursor;
	typedef BinaryTree::Node Node;

    private:
	DecisionTree * tree_;
	PropertyMap * mapFromFile_;

	XmlPropertiesDefinitionBuilderElement * propertyMapParser_;
	XmlQuestionRefListBuilderElement * questionRefsBuilder_;
	XmlBinaryTreeRootBuilderElement * rootBuilder_;
	XmlRegularElementRelay * binaryTreeElement_;

    protected:
	void map(const PropertyMap & map);

	void questionRefs(QuestionRefList * questionRefs);

	void root(Node * root);

    public:
	XmlDecisionTreeBuilderElement(
	    const char * name,
	    XmlContext * context,
	    InformationBuilder * infoBuilder = 0
	    );

	~XmlDecisionTreeBuilderElement();

	void set(DecisionTree * tree);

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };


    // ============================================================================
    /**
       Generic decision tree parser
       as produced by the generic decision tree trainer.
    */
    class XmlDecisionTreeParser :
	public XmlSchemaParser {
    protected:
	typedef XmlDecisionTreeParser Self;
	typedef XmlSchemaParser Precursor;

    private:
	XmlDecisionTreeBuilderElement * treeBuilder_;

    public:
	XmlDecisionTreeParser(
	    const Core::Configuration & config,
	    InformationBuilder * infoBuilder = 0
	    ) :
	    Precursor(config) {
	    treeBuilder_ = new XmlDecisionTreeBuilderElement(
		"decision-tree", this, infoBuilder);
	    setRoot(treeBuilder_);
	}

	~XmlDecisionTreeParser() {
	    delete treeBuilder_;
	}

	bool parseString(const std::string & str, DecisionTree * tree) {
	    treeBuilder_->set(tree);
	    return (Precursor::parseString(str.c_str()) == 0);
	}
	bool parseString(const std::string & str) {
	    return parseString(str, new DecisionTree(config));
	}

	bool parseStream(std::istream & i, DecisionTree * tree) {
	    treeBuilder_->set(tree);
	    return (Precursor::parseStream(i) == 0);
	}
	bool parseStream(std::istream & i) {
	    return parseStream(i, new DecisionTree(config));
	}

	bool parseFile(const std::string & filename, DecisionTree * tree) {
	    treeBuilder_->set(tree);
	    return (Precursor::parseFile(filename.c_str()) == 0);
	}
	bool parseFile(const std::string & filename) {
	    return parseFile(filename, new DecisionTree(config));
	}
    };


    // ============================================================================
    /**
       <properties>
       <key> history[0] </key>
       <key> central    </key> <value> a </value>
       ...
       </properties>
    */
    class XmlPropertiesBuilderElement :
	public XmlRegularElement,
	public BuildDelegation<Properties> {

    protected:
	typedef XmlPropertiesBuilderElement Self;
	typedef XmlRegularElement Precursor;

    private:
	PropertyMapRef mapRef_;
	StoredProperties::StringList keys_;
	StoredProperties::StringList values_;

	XmlStringBuilderElement * keyParser_;
	XmlStringBuilderElement * valueParser_;

    protected:
	void key(const std::string & key);

	void value(const std::string & value);

    public:
	XmlPropertiesBuilderElement(const char * name, Core::XmlContext * context);
	~XmlPropertiesBuilderElement();

	void set(PropertyMapRef mapRef) {
	    mapRef_ = mapRef;
	}

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    /**
       <matrix-f64 nRows="2" nColumns="3">
       1.0 2.0 3.0
       1.0 2.0 3.0
       </matrix-f64>

       or (positions and types of whitespaces doesn't matter)

       <matrix-f64 nRows="2" nColumns="3">
       1.0 2.0 3.0 1.0 2.0 3.0
       </matrix-f64>
    */
    class XmlFloatBoxBuilderElement :
	public XmlEmptyElement,
	public BuildDelegation<FloatBox> {

    protected:
	typedef XmlFloatBoxBuilderElement Self;
	typedef XmlEmptyElement Precursor;

    private:
	size_t nRows_, nCols_;
	std::string cdata_;

    public:
	XmlFloatBoxBuilderElement(const char * name, XmlContext * context) :
	    Precursor(name, context),
	    nRows_(0), nCols_(0),
	    cdata_() {}

	void start(const XmlAttributes atts);

	void end();

	void characters(const char * ch, int len) {
	    cdata_.append(ch, len);
	}
    };


    // ============================================================================
    /**
       <example-list>
       <property-map>
       ...
       </property-map>
       <example>
       <properties>
       ...
       </properties>
       <matrix-f64 nRows="..." nColumns="...">
       ...
       </matrix-f64>
       </example>
       ...
       </example-list>
    */
    class XmlExampleListBuilderElement :
	public XmlRegularElement,
	public BuildDelegation<ExampleList> {

    protected:
	typedef XmlExampleListBuilderElement Self;
	typedef XmlRegularElement Precursor;

    private:
	Example * example_;
	ExampleList * examples_;
	PropertyMap * mapFromFile_;

	XmlPropertiesDefinitionBuilderElement * propertyMapParser_;
	XmlPropertiesBuilderElement * propertiesBuilder_;
	XmlFloatBoxBuilderElement * valuesBuilder_;
	XmlRegularElementRelay * exampleElement_;

    protected:
	void map(const PropertyMap & map);

	void properties(Properties * props);

	void values(FloatBox * values);

	void startExample(XmlAttributes atts);

	void endExample();

    public:
	XmlExampleListBuilderElement(
	    const char * name,
	    XmlContext * context
	    );

	~XmlExampleListBuilderElement();

	void set(ExampleList * examples);

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    class XmlExampleListMergerElement :
	public XmlRegularElement,
	public BuildDelegation<ExampleList> {

    protected:
	typedef XmlExampleListMergerElement Self;
	typedef XmlRegularElement Precursor;

    private:
	Example * example_;
	ExampleList * examples_;
	PropertyMap * mapFromFile_;
	struct ExampleListMerger;
	ExampleListMerger *merger_;

	XmlPropertiesDefinitionBuilderElement * propertyMapParser_;
	XmlPropertiesBuilderElement * propertiesBuilder_;
	XmlFloatBoxBuilderElement * valuesBuilder_;
	XmlRegularElementRelay * exampleElement_;

    protected:
	void map(const PropertyMap & map);

	void properties(Properties * props);

	void values(FloatBox * values);

	void startExample(XmlAttributes atts);

	void endExample();

    public:
	XmlExampleListMergerElement(
	    const char * name,
	    XmlContext * context
	    );

	~XmlExampleListMergerElement();

	void set(ExampleList * examples);

	void init();

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    /**
       Generic example list parser
       as taken by the generic decision tree trainer
    */
    class XmlExampleListParser :
	public XmlSchemaParser {
    protected:
	typedef XmlExampleListParser Self;
	typedef XmlSchemaParser Precursor;

    private:
	XmlExampleListBuilderElement * exampleListBuilder_;

    public:
	XmlExampleListParser(const Core::Configuration & config) :
	    Precursor(config) {
	    exampleListBuilder_ = new XmlExampleListBuilderElement(
		"example-list", this);
	    setRoot(exampleListBuilder_);
	}
	~XmlExampleListParser() {
	    delete exampleListBuilder_;
	}

	bool parseString(const std::string & str, ExampleList * examples) {
	    exampleListBuilder_->set(examples);
	    return (Precursor::parseString(str.c_str()) == 0);
	}
	bool parseString(const std::string & str) {
	    return parseString(str, new ExampleList(config));
	}

	bool parseStream(std::istream & i, ExampleList * examples) {
	    exampleListBuilder_->set(examples);
	    return (Precursor::parseStream(i) == 0);
	}
	bool parseStream(std::istream & i) {
	    return parseStream(i, new ExampleList(config));
	}

	bool parseFile(const std::string & filename, ExampleList * examples) {
	    exampleListBuilder_->set(examples);
	    return (Precursor::parseFile(filename.c_str()) == 0);
	}
	bool parseFile(const std::string & filename) {
	    return parseFile(filename, new ExampleList(config));
	}
    };



    // ============================================================================
    /**
       Generic example list merger;
       merges examples lists and makes them unique w.r.t. to properties
    */
    class XmlExampleListMerger :
	public XmlSchemaParser {
    protected:
	typedef XmlExampleListMerger Self;
	typedef XmlSchemaParser Precursor;

    private:
	XmlExampleListMergerElement * exampleListMerger_;

    public:
	XmlExampleListMerger(const Core::Configuration & config, ExampleList *examples) :
	    Precursor(config) {
	    exampleListMerger_ = new XmlExampleListMergerElement(
		"example-list", this);
	    exampleListMerger_->set(examples);
	    setRoot(exampleListMerger_);
	}
	~XmlExampleListMerger() {
	    delete exampleListMerger_;
	}

	bool parseString(const std::string & str) {
	    exampleListMerger_->init();
	    return (Precursor::parseString(str.c_str()) == 0);
	}

	bool parseStream(std::istream & i) {
	    exampleListMerger_->init();
	    return (Precursor::parseStream(i) == 0);
	}

	bool parseFile(const std::string & filename) {
	    exampleListMerger_->init();
	    return (Precursor::parseFile(filename.c_str()) == 0);
	}
    };



    // ============================================================================
    /**
       <step name="silence" action="split|cluster">
       <min-obs> 0 </min-obs>
       <min-gain> 0.0 </min-gain>
       <questions> ... </questions>
       </step>
    */
    class XmlStepBuilderElement :
	public XmlMixedElement,
	public BuildDelegation<DecisionTreeTrainer::TrainingPlan::Step> {

    public:
	typedef DecisionTreeTrainer::TrainingPlan::Step Step;

    protected:
	typedef XmlStepBuilderElement Self;
	typedef XmlMixedElement Precursor;

    private:
	Step * step_;

	XmlUnsignedBuilderElement * minObsParser_;
	XmlFloatBuilderElement * minGainParser_;
	XmlEmptyElementRelay * randomizeElement_;
	XmlQuestionRefListBuilderElement * questionRefsBuilder_;

    protected:
	void minObs(const u32 & minObs) {
	    step_->minObs = minObs;
	}

	void minGain(const f64 & minGain) {
	    step_->minGain = minGain;
	}

	void randomize(const XmlAttributes atts);

	void questionRefs(QuestionRefList * questionRefs) {
	    require(questionRefs);
	    require(questionRefs->size() > 0);
	    step_->questionRefs = questionRefs;
	}

    public:
	XmlStepBuilderElement(
	    const char * name,
	    XmlContext * context
	    );

	~XmlStepBuilderElement();

	void set(PropertyMapRef map) { questionRefsBuilder_->set(map); }

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };



    // ============================================================================
    /**
       <decision-tree-training>
       <properties-definition>
       ...
       </properties-definition>
       [<max-leaves> ... </max-leaves>]
       <step name="...">
       ...
       </step>
       ...
       </decision-tree-training>
    */
    class XmlTrainingPlanBuilderElement :
	public XmlRegularElement,
	public BuildDelegation<DecisionTreeTrainer::TrainingPlan> {

    public:
	typedef DecisionTreeTrainer::TrainingPlan::Step Step;
	typedef DecisionTreeTrainer::TrainingPlan TrainingPlan;

    protected:
	typedef XmlTrainingPlanBuilderElement Self;
	typedef XmlRegularElement Precursor;

    private:
	TrainingPlan * trainingPlan_;
	u32 stepCount_;
	PropertyMap * mapFromFile_;

	XmlPropertiesDefinitionBuilderElement * propertyMapParser_;
	XmlUnsignedBuilderElement * maxLeavesParser_;
	XmlStepBuilderElement * stepBuilder_;

    protected:
	void map(const PropertyMap & map);

	void maxLeaves(const u32 & maxLeaves);

	void step(Step * step);

    public:
	XmlTrainingPlanBuilderElement(
	    const char * name,
	    XmlContext * context
	    );

	~XmlTrainingPlanBuilderElement();

	void set(TrainingPlan * trainigPlan);

	void start(const XmlAttributes atts);

	void end();

	void characters(const char *ch, int len);
    };


    // ============================================================================
    /**
       Generic training plan parser
       as taken by the generic decision tree trainer
    */
    class XmlTrainingPlanParser :
	public XmlSchemaParser {
    protected:
	typedef XmlTrainingPlanParser Self;
	typedef XmlSchemaParser Precursor;

    public:
	typedef DecisionTreeTrainer::TrainingPlan TrainingPlan;

    private:
	XmlTrainingPlanBuilderElement * trainingPlanBuilder_;

    public:
	XmlTrainingPlanParser(
	    const Core::Configuration & config) :
	    Precursor(config) {
	    trainingPlanBuilder_ = new XmlTrainingPlanBuilderElement(
		"decision-tree-training", this);
	    setRoot(trainingPlanBuilder_);
	}

	~XmlTrainingPlanParser() {
	    delete trainingPlanBuilder_;
	}

	bool parseString(const std::string & str, TrainingPlan * trainingPlan) {
	    trainingPlanBuilder_->set(trainingPlan);
	    return (Precursor::parseString(str.c_str()) == 0);
	}
	bool parseString(const std::string & str) {
	    return parseString(str, new TrainingPlan);
	}

	bool parseStream(std::istream & i, TrainingPlan * trainingPlan) {
	    trainingPlanBuilder_->set(trainingPlan);
	    return (Precursor::parseStream(i) == 0);
	}
	bool parseStream(std::istream & i) {
	    return parseStream(i, new TrainingPlan);
	}

	bool parseFile(const std::string & filename, TrainingPlan * trainingPlan) {
	    trainingPlanBuilder_->set(trainingPlan);
	    return (Precursor::parseFile(filename.c_str()) == 0);
	}
	bool parseFile(const std::string & filename) {
	    return parseFile(filename, new TrainingPlan);
	}
    };

} // namespace Cart
#endif // _CART_PARSER_HH
