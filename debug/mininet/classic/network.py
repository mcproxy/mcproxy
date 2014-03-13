#!/usr/bin/python
import sys
from time import sleep
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.node import Controller
from mininet.node import OVSController

class Example( Topo ):
    def __init__( self,**opts):
        topoopts=dict(link=TCLink)
        Topo.__init__( self,**opts )

        # Add hosts 
        proxy = self.addHost('proxy')
        s1 = self.addSwitch('s1');
        host1 = self.addHost('host1')

        s2 = self.addSwitch('s2');
        host2 = self.addHost('host2')
        host3 = self.addHost('host3')

        host4 = self.addHost('host4')

        self.addLink(proxy, s1)
        self.addLink(s1, host1)

        self.addLink(proxy, s2)
        self.addLink(s2, host2)
        self.addLink(s2, host3)

        self.addLink(proxy, host4)


def x(subnet):
    return '192.168.' + subnet 

def reset_rp_filter(host, if_list):
    for interf in if_list:
        print host.cmd('echo 0 >/proc/sys/net/ipv4/conf/' + interf +'/rp_filter')

def ping(host, subnet):
    print host
    print host.cmd('ping -qnc 2 ' + x(subnet))

def start_mcproxy(host, config_file):
    mcproxy='../../../mcproxy/mcproxy'
    print host.cmd('xterm -e "' + mcproxy + ' -sdvv -f ' + config_file + '; sleep 100"&')

def set_interface_delay(action, host, interface, delay): #action: add/change/delete
    print host.cmd('tc qdisc ' + action + ' dev ' + interface + ' root handle 1: netem delay ' + delay)

def killall(host):
    print host.cmd('killall mcproxy')    
    print host.cmd('killall tester')

def set_interface_delays(proxy, host1, host2, host3, host4):
    #action = 'add' or 'change'

    proxy_host_delay=  '20ms 5ms' #delay, jitter

    set_interface_delay('add', proxy, 'proxy-eth0', proxy_host_delay)
    set_interface_delay('add', proxy, 'proxy-eth1', proxy_host_delay)
    set_interface_delay('add', proxy, 'proxy-eth2', proxy_host_delay)

    set_interface_delay('add', host1, 'host1-eth0', proxy_host_delay)
    set_interface_delay('add', host2, 'host2-eth0', proxy_host_delay)
    set_interface_delay('add', host3, 'host3-eth0', proxy_host_delay)
    set_interface_delay('add', host4, 'host4-eth0', proxy_host_delay)

def start_tester(host, action):
    tester='../../../mcproxy/tester'
    print host.cmd('xterm -e "' + tester + ' ' + action + ' ; sleep 100"&')

def force_multicast_proto_version(host, hostnumber, version):
    print host.cmd('echo ' + str(version) + ' > /proc/sys/net/ipv4/conf/host' + str(hostnumber) + '-eth0/force_igmp_version')

def get_multicast_proto_version(host, hostnumber):
    print host.cmd('cat /proc/sys/net/ipv4/conf/host' + str(hostnumber) + '-eth0/force_igmp_version')

def help_msg():
    print "-- possible commands --"
    print "(start|stop) mcproxy"
    print "(start|stop) host"
    print "\th1_send"
    print "\th2_recv"
    print "\th3_recv"
    print "(get|set) igmp"
    print "help"
    print "exit"

def TopoTest():
    topo=Example()	
    net = Mininet(topo=topo, controller = OVSController, link=TCLink)
    net.start()
   
    proxy = net.get('proxy') 
    host1 = net.get('host1')
    host2 = net.get('host2')
    host3 = net.get('host3')
    host4 = net.get('host4')

    #configure proxy
    proxy.setIP(x('0.1'), 24, 'proxy-eth0')
    proxy.setIP(x('1.1'), 24, 'proxy-eth1')
    proxy.setIP(x('2.1'), 24, 'proxy-eth2')

    reset_rp_filter(proxy, ['all', 'proxy-eth0', 'proxy-eth1', 'proxy-eth2'])

    #configure hosts
    host1.setIP(x('0.2'), 24, 'host1-eth0')
    host2.setIP(x('1.2'), 24, 'host2-eth0')
    host3.setIP(x('1.3'), 24, 'host3-eth0')
    host4.setIP(x('2.2'), 24, 'host4-eth0')

    reset_rp_filter(host1, ['all', 'host1-eth0'])
    reset_rp_filter(host2, ['all', 'host2-eth0'])
    reset_rp_filter(host3, ['all', 'host3-eth0'])
    reset_rp_filter(host4, ['all', 'host4-eth0'])

    #delays
    set_interface_delays(proxy, host1, host2, host3, host4)

    #run programms
    ##################################################
 
    def get_host(hostnumber):
        if hostnumber == 1:
            return host1 
        elif hostnumber == 2:
            return host2
        elif hostnumber == 3:
            return host3 
        else:
            print "hostnumber unknown (uses host4)"
            return host4

    help_msg()
    running = True
    while running: 
        str_input = raw_input("> ").strip()

        if str_input == "start mcproxy":
            start_mcproxy(proxy, 'proxy.conf')
        elif str_input == "stop mcproxy":
            print get_host(hostnumber).cmd('killall mcproxy')
        elif str_input == "start host":
            try:
                hostnumber = int(raw_input("host number? ").strip())
            except:
                print "wrong input"
                continue 
            action = raw_input("host action? ")
            start_tester(get_host(hostnumber), action)
        elif str_input == "stop host":
            try:
                hostnumber = int(raw_input("host number? ").strip())
            except:
                print "wrong input"
                continue 
            
            print get_host(hostnumber).cmd('killall tester')
        elif str_input == "set igmp":
            try:
                hostnumber = int(raw_input("host number? ").strip())
                igmpversion= int(raw_input("verison? ").strip())
            except:
                print "wrong input"
                continue 
            force_multicast_proto_version(get_host(hostnumber), hostnumber, igmpversion)
        elif str_input == "get igmp":
            try:
                hostnumber = int(raw_input("host number? ").strip())
            except:
                print "wrong input"
                continue 
            get_multicast_proto_version(get_host(hostnumber), hostnumber)
        elif str_input == "exit":
            running = False
        elif str_input == "help":
            help_msg()
        else:
            print "cmd not found"

    print 'program stopped'

if __name__=='__main__':
    TopoTest()

