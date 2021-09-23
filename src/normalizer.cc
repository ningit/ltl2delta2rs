/**
 * @file normalizer.hh
 *
 * Normalize LTL formulae.
 */

#include <algorithm>

#include "normalizer.hh"

using namespace std;
using Op = Node::Op;

//
//	Step 1: removing U/M below W/R
//

struct FindUResult
{
	Node* gfa;  // The argument of the rule's GF (the right argument of U or the
	            // left argument of M)
	Node* ff;   // A formula where the U/M node has been replaced by false
	Node* weak; // A formula where the U/M node has been replaced by W/R
};

bool
containsU(Node* node)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::W:
		case Op::R:
			return any_of(node->children.begin(), node->children.end(), containsU);
		case Op::U:
		case Op::M:
			return true;
		default:
			return false;
	}
}

pair<Node*, Node*>
replaceU(Node* node, Node* gfa)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::W:
		case Op::R: {
			bool changed = false;
			// These copies may be unnecessary in many cases, we could do them lazily
			vector<Node*> ff_copy = node->children;
			vector<Node*> weak_copy = node->children;

			for (size_t i = 0; i < node->children.size(); ++i) {
				auto [ff, weak] = replaceU(node->children[i], gfa);

				if (ff != nullptr) {
					changed = true;
					ff_copy[i] = ff;
					weak_copy[i] = weak;
				}
			}

			if (changed)
				return { Node::make(node->type, move(ff_copy)),
					        Node::make(node->type, move(weak_copy)) };

			return { nullptr, nullptr };
		}
		case Op::U:
			if (equal(gfa, node->children[1]))
				return { Node::ff(), Node::W(node->children[0], gfa) };

			return { nullptr, nullptr };

		case Op::M:
			if (equal(gfa, node->children[0]))
				return { Node::ff(), Node::R(gfa, node->children[1]) };

			return { nullptr, nullptr };

		default:
			return { nullptr, nullptr };
	}
}

bool
findU(Node* node, FindUResult& result)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::W:
		case Op::R:
			for (size_t i = 0; i < node->children.size(); ++i)
				if (findU(node->children[i], result)) {
					// Rebuild 'node' with the two variants of U/M operator in the rule
					vector<Node*> ff_copy = node->children;
					vector<Node*> weak_copy = node->children;
					ff_copy[i] = result.ff;
					weak_copy[i] = result.weak;

					for (size_t j = i + 1; j < node->children.size(); ++j) {
						auto [ff, weak] = replaceU(node->children[j], result.gfa);

						if (ff != nullptr) {
							ff_copy[j] = ff;
							weak_copy[j] = weak;
						}
					}

					result.ff = Node::make(node->type, move(ff_copy));
					result.weak = Node::make(node->type, move(weak_copy));
					return true;
				}
			return false;

		case Op::U:
			result.gfa = node->children[1];
			result.ff = Node::ff();
			result.weak = Node::W(node->children[0], node->children[1]);
			return true;

		case Op::M:
			result.gfa = node->children[0];
			result.ff = Node::ff();
			result.weak = Node::R(node->children[0], node->children[1]);
			return true;

		default:
			return false;
	}
}

Node*
removeWU(Node* node)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::U:
		case Op::M: {
			bool changed = false;
			vector<Node*> copy = node->children;

			for (size_t i = 0; i < node->children.size(); ++i) {
				Node* newNode = removeWU(node->children[i]);

				if (newNode != node->children[i]) {
					changed = true;
					copy[i] = newNode;
				}
			}

			// The original node is in the original tree, so we do remove it here
			return changed ? takePlace(Node::make(node->type, move(copy)), node) : node;
		}
		case Op::W:
			// These situations are no longer possible due to the optimizations
			// if (is(node->children[0], Op::TT))
			//	return node->children[0];
			// else if (is(node->children[0], Op::FF))
			//	return removeWU(node->children[1]); else

			// (1) a W f[b U/M c] = a U f[b U/M c] | G a
			if (containsU(node->children[1])) {
				Node* uNode =
				  Node::U(removeWU(node->children[0]), removeWU(node->children[1]));
				Node* gNode = Node::G(node->children[0]);
				Node* orNode = Node::Or({ uNode, removeWU(gNode) });

				return takePlace(orNode, node);
			}
			// (2) f[a U b] W c = (GF b & f[a W b] W c) | f[a U b] U (c | G f[ff])
			// (2) f[a M b] W c = (GF a & f[a R b] W c) | f[a M b] U (c | G f[ff])
			else {
				FindUResult found;
				if (findU(node->children[0], found)) {
					Node* wwNode = Node::W(found.weak, node->children[1]);
					Node* andNode = Node::And({ Node::GF(found.gfa), removeWU(wwNode) });
					Node* gffNode = Node::G(found.ff);
					Node* urNode = Node::Or({ node->children[1], removeWU(gffNode) });
					Node* uNode = Node::U(removeWU(node->children[0]), urNode);
					Node* orNode = Node::Or({ andNode, uNode });

					return takePlace(orNode, node);
				}
			}
			return node;

		case Op::R:
			// These situations are no longer possible due to the optimizations
			// if (is(node->children[1], Op::FF))
			//	return node->children[1];
			// else if (is(node->children[0], Op::TT))
			// 	return removeWU(node->children[1]); else

			// (1) f[a U/M b] R c = f[a U/M b] M c | G c
			if (containsU(node->children[0])) {
				Node* mNode =
				  Node::M(removeWU(node->children[0]), removeWU(node->children[1]));
				Node* gNode = Node::G(node->children[1]);
				Node* orNode = Node::Or({ mNode, removeWU(gNode) });

				return takePlace(orNode, node);
			}
			// (2) a R f[a U b] = (GF b & a R f[a U b]) | (a | G f[ff]) M f[a U b]
			// (2) a R f[a M b] = (GF a & a R f[a R b]) | (a | G f[ff]) M f[a M b]
			else {
				FindUResult found;
				if (findU(node->children[1], found)) {
					Node* rrNode = Node::R(node->children[0], found.weak);
					Node* andNode = Node::And({ Node::GF(found.gfa), removeWU(rrNode) });
					Node* gffNode = Node::G(found.ff);
					Node* mlNode = Node::Or({ node->children[0], removeWU(gffNode) });
					Node* mNode = Node::M(mlNode, removeWU(node->children[1]));
					Node* orNode = Node::Or({ andNode, mNode });

					return takePlace(orNode, node);
				}
			}
			return node;

		default:
			return node;
	}
}

//
//	Step 2: remove GF
//

Node*
findGF(Node* node, bool proper = false)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::U:
		case Op::X:
		case Op::W:
		case Op::R:
		case Op::M:
			for (Node* child : node->children) {
				Node* childGF =
				  findGF(child, proper || (!is(node, Op::AND) && !is(node, Op::OR)));
				if (childGF)
					return childGF;
			}
			return nullptr;

		case Op::GF:
		case Op::FG: {
			// Only GF-nodes below a temporal operator are considered, and
			// the innermost is prefered in case there are nested ones
			Node* childGF = findGF(node->children[0], true);
			return childGF ? childGF : (proper ? node : nullptr);
		}
		default:
			return nullptr;
	}
}

Node*
replace(Node* node, Node* left, Node* right)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::U:
		case Op::W:
		case Op::R:
		case Op::M: {
			bool changed = false;
			vector<Node*> copy = node->children;

			for (size_t i = 0; i < node->children.size(); ++i) {
				Node* newNode = replace(node->children[i], left, right);

				if (newNode != node->children[i]) {
					changed = true;
					copy[i] = newNode;
				}
			}

			// We are not responsible of deleting the original node,
			// which is still accesible by the parent
			return changed ? Node::make(node->type, move(copy)) : node;
		}
		case Op::GF:
		case Op::FG:
			// This 'replace' function can only replace GF nodes
			if (node->type == left->type && equal(node->children[0], left->children[0]))
				return right;
			else {
				Node* newNode = replace(node->children[0], left, right);
				return newNode != node->children[0] ? Node::make(node->type, { newNode })
				                                    : node;
			}
		default:
			return node;
	}
}

Node*
removeGF(Node* node)
{
	// Removing GF separately on each topmost temporal formula reduces
	// in some cases (and increase in some others) the output size
	if (is(node, Op::AND) || is(node, Op::OR)) {
		for (Node*& child : node->children) {
			Node* newChild = removeGF(child);

			if (newChild != child) {
				newChild->addUser();
				child->removeUser();
				child = newChild;
			}
		}

		return node;
	}

	Node* found = findGF(node);

	// (3) f[GF a] = (GF a & f[tt]) | f[ff]
	// (4) f[FG a] = (FG a & f[tt]) | f[ff]
	if (found) {
		Node* ttVariant = replace(node, found, Node::tt());
		Node* ffVariant = replace(node, found, Node::ff());
		Node* andNode = Node::And({ found, removeGF(ttVariant) });
		Node* orNode = Node::Or({ andNode, removeGF(ffVariant) });

		// orNode could be a subformula of 'node', so we have to protect it from
		// deletion (addUser/removeUser are not used because orNode does not have
		// any user yet and removeUser would delete it)
		orNode->refCount++;
		node->release();
		orNode->refCount--;

		return orNode;
	}

	return node;
}

//
//	Step 3: remove W/R inside GF
//

struct FindWResult
{
	Node* fga;    // The argument of the rule's FG (the left argument of W or the
	              // right argument of R)
	Node* strong; // A formula where the W/R node has been replaced by U/M
	Node* tt;     // A formula where the W/R node has been replaced by true
};

pair<Node*, Node*>
replaceW(Node* node, Node* fga)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::U:
		case Op::M: {
			bool changed = false;
			// These copies may be unnecessary in many cases, we could do them lazily
			vector<Node*> tt_copy = node->children;
			vector<Node*> strong_copy = node->children;

			for (size_t i = 0; i < node->children.size(); ++i) {
				auto [tt, strong] = replaceW(node->children[i], fga);

				if (tt != nullptr) {
					changed = true;
					tt_copy[i] = tt;
					strong_copy[i] = strong;
				}
			}

			if (changed)
				return { Node::make(node->type, move(tt_copy)),
					        Node::make(node->type, move(strong_copy)) };

			return { nullptr, nullptr };
		}
		case Op::W:
			if (equal(fga, node->children[0]))
				return { Node::tt(), Node::U(fga, node->children[1]) };

			return { nullptr, nullptr };

		case Op::R:
			if (equal(fga, node->children[1]))
				return { Node::tt(), Node::M(node->children[0], fga) };

			return { nullptr, nullptr };

		default:
			return { nullptr, nullptr };
	}
}

bool
findW(Node* node, FindWResult& result)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR:
		case Op::X:
		case Op::U:
		case Op::M:
			for (size_t i = 0; i < node->children.size(); ++i)
				if (findW(node->children[i], result)) {
					// Rebuild 'node' with the three variants of U/M operator in the rule
					vector<Node*> strong_copy = node->children;
					vector<Node*> tt_copy = node->children;

					strong_copy[i] = result.strong;
					tt_copy[i] = result.tt;

					for (size_t j = i + 1; j < node->children.size(); ++j) {
						auto [tt, strong] = replaceW(node->children[j], result.fga);

						if (tt != nullptr) {
							tt_copy[j] = tt;
							strong_copy[j] = strong;
						}
					}

					result.strong = Node::make(node->type, move(strong_copy));
					result.tt = Node::make(node->type, move(tt_copy));
					return true;
				}
			return false;

		case Op::W:
			result.fga = node->children[0];
			result.strong = Node::U(node->children[0], node->children[1]);
			result.tt = Node::tt();
			return true;

		case Op::R:
			result.fga = node->children[1];
			result.strong = Node::M(node->children[0], node->children[1]);
			result.tt = Node::tt();
			return true;

		case Op::GF:
			result.fga = node->children[0];
			result.strong = Node::ff();
			result.tt = Node::tt();
			return true;

		case Op::FG:
			result.fga = node->children[0];
			result.strong = Node::ff();
			result.tt = Node::tt();
			return true;

		default:
			return false;
	}
}

Node* fixGFW(Node* node, Node* existing = nullptr);
Node* fixFGU(Node* node, Node* existing = nullptr);

Node*
fixGFW(Node* node, Node* existing)
{
	FindWResult found;

	// (4) GF f[a W b] = GF f[a U b] | (FG a & GF f[tt])
	// (4) GF f[a R b] = GF f[a M b] | (FG b & GF f[tt])
	if (findW(node, found)) {
		Node* andNode = Node::And({ fixFGU(found.fga), fixGFW(found.tt) });
		Node* orNode = Node::Or({ fixGFW(found.strong), andNode });

		return takePlace(orNode, node);
	}

	// 'existing' is the original GF-node to be fixed ('node' is its argument),
	// which is passed on to avoid creating a new node for an unchanged formula.
	return existing ? existing : Node::GF(node);
}

Node*
fixFGU(Node* node, Node* existing)
{
	FindUResult found;

	// (5) FG f[a U b] = (GF b & FG f[a W b]) | FG f[ff]
	// (5) FG f[a M b] = (GF a & FG f[a R b]) | FG f[ff]
	if (findU(node, found)) {
		Node* andNode = Node::And({ fixGFW(found.gfa), fixFGU(found.weak) });
		Node* orNode = Node::Or({ andNode, fixFGU(found.ff) });

		return takePlace(orNode, node);
	}

	return existing ? existing : Node::F(Node::G(node));
}

Node*
fixGF(Node* node)
{
	switch (node->type) {
		case Op::AND:
		case Op::OR: {
			bool changed = false;
			vector<Node*> copy = node->children;

			for (size_t i = 0; i < node->children.size(); ++i) {
				Node* newNode = fixGF(node->children[i]);

				if (newNode != node->children[i]) {
					changed = true;
					copy[i] = newNode;
				}
			}

			return changed ? takePlace(Node::make(node->type, move(copy)), node) : node;
		}
		case Op::GF:
			return fixGFW(node->children[0], node);

		case Op::FG:
			return fixFGU(node->children[0], node);

		default:
			return node;
	}
}

//
//	Complete normalization
//

Node*
normalize(Node* tree)
{
	return fixGF(removeGF(removeWU(tree)));
}
