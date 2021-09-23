/**
 * @file tree.hh
 *
 * Data structure for the LTL syntax tree or DAG (with sharing).
 */

#ifndef TREE_HH
#define TREE_HH

#include <string>
#include <vector>

struct Node
{
	enum class Op
	{
		TT,
		FF,
		APROP,
		AND,
		OR,
		X,
		U,
		W,
		R,
		M,
		GF,
		FG
	};

	Op type;
	std::vector<Node*> children;
	std::string name; // of atomic propositions

	size_t refCount = 0;

	/*
	 * Deep self-destruction of this node if not used by others.
	 */
	void release();
	/**
	 * Increase the reference count of this node.
	 */
	void addUser();
	/**
	 * Decrease the reference count of this node and tries to release it.
	 */
	void removeUser();

	bool isConstant() const;
	bool isG() const;
	bool isF() const;

	bool operator==(const Node& other) const;
	bool operator!=(const Node& other) const;

	//
	//	Functions to construct nodes with simplifications
	//

	static Node* tt();
	static Node* ff();
	static Node* ap(const std::string& name);
	static Node* X(Node* arg);
	static Node* And(std::vector<Node*>&& args);
	static Node* Or(std::vector<Node*>&& args);
	static Node* U(Node* left, Node* right);
	static Node* W(Node* left, Node* right);
	static Node* R(Node* left, Node* right);
	static Node* M(Node* left, Node* right);
	static Node* GF(Node* arg);
	static Node* FG(Node* arg);
	static Node* G(Node* arg);
	static Node* F(Node* arg);

	static Node* make(Op type, std::vector<Node*>&& args);

	/*
	 * Release the unique nodes (true, false and atomic propositions).
	 */
	static void releaseStaticNodes();

	private:
	Node(Op type);
	Node(const std::string& name);
	Node(Op type, Node* left, Node* right = nullptr);
	Node(Op type, const std::vector<Node*>& args);
	Node(Op type, std::vector<Node*>&& args);

	static Node* ttNode;
	static Node* ffNode;
};

/**
 * Compare in depth two nodes for equality, removing duplicates.
 */
bool equal(Node* left, Node*& right);

/**
 * Return the first argument and release the second, which has been superseeded
 * by the first in the context where this is called. It is only a convenience
 * method to improve readability.
 */
inline Node*
takePlace(Node* node, Node* former)
{
	former->release();
	return node;
}

inline Node*
Node::G(Node* arg)
{
	return Node::W(arg, Node::ff());
}

inline Node*
Node::F(Node* arg)
{
	return Node::U(Node::tt(), arg);
}

inline bool
is(const Node* node, Node::Op type)
{
	return node->type == type;
}

inline bool
Node::isG() const
{
	return (type == Op::W && is(children[1], Op::FF)) ||
	       (type == Op::R && is(children[0], Op::FF));
}

inline bool
Node::isF() const
{
	return (type == Op::U && is(children[0], Op::TT)) ||
	       (type == Op::M && is(children[1], Op::TT));
}

inline bool
Node::isConstant() const
{
	return type == Op::TT || type == Op::FF;
}

inline bool
Node::operator!=(const Node& other) const
{
	return !operator==(other);
}

inline void
Node::addUser()
{
	refCount++;
}

inline void
Node::removeUser()
{
	if (--refCount == 0)
		release();
}

#endif // TREE_HH
