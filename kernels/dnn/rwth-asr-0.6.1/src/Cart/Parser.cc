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
#include <Core/Hash.hh>
#include <Core/StringUtilities.hh>

#include "Parser.hh"

using Core::XmlSchemaParser;

using Core::XmlContext;
using Core::XmlAttributes;
using Core::XmlElement;
using Core::XmlEmptyElement;
using Core::XmlIgnoreElement;
using Core::XmlMixedElement;
using Core::XmlRegularElement;

using Core::XmlMixedElementRelay;
using Core::XmlRegularElementRelay;

using Core::BuildDelegation;
using XmlBuilder2::XmlStringBuilderElement;
using XmlBuilder2::XmlSignedBuilderElement;
using XmlBuilder2::XmlUnsignedBuilderElement;
using XmlBuilder2::XmlFloatBuilderElement;

using namespace Cart;

// ============================================================================
void XmlIndexedStringBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    const char * c = attr["id"];
    if (c == 0)
	pair_->second = index_++;
    else {
	verify(!index_);
	if (!(ok_ = Core::strconv(std::string(c), pair_->second)))
	    parser()->error(
		"In element \"%s\": non-interpretable value \"%s\"",
		name(), c);
    }
}

void XmlIndexedStringBuilderElement::end() {
    Precursor::end();
    Core::stripWhitespace(pair_->first);
    if (ok_)
	pair_ = delegateAndCreate(pair_);
    else
	ok_ = true;
    pair_->first.clear();
}
// ============================================================================



// ============================================================================
XmlPropertiesDefinitionBuilderElement::XmlPropertiesDefinitionBuilderElement(
    const char * name, Core::XmlContext * context) :
    Precursor(name, context),
    mapPtr_(0) {
    keyParser_ = new XmlStringBuilderElement("key", this);
    keyParser_->setParsedHandler(*this, &Self::key);
    valueParser_ = new XmlIndexedStringBuilderElement("value", this);
    valueParser_->setParsedHandler(*this, &Self::value);
    valueMapParser_ = new XmlMixedElementRelay("value-map", this);
    valueMapParser_->addChild(valueParser_);

    addTransition(initial, 1, keyParser_);
    addTransition(1, initial, valueMapParser_);
    addFinalState(initial);
}

XmlPropertiesDefinitionBuilderElement::~XmlPropertiesDefinitionBuilderElement() {
    delete keyParser_;
    delete valueParser_;
    delete valueMapParser_;
}

void XmlPropertiesDefinitionBuilderElement::key(const std::string & key) {
    keys_.push_back(key);
    values_.push_back(PropertyMap::IndexedStringList());
    Core::stripWhitespace(keys_.back());
    valueParser_->reset();
}

void XmlPropertiesDefinitionBuilderElement::value(const PropertyMap::IndexedString & pair) {
    values_.back().push_back(pair);
}

void  XmlPropertiesDefinitionBuilderElement::set(PropertyMap * mapPtr) {
    mapPtr_ = mapPtr;
}

void XmlPropertiesDefinitionBuilderElement::start(const XmlAttributes attr) {
    require(mapPtr_);
    Precursor::start(attr);
}

void XmlPropertiesDefinitionBuilderElement::end() {
    Precursor::end();
    mapPtr_->set(keys_, values_);
    delegate(mapPtr_);
    keys_.clear();
    values_.clear();
    mapPtr_ = 0;
}

void XmlPropertiesDefinitionBuilderElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
XmlPropertiesBuilderElement::XmlPropertiesBuilderElement(
    const char * name, Core::XmlContext * context) :
    Precursor(name, context),
    mapRef_() {
    keyParser_ = new XmlStringBuilderElement("key", this);
    keyParser_->setParsedHandler(*this, &Self::key);
    valueParser_ = new XmlStringBuilderElement("value", this);
    valueParser_->setParsedHandler(*this, &Self::value);

    addTransition(initial, 1, keyParser_);
    addTransition(1, initial, valueParser_);
    addTransition(1, 1, keyParser_);
    addFinalState(initial);
    addFinalState(1);
}

XmlPropertiesBuilderElement::~XmlPropertiesBuilderElement() {
    delete keyParser_;
    delete valueParser_;
}

void XmlPropertiesBuilderElement::key(const std::string & key) {
    keys_.push_back(key);
    values_.push_back(mapRef_->undefinedString);
    Core::stripWhitespace(keys_.back());
}

void XmlPropertiesBuilderElement::value(const std::string & value) {
    values_.back() = value;
    Core::stripWhitespace(values_.back());
}


void XmlPropertiesBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    require(mapRef_);
    keys_.clear();
    values_.clear();
}

void XmlPropertiesBuilderElement::end() {
    Precursor::end();
    Properties * props = new StoredProperties(mapRef_, keys_, values_);
    delegateOrDelete(props);
}

void XmlPropertiesBuilderElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
XmlQuestionRefListBuilderElement::XmlQuestionRefListBuilderElement(
    const char * name, Core::XmlContext * context) :
    Precursor(name, context),
    mapRef_(),
    questionRefs_(0),
    index_(Question::InvalidQuestionIndex), desc_(),
    key_(), values_(),
    isForEachKey_(false), isForEachValue_(false),
    keyList_(), valueList_() {

    keyBuilder_ = new XmlStringBuilderElement("key", this);
    keyBuilder_->setParsedHandler(*this, &Self::key);
    valueBuilder_ = new XmlStringBuilderElement("value", this);
    valueBuilder_->setParsedHandler(*this, &Self::values);
    valuesBuilder_ = new XmlStringBuilderElement("values", this);
    valuesBuilder_->setParsedHandler(*this, &Self::values);
    questionElement_ = new XmlRegularElementRelay(
	"question", this,
	XmlRegularElementRelay::startHandler(&Self::startQuestion),
	XmlRegularElementRelay::endHandler(&Self::endQuestion)
	);
    questionElement_->addTransition(XmlRegularElementRelay::initial, 1, keyBuilder_);
    questionElement_->addTransition(XmlRegularElementRelay::initial, 2, valueBuilder_);
    questionElement_->addTransition(XmlRegularElementRelay::initial, 2, valuesBuilder_);
    questionElement_->addTransition(                              1, 2, valueBuilder_);
    questionElement_->addTransition(                              1, 2, valuesBuilder_);
    questionElement_->addFinalState(XmlRegularElementRelay::initial);
    questionElement_->addFinalState(                              1);
    questionElement_->addFinalState(                              2);

    forEachKeyElement_ = new XmlMixedElementRelay(
	"for-each-key", this,
	XmlMixedElementRelay::startHandler(&Self::startForEachKey),
	XmlMixedElementRelay::endHandler(&Self::endForEachKey)
	);
    forEachValueElement_ = new XmlMixedElementRelay(
	"for-each-value", this,
	XmlMixedElementRelay::startHandler(&Self::startForEachValue),
	XmlMixedElementRelay::endHandler(&Self::endForEachValue)
	);
    forEachKeyElement_->addChild(questionElement_);
    forEachKeyElement_->addChild(forEachValueElement_);
    forEachValueElement_->addChild(questionElement_);
    forEachValueElement_->addChild(forEachKeyElement_);

    addChild(questionElement_);
    addChild(forEachKeyElement_);
    addChild(forEachValueElement_);

    warnOnQuestionIndex_ = true;
}

XmlQuestionRefListBuilderElement::~XmlQuestionRefListBuilderElement() {
    delete keyBuilder_;
    delete valueBuilder_;
    delete valuesBuilder_;
    delete questionElement_;
    delete forEachKeyElement_;
    delete forEachValueElement_;
}

void XmlQuestionRefListBuilderElement::addQuestion(
    const std::string & key, const std::string & value) {
    if (warnOnQuestionIndex_ && (index_ != Question::InvalidQuestionIndex)) {
	parser()->warning("Question indices are ignored");
	warnOnQuestionIndex_ = false;
    }
    questionRefs_->push_back(
	QuestionRef(new ScalarQuestion(mapRef_, key, value, desc_)));
}

void XmlQuestionRefListBuilderElement::addQuestion(
    const std::string & key, const std::vector<std::string> & values) {
    if (values.size() == 1) {
	addQuestion(key, values.front());
    } else {
	if (warnOnQuestionIndex_ && (index_ != Question::InvalidQuestionIndex)) {
	    parser()->warning("Question indices are ignored");
	    warnOnQuestionIndex_ = false;
	}
	questionRefs_->push_back(
	    QuestionRef(new SetQuestion(mapRef_, key, values, desc_)));
    }
}

void XmlQuestionRefListBuilderElement::forEachValue(const std::string & key) {
    if (isForEachValue_) {
	if (valueList_.empty()) {
	    const Core::Choice & values = (*mapRef_)[key];
	    if (values.nChoices() == 2)
		addQuestion(key, values.begin()->ident());
	    else
		for(Core::Choice::const_iterator it = values.begin();
		    it != values.end(); ++it)
		    addQuestion(key, it->ident());
	} else
	    for (std::vector<std::string>::const_iterator it = valueList_.begin();
		 it != valueList_.end(); ++it)
		addQuestion(key, *it);
    } else {
	if (values_.empty()) {
	    parser()->warning("In element \"%s\": no range for question for key \"%s\"; assume predicate",
			      name(), key.c_str());
	    values_.push_back("true");
	}
	addQuestion(key, values_);
    }
}

void XmlQuestionRefListBuilderElement::forEachKey() {
    if (isForEachKey_) {
	if (keyList_.empty())
	    for(size_t i = 0; i < mapRef_->size(); ++i)
		forEachValue(mapRef_->key(i));
	else
	    for (std::vector<std::string>::const_iterator it = keyList_.begin();
		 it != keyList_.end(); ++it)
		forEachValue(*it);
    } else {
	if (mapRef_->isDefined(key_))
	    forEachValue(key_);
	else
	    parser()->criticalError(
		"In element \"%s\": missing key",
		name());
    }
}

void XmlQuestionRefListBuilderElement::key(const std::string & key) {
    if (!Core::strconv(key, key_))
	parser()->criticalError(
	    "In element \"%s\": non-parseable list \"%s\"",
	    name(), key.c_str());
}

void XmlQuestionRefListBuilderElement::values(const std::string & values) {
    require(!values.empty());
    if (!Core::strconv(values, values_))
	parser()->criticalError(
	    "In element \"%s\": non-parseable list \"%s\"",
	    name(), values.c_str());
}

void XmlQuestionRefListBuilderElement::startForEachKey(const Core::XmlAttributes attr) {
    require(!isForEachKey_);
    isForEachKey_ = true;
    const char * c = attr["keys"];
    if (c) {
	std::string s(c);
	require(!s.empty());
	if (!Core::strconv(s, keyList_))
	    parser()->criticalError(
		"In element \"%s\": non-parseable list \"%s\"",
		name(), s.c_str());
    }
}

void XmlQuestionRefListBuilderElement::endForEachKey() {
    keyList_.clear();
    isForEachKey_ = false;
}

void XmlQuestionRefListBuilderElement::startForEachValue(const Core::XmlAttributes attr) {
    require(!isForEachValue_);
    isForEachValue_ = true;
    const char * c = attr["values"];
    if (c) {
	std::string s(c);
	require(!s.empty());
	if (!Core::strconv(s, valueList_))
	    parser()->criticalError(
		"In element \"%s\": non-parseable list \"%s\"",
		name(), s.c_str());
    }
}

void XmlQuestionRefListBuilderElement::endForEachValue() {
    valueList_.clear();
    isForEachValue_ = false;
}

void XmlQuestionRefListBuilderElement::startQuestion(const XmlAttributes attr) {
    const char * c;
    if ((c = attr["description"]))
	desc_ = c;
    else
	desc_.clear();
    if ((c = attr["index"]))
	if (!Core::strconv(std::string(c), index_)) {
	    parser()->criticalError(
		"In element \"%s\": non-interpretable value \"%s\"",
		name(), c);
	    index_ = Question::InvalidQuestionIndex;
	}
}

void XmlQuestionRefListBuilderElement::endQuestion() {
    require(mapRef_);
    forEachKey();
    key_ = mapRef_->undefinedString;
    values_.clear();
}

void XmlQuestionRefListBuilderElement::set(PropertyMapRef mapRef) {
    require(mapRef);
    mapRef_ = mapRef;
    key_ = mapRef_->undefinedString;
}

void XmlQuestionRefListBuilderElement::start(const Core::XmlAttributes attr) {
    require(!mapRef_->empty());
    Precursor::start(attr);
    questionRefs_ = new QuestionRefList();
    questionRefs_->clear();
}

void XmlQuestionRefListBuilderElement::end() {
    Precursor::end();
    delegateOrDelete(questionRefs_);
}

void XmlQuestionRefListBuilderElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
XmlTrainingInformationBuilderElement::XmlTrainingInformationBuilderElement (
    const char * name,
    Core::XmlContext * context
    ) :
    Precursor(name, context) {
    orderParser_ = new XmlUnsignedBuilderElement(
	"order", this);
    orderParser_->setParsedHandler(*this, &Self::order);
    sizeParser_ = new XmlUnsignedBuilderElement(
	"size", this);
    sizeParser_->setParsedHandler(*this, &Self::size);
    scoreParser_ = new XmlFloatBuilderElement(
	"score", this);
    scoreParser_->setParsedHandler(*this, &Self::score);

    addChild(orderParser_);
    addChild(sizeParser_);
    addChild(scoreParser_);
}

XmlTrainingInformationBuilderElement::~XmlTrainingInformationBuilderElement() {
    delete orderParser_;
    delete sizeParser_;
    delete scoreParser_;
}

void XmlTrainingInformationBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    info_ = new TrainingInformation();
}

void XmlTrainingInformationBuilderElement::end() {
    Precursor::end();
    delegateOrDelete(info_);
    info_ = 0;
}

void XmlTrainingInformationBuilderElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
XmlBinaryTreeRootBuilderElement::XmlBinaryTreeRootBuilderElement (
    const char * name,
    XmlContext * context,
    InformationBuilder * infoBuilder
    ) :
    Precursor(name, context),
    node_(0),
    infoBuilder_(infoBuilder) {
    if (!infoBuilder_)
	infoBuilder_ = new IgnoreInformationBuilder();
    infoBuilder_->init("information", this);
    infoBuilder_->getBuildDelegation()->setCreatedHandler(*this, &Self::info);

    addChild(infoBuilder_->getXmlElement());
    addChild(this);
}

XmlBinaryTreeRootBuilderElement::~XmlBinaryTreeRootBuilderElement () {
    delete infoBuilder_;
}

void XmlBinaryTreeRootBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    Node::Id id;
    const char * c = attr["id"];
    verify(c);
    if (!Core::strconv(std::string(c), id))
	parser()->error(
	    "In element \"%s\": non-interpretable value \"%s\"",
	    name(), c);
    if (!node_) {
	node_ = new BinaryTree::Node(0, id);
    } else if (!node_->leftChild_) {
	node_->leftChild_ = new Node(node_, id);
	node_ = node_->leftChild_;
    } else if (!node_->rightChild_) {
	node_->rightChild_ = new Node(node_, id);
	node_ = node_->rightChild_;
    } else {
	defect();
    }
}

void XmlBinaryTreeRootBuilderElement::end() {
    Precursor::end();
    require(
	(!node_->leftChild_ && !node_->rightChild_) ||
	( node_->leftChild_ &&  node_->rightChild_));
    if (node_->father_ == 0) {
	delegateOrDelete(node_);
	node_ = 0;
    } else {
	node_ = node_->father_;
    }
}

void XmlBinaryTreeRootBuilderElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
XmlDecisionTreeBuilderElement::XmlDecisionTreeBuilderElement(
    const char * name,
    XmlContext * context,
    InformationBuilder * infoBuilder
    ) :
    Precursor(name, context),
    tree_(0),
    mapFromFile_(0) {

    propertyMapParser_ = new XmlPropertiesDefinitionBuilderElement(
	"properties-definition", this);
    propertyMapParser_->setParsedHandler(*this, &Self::map);

    questionRefsBuilder_ = new XmlQuestionRefListBuilderElement(
	"questions", this);
    questionRefsBuilder_->setCreatedHandler(*this, &Self::questionRefs);

    rootBuilder_ = new XmlBinaryTreeRootBuilderElement(
	"node", this, infoBuilder);
    rootBuilder_->setCreatedHandler(*this, &Self::root);

    binaryTreeElement_ = new XmlRegularElementRelay(
	"binary-tree", this);
    binaryTreeElement_->addTransition(XmlRegularElement::initial, 1, rootBuilder_);
    binaryTreeElement_->addFinalState(1);

    addTransition(initial, 1, propertyMapParser_);
    addTransition(      1, 2, questionRefsBuilder_);
    addTransition(      2, 3, binaryTreeElement_);
    addFinalState(3);
}

XmlDecisionTreeBuilderElement::~XmlDecisionTreeBuilderElement() {
    delete propertyMapParser_;
    delete questionRefsBuilder_;
    delete rootBuilder_;
    delete binaryTreeElement_;
}

void XmlDecisionTreeBuilderElement::map(const PropertyMap & map) {
    if (mapFromFile_) {
	parser()->log("keep properties definition from decision tree");
	PropertyMapDiff diff(parser()->getConfiguration(), tree_->map(), *mapFromFile_, true);
	if (diff.hasDifferences())
	    parser()->warning("differences in property map of decision tree and property map defined in file");
	delete mapFromFile_;
	mapFromFile_ = 0;
    }
    questionRefsBuilder_->set(tree_->getMap());
}

void XmlDecisionTreeBuilderElement::questionRefs(QuestionRefList * questionRefs) {
    require(questionRefs);
    tree_->setQuestions(questionRefs);
}

void XmlDecisionTreeBuilderElement::root(Node * root) {
    require(root);
    tree_->setRoot(root);
}

void XmlDecisionTreeBuilderElement::set(DecisionTree * tree) {
    require(!tree_ && tree);
    tree_ = tree;
    if (tree_->map().empty()) {
	propertyMapParser_->set(const_cast<PropertyMap *>(tree_->getMap().get()));
	mapFromFile_ = 0;
    } else {
	mapFromFile_ = new PropertyMap;
	propertyMapParser_->set(mapFromFile_);
    }
}

void XmlDecisionTreeBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    require(tree_);
}

void XmlDecisionTreeBuilderElement::end() {
    Precursor::end();
    delegate(tree_);
    tree_ = 0;
}

void XmlDecisionTreeBuilderElement::characters(const char *ch, int len) {}
// ============================================================================


// ============================================================================
void XmlFloatBoxBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    const char * c;
    if ((c= attr["nRows"])) {
	if (!Core::strconv(std::string(c), nRows_)) {
	    parser()->error(
		"In element \"%s\": non-interpretable \"%s\"",
		name(), c);
	    nRows_ = 0;
	}
    } else {
	parser()->error(
	    "In element \"%s\": missing attribute nRows",
	    name());
	nRows_ = 0;
    }
    if ((c = attr["nColumns"])) {
	if (!Core::strconv(std::string(c), nCols_)) {
	    parser()->error(
		"In element \"%s\": non-interpretable \"%s\"",
		name(), c);
	    nCols_ = 0;
	}
    } else {
	parser()->error(
	    "In element \"%s\": missing attribute nColumns",
	    name());
	nCols_ = 0;
    }
}

bool convert(const std::string & buffer, FloatBox & values) {
    const char * start = buffer.c_str();
    char * end;
    for (FloatBox::vector_iterator it = values.begin();
	 it != values.end(); ++it) {
	*it = ::strtod(start, &end);
	verify_(start != end);
	start = end;
    }
    for (; ::isspace(*start); ++start);
    return (*start == '\0');
}

void XmlFloatBoxBuilderElement::end() {
    Precursor::end();
    require((nRows_ > 0) && (nCols_ > 0));
    FloatBox * values = new FloatBox(nRows_, nCols_);
    convert(cdata_, *values);
    delegateOrDelete(values);
    cdata_.clear();
}

// ============================================================================


// ============================================================================
XmlExampleListBuilderElement::XmlExampleListBuilderElement(
    const char * name,
    XmlContext * context
    ) :
    Precursor(name, context),
    example_(0),
    examples_(0),
    mapFromFile_(0) {

    propertyMapParser_ = new XmlPropertiesDefinitionBuilderElement(
	"properties-definition", this);
    propertyMapParser_->setParsedHandler(*this, &Self::map);

    propertiesBuilder_ = new XmlPropertiesBuilderElement(
	"properties", this);
    propertiesBuilder_->setCreatedHandler(*this, &Self::properties);

    valuesBuilder_ = new XmlFloatBoxBuilderElement(
	"matrix-f64", this);
    valuesBuilder_->setCreatedHandler(*this, &Self::values);

    exampleElement_ = new XmlRegularElementRelay(
	"example", this,
	XmlRegularElementRelay::startHandler(&Self::startExample),
	XmlRegularElementRelay::endHandler(&Self::endExample));
    exampleElement_->addTransition(XmlRegularElement::initial, 1, propertiesBuilder_);
    exampleElement_->addTransition(                         1, 2, valuesBuilder_);
    exampleElement_->addFinalState(2);

    addTransition(initial, 1, propertyMapParser_);
    addTransition(      1, 1, exampleElement_);
    addFinalState(1);
}

XmlExampleListBuilderElement::~XmlExampleListBuilderElement() {
    delete propertyMapParser_;
    delete propertiesBuilder_;
    delete valuesBuilder_;
    delete exampleElement_;
}

void XmlExampleListBuilderElement::map(const PropertyMap & map) {
    if (mapFromFile_) {
	parser()->log("keep properties definition from example list");
	PropertyMapDiff diff(parser()->getConfiguration(), examples_->map(), *mapFromFile_, true);
	if (diff.hasDifferences())
	    parser()->warning("differences in property map of example list and property map defined in file");
	delete mapFromFile_;
	mapFromFile_ = 0;
    }
    propertiesBuilder_->set(examples_->getMap());
}

void XmlExampleListBuilderElement::properties(Properties * props) {
    require(props);
    example_->properties = props;
}

void XmlExampleListBuilderElement::values(FloatBox * values) {
    require(values);
    example_->values = values;
}

void XmlExampleListBuilderElement::startExample(XmlAttributes atts) {
    example_ = new Example;
    const char * c = atts["nObservations"];
    if (c) {
	if (!Core::strconv(std::string(c), example_->nObs))
	    parser()->error(
		"In element \"%s\": non-interpretable value \"%s\"",
		name(), c);
    } else
	example_->nObs = 1;
}

void XmlExampleListBuilderElement::endExample() {
    examples_->add(example_);
    example_ = 0;
}

void XmlExampleListBuilderElement::set(ExampleList * examples) {
    require(!examples_ && examples);
    examples_ = examples;
    if (examples_->map().empty()) {
	propertyMapParser_->set(const_cast<PropertyMap *>(examples_->getMap().get()));
	mapFromFile_ = 0;
    } else {
	mapFromFile_ = new PropertyMap;
	propertyMapParser_->set(mapFromFile_);
    }
}

void XmlExampleListBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    require(examples_);
}

void XmlExampleListBuilderElement::end() {
    Precursor::end();
    delegate(examples_);
    examples_ = 0;
}

void XmlExampleListBuilderElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
struct XmlExampleListMergerElement::ExampleListMerger {
    XmlExampleListMergerElement *father;
    typedef Core::hash_map<Properties *, Example *, Properties::PtrHashFcn, Properties::PtrEqual> ExampleMap;
    ExampleMap exampleMap;

    ExampleListMerger(XmlExampleListMergerElement *father) : father(father) {
	for (ExampleList::iterator itExample = father->examples_->begin(), endExample = father->examples_->end();
	     itExample != endExample; ++itExample)
	    if (!exampleMap.insert(std::make_pair((*itExample)->properties, &**itExample)).second)
		father->parser()->criticalError("Initial example list contains examples eith equal properties.");
    }

    void merge(Example *example) {
	std::pair<ExampleMap::iterator, bool> result = exampleMap.insert(std::make_pair(example->properties, example));
	if (result.second) {
	    father->examples_->add(example);
	} else {
	    Example &trgExample = *result.first->second;
	    trgExample.nObs += example->nObs;
	    *trgExample.values += *example->values;
	    delete example;
	}
    }
};

XmlExampleListMergerElement::XmlExampleListMergerElement(
    const char * name,
    XmlContext * context
    ) :
    Precursor(name, context),
    example_(0),
    examples_(0),
    mapFromFile_(0),
    merger_(0) {

    propertyMapParser_ = new XmlPropertiesDefinitionBuilderElement(
	"properties-definition", this);
    propertyMapParser_->setParsedHandler(*this, &Self::map);

    propertiesBuilder_ = new XmlPropertiesBuilderElement(
	"properties", this);
    propertiesBuilder_->setCreatedHandler(*this, &Self::properties);

    valuesBuilder_ = new XmlFloatBoxBuilderElement(
	"matrix-f64", this);
    valuesBuilder_->setCreatedHandler(*this, &Self::values);

    exampleElement_ = new XmlRegularElementRelay(
	"example", this,
	XmlRegularElementRelay::startHandler(&Self::startExample),
	XmlRegularElementRelay::endHandler(&Self::endExample));
    exampleElement_->addTransition(XmlRegularElement::initial, 1, propertiesBuilder_);
    exampleElement_->addTransition(                         1, 2, valuesBuilder_);
    exampleElement_->addFinalState(2);

    addTransition(initial, 1, propertyMapParser_);
    addTransition(      1, 1, exampleElement_);
    addFinalState(1);
}

XmlExampleListMergerElement::~XmlExampleListMergerElement() {
    delete propertyMapParser_;
    delete propertiesBuilder_;
    delete valuesBuilder_;
    delete exampleElement_;
    delete merger_;
}

void XmlExampleListMergerElement::map(const PropertyMap & map) {
    if (mapFromFile_) {
	verify(!examples_->map().empty());
	PropertyMapDiff diff(parser()->getConfiguration(), examples_->map(), *mapFromFile_, true);
	if (diff.hasDifferences())
	    parser()->warning("differences in property maps of merged example files");
	delete mapFromFile_;
    }
    propertiesBuilder_->set(examples_->getMap());
}

void XmlExampleListMergerElement::properties(Properties * props) {
    require(props);
    example_->properties = props;
}

void XmlExampleListMergerElement::values(FloatBox * values) {
    require(values);
    example_->values = values;
}

void XmlExampleListMergerElement::startExample(XmlAttributes atts) {
    example_ = new Example;
    const char * c = atts["nObservations"];
    if (c) {
	if (!Core::strconv(std::string(c), example_->nObs))
	    parser()->error(
		"In element \"%s\": non-interpretable value \"%s\"",
		name(), c);
    } else
	example_->nObs = 1;
}

void XmlExampleListMergerElement::endExample() {
    merger_->merge(example_);
    example_ = 0;
}

void XmlExampleListMergerElement::set(ExampleList * examples) {
    require(!examples_ && examples);
    examples_ = examples;
    merger_ = new ExampleListMerger(this);
}

void XmlExampleListMergerElement::init() {
    require(examples_);
    if (examples_->map().empty()) {
	propertyMapParser_->set(const_cast<PropertyMap *>(examples_->getMap().get()));
	mapFromFile_ = 0;
    } else {
	mapFromFile_ = new PropertyMap;
	propertyMapParser_->set(mapFromFile_);
    }
}

void XmlExampleListMergerElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    require(examples_);
}

void XmlExampleListMergerElement::end() {
    Precursor::end();
    delegate(examples_);
}

void XmlExampleListMergerElement::characters(const char *ch, int len) {}
// ============================================================================



// ============================================================================
XmlStepBuilderElement::XmlStepBuilderElement(
    const char * name,
    XmlContext * context
    ) :
    Precursor(name, context),
    step_(0) {

    minObsParser_ = new XmlUnsignedBuilderElement(
	"min-obs", this);
    minObsParser_->setParsedHandler(*this, &Self::minObs);

    minGainParser_ = new XmlFloatBuilderElement(
	"min-gain", this);
    minGainParser_->setParsedHandler(*this, &Self::minGain);

    randomizeElement_ = new XmlEmptyElementRelay(
	"randomize", this,
	XmlRegularElementRelay::startHandler(&Self::randomize));

    questionRefsBuilder_ = new XmlQuestionRefListBuilderElement(
	"questions", this);
    questionRefsBuilder_->setCreatedHandler(*this, &Self::questionRefs);

    addChild(minObsParser_);
    addChild(minGainParser_);
    addChild(randomizeElement_);
    addChild(questionRefsBuilder_);
}

XmlStepBuilderElement::~XmlStepBuilderElement() {
    delete minObsParser_;
    delete minGainParser_;
    delete randomizeElement_;
    delete questionRefsBuilder_;
}

void XmlStepBuilderElement::randomize(const XmlAttributes atts) {
    const char * c = atts["nQuestion"];
    if (c) {
	std::string s(c);
	if (!Core::strconv(s, step_->nRandomQuestion))
	    parser()->criticalError(
		"In element \"%s\": non-parseable integer \"%s\"",
		name(), s.c_str());
	if (step_->nRandomQuestion < 1)
	    step_->nRandomQuestion = 1;
    } else
	step_->nRandomQuestion = 5;
}

void XmlStepBuilderElement::start(const XmlAttributes attr) {
    Precursor::start(attr);
    step_ = new Step;
    const char * c;
    if ((c = attr["name"]))
	step_->name = c;
    if ((c = attr["action"]))
	step_->action = c;
}

void XmlStepBuilderElement::end() {
    require(step_->questionRefs);
    Precursor::end();
    delegateOrDelete(step_);
    step_ = 0;
}

void XmlStepBuilderElement::characters(const char *ch, int len) {}
// ============================================================================


// ============================================================================
XmlTrainingPlanBuilderElement::XmlTrainingPlanBuilderElement(
    const char * name,
    XmlContext * context
    ) :
    Precursor(name, context),
    trainingPlan_(0),
    stepCount_(0),
    mapFromFile_(0) {

    propertyMapParser_ = new XmlPropertiesDefinitionBuilderElement(
	"properties-definition", this);
    propertyMapParser_->setParsedHandler(*this, &Self::map);

    maxLeavesParser_ = new XmlUnsignedBuilderElement(
	"max-leaves", this);
    maxLeavesParser_->setParsedHandler(*this, &Self::maxLeaves);

    stepBuilder_ = new XmlStepBuilderElement(
	"step", this);
    stepBuilder_->setCreatedHandler(*this, &Self::step);

    addTransition(initial, 1, propertyMapParser_);
    addTransition(initial, 2, propertyMapParser_);
    addTransition(      1, 2, maxLeavesParser_);
    addTransition(      2, 2, stepBuilder_);
    addFinalState(      2);
}

XmlTrainingPlanBuilderElement::~XmlTrainingPlanBuilderElement() {
    delete propertyMapParser_;
    delete maxLeavesParser_;
    delete stepBuilder_;
}

void XmlTrainingPlanBuilderElement::map(const PropertyMap & map) {
    if (mapFromFile_) {
	parser()->log("keep properties definition from training");
	PropertyMapDiff diff(parser()->getConfiguration(), *trainingPlan_->map, *mapFromFile_, true);
	if (diff.hasDifferences())
	    parser()->warning("differences in property map of training and property map defined in file");
	delete mapFromFile_;
	mapFromFile_ = 0;
    }
    stepBuilder_->set(trainingPlan_->map);
}

void XmlTrainingPlanBuilderElement::maxLeaves(const u32 & maxLeaves) {
    trainingPlan_->maxLeaves = maxLeaves;
}

void XmlTrainingPlanBuilderElement::step(Step * step) {
    ++stepCount_;
    if (step->name == "") {
	std::ostringstream oss;
	oss << stepCount_ << ". step";
	step->name = oss.str();
    }
    trainingPlan_->steps.push_back(step);
}

void XmlTrainingPlanBuilderElement::set(TrainingPlan * trainingPlan) {
    require(!trainingPlan_ && trainingPlan);
    trainingPlan_ = trainingPlan;
    if (trainingPlan_->map->empty()) {
	propertyMapParser_->set(const_cast<PropertyMap *>(trainingPlan_->map.get()));
	mapFromFile_ = 0;
    } else {
	mapFromFile_ = new PropertyMap;
	propertyMapParser_->set(mapFromFile_);
    }
}

void XmlTrainingPlanBuilderElement::start(const XmlAttributes attr) {
    require(trainingPlan_);
    Precursor::start(attr);
    stepCount_ = 0;
}

void XmlTrainingPlanBuilderElement::end() {
    require(trainingPlan_->steps.size() > 0);
    Precursor::end();
    delegate(trainingPlan_);
    trainingPlan_ = 0;
}

void XmlTrainingPlanBuilderElement::characters(const char *ch, int len) {}
// ============================================================================
