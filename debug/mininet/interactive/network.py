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
        host1 = self.addHost('host1')
        host2 = self.addHost('host2')
        host3 = self.addHost('host3')
        host4 = self.addHost('host4')
        host5 = self.addHost('host5')
        dummy = self.addHost('dummy')

        s1 = self.addSwitch('s1');
        s2 = self.addSwitch('s2');
        s3 = self.addSwitch('s3');
        s4 = self.addSwitch('s4');

        self.addLink(proxy, s1)
        self.addLink(proxy, s2)
        self.addLink(proxy, s3)
        self.addLink(proxy, s4)


        self.addLink(s1, host1)
        self.addLink(s2, host2)
        self.addLink(s3, host3)
        self.addLink(s3, host4)
        self.addLink(s4, host5)

def x(subnet):
    return '192.168.' + subnet 

def reset_rp_filter(host, if_list):
    for interf in if_list:
        print host.cmd('echo 0 >/proc/sys/net/ipv4/conf/' + interf + '/rp_filter')

def ping(host, subnet):
    print host
    print host.cmd('ping -qnc 2 ' + x(subnet))

def start_mcproxy(host, config_file):
    mcproxy='../../../mcproxy/mcproxy'
    print host.cmd('xterm -e "' + mcproxy + ' -sdvv -f ' + config_file + '; sleep 10000"&')

def set_interface_delay(action, host, interface, delay): #action: add/change/delete
    print host.cmd('tc qdisc ' + action + ' dev ' + interface + ' root handle 1: netem delay ' + delay)

def set_interface_packet_lost(action, host, interface, packet_lost): #action: add/change/delete
    print host.cmd('tc qdisc ' + action + ' dev ' + interface + ' parent 1:1 netem loss ' + str(packet_lost) + '%')

def killall(host):
    print host.cmd('killall mcproxy')    
    print host.cmd('killall tester')

def set_interface_delays(proxy, host1, host2, host3, host4, host5):
    #action = 'add' or 'change'

    proxy_host_delay=  '20ms 5ms' #delay, jitter

    set_interface_delay('add', proxy, 'proxy-eth0', proxy_host_delay)
    set_interface_delay('add', proxy, 'proxy-eth1', proxy_host_delay)
    set_interface_delay('add', proxy, 'proxy-eth2', proxy_host_delay)
    set_interface_delay('add', proxy, 'proxy-eth3', proxy_host_delay)

    set_interface_delay('add', host1, 'host1-eth0', proxy_host_delay)
    set_interface_delay('add', host2, 'host2-eth0', proxy_host_delay)
    set_interface_delay('add', host3, 'host3-eth0', proxy_host_delay)
    set_interface_delay('add', host4, 'host4-eth0', proxy_host_delay)
    set_interface_delay('add', host5, 'host5-eth0', proxy_host_delay)

def start_tester(host, action):
    tester='../../../mcproxy/tester'
    print host.cmd('xterm -e "' + tester + ' ' + action + ' ; sleep 10000"&')

def force_multicast_proto_version(host, hostnumber, version):
    print host.cmd('echo ' + str(version) + ' > /proc/sys/net/ipv4/conf/host' + str(hostnumber) + '-eth0/force_igmp_version')

def get_multicast_proto_version(host, hostnumber):
    print host.cmd('cat /proc/sys/net/ipv4/conf/host' + str(hostnumber) + '-eth0/force_igmp_version')

def help_msg():
    print "-- possible commands --"
    print "start mcproxy"
    print "start host"
    print "\th1_send"
    print "\th2_recv"
    print "\th3_recv"
    print "(get|set) igmp"
    print "set packet lost"
    print "cmd"
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
    host5 = net.get('host5')
    dummy = net.get('dummy')

    #configure proxy
    proxy.setIP(x('0.1'), 24, 'proxy-eth0')
    proxy.setIP(x('1.1'), 24, 'proxy-eth1')
    proxy.setIP(x('2.1'), 24, 'proxy-eth2')
    proxy.setIP(x('3.1'), 24, 'proxy-eth3')

    reset_rp_filter(proxy, ['all', 'proxy-eth0', 'proxy-eth1', 'proxy-eth2', 'proxy-eth3'])

    #configure hosts
    host1.setIP(x('0.2'), 24, 'host1-eth0')
    host2.setIP(x('1.2'), 24, 'host2-eth0')
    host3.setIP(x('2.2'), 24, 'host3-eth0')
    host4.setIP(x('2.3'), 24, 'host4-eth0')
    host5.setIP(x('3.2'), 24, 'host5-eth0')

    reset_rp_filter(host1, ['all', 'host1-eth0'])
    reset_rp_filter(host2, ['all', 'host2-eth0'])
    reset_rp_filter(host3, ['all', 'host3-eth0'])
    reset_rp_filter(host4, ['all', 'host4-eth0'])
    reset_rp_filter(host5, ['all', 'host5-eth0'])

    #delays
    set_interface_delays(proxy, host1, host2, host3, host4, host5)

    #run programms
    ##################################################
 
    def get_host(hostnumber):
        if str(hostnumber) == "1":
            return host1 
        elif str(hostnumber) == "2":
            return host2
        elif str(hostnumber) == "3":
            return host3 
        elif str(hostnumber) == "4":
            return host4 
        elif str(hostnumber) == "5":
            return host5 
        elif str(hostnumber) == "p":
            return proxy
        else:
            print "hostnumber unknown (uses host6)"
            return host6

    help_msg()
    running = True
    while running: 
        str_input = raw_input("> ").strip()

        if str_input == "start mcproxy":
            start_mcproxy(proxy, 'proxy.conf')
        elif str_input == "start host":
            try:
                hostnumber = raw_input("host number? ").strip()
            except:
                print "wrong input"
                continue 
            action = raw_input("host action? ")
            start_tester(get_host(hostnumber), action)
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
        elif str_input == "cmd":
            try:
                hostnumber = raw_input("host number or p for proxy? ").strip()
                cmd= raw_input("cmd? ").strip()
            except:
                print "wrong input"
                continue 
            print get_host(hostnumber).cmd(cmd)
        elif str_input == "set packet lost":
            try:
                host = get_host(raw_input("host number or p for proxy? ").strip())
                interf = raw_input(str(host) + "-<interface>? ").strip()
                lost_rate = int(raw_input("lost rate(%)? ").strip())
                if lost_rate > 0:
                    set_interface_packet_lost('add', host, str(host) + interf, packet_lost)
                else:  
                    set_interface_packet_lost('del', host, str(host) + interf, packet_lost)
            except:
                print "wrong input"
                continue 
        elif str_input == "exit":
            running = False
        elif str_input == "help":
            help_msg()
        else:
            print "cmd not found"

    print 'program stopped'

if __name__=='__main__':
    TopoTest()


