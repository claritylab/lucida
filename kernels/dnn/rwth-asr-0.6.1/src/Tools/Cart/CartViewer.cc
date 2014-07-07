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
#include <Core/Channel.hh>
#include <Core/Choice.hh>
#include <Core/Parameter.hh>
#include <Core/XTermUtilities.hh>
#include <Cart/DecisionTree.hh>
#include <Cart/Properties.hh>


class CartViewer :
    public Core::Application {
protected:
    typedef std::vector<std::string> StringList;

    static const u32 HEADER_X_OFFSET = 0;
    static const u32 HEADER_Y_OFFSET = 1;
    static const u32 BODY_X_OFFSET   = 1;
    static const u32 BODY_Y_OFFSET   = 1;
    static const u32 FOOTER_X_OFFSET = 0;
    static const u32 FOOTER_Y_OFFSET = 1;

protected:
    Cart::DecisionTree * tree_;
    u8 mode_;
    StringList keys_;
    size_t maxKeySize_;

protected:
    void displayMask(u32 row, u32 col);

    void displayValues(u32 row, u32 col, Cart::PropertyMap::Index keyIndex);

    void displayError(u32 row, u32 col, const std::string & error);

    void displayString(u32 row, u32 col, const std::string & s);

    bool readValue(u32 row, u32 col, Cart::PropertyMap::Index keyIndex, std::string & value);

    void displayPath(Cart::DecisionTree::Path & path);

    void displayMultiple(Cart::DecisionTree::NodePtrList & list);

    bool readCommand();

    void run();

public:
    CartViewer() :
	Core::Application(),
	tree_(0),
	mode_(0),
	keys_(),
	maxKeySize_(0) {
	setTitle("cart-viewer");
    }
    ~CartViewer() {
	delete tree_;
    }

    virtual std::string getUsage() const {
	return
	    "Generic CART viewer\n\nSpecify tree either as (first) command line\nparameter or as value of decision-tree.file.";
    }

    int main(const std::vector<std::string> &arguments);

};

APPLICATION(CartViewer)


// ============================================================================
using namespace Cart;
using std::cout;
using std::cin;
using std::flush;
using std::endl;

const Core::ParameterBool paramInteractive(
    "interactive",
    "activate interactive mode, i.e. browse the cart",
    false);

int CartViewer::main(const std::vector<std::string> & argv) {
    {
	std::string filename =
	    (!argv.empty()) ? argv[0] : DecisionTree::paramCartFilename(select("decision-tree"));
	verify(!filename.empty());
	tree_ = new DecisionTree(config);
	if (!tree_->loadFromFile(filename)) {
	    error("Could not load decision tree from file \"%s\"", filename.c_str());
	    return 1;
	}

	Core::Channel plainChannel(select("decision-tree"), "plain", Core::Channel::disabled);
	if (plainChannel.isOpen())
	    tree_->write(plainChannel);
	Core::XmlChannel xmlChannel(select("decision-tree"), "xml", Core::Channel::disabled);
	if (xmlChannel.isOpen())
	    tree_->writeXml(xmlChannel);
	Core::Channel dotChannel(select("decision-tree"), "dot", Core::Channel::disabled);
	if (dotChannel.isOpen())
	    tree_->draw(dotChannel);
	Core::Channel statChannel(select("decision-tree"), "statistics", Core::Channel::disabled);
	if (statChannel.isOpen())
	    statChannel << "depth  " << tree_->depth()   << std::endl <<
			   "nodes  " << tree_->nNodes()  << std::endl <<
			   "leaves " << tree_->nLeaves() << std::endl;
    }

    if (paramInteractive(config)) {
	{
	    const PropertyMap & map = tree_->map();
	    keys_.resize(map.size());
	    for (PropertyMap::Index i = 0; i < PropertyMap::Index(map.size()); ++i) {
		keys_[i] = map.key(i);
		maxKeySize_ = std::max(maxKeySize_, keys_[i].size());
	    }
	}
	log("start interactive mode");
	run();
    }

    return 0;
}

void CartViewer::displayMask(u32 row, u32 col) {
    for (StringList::iterator it = keys_.begin();
	 it != keys_.end(); ++it, ++row) {
	cout << xterm::move(row, col) << *it
	     << xterm::move(row, col + maxKeySize_ + 1) << ":";
    }
}

void CartViewer::displayValues(u32 row, u32 col, PropertyMap::Index keyIndex) {
    cout << xterm::move(row, col) << xterm::kill
	 << "values: ";
    tree_->map()[keyIndex].printIdentifiers(cout);
}

void CartViewer::displayError(u32 row, u32 col, const std::string & error) {
    cout << xterm::move(row, col) << xterm::kill
	 << xterm::red << "Error: \"" << error << "\""
	 << xterm::normal;
}

void CartViewer::displayString(u32 row, u32 col, const std::string & s) {
    cout << xterm::move(row, col) << xterm::kill << s;
}

bool CartViewer::readValue(u32 row, u32 col, PropertyMap::Index keyIndex, std::string & value) {
    value.clear();
    cout << xterm::move(row, col) << " " << flush;
    std::getline(cin, value);
    if (value.empty()) {
	value = tree_->map().undefinedString;
	return true;
    }
    const Core::Choice & choice = tree_->map()[keyIndex];
    if (tree_->map().isDefined(choice[value]))
	return true;
    for (Core::Choice::const_iterator it = choice.begin();
	 it != choice.end(); ++it) {
	if (it->ident().find(value) == 0) {
	    value = it->ident();
	    return true;
	}
    }
    return false;
}

void CartViewer::displayPath(DecisionTree::Path & path) {
    AnswerList::const_iterator     ait = path.answers.begin();
    QuestionRefList::const_iterator qit = path.questionRefs.begin();
    for (size_t i = 1; ait != path.answers.end(); ++i, ++ait, ++qit) {
	cout << std::setw(2) << i << ". ";
	(*qit)->write(cout, *ait);
	cout << endl;
    }
    cout << endl
	 << "class id is " << path.leaf->id() << endl
	 << endl;
}

void CartViewer::displayMultiple(DecisionTree::NodePtrList & list) {
    switch (list.size()) {
    case 1:
	cout << "class id is " << list.front()->id() << endl;
	break;
    case 2:
	cout << "class ids are " << list.front()->id()
	     << " and " << list.back()->id() << endl;
	break;
    default:
	DecisionTree::NodePtrList::const_iterator it = list.begin();
	cout << "class ids are ";
	for (size_t i = 0; i < list.size() - 1; ++i)
	    cout << list[i]->id() << ", ";
	cout << "and " << list.back()->id() << endl;
    }
    cout << endl;
}

bool CartViewer::readCommand() {
    std::string cmd;
    for (;;) {
	cout << "Press enter or choose [q]uit, [c]lassification, [m]ultification" << endl
	     << ">  " << flush;
	getline(cin, cmd); cout << '\b';
	if (cmd.empty())
	    return true;
	else switch (cmd.at(0)) {
	case 'q':
	    return false;
	case 'c':
	    mode_ = 0;
	    cout << "Switch to classification mode." << endl;
	    break;
	case 'm':
	    mode_ = 1;
	    cout << "Switch to multification mode." << endl;
	    break;
	default:
	    cout << "Unknown command \"" << cmd << "\"" << endl;
	}
    }
}


void CartViewer::run() {
    StringList values(tree_->map().size(), tree_->map().undefinedString);
    PropertyMap::Index numberOfKeys = PropertyMap::Index(keys_.size());

    u32 headerX =
	1 + HEADER_X_OFFSET;
    u32 headerY =
	1 + HEADER_Y_OFFSET;
    u32 bodyX =
	1 + BODY_X_OFFSET;
    u32 bodyY =
	headerY + 1 + BODY_Y_OFFSET;
    u32 footerX =
	1 + FOOTER_X_OFFSET;
    u32 footerY =
	bodyY + u32(numberOfKeys) +
	FOOTER_Y_OFFSET;
    u32 inputY;
    u32 inputX =
	bodyX + u32(maxKeySize_) + 3;

    do {
	cout << xterm::clear;
	displayMask(bodyY, bodyX);
	StringList::iterator valueIt = values.begin();
	inputY = bodyY;
	for (PropertyMap::Index i = 0; i < numberOfKeys; ++i, ++inputY, ++valueIt) {
	    displayString(headerY, headerX, "Please enter value");
	    displayValues(footerY, footerX, i);
	    while (!readValue(inputY, inputX, i, *valueIt)) {
		displayError(headerY, headerX, "Could not find matching value, please try again");
		cout << xterm::move(inputY, inputX) << xterm::kill;
	    }
	    displayString(inputY, inputX, *valueIt);
	}
	displayString(headerY, headerX, "Classify ...");
	displayString(footerY, footerX, "");
	cout << xterm::move(footerY, 0) << endl;

	Properties * props = new StoredProperties(tree_->getMap(), keys_, values);
	if (mode_ == 0) {
	    DecisionTree::Path path = tree_->findPath(*props);
	    displayPath(path);
	} else {
	    DecisionTree::NodePtrList list = tree_->findAll(*props);
	    displayMultiple(list);
	}
	delete props;

    } while(readCommand());
}
// ============================================================================
