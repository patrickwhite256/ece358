#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

bin/addpeer $addr > out
addr2=$(cat out)

key=$(bin/addcontent $addr "content")
bin/removepeer $addr2 $key
