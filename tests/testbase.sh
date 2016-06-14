#!/bin/bash

set -e

trap "rm -f out; kill_addpeer_procs" EXIT

kill_addpeer_procs() {
   ps aux | fgrep "bin/addpeer" | grep -v grep | awk '{print $2}' | xargs -r kill
}

count_addpeer_procs(){
    echo $(ps aux | fgrep "bin/addpeer" | grep -v grep | wc -l)
}

# creates $1 peers and sets their addr and ports to the ADDRS array
create_network() {
    bin/addpeer > out
    addr=$(cat out)
    ADDRS=("$addr")
    for (( i=1; i<$1; i++ )); do
        bin/addpeer $addr> out
        addr=$(cat out)
        ADDRS[$i]=$addr
    done
}

# assumes the network is stored in ADDRS as from create_network
# validates the load balancing criteria is met and that the total # of keys is $1
# format is a,b,c,0
validate_load_balancing() {
    total_keys=0
    total_peers=0
    counts=()
    for addr in "${ADDRS[@]}"; do
        keylist=$(bin/allkeys $addr)
        count=$(fgrep -o "," <<< $keylist | wc -l)
        counts[$total_peers]=$count
        total_peers=$(( $total_peers + 1 ))
        total_keys=$(( $total_keys + $count ))
    done
    upper=$(python -c "import math;print int(math.ceil($total_keys / float($total_peers)))")
    lower=$(python -c "import math;print int(math.floor($total_keys / float($total_peers)))")
    [[ $total_keys == $1 ]]
    for count in ${counts[@]}; do
        [ $count -le $upper ]
        [ $count -ge $lower ]
    done
}

kill_addpeer_procs
[[ $(count_addpeer_procs) == 0 ]]
