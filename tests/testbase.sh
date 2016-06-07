#!/bin/bash

set -e

trap "rm -f out; kill_addpeer_procs" EXIT

kill_addpeer_procs() {
   ps aux | fgrep "bin/addpeer" | grep -v grep | awk '{print $2}' | xargs -r kill 
}

count_addpeer_procs(){
    echo $(ps aux | fgrep "bin/addpeer" | grep -v grep | wc -l)
}

kill_addpeer_procs
[[ $(count_addpeer_procs) == 0 ]]

