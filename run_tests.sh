#!/usr/bin/env bash
# Runs the three module test suites and reports an aggregate pass/fail.
# Usage: ./run_tests.sh   (after `make test_io_unit test_memory_unit test_compute_unit`,
#                          or just run `make test` which builds and runs everything)

set -u
overall=0

for bin in test_io_unit test_memory_unit test_compute_unit; do
    echo "=============================="
    if [ ! -x "./$bin" ]; then
        echo "MISSING: ./$bin (build it first, e.g. 'make $bin')"
        overall=1
        continue
    fi
    ./"$bin"
    rc=$?
    if [ $rc -ne 0 ]; then
        overall=1
    fi
done

echo "=============================="
if [ $overall -eq 0 ]; then
    echo "ALL MODULE TEST SUITES PASSED"
else
    echo "ONE OR MORE MODULE TEST SUITES REPORTED FAILURES"
fi
exit $overall
