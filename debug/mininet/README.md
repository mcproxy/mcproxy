<!--vim: set textwidth=80 formatoptions+=t wrapmargin=5 -->

Mcproxy and Mininet
===================
Here you can find examples of how to use Mcproxy within [Mininet](mininet.org).

#### Requirements
You have to install Mininet, which can usually be done with the following command:

    sudo apt-get install mininet
    
You have to compile the Mcproxy and the Mcproxy Tester (see [README](../README.md#mcproxy-tester)).

#### Usage   
For Mininet you need a root shell:

    sudo su

To run the network simulation type:

    python2 network.py  

If you have problems to start mininet, try:

    killall ovs-controller
    mn -c

Example **PMIPv6**
=================
The topologie discribed by the file [network.py](pmipv6/network.py) looks like this:   
                  
<img src="figures/pmipv6.png" alt="pmipv6 topologie" height="170"> 

On the boxes LMA, MAG1, and MAG2 runs a single proxy instance each with a
different multicast routing table. The behaviour every instance are described
in configuration files lma.conf, mag1.conf, mag2.conf.  The Host1 sends group
packages of the group 239.99.99.99 to the interface host1-eth0 and Host2
receives hopefully these packages on inteface host2-eth0. The exact behaviours
of the hosts are described in the file tester.ini. as
     
   
   
