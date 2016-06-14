#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)
bin/addpeer $addr > out
addr2=$(cat out)

key1=$(bin/addcontent $addr "one")
key2=$(bin/addcontent $addr "two")

bin/removepeer $addr2
bin/addpeer $addr

key3=$(bin/addcontent $addr "three")
key4=$(bin/addcontent $addr "four")

value1=$(bin/lookupcontent $addr $key1)
value2=$(bin/lookupcontent $addr $key2)
value3=$(bin/lookupcontent $addr $key3)
value4=$(bin/lookupcontent $addr $key4)

[[ $value1 == "one" ]]
[[ $value2 == "two" ]]
[[ $value3 == "three" ]]
[[ $value3 == "four" ]]
