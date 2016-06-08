#!/bin/bash

verbose=
if [[ "$1" == "-v" ]]; then
    verbose=1
fi

TEST_DIR=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
REPO_DIR=$(cd $TEST_DIR/.. && pwd)

cd $REPO_DIR
make -s clean
DEBUG= make -s

count=0
failed=0
failed_names=()
for test_file in $TEST_DIR/test_*.sh
do
    count=$(( $count + 1 ))
    if [[ -n "$verbose" ]]; then
        (
            bash $test_file
        )
    else
        (
            bash $test_file
        ) > /dev/null 2>&1
    fi
    if [[ $? == 0 ]]; then
        echo -ne "\e[0;32m.\e[0m"
    else
        echo -ne "\e[0;31m.\e[0m"
        failed_names[$failed]=$(basename $test_file)
        failed=$(( $failed + 1 ))
    fi
done
echo ""
passed=$(( count - failed ))
echo -n "$passed/$count "
if [[ $failed == 0 ]]; then
    echo -e "\e[0;32m(ALL PASSED)\e[0m"
else
    echo -e "\e[0;31m($failed FAILED)\e[0m"
    echo "Failed cases:"
    for failed in $failed_names
    do
        echo "- $failed"
    done
fi

