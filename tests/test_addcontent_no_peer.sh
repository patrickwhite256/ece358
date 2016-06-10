#!/bin/bash

source tests/testbase.sh

set +e
output=$(bin/addcontent 127.0.0.1 1234 "content" 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ "^Error" ]]
