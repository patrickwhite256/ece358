#!/bin/bash

source tests/testbase.sh

create_network 5

addr=$ADDRS[0]
validate_load_balancing 0
key1=$(bin/addcontent $addr "1")
validate_load_balancing 1
key2=$(bin/addcontent $addr "2")
validate_load_balancing 2
key3=$(bin/addcontent $addr "3")
validate_load_balancing 3
key4=$(bin/addcontent $addr "4")
validate_load_balancing 4
key5=$(bin/addcontent $addr "5")
validate_load_balancing 5
key6=$(bin/addcontent $addr "6")
validate_load_balancing 6
key7=$(bin/addcontent $addr "7")
validate_load_balancing 7
key8=$(bin/addcontent $addr "8")
validate_load_balancing 8

bin/removecontent $addr $key1
validate_load_balancing 7

bin/removepeer $ADDRS[1]
validate_load_balancing 7

bin/removepeer $ADDRS[3]
validate_load_balancing 7

bin/addpeer $addr
validate_load_balancing 7

bin/addpeer $addr
validate_load_balancing 7

bin/removecontent $addr $key8
validate_load_balancing 6
