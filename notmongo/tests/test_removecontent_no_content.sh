#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

set +e
output=$(bin/removecontent $addr 55 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ ^Error ]]
