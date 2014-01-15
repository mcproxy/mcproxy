#!/usr/bin/python
"""Custom topology example

Two directly connected switches plus a host for each switch:

   host --- switch --- switch --- host

Adding the 'topos' dict with a key/value pair to generate our newly defined
topology enables one to pass in '--topo=mytopo' from the command line.
"""
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

        # Add hosts and switches
        h1 = self.addHost( 'h1' )
        h2 = self.addHost( 'h2' )
        p1 = self.addHost( 'p1' )
        
        s1 =self.addSwitch('s1')
        s2 =self.addSwitch('s2')

        #self.addLink(host, switch, bw=10, delay='5ms', loss=10, max_queue_size=1000, use_htb=True)        # Add links
        #linkopts = dict(bw=10, delay='5ms')
        #RTT: 24ms
        #self.addLink( h1,h2,delay='2ms')
        self.addLink( h1,s1,delay='2ms')
        self.addLink(s1,p1,delay='2ms')# eth0 weil zuerst
        
        self.addLink( p1,s2,delay='2ms')
        self.addLink(s2,h2,delay='2ms')
        #self.addLink( s2,s3,delay='2ms',bw=1)



#dumbbel topology
class Example3( Topo ):
    def __init__( self,**opts):
        topoopts=dict(link=TCLink)
        Topo.__init__( self,**opts )
            
        # Add hosts and switches
        h1 = self.addHost( 'h1' )
        h2 = self.addHost( 'h2' )

        h3 = self.addHost( 'h3' )
        h4 = self.addHost( 'h4' )


        s1 = self.addSwitch( 's1' )
        s2 = self.addSwitch( 's2' )

        #self.addLink(host, switch, bw=10, delay='5ms', loss=10, max_queue_size=1000, use_htb=True)        # Add links
        #linkopts = dict(bw=10, delay='5ms')
        #RTT: 24ms
        self.addLink( h1,s1,delay='2ms')
        self.addLink( h3,s1,delay='2ms')

        self.addLink( s2,h2,delay='2ms')
        self.addLink( s2,h4,delay='2ms')
        self.addLink( s1,s2,delay='8ms', bw=2)



def TopoTest2():
    topo=Example2()	
    net = Mininet(topo=topo,controller = OVSController,link=TCLink)
    net.start()
    #print("trying to ping: ")
    #net.pingAll()
    print("staring the script...")
    h1 = net.get('h1')
    h2 = net.get('h2')
    p1 = net.get('p1')
    
    h1.setIP('192.168.0.1',24,'h1-eth0')
    h2.setIP('192.168.1.1',24,'h2-eth0')
    
    p1.setIP('192.168.0.2',24,'p1-eth0')
    p1.setIP('192.168.1.2',24,'p1-eth1')

    print "Server: "+h1.IP()+" Client: "+h2.IP()
    print "P1 IP: "+p1.IP()
    print p1.cmd('ifconfig')
    print p1.cmd('ping -c 2 192.168.0.1')
    print p1.cmd('ping -c 2 192.168.1.1')

    print h1.cmd('ping -c 2 192.168.0.2')
    print h2.cmd('ping -c 2 192.168.1.2')
    result="No command"		
#	result = h1.cmd('ifconfig')
    #result=h1.cmd('ping -c 4 '+h2.IP())
    print result
    result=h2.cmd('ifconfig')
    print ("h2 ifconfig: "+result)
    #result=h2.cmd('./mn_c.sh')
    print h2.cmd('echo 0 >/proc/sys/net/ipv4/conf/all/rp_filter')
    print h2.cmd('echo 0 >/proc/sys/net/ipv4/conf/h2-eth0/rp_filter')
    result=p1.cmd('xterm -e "/home/woelke/Desktop/tmp/mcproxy/mcproxy/mcproxy -sdvvf /home/woelke/Desktop/tmp/mcproxy/mcproxy/mcproxy.conf ; echo \\"mcproxy ende\\""&')
    sleep(2)
    #result=h2.cmd('/home/woelke/Desktop/tmp/mcproxy/mcproxy/mcproxy; sleep 10; ')
    result=h2.cmd('xterm -e "/home/woelke/Desktop/tmp/mcproxy/mcproxy/tester receiver /home/woelke/Desktop/tmp/mcproxy/debug/tester/tester.ini;sleep 10"&')
    #result=h2.cmd('~/dev/VideoStreamer/VideoStreamerCMake')
    #print result
    sleep(2)
    print("READY FOR TESTING")
    result= h1.cmd('/home/woelke/Desktop/tmp/mcproxy/mcproxy/tester sender /home/woelke/Desktop/tmp/mcproxy/debug/tester/tester.ini')
    print result
    sleep(2)
    print("Killing all applications")
    result=h2.cmd('killall tester')
    p1.cmd('killall mcproxy')
    print "Result from killing the client: "+result
    print("Stopping mininet")
    sleep(2)
    net.stop()

if __name__=='__main__':
	print("hallo")
	
	#Topo2Test()
	#Topo3Test()
	#TopoTest("topotest3",Example3(),["TW","SM","KO","TC","UH","G4"],["fs"])
	#TopoTest2("topotest2",["TW","SM","KO","TC","UH","G4"],["fs"])
	#TopoTest2("topotest2",["TW"],["fs","ns","r","s"])
	#TopoTest2("topotest2",["TW"],["fs"])
	
	TopoTest2()
	#Topo3Test("topotest3",["TW"],["fs"])
	print("Alle tests beendet")
