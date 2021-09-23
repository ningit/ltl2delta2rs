#!/bin/sh
#
# Generate a summary with the test cases mentioned in TESTS
#

TESTS="random1000 tlsf21_100 tlsf21_300 uwuw wu"
CSVS=""

identity="$(date --iso-8601=minutes)"

echo "Date: $identity."

for f in $TESTS; do
	python scripts/check.py --imp cpp,owl "tests/$f.spot" -o "results/$f.csv"
	CSVS+="results/$f.csv "
done

python scripts/summarize.py -o "results/summary-$identity.json" $CSVS
