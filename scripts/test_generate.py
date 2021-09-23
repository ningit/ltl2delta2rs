#!/usr/bin/env python3
#
# Test generators for the normalization algorithms
#

import itertools
import os
import subprocess

import spot

from formula_props import normalized_GF


def hard_formula():
	"""Generate a family of formulas of growing size alternating W and U operators"""

	n = 0
	f = spot.formula.ap('a0')

	while True:
		yield f

		n = n + 1
		a = spot.formula.ap(f'a{n}')
		b = spot.formula.ap(f'b{n}')

		f = spot.formula.W(spot.formula.U(f, b), a)


def uw_formula():
	"""Generate a family of formulas of growing size with a single W on top and U below"""

	n = 0
	f = spot.formula.ap('a0')

	while True:
		yield spot.formula.W(f, spot.formula.ap('b'))

		n = n + 1
		a = spot.formula.ap(f'a{n}')

		f = spot.formula.U(f, a)


def tlsf_convert():
	"""Convert TLSF benchmarks into Spot formulas using syfco"""

	benchmark_root = 'tests/TLSF_2021/benchmarks'
	syfco_path = 'scripts/syfco'

	for bn in os.listdir(benchmark_root):
		# As seen in https://spot.lrde.epita.fr/ltlsynt.html
		syfco = subprocess.run([syfco_path, os.path.join(benchmark_root, bn), '-f', 'ltlxba', '-m', 'fully'],
		                       stdout=subprocess.PIPE)

		# Formulae are converted to negative normal form
		yield spot.negative_normal_form(spot.formula(syfco.stdout.decode('utf-8')))


def get_generator(args):
	"""Get a generator for test formulae"""

	if args.method == 'hard':
		generator = hard_formula()

	elif args.method == 'uw':
		generator = uw_formula()

	elif args.method == 'tlsf':
		generator = tlsf_convert()

	else:
		generator = spot.randltl(['a', 'b', 'c', 'd'],
		                         ltl_priorities='not=0,equiv=0,implies=0,xor=0',
		                         tree_size=(args.s, args.s), seed=31788)

		if 'notnorm' in args.method:
			generator = filter(lambda f: not normalized_GF(f), generator)

		elif 'norm' in args.method:
			generator = filter(normalized_GF, generator)

	return itertools.islice(generator, args.n)


if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser(description='Test generator for the normalization algorithms')
	parser.add_argument('method', help='Method for generating tests',  default='random',
	                    choices=['random', 'hard', 'uw', 'random_norm', 'random_notnorm', 'tlsf'])
	parser.add_argument('-n', help='Number of test cases', type=int, default=50)
	parser.add_argument('-s', help='Maximum size of the random formulae (for Spot)', type=int, default=15)

	args = parser.parse_args()
	case_generator = get_generator(args)

	for case in case_generator:
		print(case)
