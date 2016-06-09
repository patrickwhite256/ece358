#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

bin/addpeer $addr
[[ $(count_addpeer_procs) == 2 ]]

bin/addpeer $addr
[[ $(count_addpeer_procs) == 3 ]]

bin/addpeer $addr
[[ $(count_addpeer_procs) == 4 ]]
