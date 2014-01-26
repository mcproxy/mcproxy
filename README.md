Introduction
============
Mcproxy is an IGMP/MLD Proxy daemon for Linux.

IGMP/MLD proxies offer the possibility option to combine local 
multicast networks with a larger multicast infrastructure. In contrast 
to multicast routers, proxies are lightweight and do not require the 
support of a multicast routing protocol such as PIM or DVMRP. A 
common use case is a local stub networks that interconnects with a 
remote multicast routing domain, e.g. via a tunnel. But it can also be
used in PMIPv6 domain to enable multicast for sources ([pmipv6-source-draft](http://tools.ietf.org/html/draft-ietf-multimob-pmipv6-source)) 
and listeners ([RFC 6224](http://tools.ietf.org/html/rfc6224)). The mcproxy
meets the requirements of the IGMP/MLD proxying standard 
([RFC 4605](http://tools.ietf.org/html/rfc4605)) and has additional functionalities. 
The multicast proxy can be instantiated multiple times, is dynamically
configurable at runtime, supports multiple upstreams and peering-interfaces 
for a non hierarchical interconnection of multicast proxies.


Requirements
============
*  To generate a makefile, qmake must be installed. This can be done with
the following command:
  
  newer systems: 

        apt-get install qt5-qmake
        apt-get install qt5-default

  older systems: 

        apt-get install qt4-qmake

*  To build the mcproxy, the libraries boost_threat, boost_date_time and 
boost_system must be installed. This can be done with the following 
command:
  
        apt-get install libboost-all-dev

*  To use the IPv6 functionality the kernel has to be configured and 
compiled with the experimental kernel feature _IPv6: multicast routing_.
For more details go to chapter [Startup](#startup).

*  To use more then one proxy instance for IPv4 and IPv6 the kernel has
to  be configured and compiled with the experimental kernel feature
_IP: multicast policy routing_ and _IPv6: multicast policy routing_. 
For more details go to chapter [Startup](#startup).

*  To build the documentation, doxygen must be installed. This can be
done with the following command:

        apt-get install doxygen

*  The mcproxy has to be started with root privileges.

*  A Linux kernel version greater than version 2.6.32 is required.


Compilation
===========
Build mcproxy in release mode:

    cd mcproxy/
    qmake 
    make

Build mcproxy in debug mode:

    cd mcproxy/
    qmake CONFIG+=debug
    make


Installation
============
To copy mcproxy to the system directory, run (optional):

    make install


Documentation
=============
Mcproxy includes a HTML documentation. The documentation will 
be located in the docs/ directory after the execution of:

    make doc


Startup
=======
At first you should check the available kernel features of your
system. Type the following command:

    sudo mcproxy -c
   
If a kernel feature you need failed you have to reconfigure and
recompile your linux kernel. In the debug folder is a README file 
which could help you with this problem.

To run the mcproxy you need to create a valid configuration file.
There is an example in the project folder (mcproxy.conf).

*  To run the mcproxy in the background type the following command:

        sudo nohup mcproxy -f <path/to/config_file> &

*  To run the mcprocy with all available status and debug messages:

        sudo mcproxy -dsvv -f <path/to/config_file>

For more information see `mcproxy -h` or visit our project page.


Contact
=======
Project page: http://mcproxy.realmv6.org/

Mailing list: multicast-proxy@googlegroups.com

[![githalytics.com alpha](https://cruel-carlota.pagodabox.com/b9a43dd68bb4fde3e3569ad98eda4ecc "githalytics.com")](http://githalytics.com/mcproxy/mcproxy)
