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
#include <Core/Parameter.hh>
#include <Core/XmlStream.hh>
#include <Cart/Example.hh>
#include <Cart/Cluster.hh>
#include <Cart/DecisionTree.hh>
#include <Cart/DecisionTreeTrainer.hh>

class CartTrainer :
    public Core::Application {

public:
    CartTrainer() { setTitle("cart-trainer"); }

    std::string getUsage() const { return "Generic CART trainer"; }

    virtual int main(const std::vector<std::string> &arguments);
};

APPLICATION(CartTrainer)


// ============================================================================
using namespace Cart;

static const Core::ParameterBool paramDry(
    "dry",
    "load training file and examples, but don't learn the tree",
    false);

/*
  The default score function calculates the entropy
  over the occuring matrixes, i.e. the propability
  of an arbitrary matrix is just calculated as its
  relative frequency (remember: the input of the scorer
  is list of examples, where each example consists of a
  collection of properties and a matrix).
*/
static const Core::ParameterString paramScorer(
    "score-function",
    "function to minimize",
    "ID3");

int CartTrainer::main(const std::vector<std::string> &arguments) {
    bool train = !paramDry(config);

    if (paramScorer(config) != paramScorer.defaultValue()) {
	error("unknown score function \"%s\"; scorer support is so far limited to \"%s\"",
	      paramScorer(config).c_str(), paramScorer.defaultValue().c_str());
	return 1;
    }
    Cart::ConstScorerRef scorer = Cart::ConstScorerRef(new Cart::ID3(select("id3")));

    DecisionTreeTrainer trainer(config);
    trainer.setScorer(scorer);
    log("load training plan");
    std::string trainingFilename = DecisionTreeTrainer::paramTrainingFilename(config);
    if (!trainingFilename.empty()) {
	trainer.loadFromFile(trainingFilename);
	Core::Channel trainerPlainChannel(select("training"), "plain", Core::Channel::disabled);
	if (trainerPlainChannel.isOpen())
	    trainer.write(trainerPlainChannel);
	Core::XmlChannel trainerXmlChannel(select("training"), "xml", Core::Channel::disabled);
	if (trainerXmlChannel.isOpen())
	    trainer.writeXml(trainerXmlChannel);
    } else {
	train = false;
	warning("unable to parse training plan file \"%s\"; cannot proceed training",
		trainingFilename.c_str());
    }

    ExampleList examples(config);
    log("load examples");
    std::string exampleFilename = ExampleList::paramExampleFilename(config);
    if (!exampleFilename.empty()) {
	examples.loadFromFile(exampleFilename);
	Core::Channel examplePlainChannel(select("example"), "plain", Core::Channel::disabled);
	if (examplePlainChannel.isOpen())
	    examples.write(examplePlainChannel);
	Core::XmlChannel exampleXmlChannel(select("example"), "xml", Core::Channel::disabled);
	if (exampleXmlChannel.isOpen())
	    examples.writeXml(exampleXmlChannel);
    } else {
	train = false;
	warning("unable to parse example list file \"%s\"; cannot proceed training",
		exampleFilename.c_str());
    }

    if (train) {
	DecisionTree tree(config);
	ClusterList * clusters;
	log("train decision tree");
	if (!(clusters = trainer.train(&tree, examples))) {
	    error("error while training decision tree");
	    return 2;
	}

	Core::Channel treePlainChannel(select("decision-tree"), "plain", Core::Channel::disabled);
	if (treePlainChannel.isOpen())
	    tree.write(treePlainChannel);
	Core::XmlChannel treeXmlChannel(select("decision-tree"), "xml", Core::Channel::disabled);
	if (treeXmlChannel.isOpen())
	    tree.writeXml(treeXmlChannel);
	Core::Channel treeDotChannel(select("decision-tree"), "dot", Core::Channel::disabled);
	if (treeDotChannel.isOpen())
	    tree.draw(treeDotChannel);
	tree.writeToFile();

	Core::Channel clusterPlainChannel(select("cluster"), "plain", Core::Channel::disabled);
	if (clusterPlainChannel.isOpen())
	    clusters->write(clusterPlainChannel);
	Core::XmlChannel clusterXmlChannel(select("cluster"), "xml", Core::Channel::disabled);
	if (clusterXmlChannel.isOpen())
	    clusters->writeXml(clusterXmlChannel);
	clusters->writeToFile();
	delete clusters;
    }

    return 0;
}
// ============================================================================
