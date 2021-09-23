#!/usr/bin/env python3
#
# Auxiliary program to process the results of tests
#

import json
import os
import sys

import pandas as pd


def emph(s):
	"""Emphasize text to make it more readable"""

	return f'\x1b[33m{s}\x1b[0m' if os.isatty(1) else ''


def percent(series, value=True):
	"""Calculate the percentage of a value in a given series"""

	return 100 * series.value_counts(normalize=True).get(value, 0)


def process_file(filename, ignore_normalized=False):
	"""Calculate some statistics of the implementation runs"""

	data = pd.read_csv(filename, index_col=['file', 'formula', 'imp']).unstack()

	# Implementations benchmarked within this file
	implementations = tuple(data.columns.get_level_values('imp').drop_duplicates())

	if not implementations:
		print(f'No data in {filename}.')
		return None

	first_imp = implementations[0]

	# If normalized formulae are to be ignored
	if ignore_normalized:
		data = data[~data.already_normal[first_imp]]

	summary = {
		'file': filename,
		'filters': ['not-normalized'] if ignore_normalized else [],
		'implementations': implementations,
		'formulae': len(data),
		'already_normalized': percent(data.already_normal[first_imp]),
		'already_gfnorm': percent(data.already_gfnorm[first_imp]),
	}

	# Fastest implementation
	min_times = data.time.min(axis=1)

	summary['fastest'] = {imp: percent(data.time[imp] == min_times)
	                      for imp in implementations}


	# Total execution time (sum of the time for each formula)
	summary['total_time'] = {imp: data.time[imp].sum() / 1e6
	                         for imp in implementations}

	# Percentage of input formulas for which the implementation produces the smallest formula
	min_sizes = data.fin_size.min(axis=1)

	summary['smallest_formula'] = {imp: percent(data.fin_size[imp] == min_sizes)
	                               for imp in implementations}

	# The same as above, but only when the output formula is strictly smaller that the rest
	summary['strictly_smallest'] = {imp: percent(data.fin_size.drop(imp, axis=1).gt(data.fin_size[imp], axis=0).all(axis=1))
	                                for imp in implementations}

	# Mean blow-up
	summary['mean_blowup'] = {imp: (data.fin_size[imp] / data.init_size[imp]).mean()
	                          for imp in implementations}

	# Median blow-up
	summary['median_blowup'] = {imp: (data.fin_size[imp] / data.init_size[imp]).median()
	                            for imp in implementations}

	# Worst-case blow-up
	summary['worst_blowup'] = {imp: (data.fin_size[imp] / data.init_size[imp]).max()
	                           for imp in implementations}

	# Mean blow-up (with DAG size)
	summary['mean_blowup_dag'] = {imp: (data.fin_dagsize[imp] / data.init_dagsize[imp]).mean()
	                          for imp in implementations}

	# Median blow-up (with DAG size)
	summary['median_blowup_dag'] = {imp: (data.fin_dagsize[imp] / data.init_dagsize[imp]).median()
	                            for imp in implementations}

	# Worst-case blow-up (with DAG size)
	summary['worst_blowup_dag'] = {imp: (data.fin_dagsize[imp] / data.init_dagsize[imp]).max()
	                           for imp in implementations}

	# Not GF-normalized
	summary['not_gfnorm'] = {imp: percent(data.final_gfnorm[imp], False)
	                         for imp in implementations}

	# Normalization errors
	summary['errors'] = {imp: int(data.final_normal[imp].value_counts().get(False, 0))
	                     for imp in implementations}

	return summary


def print_results(summary):
	"""Print the summary of the results to the command line"""

	measures = [
		('fastest', 'Fastest', True),
		('total_time', 'Total time', False),
		('smallest_formula', 'Smallest formula', True),
		('strictly_smallest', 'Strictly smallest formula', True),
		('mean_blowup', 'Mean blow-up', False),
		('median_blowup', 'Median blow-up', False),
		('worst_blowup', 'Worst-case blow-up', False),
		('mean_blowup_dag', 'Mean blow-up (DAG)', False),
		('median_blowup_dag', 'Median blow-up (DAG)', False),
		('worst_blowup_dag', 'Worst-case blow-up (DAG)', False),
		('not_gfnorm', 'Not GF-normalized', True),
		('errors', 'Normalization errors', False),
	]

	impls = summary['implementations']

	print(emph('File:'), summary['file'])
	print(emph('Implementations:'), ' + '.join(impls))
	print(emph('Formulae:'), summary['formulae'])
	print(emph('Already in Δ₂:'), round(summary['already_normalized'], 2), '%')
	print(emph('Already GF-normalized:'), round(summary['already_gfnorm'], 2), '%')


	for measure, name, is_percentage in measures:
		print(f'\n{name}:')
		measure_dict = summary[measure]

		for imp in impls:
			print(f'\t{emph(imp)}\t', f'{round(measure_dict[imp], 2)} %' if is_percentage else round(measure_dict[imp], 3))


def decode_filename(dfilename):
	"""Separate filters from filenames in decorated filenames"""

	return (dfilename[1:], True) if dfilename[0] == '#' else (dfilename, False)


if __name__ == '__main__':
	import argparse

	parser = argparse.ArgumentParser(description='Make a report of the implementation runs')
	parser.add_argument('csvfile', help='CSV file with the results', nargs='+')
	parser.add_argument('-o', help='Write the summary in JSON format to the given file', type=str)
	args = parser.parse_args()

	summary = [process_file(filename, ignore_normal)
	           for filename, ignore_normal in map(decode_filename, args.csvfile)]

	if args.o:
		with open(args.o, 'w') as ofile:
			json.dump(summary, ofile)

	print_results(summary[0])

	for entry in summary[1:]:
		print('-' * 80)
		print_results(entry)
