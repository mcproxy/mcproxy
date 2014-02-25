#!/usr/bin/python

import random
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

        self.addLink(proxy, host1)
        self.addLink(proxy, host2)
        self.addLink(proxy, host3)
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
    host.cmd('xterm -e "' + mcproxy + ' -sdvv -f ' + config_file + '"&')

def set_interface_delay(action, host, interface, delay): #action: add/change/delete
    print host.cmd('tc qdisc ' + action + ' dev ' + interface + ' root handle 1: netem delay ' + delay)

def killall(host):
    host.cmd('killall mcproxy')    
    host.cmd('killall tester')

def get_random_delay_value(delay_from, delay_to):
    return str(random.randint(delay_from, delay_to)) + 'ms'

def set_random_interface_delays(action, proxy, host1, host2, host3, host4):
    #action = 'add' or 'change'

    proxy_host_delay=  '20ms 5ms' #delay, jitter

    set_interface_delay(action, proxy, 'proxy-eth0', proxy_host_delay)
    set_interface_delay(action, proxy, 'proxy-eth1', proxy_host_delay)
    set_interface_delay(action, proxy, 'proxy-eth2', proxy_host_delay)
    set_interface_delay(action, proxy, 'proxy-eth3', proxy_host_delay)

    set_interface_delay(action, host1, 'host1-eth0', proxy_host_delay)
    set_interface_delay(action, host2, 'host2-eth0', proxy_host_delay)
    set_interface_delay(action, host3, 'host3-eth0', proxy_host_delay)
    set_interface_delay(action, host4, 'host4-eth0', proxy_host_delay)

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
    proxy.setIP(x('3.1'), 24, 'proxy-eth3')

    reset_rp_filter(proxy, ['all', 'proxy-eth0', 'proxy-eth1', 'proxy-eth2', 'proxy-eth3'])

    #configure hosts
    host1.setIP(x('0.2'), 24, 'host1-eth0')
    host2.setIP(x('1.2'), 24, 'host2-eth0')
    host3.setIP(x('2.2'), 24, 'host3-eth0')
    host4.setIP(x('3.2'), 24, 'host4-eth0')

    reset_rp_filter(host1, ['all', 'host1-eth0'])
    reset_rp_filter(host2, ['all', 'host2-eth0'])
    reset_rp_filter(host3, ['all', 'host3-eth0'])
    reset_rp_filter(host4, ['all', 'host4-eth0'])

    #delays
    set_random_interface_delays('add', proxy, host1, host2, host3, host4)

    #run programms
    ##################################################
    start_mcproxy(proxy, 'proxy.conf')

    tester='../../../mcproxy/tester'

    host1.cmd('xterm -e "' + tester + ' h1_recv"&')
    host2.cmd('xterm -e "' + tester + ' h2_recv"&')
    host3.cmd('xterm -e "' + tester + ' h3_send"&')
    #host4.cmd('xterm -e "' + tester + ' h4_send"&')
    
    sleep(300)

    killall(proxy)    
    killall(host1)    
    killall(host2)    
    killall(host3)    
    killall(host4)    
    print 'all killed'

if __name__=='__main__':
    TopoTest()


