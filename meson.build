project('ltlnorm', 'cpp', version : '1.0', default_options : ['warning_level=3', 'cpp_std=c++17'])

# Spot is used to parse LTL formulae
spot = dependency('libspot', static: get_option('static-spot'))

ltlnorm_sources = [
	'src/main.cc',
	'src/normalizer.cc',
	'src/tfspot.cc',
	'src/tree.cc',
]

ltlnorm = executable('ltlnorm',
	ltlnorm_sources,
	dependencies: [spot],
	install : true
)

# Tests
checkpy = find_program('scripts/check.py')

test('Test against sample formulae',
	checkpy,
	args: ['-i', 'cpp', '--equiv-check', 'tests/random1000.spot'],
	env: {'LTLNORM_PATH': ltlnorm.full_path()},
	workdir: meson.source_root()
)
