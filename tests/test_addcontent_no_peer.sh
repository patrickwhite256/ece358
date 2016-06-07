#!/bin/bash

source tests/testbase.sh

set +e
bin/addcontent 127.0.0.1 1234 "content"
res=$?
set -e

[[ $res != 0 ]]
