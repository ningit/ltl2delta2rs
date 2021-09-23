#!/usr/bin/env python3
#
# Filter tests according to some given criteria
#

import spot

from formula_props import normalized, number_of_nodes


def str2bool(v):
	"""Allow parsing Boolean value in command-line arguments"""

	if v is None:
		return None

	if v.lower() in ('yes', 'true', 't', 'y', '1'):
		return True

	if v.lower() in ('no', 'false', 'f', 'n', '0'):
		return False

	raise argparse.ArgumentTypeError('Boolean value expected.')


if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser(description='Test filter')
	parser.add_argument('source', help='Source test file')
	parser.add_argument('--max', help='Maximum number of nodes', type=int)
	parser.add_argument('--min', help='Minimum number of nodes', type=int)
	parser.add_argument('--normalized', type=str2bool, help='Filter for whether they are normalized')
	parser.add_argument('--no-dups', help='Remove duplicates', action='store_true')
	args = parser.parse_args()

	# Seen set (for removing duplicated formulas)
	seen = set()

	with open(args.source) as intest:
		for line in intest:
			f = spot.formula(line)

			# Filter by maximum and minimum number of nodes
			if args.max is not None or args.min is not None:
				nn = number_of_nodes(f)

				if args.max is not None and nn > args.max or args.min is not None and nn < args.min:
					continue

			# Filter by normalization status
			if args.normalized is not None and normalized(f) != args.normalized:
				continue

			# Filter that removes duplicates
			if args.no_dups:
				if line in seen:
					continue

				seen.add(line)

			print(line, end='')
