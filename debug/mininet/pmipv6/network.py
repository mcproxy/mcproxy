#!/usr/bin/python

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
        lma = self.addHost('lma')
        mag1 = self.addHost('mag1')
        mag2 = self.addHost('mag2')
        host1 = self.addHost('host1')
        host2 = self.addHost('host2')

        ##lma to mags
        self.addLink(lma, mag1)
        self.addLink(lma, mag2)

        ##host to mags
        self.addLink(host1, mag1) 
        self.addLink(host2, mag2)

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
    host.cmd('xterm -e "' + mcproxy + ' -sdvv -f ' + config_file + '; sleep 2"&')

def set_interface_delay(action, host, interface, delay): #action: add/change/delete
    print host.cmd('tc qdisc ' + action + ' dev ' + interface + ' root handle 1: netem delay ' + delay)

def killall(host):
    host.cmd('killall mcproxy')    
    host.cmd('killall tester')

def set_interface_delays(lma, mag1, mag2, host1, host2):
    #action = 'add' or 'change'

    lma_mag_delay =  '20ms 5ms' #delay, jitter
    mag_host_delay =  '30ms 10ms' #delay, jitter

    set_interface_delay('add', lma, 'lma-eth0', lma_mag_delay)
    set_interface_delay('add', lma, 'lma-eth1', lma_mag_delay)

    set_interface_delay('add', mag1, 'mag1-eth0', lma_mag_delay)
    set_interface_delay('add', mag1, 'mag1-eth1', mag_host_delay)

    set_interface_delay('add', mag2, 'mag2-eth0', lma_mag_delay)
    set_interface_delay('add', mag2, 'mag2-eth1', mag_host_delay)

    set_interface_delay('add', host1, 'host1-eth0', mag_host_delay)
    set_interface_delay('add', host2, 'host2-eth0', mag_host_delay)

def TopoTest():
    topo=Example()	
    net = Mininet(topo=topo, controller = OVSController, link=TCLink)
    net.start()
    
    mag1 = net.get('mag1') 
    mag2 = net.get('mag2') 
    lma = net.get('lma')
    host1 = net.get('host1')
    host2 = net.get('host2')

    #config lma
    lma.setIP(x('1.1'), 24, 'lma-eth0')
    lma.setIP(x('2.1'), 24, 'lma-eth1')

    reset_rp_filter(lma, ['all', 'lma-eth0', 'lma-eth1'])
    start_mcproxy(lma, 'lma.conf')

    #config mag1
    mag1.setIP(x('1.2'), 24, 'mag1-eth0')
    mag1.setIP(x('10.1'), 24, 'mag1-eth1')

    reset_rp_filter(mag1, ['all', 'mag1-eth0', 'mag1-eth1'])
    start_mcproxy(mag1, 'mag1.conf')

    #config mag2
    mag2.setIP(x('2.2'), 24, 'mag2-eth0')
    mag2.setIP(x('11.1'), 24, 'mag2-eth1')

    reset_rp_filter(mag2, ['all', 'mag2-eth0', 'mag2-eth1'])
    start_mcproxy(mag2, 'mag2.conf')

    #config host1 
    host1.setIP(x('10.2'), 24, 'host1-eth0')
    reset_rp_filter(host1, ['all', 'host1-eth0'])

    #config h2 
    host2.setIP(x('11.2'), 24, 'host2-eth0')
    reset_rp_filter(host2, ['all', 'host2-eth0'])

    #delays
    set_interface_delays(lma, mag1, mag2, host1, host2)

    #run programms
    ##################################################
    tester='../../../mcproxy/tester'

    host2.cmd('xterm -e "' + tester + ' h2_recv; sleep 2"&')
    host1.cmd('xterm -e "' + tester + ' h1_send; sleep 2"&')
    
    sleep(300)

    killall(lma)    
    killall(mag1)    
    killall(mag2)    
    killall(host1)    
    killall(host2)    
    print 'all killed'

if __name__=='__main__':
    TopoTest()


