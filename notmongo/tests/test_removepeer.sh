#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)
output=$(bin/removepeer $addr)

[[ $output == "" ]]
[[ $(count_addpeer_procs) == 0 ]]
