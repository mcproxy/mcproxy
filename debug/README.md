<!--vim: set textwidth=80 formatoptions+=t wrapmargin=5 -->

Debugging and Testing the Mcproxy
=================================
Here you find a tools and backround knowledge to debug and test the mcproxy.


Dummy interface
===============
An easy way to test the mcproxy without physical hardware is to create dummy
network interfaces.

#### Usage
Create 12 dummy interfaces:

    sudo dummy_interfaces/auto_dummy.sh create 12

Delete all dummy interface:

    sudo dummy_interfaces/auto_dummy.sh delete

Mininet
=======
"Mininet creates a realistic virtual network, running real kernel, switch and
application code, on a single machine" ([mininet](mininet.org)). This
[REAMDE](mininet/README.md) explains how to use mcproxy within mininet.

Mcproxy tester
==============
With the mcproxy tester you can join groups, set source filter and send
multicast packets for ipv4 and 6.

#### Requirements
To build the mcproxy tester, the library boost_regex must be installed. This
can be done with the following command:
  
    apt-get install libboost-all-dev

#### Usage
Build the tester and move it do directory tester:

    cd ../mcproxy/
    make clean 
    qmake CONFIG+=tester
    make
    cp tester ../debug/tester/.

The Tester loads per default the ini script tester.ini.  The ini script example
debug/tester/tester.ini defines two actions: 

    receive_data 

        subscribes group 239.99.99.99 with three sources
        in include mode and wait for arriving data

    send_a_hello

        sends a hello message to group 239.99.99.99       

These actions can be executed with the following command:

    ./tester receive_data

or

    ./tester send_a_hello tester.ini 

Packet Dropper
==============
With the packet dropper it is possible to interrupt links without changing
interface states. packet_dropper configure an interface to drop all outgoing
packets (Maybe it is nessesary to run it on both sides of the connection).

#### Usage
To interrupt the link:

    cd packet_dropper
    packet_dropper eth0 drop

To connect that link

    cd packet_dropper
    packet_dropper eth0 clear

Kernel
======
If you miss a kernel module you have to build your own kernel. The script
(install_kernel-sh)[./kernel/install_kernel-sh] download, configure, compile and
install one for you.

#### Usage
    see ./kernel/install_kernel-sh help

#### Kernel Configuration Options
    The following options should be enabled:
        IP_MULTICAST=y
        IP_MROUTE=y
        IP_MROUTE_MULTIPLE_TABLES=y

        IPV6=y
        IPV6_MROUTE=y
        IPV6_MROUTE_MULTIPLE_TABLES=y

    Locations:
    +-> Networking support
    |-+-> Networking options
      |-+-> TCP/IP networking
        |  
        |====> IP: multicast routing
        |
        |-+-> The IPv6 protocol
          |
          |====> IPv6: multicast routing


General Debugging Stuff
=======================
    /proc/net/igmp
    /proc/net/igmp6
    ip maddress show
        shows the subscribed groups per interface

    /proc/net/ip_mr_vif
    /proc/net/ip6_mr_vif
        shows announced network interfaces for multicast routing
        (only for the default multicast talbe)

    /proc/net/ip_mr_cache
    /proc/net/ip6_mr_cache
        shows the multicast forwarding rules
        (only for the default multicast table)

    /proc/sys/net/ipv4/conf/all/mc_forwarding
    /proc/sys/net/ipv6/conf/all/mc_forwarding
        shows if the multicast routing (mrt) flag is set
        can only be hold once per multicast routing table
            by a socket at a time 

    /proc/sys/net/ipv4/conf/eth0/mc_forwarding
    /proc/sys/net/ipv6/conf/eth0/mc_forwarding
        shows if the interface is announced for multicast routing

    ip -s mroute show table 0
    ip -6 -s mroute show table 0
        shows the multicast routing rules for a specific table 
        table 0 is the default table and summaries all tables  

    /proc/sys/net/ipv4/igmp_max_memberships
        defines the maximum allowed memberships 

    /proc/sys/net/ipv4/igmp_max_msf
    /proc/sys/net/ipv6/igmp_max_msf
        defines the maximum allowed source filter 


