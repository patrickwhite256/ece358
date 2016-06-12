#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

key=$(bin/addcontent $addr "content")
bin/removepeer $addr

set +e
output=$(bin/lookupcontent $addr $key 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ ^Error ]]
