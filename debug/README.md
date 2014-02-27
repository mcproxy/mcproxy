<!--vim: set textwidth=80 formatoptions+=t wrapmargin=5 -->

Debugging and Testing the Mcproxy
=================================
Here you find tools and backround knowledge to debug and test the Mcproxy.


Dummy Interface
===============
An easy way to test the Mcproxy without physical hardware is to create dummy
network interfaces.

#### Usage
Create 12 dummy interfaces:

    sudo dummy_interfaces/auto_dummy.sh create 12

Delete all dummy interfaces:

    sudo dummy_interfaces/auto_dummy.sh delete

Mininet
=======
[Mininet](mininet.org) creates a realistic virtual network, running real
kernel, switch and application code, on a single machine. This
[REAMDE](mininet/README.md) explains how to use Mcproxy within Mininet.

Mcproxy Tester
==============
With the _Mcproxy Tester_ you can join groups, set source filter and send
multicast packets for IPv4 and IPv6. 

#### Requirements
To build the _Mcproxy Tester_, the library boost_regex must be installed. This
can be done with the following command:
  
    sudo apt-get install libboost-all-dev

#### Compilation
Build the _Tester_ and move it to the directory tester:

    cd ../mcproxy/
    make clean 
    qmake CONFIG+=tester
    make
    cp tester ../debug/tester/.

#### Configuration
The _Tester_ loads per default an INI file with the name _tester.ini_. This INI file
[example](tester/tester.ini) defines two simple configurable actions: 

* **receive_data** subscribes group 239.99.99.99 with three sources in include mode and wait for arriving data

* **send_a_hello** sends a hello message to group 239.99.99.99       

#### Usage
These actions can be executed with the following command:

    ./tester receive_data

or

    ./tester send_a_hello tester.ini 

Packet Dropper
==============
With the _Packet Dropper_ it is possible to interrupt links without changing
interface states. It configures an interface to drop all outgoing
packets (**Hint**: It is nessesary to run it on both sides of the link).

#### Usage
To interrupt the link:

    packet_dropper/packet_dropper eth0 drop

To reconnect the link:

    packet_dropper/packet_dropper eth0 clear

Kernel
======
If you miss a kernel module you have to build your own kernel. The script
[install_kernel-sh](./kernel/install_kernel-sh) downloads, configures, compiles and
installs one for you.

#### Usage
Type the following command for more information:

    kernel/install_kernel-sh help

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


General Debugging Informations
==============================
Displays the subscribed groups per interface:

    /proc/net/igmp
    /proc/net/igmp6
    ip maddress show


Displays announced network interfaces for multicast routing (only for the default
multicast talbe):

    /proc/net/ip_mr_vif
    /proc/net/ip6_mr_vif


Displays the multicast forwarding rules (only for the default multicast table):

    /proc/net/ip_mr_cache
    /proc/net/ip6_mr_cache


Displays the status of the multicast routing flag (mrt). It can only be hold by
one socket per multiast routing table at a time):

    /proc/sys/net/ipv4/conf/all/mc_forwarding
    /proc/sys/net/ipv6/conf/all/mc_forwarding


Displays all announced interfaces for multicast routing:

    /proc/sys/net/ipv4/conf/eth0/mc_forwarding
    /proc/sys/net/ipv6/conf/eth0/mc_forwarding
    

Displays the multicast routing rules for a specific table (table 0 is the
default table and summaries all tables):

    ip -s mroute show table 0
    ip -6 -s mroute show table 0


Defines the maximum allowed memberships:

    /proc/sys/net/ipv4/igmp_max_memberships


Defines the maximum allowed source filter:

    /proc/sys/net/ipv4/igmp_max_msf
    /proc/sys/net/ipv6/igmp_max_msf
