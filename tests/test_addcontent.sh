#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

[[ $(count_addpeer_procs) == 0 ]]

output=$(bin/addcontent $addr "content")

[[ $output =~ [0-9]+ ]]
