/**
 * @file tree.cc
 *
 * Data structure for the LTL syntax tree or DAG (with sharing).
 */

#include <algorithm>
#include <map>

#include "tree.hh"

using namespace std;
using Op = Node::Op;

//
// Nodes with a single instance (TT, FF, and each atomic proposition)
//

Node* Node::ttNode = new Node(Op::TT);
Node* Node::ffNode = new Node(Op::FF);
map<string, Node*> aprops;

/*
 * Apply a function every element of a vector generating a new one.
 */
inline vector<Node*>
mapfn(const vector<Node*>& args, Node* fn(Node*))
{
	vector<Node*> newArgs(args.size());
	transform(args.begin(), args.end(), newArgs.begin(), fn);
	return newArgs;
}

//
//	Private constructors without simplification
//

Node::Node(const std::string& name)
  : type(Op::APROP)
  , name(name)
  , refCount(1)
{}

Node::Node(Op type)
  : type(type)
  , refCount(1)
{}

Node::Node(Op type, Node* left, Node* right)
  : type(type)
  , children(right ? 2 : 1)
{
	children[0] = left;
	left->addUser();
	if (right) {
		children[1] = right;
		right->addUser();
	}
}

Node::Node(Op type, const std::vector<Node*>& args)
  : type(type)
  , children(args)
{
	for (Node* child : children)
		child->addUser();
}

Node::Node(Op type, std::vector<Node*>&& args)
  : type(type)
  , children(args)
{
	for (Node* child : children)
		child->addUser();
}

//
//	Public constructor with simplification
//

Node*
Node::tt()
{
	return ttNode;
}

Node*
Node::ff()
{
	return ffNode;
}

Node*
Node::ap(const string& name)
{
	auto it = aprops.find(name);

	if (it != aprops.end())
		return it->second;

	return (aprops[name] = new Node(name));
}

Node*
Node::X(Node* arg)
{
	if (arg->isConstant() || is(arg, Op::GF) || is(arg, Op::FG))
		return arg;

	return new Node(Op::X, arg);
}

Node*
Node::U(Node* left, Node* right)
{
	// Owl use a proper comparison in left == right

	if (is(left, Op::FF) || left == right)
		return right;

	if (right->isConstant() || right->isF()) {
		left->release();
		return right;
	}

	if (is(left, Op::TT)) { // F operator
		if (is(right, Op::OR))
			return takePlace(Node::Or(mapfn(right->children, Node::F)), right);

		if (right->isG())
			return takePlace(
			  Node::FG(is(right, Op::W) ? right->children[0] : right->children[1]),
			  right);
	}

	return new Node(Op::U, left, right);
}

Node*
Node::W(Node* left, Node* right)
{
	if (is(left, Op::FF) || left == right)
		return right;

	if (is(right, Op::TT)) {
		left->release();
		return Node::tt();
	}

	if (is(left, Op::TT)) {
		right->release();
		return Node::tt();
	}

	if (left->isG())
		return Node::Or({ left, right });

	if (is(right, Op::FF)) { // G operator
		if (is(left, Op::AND))
			return takePlace(Node::And(mapfn(left->children, Node::G)), left);

		if (left->isF())
			return takePlace(
			  Node::GF(is(left, Op::U) ? left->children[1] : left->children[0]), left);
	}

	return new Node(Op::W, left, right);
}

Node*
Node::R(Node* left, Node* right)
{
	if (is(left, Op::TT) || left == right)
		return right;

	if (right->isConstant() || right->isG()) {
		left->release();
		return right;
	}

	if (is(left, Op::FF)) { // G Operator
		if (is(right, Op::AND))
			return takePlace(Node::And(mapfn(right->children, Node::G)), right);

		if (right->isF())
			return takePlace(
			  Node::GF(is(right, Op::U) ? right->children[1] : right->children[0]),
			  right);
	}

	return new Node(Op::R, left, right);
}

Node*
Node::M(Node* left, Node* right)
{
	if (is(left, Op::TT) || left == right)
		return right;

	if (is(right, Op::FF)) {
		left->release();
		return Node::ff();
	}

	if (is(left, Op::FF)) {
		right->release();
		return Node::ff();
	}

	if (left->isF())
		return Node::And({ left, right });

	if (is(right, Op::TT)) { // F operator
		if (is(left, Op::OR))
			return takePlace(Node::Or(mapfn(left->children, Node::F)), left);

		if (left->isG())
			return takePlace(
			  Node::FG(is(left, Op::W) ? left->children[0] : left->children[1]), left);
	}

	return new Node(Op::M, left, right);
}

Node*
Node::GF(Node* arg)
{
	if (arg->isConstant())
		return arg;

	if (is(arg, Op::X))
		return takePlace(Node::GF(arg->children[0]), arg);

	if (arg->isF())
		return takePlace(
		  Node::GF(is(arg, Op::U) ? arg->children[1] : arg->children[0]), arg);

	return new Node(Op::GF, arg);
}

Node*
Node::FG(Node* arg)
{
	if (arg->isConstant())
		return arg;

	if (is(arg, Op::X))
		return takePlace(Node::FG(arg->children[0]), arg);

	if (arg->isG())
		return takePlace(
		  Node::FG(is(arg, Op::W) ? arg->children[0] : arg->children[1]), arg);

	return new Node(Op::FG, arg);
}

Node*
Node::And(vector<Node*>&& args)
{
	for (auto it = args.rbegin(); it != args.rend(); it++) {
		if (is(*it, Op::FF)) {
			for (Node* node : args)
				node->release();

			return Node::ff();
		}

		if (is(*it, Op::TT))
			args.erase(next(it).base());
	}

	if (args.size() == 1)
		return args[0];

	return new Node(Op::AND, args);
}

Node*
Node::Or(vector<Node*>&& args)
{
	for (auto it = args.rbegin(); it != args.rend(); it++) {
		if (is(*it, Op::TT)) {
			for (Node* node : args)
				node->release();

			return Node::tt();
		}

		if (is(*it, Op::FF))
			args.erase(next(it).base());
	}

	if (args.size() == 1)
		return args[0];

	return new Node(Op::OR, args);
}

Node*
Node::make(Op type, vector<Node*>&& args)
{
	switch (type) {
		case Op::TT:
			return Node::tt();

		case Op::FF:
			return Node::ff();

		case Op::X:
			return Node::X(args[0]);

		case Op::AND:
			return Node::And(move(args));

		case Op::OR:
			return Node::Or(move(args));

		case Op::U:
			return Node::U(args[0], args[1]);

		case Op::W:
			return Node::W(args[0], args[1]);

		case Op::R:
			return Node::R(args[0], args[1]);

		case Op::M:
			return Node::M(args[0], args[1]);

		case Op::GF:
			return Node::GF(args[0]);

		case Op::FG:
			return Node::FG(args[0]);

		default:
			return nullptr; // Cannot happen
	}
}

void
Node::release()
{
	if (refCount == 0) {
		for (Node* child : children)
			child->removeUser();

		delete this;
	}
}

void
Node::releaseStaticNodes()
{
	delete ttNode;
	delete ffNode;

	for (auto& [name, node] : aprops)
		delete node;

	ttNode = nullptr;
	ffNode = nullptr;
	aprops.clear();
}

bool
Node::operator==(const Node& other) const
{
	if (this == &other)
		return true;

	if (!is(this, Op::APROP) && type == other.type &&
	    children.size() == other.children.size()) {
		const size_t nrChildren = children.size();
		for (size_t i = 0; i < nrChildren; ++i)
			if (*children[i] != *other.children[i])
				return false;
		return true;
	}

	return false;
}

bool
equal(Node* left, Node*& right)
{
	if (left == right)
		return true;

	if (!is(left, Op::APROP) && left->type == right->type &&
	    left->children.size() == right->children.size()) {
		const size_t nrChildren = left->children.size();
		for (size_t i = 0; i < nrChildren; ++i)
			if (!equal(left->children[i], right->children[i]))
				return false;

		right->removeUser();
		right = left;
		right->addUser();
		return true;
	}

	return false;
}
