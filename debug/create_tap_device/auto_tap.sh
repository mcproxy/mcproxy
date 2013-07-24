#!/bin/bash
#add a tap device
#start it without root previleges
if [ "$1" = "-h" ] || [ "$1" = "help" ]; then

    echo "##-- $0 creates a many tap devices --##"
    echo "Usage: auto_tap.sh [-h | help]"
    echo "Usage: auto_tap.sh [delete | create] <quantity>"

elif [ "$1" = "create" ] || [ "$1" = "delete" ]; then

    for (( c=1; c<=$2; c++ ))
    do
        ./tap.sh $1 tap$c add 192.168.33.$c netmask 255.255.255.255 
    done   

else

    echo "$0: Wrong or missing arguments. See -h for more details."

fi

