#!/usr/bin/env python3
#
# Auxiliary program to test the normalization algorithm
#

import csv
import os
import subprocess
import sys
import time

import spot

from formula_props import number_of_nodes, dag_size, normalized, normalized_GF


#
# Normalize formulae by communicating with the different implementations
#

def check_owl(f, owl):
	"""Get the normal form from the Owl or the C++ implementation"""

	owl.stdin.write(f'{f}\n'.encode('ascii'))
	owl.stdin.flush()

	line = owl.stdout.readline()

	return spot.formula(line.decode('ascii')) if line else None


def handle_error(proc, imp):
	"""Show information about unrecoverable errors of the implementation"""

	status = proc.poll()

	if status < 0:
		import signal
		status = f'killed with signal {signal.strsignal(-status)}'
	else:
		status = f'returned {signal}'

	print(f'\n\x1b[1m\x1b[31mSomething went wrong during the execution of {imp} ({status}).\x1b[0m')


#
# Main program
#

def benchmark_formulae(args, ifile, impls, csvw):
	"""Benchmark the implementations"""

	total_num_errors = 0

	try:
		# Implementation are handled sequentially
		for imp, command in impls.items():

			check_fn = check_owl
			num_errors = 0

			# We repeatedly write a formula to the process and wait to obtain its normal form
			with subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
				for k, f_text in enumerate(ifile):
					f_text = f_text.rstrip()

					# Empty lines in the test cases are ignored
					if f_text == '':
						continue

					print(f'({k:4}) [{imp}] {f_text}', end='')
					sys.stdout.flush()

					f = spot.formula(f_text)

					start = time.perf_counter_ns()
					result = check_fn(f, proc)
					stop = time.perf_counter_ns()

					# If result is None probably something wrong happened
					if result is None:
						handle_error(proc, imp)
						return False

					final_normal = normalized(result)

					if args.equiv_check and not f.equivalent_to(result):
						print(f'     \x1b[1m\x1b[31m---- not equivalent {result}\x1b[0m')
						num_errors = num_errors + 1

					elif not final_normal:
						print(f'     \x1b[1m\x1b[31m---- not normalized {result}\x1b[0m')
						num_errors = num_errors + 1

					else:
						# Erase the last formula to print the next one (just for aesthetic reasons)
						if os.isatty(1):
							line_span = (9 + len(imp) + len(f_text)) // os.get_terminal_size()[0]
							print(f'\x1b[{line_span}F\x1b[J' if line_span > 0 else '\r\x1b[K', end='')
						else:
							print()

					csvw.writerow([args.test, f_text, normalized(f), normalized_GF(f),
					               imp, stop - start, number_of_nodes(f), dag_size(f),
					               number_of_nodes(result), dag_size(result), final_normal,
					               normalized_GF(result)])

				proc.stdin.close()
				proc.terminate()
				proc.wait(timeout=2)

				print(f'Finished with {imp}: {num_errors} errors.')
				ifile.seek(0)
				total_num_errors += num_errors

		return total_num_errors == 0

	except KeyboardInterrupt:
		print(f'\nInterrupted by the user with {num_errors} errors.')


if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser(description='Test and benchmark the normalization algorithms')
	parser.add_argument('test', help='Test to be run')
	parser.add_argument('--imp', '-i', help='Choose which implementations to consider (among owl, cpp)',
	                    default='owl,cpp')
	parser.add_argument('--equiv-check', help='Check whether the normal form is equivalent to the input formula',
	                    action='store_true')
	parser.add_argument('-o', help='Path for the output CSV file with the experimental data', default='result.csv')

	args = parser.parse_args()

	COMMANDS = {
		'owl': ['owl/bin/owl', 'ltl2delta2', '--method', 'SE20_SIGMA_2_AND_GF_SIGMA_1', '--strict'],
		'cpp': [os.getenv('LTLNORM_PATH') or 'build/ltlnorm'],
	}

	# Select the implementation given in the --imp argument
	selected_imps = args.imp.split(',')
	unknown_imp = next((imp for imp in selected_imps if imp not in COMMANDS), None)

	if unknown_imp is not None:
		print(f'Unknown implementation {unknown_imp}. Ignoring.')

	impls = {name: command for name, command in COMMANDS.items() if name in selected_imps}

	with open(args.test) as ifile:
		with open(args.o, 'w') as csvfile:
			csvw = csv.writer(csvfile)
			csvw.writerow(['file', 'formula', 'already_normal', 'already_gfnorm',
			               'imp', 'time', 'init_size', 'init_dagsize',
			               'fin_size', 'fin_dagsize', 'final_normal', 'final_gfnorm'])

			ok = benchmark_formulae(args, ifile, impls, csvw)
			sys.exit(0 if ok else 1)
