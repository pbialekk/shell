#!/bin/bash
trap '' 2

cd tests
touch .testout
tail -f .testout &

if [ "$(bash ./run_all.sh ../shell/bin/mshell | tee .testout | grep FAIL)" ]; then
    rm .testout
    exit 1
fi

rm .testout