#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)
bin/removepeer $addr

[[ $(count_addpeer_procs) == 0 ]]

set +e
output=$(bin/addcontent $addr "content" 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ "^Error" ]]
