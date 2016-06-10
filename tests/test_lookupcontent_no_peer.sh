#!/bin/bash

source tests/testbase.sh

set +e
output=$(bin/lookupcontent 127.0.0.1 1234 33 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ "^Error" ]]
