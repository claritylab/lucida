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
#ifndef _CART_BINARY_TREE_HH
#define _CART_BINARY_TREE_HH

#include <Core/Assertions.hh>
#include <Core/BinaryTree.hh>
#include <Core/Component.hh>
#include <Core/Types.hh>
#include <Core/XmlStream.hh>

#include <stack>


namespace Cart {


// ============================================================================
    /**
       Generic information;
       empty by default.
    */
    struct Information {
	virtual ~Information() {}
	virtual void writeXml(Core::XmlWriter & xml) const {}
    };

// ============================================================================
    /**
     <binary-tree>
       <node>
	 <node id="...">
	   [<information>
	     ...
	   </information>]
	 </node>
	 <node>
	   ...
	 </node>
       </node>
     </binary-tree>
    */
    class BinaryTree :
	public Core::Component {
    protected:
	typedef BinaryTree Self;
	typedef Core::Component Precursor;

    public:
	class Node {
	public:
	    typedef u32 Id;
	    static const Id InvalidId;

	public:
	    Id id_;
	    Information * info_;

	    Node * father_;
	    Node * leftChild_;
	    Node * rightChild_;

	public:
	    Node(Node * father = 0, Id id = InvalidId, Information * info = 0) :
		id_(id),
		info_(info),
		father_(father),
		leftChild_(0),
		rightChild_(0) {}
	    ~Node() {
		delete leftChild_;
		delete rightChild_;
		delete info_;
	    }

	    void setChilds(Node * leftChild, Node * rightChild);
	    void removeChilds() { setChilds(0, 0); }

	    void setInfo(Information * info) {
		delete info_;
		info_ = info;
	    }
	    bool hasInfo() const { return info_ != 0; }
	    template<class InformationType>
	    const InformationType & info() const {
		require((info_));
		return dynamic_cast<const InformationType &>(*info_);
	    }

	    bool isRoot() const { return father_ == 0; }
	    bool isLeaf() const { return leftChild_ == 0; }
	    Id id() const { return id_; }
	    const Node & father() const { verify((father_)); return *father_; }
	    const Node & leftChild() const { verify((leftChild_)); return *leftChild_; }
	    const Node & rightChild() const { verify((rightChild_)); return *rightChild_; }
	};

	static const u32 InvalidDepth;
	static const u32 InvalidSize;

    protected:
	typedef std::stack<Node *> NodePtrStack;

    protected:
	Node * root_;
	u32 depth_;
	u32 internalNodeN_;
	u32 leafN_;

    protected:
	virtual void checkNode(const Node & node) {}

	u32 checkTree(const Node & node);

	void checkTree();

    protected:
	void reset() {
	    delete root_;
	    root_ = 0;
	}

    public:
	BinaryTree(
	    const Core::Configuration & config,
	    Node * root = 0);
	virtual ~BinaryTree() {
	    reset();
	}

	void setRoot(Node * root) {
	    reset();
	    root_ = root;
	    checkTree();
	}

	const Node & root() const {
	    require(root_);
	    return *root_;
	}

	u32 depth() const {
	    return depth_;
	}
	u32 nNodes() const {
	    return internalNodeN_ + leafN_;
	}
	u32 nLeaves() const {
	    return leafN_;
	}

	virtual bool loadFromString(const std::string & str) { defect(); }

	virtual bool loadFromStream(std::istream & i) { defect(); }

	virtual bool loadFromFile(const std::string & filename) { defect(); }

	virtual void draw(std::ostream & out) const;

	virtual void write(std::ostream & out) const;

	virtual void writeXml(Core::XmlWriter & xml, const std::string & name = "binary-tree") const;

    };


} // namespace Cart

inline std::ostream & operator<<(std::ostream & out, const Cart::BinaryTree & b) {
    b.write(out);
    return out;
}

#endif // _CART_BINARY_TREE_HH
