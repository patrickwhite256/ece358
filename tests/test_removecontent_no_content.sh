#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

set +e
output=$(bin/removepeer $addr 55 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ "^Error" ]]
