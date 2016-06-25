#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
[[ $(count_addpeer_procs) == 1 ]]
output=$(cat out)
[[ $output =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)\ [0-9]+$ ]]
