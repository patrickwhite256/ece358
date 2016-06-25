#!/bin/bash

source tests/testbase.sh

bin/addpeer > out
addr=$(cat out)

key=$(bin/addcontent $addr "content")
bin/removecontent $addr $key
