#!/usr/bin/python
"""Custom topology example"""

from time import sleep
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.util import dumpNodeConnections
from mininet.log import setLogLevel
from mininet.node import Controller
from mininet.node import OVSController

class Example2( Topo ):
    def __init__( self,**opts):
        topoopts=dict(link=TCLink)
        Topo.__init__( self,**opts )

        # Add hosts 
        mag1 = self.addHost('mag1')
        mag2 = self.addHost('mag2')
        lma1 = self.addHost('lma1')
        lma2 = self.addHost('lma2')
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        h3 = self.addHost('h3')
              
        # link hosts
        ##mag peering
        self.addLink(mag1, mag2) 

        ##lma interconection
        self.addLink(lma1, lma2)

        ##lmas to mas
        self.addLink(lma1, mag1)
        self.addLink(lma2, mag2)
        self.addLink(lma1, mag2)
        self.addLink(lma2, mag1)

        ##host to mags
        self.addLink(h1, mag1) 
        self.addLink(h3, mag2) 
        self.addLink(h2, mag1) 
        self.addLink(h2, mag2) 

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

def TopoTest():
    topo=Example2()	
    net = Mininet(topo=topo, controller = OVSController, link=TCLink)
    net.start()

    tester='../../mcproxy/tester'

    mag1 = net.get('mag1') 
    mag2 = net.get('mag2') 
    lma1 = net.get('lma1')
    lma2 = net.get('lma2')
    h1 = net.get('h1')
    h2 = net.get('h2')
    h3 = net.get('h3')

    #config lma1
    lma1.setIP(x('1.1'), 24, 'lma1-eth0')
    lma1.setIP(x('2.1'), 24, 'lma1-eth1')
    lma1.setIP(x('4.1'), 24, 'lma1-eth2')

    reset_rp_filter(lma1, ['all', 'lma1-eth0', 'lma1-eth1','lma1-eth2'])
    start_mcproxy(lma1, 'lma1.conf')

    #config lma2
    lma2.setIP(x('1.2'), 24, 'lma2-eth0')
    lma2.setIP(x('3.1'), 24, 'lma2-eth1')
    lma2.setIP(x('5.1'), 24, 'lma2-eth2')

    reset_rp_filter(lma2, ['all', 'lma2-eth0', 'lma2-eth1','lma2-eth2'])
    #start_mcproxy(lma1, 'lma2.conf')

    #config mag1
    mag1.setIP(x('0.1'), 24, 'mag1-eth0')
    mag1.setIP(x('2.2'), 24, 'mag1-eth1')
    mag1.setIP(x('5.2'), 24, 'mag1-eth2')
    mag1.setIP(x('10.1'), 24, 'mag1-eth3')
    mag1.setIP(x('11.1'), 24, 'mag1-eth4')

    reset_rp_filter(mag1, ['all', 'mag1-eth0', 'mag1-eth1','mag1-eth2', 'mag1-eth3', 'mag1-eth4'])
    start_mcproxy(mag1, 'mag1.conf')

    #config mag2
    mag2.setIP(x('0.2'), 24, 'mag2-eth0')
    mag2.setIP(x('3.2'), 24, 'mag2-eth1')
    mag2.setIP(x('4.2'), 24, 'mag2-eth2')
    mag2.setIP(x('13.1'), 24, 'mag2-eth3')
    mag2.setIP(x('12.1'), 24, 'mag2-eth4')

    reset_rp_filter(mag2, ['all', 'mag2-eth0', 'mag2-eth1','mag2-eth2', 'mag2-eth3', 'mag2-eth4'])
    start_mcproxy(mag2, 'mag2.conf')

    #config h1 
    h1.setIP(x('10.2'), 24, 'h1-eth0')
    reset_rp_filter(h1, ['all', 'h1-eth0'])

    #config h2 
    h2.setIP(x('11.2'), 24, 'h2-eth0')
    h2.setIP(x('12.2'), 24, 'h2-eth1')
    reset_rp_filter(h2, ['all', 'h2-eth0', 'h2-eth1'])

    #config h3 
    h3.setIP(x('13.2'), 24, 'h3-eth0')
    reset_rp_filter(h3, ['all', 'h3-eth0'])

    #print '##-- mag1 --##'
    #print mag1.cmd('ifconfig')
    #print '##-- mag2 --##'
    #print mag2.cmd('ifconfig')
    #print '##-- lma1 --##'
    #print lma1.cmd('ifconfig')
    #print '##-- mag2 --##'
    #print mag2.cmd('ifconfig')
    #print '##-- h3 --##'
    #print h3.cmd('ifconfig')
    #print '##-- ping --##'
    #ping(mag1,'0.2')
    #ping(lma1,'1.2')
    #ping(lma1,'2.2')
    #ping(lma1,'4.2')
    #ping(lma2,'5.2')
    #ping(lma2,'3.2')
    #ping(h1,'10.1')
    #ping(h2,'11.1')
    #ping(h2,'12.1')
    #ping(h3,'13.1')
    sleep(10000000)

if __name__=='__main__':
    #TwoHostsTest()
    TopoTest()
