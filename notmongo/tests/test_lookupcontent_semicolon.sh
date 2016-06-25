#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)
bin/addpeer $addr

key1=$(bin/addcontent $addr "con;tent")
key2=$(bin/addcontent $addr "cont;ent")
value1=$(bin/lookupcontent $addr $key1)
value2=$(bin/lookupcontent $addr $key2)

[[ $value1 == "con;tent" ]]
[[ $value2 == "cont;ent" ]]
