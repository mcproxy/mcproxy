#!/bin/bash
#add a tap device
#start it without root previleges
if [ "$1" = "-h" ] || [ "$1" = "help" ]; then

    echo "##-- $0 creates a tap device --##"
    echo "Usage: tap.sh [-h | help | create <tap name> | delete <tap name>] [add <addr> [netmask <addr]]"
    echo "show help message:     -h or help"
    echo "create a tap:          create <tap name>"
    echo "delete a tap device:   create <tap name>"

elif [ "$1" = "create" ] && [ "$2" ]; then

    sudo modprobe tun
    sudo mkdir -p /dev/net
    sudo mknod /dev/net/$2 c 10 200
    sudo ./create_tap_device -c $2

    if [ "$3" = "add" ] && [ "$5" = "netmask" ] && [ "$6" ]; then
        sudo ifconfig $2 $4 $5 $6 
    elif [ "$3" = "add" ] && [ "$4" ]; then
        sudo ifconfig $2 $4 
    fi

elif [ "$1" = "delete" ] && [ "$2" ]; then

    sudo rm -f /dev/net/$2 
    sudo ./create_tap_device -d $2

else

    echo "$0: Wrong or missing arguments. See -h for more details."

fi



#if server
#sudo ifconfig tap$1 192.168.1.1/24
#sudo simpletun/simpletun -i tap$1 -s -a

#if client
#sudo ifconfig tap$1 192.168.1.100/24
#sudo simpletun/simpletun -i tap$1 -c <serverIP> -a

#was passiert:
#-es wird unter /dev/net ein device erstellt
#-es wird ein tap netzwerkinterface erstellt, verküpft mit /dev/net/<tunnelinterface>, ohne bezug zu irgendeiner hardware
#-über simpletun wird eine verbindung zwischen zwei rechnern aufgebaut auf z.B eth0,
# simpletun ließt aus /dev/net/<tunnelinterfae> und schickt es über tcp zum anderen rechner und schreibt es dort wieder in das tunnelinterface
#-programme können die tunnelinterfaces so nutzen als währen es normale interfaces

 
