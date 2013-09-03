#!/bin/bash
#add a tap device
#start it without root previleges
if [ "$1" = "-h" ] || [ "$1" = "help" ]; then

    echo "##-- $0 creates dummy network devices --##"
    echo "Usage: auto_tap.sh [-h | help]"
    echo "Usage: auto_tap.sh [delete | change | create <quantity>] [-ssm | +ssm]"

elif [ "$1" = "create" ] && [ "$2" ] && [ "$2" -lt 255 ]; then 

    $0 delete #delete all dummies
    sudo modprobe dummy numdummies=$2
    for (( c=0; c<$2; c++ ))
    do
        sudo ifconfig dummy$c up
        sudo ifconfig dummy$c 192.168.100.$(($c+1)) netmask 255.255.255.255
        sudo ifconfig dummy$c inet6 add fd00::$(($c+1))/64
        if [ "$3" = "-ssm" ]; then
            sudo sh -c "echo 2 > /proc/sys/net/ipv4/conf/dummy$c/force_igmp_version"
            sudo sh -c "echo 1 > /proc/sys/net/ipv6/conf/dummy$c/force_mld_version"
        elif [ "$3" = "+ssm" ]; then
            sudo sh -c "echo 3 > /proc/sys/net/ipv4/conf/dummy$c/force_igmp_version"
            sudo sh -c "echo 2 > /proc/sys/net/ipv6/conf/dummy$c/force_mld_version"
        fi
    done   
    
        echo "create new dummies"
        echo "set ipv4 and ipv6 address" 

        if [ "$3" = "-ssm" ]; then
            echo "force igmp version to igmpv2"
            echo "force mld version to mldv1"
        elif [ "$3" = "+ssm" ]; then
            echo "force igmp version to igmpv3"
            echo "force mld version to mldv2"
        fi
        
elif [ "$1" = "change" ] && [ "$2" ] && [ "$2" -lt 255 ]; then 
    
    if [ "$3" = "-ssm" ] || [ "$3" = "+ssm" ]; then
        for (( c=0; c<$2; c++ ))
        do
            if [ "$3" = "-ssm" ]; then
                sudo sh -c "echo 2 > /proc/sys/net/ipv4/conf/dummy$c/force_igmp_version"
                sudo sh -c "echo 1 > /proc/sys/net/ipv6/conf/dummy$c/force_mld_version"
            elif [ "$3" = "+ssm" ]; then
                sudo sh -c "echo 3 > /proc/sys/net/ipv4/conf/dummy$c/force_igmp_version"
                sudo sh -c "echo 2 > /proc/sys/net/ipv6/conf/dummy$c/force_mld_version"
            fi
        done   

        if [ "$3" = "-ssm" ]; then
            echo "force igmp version to igmpv2"
            echo "force mld version to mldv1"
        elif [ "$3" = "+ssm" ]; then
            echo "force igmp version to igmpv3"
            echo "force mld version to mldv2"
        fi

    else

        echo "$0: Wrong or missing arguments. See -h for more details."
        
    fi

elif [ "$1" = "delete" ]; then

    sudo modprobe -r dummy
    echo "delete all dummies"

else

    echo "$0: Wrong or missing arguments. See -h for more details."

fi

