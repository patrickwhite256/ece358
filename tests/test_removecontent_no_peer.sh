#!/bin/bash

source tests/testbase.sh

set +e
output=$(bin/removepeer 127.0.0.1 1234 11 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ "^Error" ]]
