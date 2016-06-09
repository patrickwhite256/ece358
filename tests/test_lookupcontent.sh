#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

key=$(bin/addcontent $addr "content")
value=$(bin/lookupcontent $addr $key)

[[ $value == "content" ]] 
