#!/bin/sh

set -e

TESTS_DIR="$1"

cd ${TESTS_DIR}

# create a temporary copy of old checksums list
if [ -f "sha1sums.txt" ] && [ -r "sha1sums.txt" ] && [ -w "sha1sums.txt" ]; then
    cp sha1sums.txt sha1sums_old.txt
elif ! [ -e "sha1sums.txt" ]; then
    touch sha1sums_old.txt
else
    echo "${TESTS_DIR}/sha1sums.txt: exists but is not a regular, readable/writable file"
    echo "ABORTING"
    exit 1
fi

# generate a temporary new checksums list
if ! [ -e "sha1sums_new.txt" ] || ( [ -f "sha1sums_new.txt" ] && [ -r "sha1sums_new.txt" ] && [ -w "sha1sums_new.txt" ] ); then
    find . -name *.pgm -exec sha1sum '{}' \; | sort --key=2 > sha1sums_new.txt
else
    echo "${TESTS_DIR}/sha1sums_new.txt: exists but is not a regular, readable/writable file"
    echo "ABORTING"
    exit 1
fi

# combine the lists: remove duplicates, update checksums that have changed
sort --stable --unique --merge --key=2 sha1sums_new.txt sha1sums_old.txt > sha1sums.txt
rm sha1sums_new.txt sha1sums_old.txt
