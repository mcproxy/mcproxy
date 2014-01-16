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

class TwoHosts( Topo ):
    def __init__( self,**opts):
        topoopts=dict(link=TCLink)
        Topo.__init__( self,**opts )

        # Add hosts and switches
        s = self.addHost( 's' )
        r = self.addHost( 'r' )

        ##host to mags
        self.addLink(r, s, delay='10ms', jitter='5ms') 

def TwoHostsTest():
    topo=TwoHosts()	
    net = Mininet(topo=topo, controller = OVSController, link=TCLink)
    net.start()

    print '##-- TwoHosts --##'
    s = net.get('s')
    r = net.get('r')

    print 'sender'
    print s.cmd('ifconfig') 

    print 'receiver'
    print r.cmd('ifconfig') 

    tester='/home/woelke/Desktop/tmp/mcproxy/mcproxy/tester'
    ini='/home/woelke/Desktop/tmp/mcproxy/debug/mininet/tester_twohosts.ini'

    r.cmd('xterm -e "' + tester + ' recv ' + ini +'; echo receiver ende; sleep 20s"&')
    #s.cmd('xterm -e "' + tester + ' send ' + ini +'; echo sender ende; sleep 20s"&')

    print '##-- End --##'
    sleep(20)


class Example2( Topo ):
    def __init__( self,**opts):
        topoopts=dict(link=TCLink)
        Topo.__init__( self,**opts )

        # Add hosts and switches
        #h1 = self.addHost( 'h1' )
        #h2 = self.addHost( 'h2' )
        #p1 = self.addHost( 'p1' )
        
        #s1 =self.addSwitch('s1')
        #s2 =self.addSwitch('s2')

        ##self.addLink(host, switch, bw=10, delay='5ms', loss=10, max_queue_size=1000, use_htb=True)        # Add links
        ##linkopts = dict(bw=10, delay='5ms')
        ##RTT: 24ms
        ##self.addLink( h1,h2,delay='2ms')
        #self.addLink( h1,s1,delay='2ms')
        #self.addLink(s1,p1,delay='2ms')# eth0 weil zuerst
        
        #self.addLink( p1,s2,delay='2ms')
        #self.addLink(s2,h2,delay='2ms')
        ##self.addLink( s2,s3,delay='2ms',bw=1)

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

def TopoTest():
    topo=Example2()	
    net = Mininet(topo=topo, controller = OVSController, link=TCLink)
    net.start()

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

    #config lma2
    lma2.setIP(x('1.2'), 24, 'lma2-eth0')
    lma2.setIP(x('3.1'), 24, 'lma2-eth1')
    lma2.setIP(x('5.1'), 24, 'lma2-eth2')

    reset_rp_filter(lma2, ['all', 'lma2-eth0', 'lma2-eth1','lma2-eth2'])

    #config mag1
    mag1.setIP(x('0.1'), 24, 'mag1-eth0')
    mag1.setIP(x('2.2'), 24, 'mag1-eth1')
    mag1.setIP(x('5.2'), 24, 'mag1-eth2')
    mag1.setIP(x('10.1'), 24, 'mag1-eth3')
    mag1.setIP(x('11.1'), 24, 'mag1-eth4')

    reset_rp_filter(mag1, ['all', 'mag1-eth0', 'mag1-eth1','mag1-eth2', 'mag1-eth3', 'mag1-eth4'])

    #config mag2
    mag2.setIP(x('0.2'), 24, 'mag2-eth0')
    mag2.setIP(x('3.2'), 24, 'mag2-eth1')
    mag2.setIP(x('4.2'), 24, 'mag2-eth2')
    mag2.setIP(x('13.1'), 24, 'mag2-eth3')
    mag2.setIP(x('12.1'), 24, 'mag2-eth4')

    reset_rp_filter(mag2, ['all', 'mag2-eth0', 'mag2-eth1','mag2-eth2', 'mag2-eth3', 'mag2-eth4'])

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
    ping(mag1,'0.2')
    ping(lma1,'1.2')
    ping(lma1,'2.2')
    ping(lma1,'4.2')
    ping(lma2,'5.2')
    ping(lma2,'3.2')
    ping(h1,'10.1')
    ping(h2,'11.1')
    ping(h2,'12.1')
    ping(h3,'13.1')

#def TopoTest2():
    #topo=Example2()	
    #net = Mininet(topo=topo,controller = OVSController,link=TCLink)
    #net.start()
    ##print("trying to ping: ")
    ##net.pingAll()
    #print("staring the script...")
    #h1 = net.get('h1')
    #h2 = net.get('h2')
    #p1 = net.get('p1')
    
    #h1.setIP('192.168.0.1',24,'h1-eth0')
    #h2.setIP('192.168.1.1',24,'h2-eth0')
    
    #p1.setIP('192.168.0.2',24,'p1-eth0')
    #p1.setIP('192.168.1.2',24,'p1-eth1')

    #print "Server: "+h1.IP()+" Client: "+h2.IP()
    #print "P1 IP: "+p1.IP()
    #print p1.cmd('ifconfig')
    #print p1.cmd('ping -c 2 192.168.0.1')
    #print p1.cmd('ping -c 2 192.168.1.1')

    #print h1.cmd('ping -c 2 192.168.0.2')
    #print h2.cmd('ping -c 2 192.168.1.2')
    #result="No command"		
##	result = h1.cmd('ifconfig')
    ##result=h1.cmd('ping -c 4 '+h2.IP())
    #print result
    #result=h2.cmd('ifconfig')
    #print ("h2 ifconfig: "+result)
    ##result=h2.cmd('./mn_c.sh')
    #print h2.cmd('echo 0 >/proc/sys/net/ipv4/conf/all/rp_filter')
    #print h2.cmd('echo 0 >/proc/sys/net/ipv4/conf/h2-eth0/rp_filter')
    #result=p1.cmd('xterm -e "/home/woelke/Desktop/tmp/mcproxy/mcproxy/mcproxy -sdvvf /home/woelke/Desktop/tmp/mcproxy/mcproxy/mcproxy.conf ; echo \\"mcproxy ende\\""&')
    #sleep(2)
    ##result=h2.cmd('/home/woelke/Desktop/tmp/mcproxy/mcproxy/mcproxy; sleep 10; ')
    #result=h2.cmd('xterm -e "/home/woelke/Desktop/tmp/mcproxy/mcproxy/tester receiver /home/woelke/Desktop/tmp/mcproxy/debug/tester/tester.ini;sleep 10"&')
    ##result=h2.cmd('~/dev/VideoStreamer/VideoStreamerCMake')
    ##print result
    #sleep(2)
    #print("READY FOR TESTING")
    #result= h1.cmd('/home/woelke/Desktop/tmp/mcproxy/mcproxy/tester sender /home/woelke/Desktop/tmp/mcproxy/debug/tester/tester.ini')
    #print result
    #sleep(2)
    #print("Killing all applications")
    #result=h2.cmd('killall tester')
    #p1.cmd('killall mcproxy')
    #print "Result from killing the client: "+result
    #print("Stopping mininet")
    #sleep(2)
    #net.stop()

if __name__=='__main__':
    TwoHostsTest()
    #TopoTest()
