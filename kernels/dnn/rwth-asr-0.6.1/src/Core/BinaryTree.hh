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
#ifndef _CORE_BINARY_TREE_HH
#define _CORE_BINARY_TREE_HH

#include <vector>
#include <list>
#include <map>
#include <Math/Vector.hh>
#include "ReferenceCounting.hh"
#include "StringUtilities.hh"
#include "Types.hh"

namespace Core{



    /*********************************************************************/
    class BinaryTree : public ReferenceCounted {
    /*********************************************************************/
    public:
	typedef u16 Id;
	typedef u16 LeafNumber;
	typedef Core::Ref<Math::ReferenceCountedVector<LeafNumber> > LeafIndexVector;

	struct TreeStructureEntry {
	    Id id;
	    LeafNumber leafNumber;

	    TreeStructureEntry():id(0),leafNumber(0){};
	    TreeStructureEntry(Id i, LeafNumber n):id(i),leafNumber(n){};

	    void read(Core::BinaryInputStream &i) { i >> id >> leafNumber; }
	    void write(Core::BinaryOutputStream &o) const { o << id << leafNumber; }
	};
	/* This vector defines the structure of the tree to be build in pre-order direction.
	   TreeStructure = invalidId;  indicates a leaf for building the tree.
	*/
	typedef std::vector<TreeStructureEntry> TreeStructure;

	struct Node{
	    Node *left_, *right_, *previous_;
	    u16 id_;
	    u16 leafNumber_;

	    Node():
		left_(0), right_(0), previous_(0),  id_(0), leafNumber_(0) {};
	};
	typedef std::list<const Node*> LeafList;
    public:
	static const Id invalidId;
    private:
	Node *root_;
	LeafList leafList_;
	u16 nLeafs_;
	u16 nNodes_;
	std::map <Id,const Node*> idToNode;
	mutable TreeStructure treeStructure;
	Id cutTreeCounter_;

    private:
	void checkId(Id id) const { require(idToNode.find(id) != idToNode.end()); /* no such id */ }
	void buildTreeStructure() const ;
	void cutTree(
	    Node* node, Id lastNode,
	    Id cutoff,
	    LeafIndexVector newLeafId,
	    Math::Vector<Id>& lookupTable);
    public:
	BinaryTree();
	explicit BinaryTree(const TreeStructure &s);
	~BinaryTree ();
	void build(const TreeStructure &s);
	void draw(std::ostream &os) const;
	void split(Id);
	const LeafList& leafList() const{return  leafList_;}
	Id leafNumber(const LeafList::const_iterator p) const {return (*p)->leafNumber_;}
	Id id(const LeafList::const_iterator p) const {return (*p)->id_;};
	//id is assumed to be sequential
	Id leafNumber(Id id) const;
	Id root() const {return root_->id_;}
	Id left(Id id) const;
	Id right(Id id) const;
	Id previous(Id id) const;
	bool isLeaf(Id id) const;
	LeafIndexVector prune(Id maxLeafs);
	u16 numberOfNodes(){return nNodes_;}
	u16 numberOfLeafs(){return nLeafs_;}

	void write(Core::BinaryOutputStream &o) const;
	void read(Core::BinaryInputStream &i);
    };

    inline Core::BinaryOutputStream& operator<<(Core::BinaryOutputStream &o, const BinaryTree &t) {
	t.write(o); return o;
    }
    inline Core::BinaryInputStream& operator>> (Core::BinaryInputStream& i, BinaryTree &t) {
	t.read(i); return i;
    }

}//namespace

#endif //_CORE_BINARY_TREE_HH
