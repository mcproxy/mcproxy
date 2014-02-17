#!/bin/bash
#configure an interface to drop all outgoing packets
#start it without root previleges
if [ "$1" = "-h" ] || [ "$1" = "help" ]; then

    echo "##-- $0 --##"
    echo "Usage: $0 <interface> drop"
    echo "Usage: $0 <interface> clear"

elif [ "$2" ] && [ "$2" = "drop" ]; then 

    echo "configure $1 to drop all outgoing packets"
    sudo tc qdisc add dev $1 root handle 1: netem loss 100%

elif [ "$2" ] && [ "$2" = "clear" ]; then 
    
    echo "clear drop filter on interface $1"
    sudo tc qdisc delete dev $1 root handle 1: netem loss 100%

else

    echo "$0: Wrong or missing arguments. See -h for more details."

fi

