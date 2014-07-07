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
#include <stack>

#include "BinaryTree.hh"
#include <Core/Assertions.hh>

namespace Core {

    const BinaryTree::Id BinaryTree::invalidId=Type<u16>::max;

    BinaryTree::BinaryTree(){
	root_ = new Node;
	root_->id_=0;
	idToNode[root_->id_]=root_;
	leafList_.push_back(root_);
	root_->leafNumber_=0;
	treeStructure.clear();
	nNodes_++;
	cutTreeCounter_ = 0;
    }


    BinaryTree::BinaryTree(const TreeStructure  &s) :
	root_(0) {
	cutTreeCounter_ = 0;
	build(s);
    }

    BinaryTree::~BinaryTree(){
	std::stack<Node*> stack;
	stack.push(root_);
	while (!stack.empty()) {
	    Node *node = stack.top(); stack.pop();
	    if (node->right_!=0) stack.push(node->right_);
	    if (node->left_!=0)  stack.push(node->left_);
	    delete node;
	}
    }

    void BinaryTree::buildTreeStructure(void) const{
	require(root_);
	std::stack<const Node*> stack;
	stack.push(root_);
	while (!stack.empty()) {
	    const Node *node = stack.top(); stack.pop();
	    if ((node->left_!= 0) && (node->right_ != 0) ){
		treeStructure.push_back(TreeStructureEntry(node->id_,invalidId)); //node
		stack.push(node->right_);
		stack.push(node->left_);
	    }
	    else treeStructure.push_back(TreeStructureEntry(node->id_,node->leafNumber_)); //leaf
	}
    }

    void BinaryTree::build(const TreeStructure  &s){
	leafList_.clear();
	treeStructure=s;
	u16 i=0;
	nLeafs_=0;
	nNodes_=0;
	require(!root_);
	root_ = new Node;
	std::stack<Node*> stack;
	stack.push(root_);
	while (!stack.empty()) {
	    Node *node = stack.top(); stack.pop();
	    verify(i<s.size());
	    if(s[i].leafNumber==invalidId){ //node
		node->id_=s[i].id;
		idToNode[node->id_]=node;
		node->leafNumber_=invalidId; //no leaf
		node->left_=new Node;
		node->left_->previous_=node;
		node->right_=new Node;
		node->right_->previous_=node;
		stack.push(node->right_);
		stack.push(node->left_);
		nNodes_++;
	    }
	    else { //leaf
		leafList_.push_back(node);
		node->id_=s[i].id;;
		idToNode[node->id_]=node;
		node->leafNumber_=s[i].leafNumber;
		++nLeafs_;
	    }
	    ++i;
	}
    }

    void BinaryTree::draw(std::ostream &os) const {
	os << "digraph G {" << std::endl
	   << "node [fontname=\"Helvetica\"]" << std::endl
	   << "edge [fontname=\"Helvetica\"]" << std::endl;
	std::stack<const Node*> stack;
	stack.push(root_);
	while (!stack.empty()) {
	    const Node *node = stack.top(); stack.pop();
	    if ((node->left_ != 0) && (node->right_ != 0)) {
		os << form("n%p [label=\"%d\\nleaf  number: %d\"]\n",
			   (void*) node, node->id_,node->leafNumber_)
		   << form("n%p -> n%p \n", (void*) node, (void*) node->left_)
		   << form("n%p -> n%p \n", (void*) node, (void*) node->right_)
		   << form("n%p -> n%p \n", (void*) node, (void*) node->previous_);
		stack.push(node->right_);
		stack.push(node->left_);
	    } else {
		os << form("n%p [shape=box label=\"%d\\nclass: %d\"]\n",
			   (void*) node, node->id_,node->leafNumber_)
		   << form("n%p -> n%p \n",  (void*) node, (void*) node->previous_);
	    }
	}

	os << "}" << std::endl;
    }

    BinaryTree::LeafIndexVector BinaryTree::prune(Id maxLeafs){

	leafList_.clear();

	BinaryTree::LeafIndexVector newLeafId(
	    new Math::ReferenceCountedVector<BinaryTree::LeafNumber> (nLeafs_,invalidId));

	Math::Vector<Id> lookupTable(2*nLeafs_-1);

	if(nLeafs_<=maxLeafs){
	    for(size_t i=0; i<newLeafId->size(); ++i)
		(*newLeafId)[i]=i;
	}
	else{
	    for(size_t i=0; i<newLeafId->size(); ++i)
		(*newLeafId)[i]=i;
	    Id cutoff=2*maxLeafs-1;
	    cutTreeCounter_ = 0;
	    cutTree(root_, root_->id_, cutoff , newLeafId, lookupTable);

	    for(size_t i=0; i<newLeafId->size(); ++i)
		(*newLeafId)[i]=lookupTable[(*newLeafId)[i]];
	}
	nLeafs_=maxLeafs;
	nNodes_=nLeafs_-1;
	return newLeafId;
    }

    void BinaryTree::cutTree(
	Node* node,
	Id lastNode,
	Id cutoff,
	LeafIndexVector newLeafId,
	Math::Vector<Id>& lookupTable ){

	if ((node->left_ != 0) && (node->right_ != 0)){

	    if(node->left_->id_>=cutoff){
		ensure(node->right_->id_ >= cutoff);
		cutTree(node->left_,lastNode,cutoff,newLeafId,lookupTable);
		delete node->left_;
		node->left_=0;
		cutTree(node->right_,lastNode,cutoff,newLeafId,lookupTable);
		delete node->right_;
		node->right_=0;
		if (node->id_ < cutoff) {
		    node->leafNumber_=cutTreeCounter_;
		    lookupTable[node->id_]=cutTreeCounter_;
		    leafList_.push_back(node);
		    ++cutTreeCounter_;
		}
	    }
	    else{
		ensure(node->right_->id_ < cutoff);
		cutTree(node->left_,node->left_->id_,cutoff,newLeafId,lookupTable);
		cutTree(node->right_,node->right_->id_ ,cutoff,newLeafId,lookupTable);
	    }
	}
	else { //leaf
	    (*newLeafId)[node->leafNumber_]=lastNode;
	    if(node->id_<cutoff){
		node->leafNumber_=cutTreeCounter_;
		lookupTable[node->id_]=cutTreeCounter_;
		leafList_.push_back(node);
		++cutTreeCounter_;
	    }
	    return;
	}
    }



    BinaryTree::Id BinaryTree::leafNumber(BinaryTree::Id id) const{
	checkId(id);
	return idToNode.find(id)->second->leafNumber_;
    }

    BinaryTree::Id BinaryTree::left(BinaryTree::Id id) const{
	checkId(id);
	if (idToNode.find(id)->second->left_ != 0 )
	    return idToNode.find(id)->second->left_->id_;
	else return invalidId;
    }

    BinaryTree::Id BinaryTree::right(BinaryTree::Id id) const{
	checkId(id);
	if (idToNode.find(id)->second->right_ != 0 ) return idToNode.find(id)->second->right_->id_;
	else return invalidId;
    }

    BinaryTree::Id BinaryTree::previous(BinaryTree::Id id) const{
	checkId(id);
	if (idToNode.find(id)->second->previous_ != 0 ) return idToNode.find(id)->second->previous_->id_;
	else return invalidId;

    }

    bool BinaryTree::isLeaf(Id id) const {
	const Node * node = idToNode.find(id)->second;
	if(node->left_==0 && node->right_==0) return true;
	else return false;
    }

    void BinaryTree::write(Core::BinaryOutputStream &o) const
    {
	if (treeStructure.empty())
	    buildTreeStructure();

	o << (u32)treeStructure.size();
	std::vector<TreeStructureEntry>::const_iterator entry;
	for(entry = treeStructure.begin(); entry != treeStructure.end(); ++ entry)
	    entry->write(o);
    }

    void BinaryTree::read(Core::BinaryInputStream &i)
    {
	u32 s; i >> s; treeStructure.resize(s);
	std::vector<TreeStructureEntry>::iterator entry;
	for(entry = treeStructure.begin(); entry != treeStructure.end(); ++ entry)
	    entry->read(i);

	build(treeStructure);
    }
} //namespace
