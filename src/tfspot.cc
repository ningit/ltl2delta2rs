/**
 * @file tfspot.cc
 *
 * Convert to and from Spot formulae.
 */

#include <iostream>

#include "tfspot.hh"

using namespace std;
using formula = spot::formula;
using op = spot::op;
using Op = Node::Op;

formula
to_spot(Node* tree)
{
	switch (tree->type) {
		case Op::APROP:
			return formula::ap(tree->name);

		case Op::TT:
			return formula::tt();

		case Op::FF:
			return formula::ff();

		case Op::AND: {
			vector<spot::formula> args(tree->children.size());
			for (size_t i = 0; i < args.size(); i++)
				args[i] = to_spot(tree->children[i]);
			return formula::And(move(args));
		}
		case Op::OR: {
			vector<spot::formula> args(tree->children.size());
			for (size_t i = 0; i < args.size(); i++)
				args[i] = to_spot(tree->children[i]);
			return formula::Or(move(args));
		}
		case Op::X:
			return formula::X(to_spot(tree->children[0]));

		case Op::U:
			if (is(tree->children[0], Op::TT))
				return formula::F(to_spot(tree->children[1]));
			else
				return formula::U(to_spot(tree->children[0]), to_spot(tree->children[1]));
		case Op::W:
			if (is(tree->children[1], Op::FF))
				return formula::G(to_spot(tree->children[0]));
			else
				return formula::W(to_spot(tree->children[0]), to_spot(tree->children[1]));

		case Op::R:
			if (is(tree->children[0], Op::FF))
				return formula::G(to_spot(tree->children[1]));
			else
				return formula::R(to_spot(tree->children[0]), to_spot(tree->children[1]));

		case Op::M:
			if (is(tree->children[0], Op::TT))
				return formula::F(to_spot(tree->children[0]));
			else
				return formula::M(to_spot(tree->children[0]), to_spot(tree->children[1]));

		case Op::GF:
			return formula::G(formula::F(to_spot(tree->children[0])));

		case Op::FG:
			return formula::F(formula::G(to_spot(tree->children[0])));

		default:
			// All cases are covered, but to remove warnings...
			return spot::formula();
	}
}

Node*
from_spot(const formula& form)
{
	switch (form.kind()) {
		case op::ap:
			return Node::ap(form.ap_name());

		case op::tt:
			return Node::tt();

		case op::ff:
			return Node::ff();

		case op::And: {
			vector<Node*> args(form.size());
			for (size_t i = 0; i < args.size(); i++)
				args[i] = from_spot(form[i]);
			return Node::And(move(args));
		}
		case op::Or: {
			vector<Node*> args(form.size());
			for (size_t i = 0; i < args.size(); i++)
				args[i] = from_spot(form[i]);
			return Node::Or(move(args));
		}
		case op::X:
			return Node::X(from_spot(form[0]));

		case op::U:
			return Node::U(from_spot(form[0]), from_spot(form[1]));

		case op::W:
			return Node::W(from_spot(form[0]), from_spot(form[1]));

		case op::F:
			if (form[0].kind() == op::G)
				return Node::FG(from_spot(form[0][0]));
			else
				return Node::F(from_spot(form[0]));

		case op::G:
			if (form[0].kind() == op::F)
				return Node::GF(from_spot(form[0][0]));
			else
				return Node::G(from_spot(form[0]));

		case op::R:
			return Node::R(from_spot(form[0]), from_spot(form[1]));

		case op::M:
			return Node::M(from_spot(form[0]), from_spot(form[1]));

		case op::Not:
			if (form[0].kind() == op::ap)
				return Node::ap("not" + form[0].ap_name());
			else {
				cerr << "Warning: not in negation normal form. Seen as tt.\n";
				return Node::tt();
			}
		default:
			cerr << "Warning: unsupported operator " << form.kindstr()
			     << ". Seen as tt.\n";
			return Node::tt();
	}
}
