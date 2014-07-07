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
#include "BinaryTree.hh"

using namespace Cart;

// ============================================================================
const BinaryTree::Node::Id BinaryTree::Node::InvalidId =
    Core::Type<BinaryTree::Node::Id>::max;

void BinaryTree::Node::setChilds(Node * leftChild, Node * rightChild) {
    delete leftChild_;
    leftChild_ = leftChild;
    if (leftChild)
	leftChild_->father_ = this;
    delete rightChild_;
    rightChild_ = rightChild;
    if (rightChild)
	rightChild_->father_ = this;
}
// ============================================================================


// ============================================================================
const u32 BinaryTree::InvalidDepth = Core::Type<u32>::max;
const u32 BinaryTree::InvalidSize = Core::Type<u32>::max;

BinaryTree::BinaryTree(
    const Core::Configuration & config,
    Node * root) :
    Precursor(config),
    root_(0),
    depth_(InvalidDepth),
    internalNodeN_(InvalidSize),
    leafN_(InvalidSize) {
    if (root)
	setRoot(root);
}

u32 BinaryTree::checkTree(const Node & node) {
    checkNode(node);
    if (node.isLeaf()) {
	++leafN_;
	return 1;
    } else {
	++internalNodeN_;
	return 1 + std::max(
	    checkTree(node.leftChild()),
	    checkTree(node.rightChild())
	    );
    }
}

void BinaryTree::checkTree() {
    if (root_) {
	depth_ = checkTree(*root_);
    } else {
	depth_ = InvalidDepth;
	internalNodeN_ = InvalidSize;
	leafN_ = InvalidSize;
    }
}

u32 drawBinaryTreeNode(std::ostream & out, const BinaryTree::Node & node, u32 id = 0) {
    if (node.isLeaf()) {
	out << "n" << id++ << " [shape=doublecircle label=" << node.id() << "]" << std::endl;
    } else {
	u32 nodeId = id++;
	out << "n" << nodeId << " [shape=circle label=" << node.id() << "]" << std::endl;
	out << "n" << nodeId << " -> n" << id << std::endl;
	id = drawBinaryTreeNode(out, node.leftChild(), id);
	out << "n" << nodeId << " -> n" << id  << std::endl;
	id = drawBinaryTreeNode(out, node.rightChild(), id);
    }
    return id;
}

void BinaryTree::draw(std::ostream & out) const {
    out << "digraph \"" << fullName() << "\" {" << std::endl
	<< "node [fontname=\"Helvetica\"]" << std::endl
	<< "edge [fontname=\"Helvetica\"]" << std::endl;
    drawBinaryTreeNode(out, root());
    out << "}" << std::endl;
}

void writeBinaryTreeNode(std::ostream & out, const BinaryTree::Node & node, const u32 depth = 0) {
    for (size_t i = 0; i < depth; ++i)
	out << ' ';
    out << "node id: ";
    if (node.id() == BinaryTree::Node::InvalidId)
	out << "[invalid]" << std::endl;
    else
	out << node.id() << std::endl;
    if (!node.isLeaf()) {
	writeBinaryTreeNode(out, node.leftChild(), depth + 2);
	writeBinaryTreeNode(out, node.rightChild(), depth + 2);
    }
}

void BinaryTree::write(std::ostream & out) const {
    out << "binary tree:" << std::endl;
    writeBinaryTreeNode(out, root());
}

void writeXmlBinaryTreeNode(Core::XmlWriter & xml, const BinaryTree::Node & node) {
    Core::XmlOpen open("node");
    if (node.id() != BinaryTree::Node::InvalidId)
	open.operator+(Core::XmlAttribute("id", node.id()));
    xml << open;
    if (node.hasInfo())
	node.info_->writeXml(xml);
    if (!node.isLeaf()) {
	writeXmlBinaryTreeNode(xml, node.leftChild());
	writeXmlBinaryTreeNode(xml, node.rightChild());
    }
    xml << Core::XmlClose("node");
}

void BinaryTree::writeXml(Core::XmlWriter & xml, const std::string & name) const {
    xml << Core::XmlOpen(name);
    writeXmlBinaryTreeNode(xml, root());
    xml << Core::XmlClose(name);
}
// ============================================================================
