#!/bin/sh

set -e

TESTS_DIR="$1"

cd ${TESTS_DIR}

if [ -f "sha1sums.txt" ] && [ -r "sha1sums.txt" ]; then
    sha1sum -c --quiet --ignore-missing sha1sums.txt && echo PASSED
    exit 0
else
    echo "${TESTS_DIR}/sha1sums.txt: not found or is not a regular, readable file"
    echo "ABORTING"
    exit 1
fi
