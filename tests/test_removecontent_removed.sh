#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

key=$(bin/addcontent $addr "content")
bin/removecontent $addr $key

set +e
output=$(bin/removecontent $addr $key 2>&1)
res=$?
set -e

[[ $res != 0 ]]
[[ $output =~ ^Error ]]
