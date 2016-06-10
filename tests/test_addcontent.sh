#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

output=$(bin/addcontent $addr "content")

[[ $output =~ [0-9]+ ]]
