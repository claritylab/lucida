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
#include "DecisionTree.hh"
#include "Parser.hh"

#include <Core/CompressedStream.hh>
#include <Core/IoUtilities.hh>
#include <Core/Parameter.hh>
#include <Core/XmlStream.hh>

using namespace Cart;


// ============================================================================
void Cart::write(std::ostream & out, const QuestionRefList & questionRefs) {
    out << "questions:" << std::endl;
    Question::QuestionIndex i = 0;
    out << std::right;
    for (QuestionRefList::const_iterator it = questionRefs.begin();
	 it != questionRefs.end(); ++it, ++i) {
	out << std::setw(3) << i << ". ";
	(*it)->write(out);
	out << std::endl;
    }
}

void Cart::writeXml(Core::XmlWriter & xml, const QuestionRefList & questionRefs) {
    Question::QuestionIndex index = 0;
    xml << Core::XmlOpen("questions");
    for (QuestionRefList::const_iterator it = questionRefs.begin();
	 it != questionRefs.end(); ++it, ++index) {
	// question indicies are not used in the current implementation, leave it unset
	// (*it)->index = index;
	(*it)->writeXml(xml);
    }
    xml << Core::XmlClose("questions");
}
// ============================================================================


// ============================================================================
const Question::QuestionIndex Question::InvalidQuestionIndex = Core::Type<Question::QuestionIndex>::max;

void Question::write(std::ostream & out, const Answer & a) const {
    write(out);
    switch (a) {
    case TRUE:
	out << " Yes!";
	break;
    case FALSE:
	out << " No!";
	break;
    case UNDEF:
	out << " Maybe, maybe not ...";
	break;
    }
}
// ============================================================================



// ============================================================================
ScalarQuestion::ScalarQuestion(
    PropertyMapRef map,
    const std::string & key,
    const std::string & value,
    const std::string & desc
    ) :
    Question(map, desc),
    keyIndex_(map_->key(key)),
    valueIndex_(map_->undefinedIndex) {
    verify(map_->isDefined(keyIndex_));
    valueIndex_ = (*map_)[keyIndex_][value];
    verify(map_->isDefined(valueIndex_));
}

void ScalarQuestion::write(std::ostream & out) const {
    if (!desc_.empty())
	out << "[" << desc_ << "] ";
    out  << "Does the value of \"" << map_->key(keyIndex_) << "\" equal "
	 << "\"" << (*map_)[keyIndex_][valueIndex_] << "\"?";
}

void ScalarQuestion::writeXml(Core::XmlWriter & xml) const {
    Core::XmlOpen xmlOpen("question");
    if (index != InvalidQuestionIndex)
	xmlOpen.operator+(Core::XmlAttribute("index", index));
    if (!desc_.empty())
	xmlOpen.operator+(Core::XmlAttribute("description", desc_));
    xml << xmlOpen
	<< Core::XmlFull("key", map_->key(keyIndex_))
	<< Core::XmlFull("value", (*map_)[keyIndex_][valueIndex_])
	<< Core::XmlClose("question");
}
// ============================================================================



// ============================================================================
SetQuestion::SetQuestion(
    PropertyMapRef map,
    const std::string & key,
    const std::vector<std::string> & values,
    const std::string & desc
    ) :
    Question(map, desc),
    keyIndex_(map_->key(key)),
    begin_(0), end_(0) {
    if(!map_->isDefined(keyIndex_))
    {
		std::cerr << "Key not defined in map: " << key << std::endl;
		verify(0);
    }
    begin_ = new ValueIndex[values.size()];
    end_   = begin_ + values.size();
    std::vector<std::string>::const_iterator it;
    ValueIndex * itt;
    for (it = values.begin(), itt = begin_;
	 it != values.end(); ++it, ++itt) {
	*itt = (*map_)[keyIndex_][*it];
    if(!map_->isDefined(*itt))
    {
		std::cerr << "Value not defined in map: " << *it << " for key " << key << std::endl;
		std::cerr << "Available values: ";
		map_->operator[](keyIndex_).printIdentifiers(std::cerr);
		std::cerr << std::endl;
		verify(0);
    }
    }
    std::sort(begin_, end_);
}

std::string SetQuestion::values2str() const {
    std::ostringstream oss;
    if (begin_ != end_) {
	const ValueIndex * it = begin_;
	oss << (*map_)[keyIndex_][*it];
	for (++it; it != end_; ++it)
	    oss << " " << (*map_)[keyIndex_][*it];
    }
    return oss.str();
}

void SetQuestion::write(std::ostream & out) const {
    if (!desc_.empty())
	out << "[" << desc_ << "] ";
    out << "Is the value of \"" << map_->key(keyIndex_) << "\" in "
	<< "{" << values2str() << "}?";
}

void SetQuestion::writeXml(Core::XmlWriter & xml) const {
    Core::XmlOpen xmlOpen("question");
    if (index != InvalidQuestionIndex)
	xmlOpen.operator+(Core::XmlAttribute("index", index));
    if (!desc_.empty())
	xmlOpen.operator+(Core::XmlAttribute("description", desc_));
    xml << xmlOpen
	<< Core::XmlFull("key", map_->key(keyIndex_))
	<< Core::XmlFull("values", values2str())
	<< Core::XmlClose("question");
}
// ============================================================================



// ============================================================================
const u32 TrainingInformation::InvalidOrder = Core::Type<u32>::max;
const u32 TrainingInformation::InvalidSize  = Core::Type<u32>::max;
const f32 TrainingInformation::InvalidScore = Core::Type<u32>::max;

void TrainingInformation::writeXml(Core::XmlWriter & xml) const {
    xml << Core::XmlOpen("information")
	<< Core::XmlFull("order", order)
	<< Core::XmlFull("size", size)
	<< Core::XmlFull("score", score)
	<< Core::XmlClose("information");
}
// ============================================================================



// ============================================================================
void DecisionTree::Path::write(std::ostream & out) const {
    QuestionRefList::const_iterator questionRefIt;
    AnswerList::const_iterator answerIt;
    for (questionRefIt = questionRefs.begin(), answerIt = answers.begin();
	 questionRefIt != questionRefs.end(); ++questionRefIt, ++answerIt) {
	(*questionRefIt)->write(out, *answerIt);
	out << std::endl;
    }
    out << "class id: " << leaf->id() << std::endl;
}
// ============================================================================



// ============================================================================
const Core::ParameterString DecisionTree::paramCartFilename(
    "decision-tree-file",
    "name of decision tree (aka cart) file");

DecisionTree::DecisionTree(
    const Core::Configuration & config,
    PropertyMapRef map,
    QuestionRefList * questionRefs,
    Node * root
    ) :
    Precursor(config),
    map_(),
    questionRefs_(0) {
    setMap(map);
    if (questionRefs)
	setQuestions(questionRefs);
    if (root)
	setRoot(root);
}

const DecisionTree::Node * DecisionTree::find(const Properties & props) const {
    require_(root_);
    QuestionRefList & questions = *questionRefs_;
    Node * node = root_;
    while (!node->isLeaf())
	switch ((*questions[node->id()])(props)) {
	case TRUE:
	    node = node->leftChild_;
	    break;
	case FALSE:
	    node = node->rightChild_;
	    break;
	case UNDEF:
	    warning() << "undefined answer to \"" << *questions[node->id()] << "\", assume false";
	    node = node->rightChild_;
	    break;
	}
    return node;
}

DecisionTree::NodePtrList DecisionTree::findAll(const Properties & props) const {
    require_(root_);
    QuestionRefList & questions = *questionRefs_;
    NodePtrList nodes;
    NodePtrStack stack;
    stack.push(root_);
    while (!stack.empty()) {
	Node * node = stack.top();
	stack.pop();
	if (node->isLeaf()) {
	    nodes.push_back(node);
	} else {
	    switch ((*questions[node->id()])(props)) {
	    case TRUE:
		stack.push(node->leftChild_);
		break;
	    case FALSE:
		stack.push(node->rightChild_);
		break;
	    case UNDEF:
		stack.push(node->leftChild_);
		stack.push(node->rightChild_);
		break;
	    }
	}
    }
    return nodes;
}

DecisionTree::Path DecisionTree::findPath(const Properties & props) const {
    require_(questionRefs_);
    require_(root_);
    QuestionRefList & questions = *questionRefs_;
    Node * node = root_;
    Path path;
    while (!node->isLeaf()) {
	QuestionRef question = questions[node->id()];
	Answer answer = (*question)(props);
	path.questionRefs.push_back(question);
	path.answers.push_back(answer);
	switch (answer) {
	case TRUE:
	    node = node->leftChild_;
	    break;
	case FALSE:
	    node = node->rightChild_;
	    break;
	case UNDEF:
	    warning() << "undefined answer to \"" << *question << "\", assume false";
	    node = node->rightChild_;
	    break;
	}
    }
    path.leaf = node;
    return path;
}

bool DecisionTree::loadFromString(const std::string & str) {
    XmlDecisionTreeParser parser(config);
    return parser.parseString(str, this);
}

bool DecisionTree::loadFromStream(std::istream & i) {
    XmlDecisionTreeParser parser(config);
    return parser.parseStream(i, this);
}

bool DecisionTree::loadFromFile(std::string filename) {
    if (filename.empty()) {
	filename = paramCartFilename(config);
	verify(!filename.empty());
    }
    log() << "load decision tree from \"" << filename << "\"";
    XmlDecisionTreeParser parser(config);
    return parser.parseFile(filename, this);
}

u32 drawDecisionTreeNode(std::ostream & out, const DecisionTree::Node & node, u32 id = 0) {
    if (node.isLeaf()) {
	out << "n" << id++ << " [shape=doublecircle label=" << node.id() << "]" << std::endl;
    } else {
	u32 nodeId = id++;
	out << "n" << nodeId << " [shape=circle label=" << node.id() << "]" << std::endl;
	out << "n" << nodeId << " -> n" << id << "[label=yes]" << std::endl;
	id = drawDecisionTreeNode(out, node.leftChild(), id);
	out << "n" << nodeId << " -> n" << id << "[label=no]" << std::endl;
	id = drawDecisionTreeNode(out, node.rightChild(), id);
    }
    return id;
}

void DecisionTree::draw(std::ostream & out) const {
    out << "digraph \"" << fullName() << "\" {" << std::endl
	<< "node [fontname=\"Helvetica\"]" << std::endl
	<< "edge [fontname=\"Helvetica\"]" << std::endl;
    drawDecisionTreeNode(out, root());
    out << "}" << std::endl;
}

void writeDecisionTreeNode(std::ostream & out, const DecisionTree::Node & node, const u32 depth = 0) {
    for (size_t i = 0; i < 2 * depth; ++i)
	out << ' ';
    if (node.isLeaf())
	out << "class id: ";
    else
	out << "question id: ";
    if (node.id() == DecisionTree::Node::InvalidId)
	out << "[invalid]" << std::endl;
    else
	out << node.id() << std::endl;
    if (!node.isLeaf()) {
	writeDecisionTreeNode(out, node.leftChild(), depth + 1);
	writeDecisionTreeNode(out, node.rightChild(), depth + 1);
    }
}

void DecisionTree::write(std::ostream & out) const {
    map_->write(out);
    out << std::endl;
    Cart::write(out, *questionRefs_);
    out << std::endl;
    out << "decision tree:" << std::endl;
    writeDecisionTreeNode(out, root());
}

void DecisionTree::writeXml(Core::XmlWriter & xml, const std::string & name) const {
    xml << Core::XmlOpen(name);
    map_->writeXml(xml);
    Cart::writeXml(xml, *questionRefs_);
    Precursor::writeXml(xml, "binary-tree");
    xml << Core::XmlClose(name);
}

void DecisionTree::writeToFile() const {
    std::string filename(paramCartFilename(config));
    if (!filename.empty()) {
	log() << "write decision tree to \"" << filename << "\"";
	Core::XmlOutputStream xml(new Core::CompressedOutputStream(filename));
	xml.generateFormattingHints(true);
	xml.setIndentation(4);
	xml.setMargin(78);
	writeXml(xml);
    } else {
	warning("cannot store decision tree, because no filename is given");
    }
}
// ============================================================================
