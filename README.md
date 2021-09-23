A Simple Rewrite System for the Normalization of LTL formulas
=============================================================

This repository contains the implementation of a simple rewrite system to normalize any LTL formula into the class Δ<sub>2</sub> of the [safety-progress hierarchy](https://doi.org/10.1145/93385.93442). More precisely, the normal forms produced are Boolean combinations of formulas in Σ<sub>2</sub> and formulas in **GF** Σ<sub>1</sub>.

Usage
-----

The `ltlnorm` program interactively reads LTL formulas in the [Spot](https://spot.lrde.epita.fr/) format, line by line, and prints their normal forms using the same syntax.

Several test cases in `tests` and auxiliary scripts in `scripts` are provided to test and benchmark the implementation. For example, the following command runs a test suite of 1000 random formulas:

```bash
$ python scripts/check.py tests/random1000.spot
```

The option `--equiv-check` can be passed to check the equivalence of the normal form with regard to the original formula (it may take some time). [Owl](https://owl.model.in.tum.de/) can also be executed using this script. A statistical summary of the results can be obtained with

```bash
$ python scripts/summarize.py result.csv
```

Spot's Python library is required for the first and other scripts, and [Pandas](https://pandas.pydata.org/) is required for the last one.

How to build
------------

The [Meson](https://mesonbuild.com/) build system can be used to generate the `ltlnorm` program.

```bash
$ meson --buildtype release build
```

Input formulas are parsed with [Spot](https://spot.lrde.epita.fr/), so this library is a required dependency for `ltlnorm`. If Spot is properly installed, it will be found by Meson through `pkg-config`.
