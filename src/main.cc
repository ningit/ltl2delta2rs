/**
 * @file main.cc
 */

#include <iostream>
#include <string>

#include <spot/tl/nenoform.hh>
#include <spot/tl/parse.hh>

#include "normalizer.hh"
#include "tfspot.hh"

using namespace std;
using Op = Node::Op;

//
//	A printNode function just for debugging
//

const char*
node2Text(const Node* node)
{
	switch (node->type) {
		case Op::TT:
			return "tt";
		case Op::FF:
			return "ff";
		case Op::APROP:
			return node->name.c_str();
		case Op::AND:
			return "And";
		case Op::X:
			return "X";
		case Op::OR:
			return "Or";
		case Op::U:
			return "U";
		case Op::W:
			return "W";
		case Op::R:
			return "R";
		case Op::M:
			return "M";
		case Op::GF:
			return "GF";
		case Op::FG:
			return "FG";
		default:
			return "unknown";
	}
}

string
printNode(const Node* node, bool color = true)
{
	// Just for debugging

	string head =
	  color ? string("\x1b[33m") + node2Text(node) + "\x1b[0m" : node2Text(node);

	if (!node->children.empty()) {
		head += "(";
		for (Node* child : node->children)
			head += printNode(child, color) + ",";
		head.pop_back();
		head += ")";
	}

	return head;
}

//
//	Main loop reading formulae in the Spot format and printing
//	their normal forms (line by line)
//

void
normalizeLoop()
{
	string line;
	getline(cin, line);

	while (!line.empty()) {
		spot::parsed_formula parsed_form = spot::parse_infix_psl(line);

		if (!parsed_form.format_errors(cerr)) {
			spot::formula form = spot::negative_normal_form(parsed_form.f);

			Node* input = from_spot(form);
			input->addUser();

			Node* output = normalize(input);
			spot::formula result = to_spot(output);

			cout << result << endl;

			output->release();
			input->removeUser();
		}

		getline(cin, line);
	}
}

int
main()
{
	normalizeLoop();
	Node::releaseStaticNodes();

	return 0;
}
