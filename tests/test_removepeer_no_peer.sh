#!/bin/bash

source tests/testbase.sh

set +e
bin/removepeer 127.0.0.1 1234
res=$?
set -e

[[ $res != 0 ]]
