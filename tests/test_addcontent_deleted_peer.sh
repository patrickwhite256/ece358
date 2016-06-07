#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)
bin/removepeer $addr

[[ $(count_addpeer_procs) == 0 ]]

set +e
bin/addcontent $addr "content"
res=$?
set -e

[[ $res != 0 ]]
