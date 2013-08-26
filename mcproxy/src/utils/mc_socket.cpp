/*
 * This file is part of mcproxy.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */


#include "include/hamcast_logging.h"
#include "include/utils/mc_socket.hpp"

#include <boost/lexical_cast.hpp>
#include <netpacket/packet.h>
#include <cstring> //memset
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>


using namespace std;

string ipAddrResolver(string ipAddr)
{
    string str[][2] = {
        {IPV4_IGMPV3_ADDR, "IPV4_IGMPV3_ADDR"},
        {IPV4_ALL_HOST_ADDR, "IPV4_ALL_HOST_ADDR"},
        {IPV4_ALL_IGMP_ROUTERS_ADDR, "IPV4_ALL_ROUTERS_ADDR"},
        {IPV4_PIMv2_ADDR, "IPV4_PIMv2_ADDR"},
        {IPV4_MCAST_DNS_ADDR, "IPV4_MCAST_DNS_ADDR"},
        {IPV6_ALL_MLDv2_CAPABLE_ROUTERS, "IPV6_ALL_MLDv2_CAPABLE_ROUTERS"},
        {IPV6_ALL_NODES_ADDR, "IPV6_ALL_NODES_ADDR"},
        {IPV6_ALL_LINK_LOCAL_ROUTER, "IPV6_ALL_LINK_LOCAL_ROUTER"},
        {IPV6_ALL_SITE_LOCAL_ROUTER, "IPV6_ALL_SITE_LOCAL_ROUTER"},
        {IPV6_ALL_PIM_ROUTERS, "IPV6_ALL_PIM_ROUTERS"}
    };

    unsigned int nCount = 9;

    for (unsigned int i = 0; i < nCount; i++) {
        if (ipAddr.compare(str[i][0]) == 0) {
            return str[i][1];
        }
    }

    return string();
}

int family_to_level(int family)
{
    switch (family) {
    case AF_INET:
        return IPPROTO_IP;
    case AF_INET6:
        return IPPROTO_IPV6;
    default:
        return -1;
    }
}

mc_socket::mc_socket() :
    m_sock(0), m_addrFamily(-1), m_own_socket(true)
{
    HC_LOG_TRACE("");
}

bool mc_socket::create_udp_ipv4_socket()
{
    HC_LOG_TRACE("");

    if (is_udp_valid()) {
        close(m_sock);
    }

    //          IP-Protokollv4, UDP,    Protokoll
    m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP); //SOCK_DGRAM //IPPROTO_IP
    if (m_sock < 0) {
        HC_LOG_ERROR("failed to create! Error: " << strerror(errno) << " errno: " << errno);
        return false; // failed
    } else {
        HC_LOG_DEBUG("get socket discriptor number: " << m_sock);
        m_addrFamily = AF_INET;
        m_own_socket = true;
        return true;
    }

}

bool mc_socket::create_udp_ipv6_socket()
{
    HC_LOG_TRACE("");

    if (is_udp_valid()) {
        close(m_sock);
    }

    //          IP-Protokollv6, UDP,    Protokoll
    m_sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_IP); //SOCK_DGRAM //IPPROTO_IP
    if (m_sock < 0) {
        HC_LOG_ERROR("failed to create! Error: " << strerror(errno) << " errno: " << errno);
        return false; // failed
    } else {
        HC_LOG_DEBUG("get socket discriptor number: " << m_sock);
        m_addrFamily = AF_INET6;
        m_own_socket = true;
        return true;
    }
}

bool mc_socket::set_own_socket(int sck, int addr_family)
{
    HC_LOG_TRACE("");

    if (is_udp_valid()) {
        close(m_sock);
    }

    if (sck < 0) {
        HC_LOG_ERROR("wrong socket discriptor! socket: " << sck);
        return false; // failed
    } else {
        if (addr_family == AF_INET || addr_family == AF_INET6) {
            m_sock = sck;
            m_addrFamily = addr_family;
            m_own_socket = false;
        } else {
            HC_LOG_ERROR("wrong address family: " << addr_family);
            return false; // failed
        }
        return true;
    }
}

int mc_socket::get_addr_family()
{
    return m_addrFamily;
}

bool mc_socket::bind_udp_socket(int port)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    //struct sockaddr_storage tmp;
    struct sockaddr* m_addr;
    struct sockaddr_in m_addr_v4;
    struct sockaddr_in6 m_addr_v6;
    int size;
    int rc;

    if (m_addrFamily == AF_INET) {
        m_addr_v4.sin_family = AF_INET;
        m_addr_v4.sin_addr.s_addr = INADDR_ANY;
        m_addr_v4.sin_port = htons(port);
        m_addr = (sockaddr*) &m_addr_v4;
        size = sizeof(m_addr_v4);
    } else if (m_addrFamily == AF_INET6) {
        m_addr_v6.sin6_family = AF_INET6;
        m_addr_v6.sin6_flowinfo = 0;
        m_addr_v6.sin6_port =  htons(port);
        m_addr_v6.sin6_addr = in6addr_any;
        m_addr = (sockaddr*) &m_addr_v6;
        size = sizeof(m_addr_v6);
    } else {
        HC_LOG_ERROR("Unknown Errno");
        return false;
    }

    rc = bind(m_sock, m_addr, size);
    if (rc == -1) {
        HC_LOG_ERROR("failed to bind! Error: " << strerror(errno) << " errno: " << errno);
        return false;
    } else {
        HC_LOG_DEBUG("bind to port: " << port);
        return true;
    }
}

bool mc_socket::set_loop_back(bool enable)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    int rc;
    int loopArg;
    int level;

    //u_char loop;
    int loop;
    if (enable == true) {
        loop = 1;
    } else {
        loop = 0;
    }

    if (m_addrFamily == AF_INET) {
        level = IPPROTO_IP;
        loopArg = IP_MULTICAST_LOOP;
    } else if (m_addrFamily == AF_INET6) {
        level = IPPROTO_IPV6;
        loopArg = IPV6_MULTICAST_LOOP;
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    rc = setsockopt(m_sock, level, loopArg, &loop, sizeof(loop));

    if (rc == -1) {
        HC_LOG_ERROR("failed to setLoopBack(on/off)! Error: " << strerror(errno) << " errno: " << errno);
        return false;
    } else {
        return true;
    }
}

bool mc_socket::send_packet(const addr_storage& addr, string data)
{
    return send_packet(addr, reinterpret_cast<const unsigned char*>(data.c_str()), data.size());
}

bool mc_socket::send_packet(const addr_storage& addr, const unsigned char* data, unsigned int data_size)
{
    HC_LOG_TRACE("addr: " << addr << " port: " << addr.get_port() << " data_size: "<< data_size);

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    int rc = 0;

    rc = sendto(m_sock, data, data_size, 0, &addr.get_sockaddr() ,addr.get_addr_len());

    if (rc == -1) {
        HC_LOG_ERROR("failed to send! Error: " << strerror(errno)  << " errno: " << errno);
        return false; //failed to send
    } else {
        return true;
    }
}

bool mc_socket::receive_packet(unsigned char* buf, int sizeOfBuf, int& sizeOfInfo)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    int rc;
    rc = recv(m_sock, buf, sizeOfBuf, 0);
    sizeOfInfo = rc;
    if (rc == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            sizeOfInfo = 0;
            return true;
        } else {
            HC_LOG_ERROR("failed to receive Error: " << strerror(errno)  << " errno: " << errno);
            return false;
        }
    } else {
        return true;
    }
}

bool mc_socket::receive_msg(struct msghdr* msg, int& sizeOfInfo)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    int rc;
    rc = recvmsg(m_sock, msg, 0);
    sizeOfInfo = rc;
    if (rc == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            sizeOfInfo = 0;
            return true;
        } else {
            HC_LOG_ERROR("failed to receive msg Error: " << strerror(errno)  << " errno: " << errno);
            return false;
        }
    } else {
        return true;
    }

    //example
    //     //########################
    //     //create msg
    //     //msg_name
    //     struct sockaddr_in6 recv_addr;
    //     recv_addr.sin6_family = AF_INET6;
    //     recv_addr.sin6_addr = in6addr_any;
    //     recv_addr.sin6_flowinfo= 0;
    //     recv_addr.sin6_port = 0;
    //     recv_addr.sin6_scope_id = 2;

    //     //iov
    //     unsigned char buf[400];
    //     struct iovec iov;
    //     iov.iov_base = buf;
    //     iov.iov_len = sizeof(buf);

    //     //control
    //     unsigned char ctrl[400];

    //     //create msghdr
    //     struct msghdr msg;
    //     msg.msg_name = &recv_addr;
    //     msg.msg_namelen = sizeof(struct sockaddr_in6);

    //     msg.msg_iov = &iov;
    //     msg.msg_iovlen = 1;

    //     msg.msg_control = ctrl;
    //     msg.msg_controllen = sizeof(ctrl);

    //     msg.msg_flags = 0;
    //     //########################

    //     //iterate
    //     struct cmsghdr* cmsgptr;

    //     for (cmsgptr = CMSG_FIRSTHDR(&msg); cmsgptr != NULL; cmsgptr = CMSG_NXTHDR(&msg, cmsgptr)) {
    //          if (cmsgptr->cmsg_len == 0) {
    //               cout << "hier fehler" << endl;
    //               /* Error handling */
    //               break;
    //          }
    //          cout << "\tinhalt ..." << endl;
    //               if (cmsgptr->cmsg_level == ... && cmsgptr->cmsg_type == ... ) {
    //                    u_char *ptr;
    //                    ptr = CMSG_DATA(cmsgptr);
    //                    /* process data pointed to by ptr */
    //               }
    //     }
    //     //#######################
}

bool mc_socket::set_receive_timeout(long msec)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    struct timeval t;
    t.tv_sec = msec / 1000;
    t.tv_usec = 1000 * (msec % 1000);;

    int rc = setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

    if (rc == -1) {
        HC_LOG_ERROR("failed to set timeout! Error: " << strerror(errno)  << " errno: " << errno);
        return false;
    } else {
        return true;
    }
}

bool mc_socket::choose_if(int if_index)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        struct in_addr inaddr;
        struct ifreq ifreq;

        if ( if_index > 0) {
            if (if_indextoname(if_index, ifreq.ifr_name) == nullptr) {
                HC_LOG_ERROR("failed to get interface name! if_index:" << if_index << "! Error: " << strerror(
                                 errno)  << " errno: " << errno);
                return false;
            }

            if (ioctl(m_sock, SIOCGIFADDR, &ifreq) < 0) {
                HC_LOG_ERROR("failed to get interface address! if_name: " << ifreq.ifr_name);
                return false;
            }

            memcpy(&inaddr, &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr, sizeof(struct in_addr));
        } else {
            inaddr.s_addr = htonl(INADDR_ANY);
        }

        int rc = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_IF, &inaddr, sizeof(struct in_addr));

        if (rc == -1) {
            HC_LOG_ERROR("failed to choose_if! Error: " << strerror(errno)  << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else if (m_addrFamily == AF_INET6) {
        int rc = setsockopt(m_sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &if_index, sizeof(if_index));

        if (rc == -1) {
            HC_LOG_ERROR("failed to choose_if! Error: " << strerror(errno)  << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mc_socket::set_ttl(int ttl)
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        rc = setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    } else if (m_addrFamily == AF_INET6) {
        rc = setsockopt(m_sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl));
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    if (rc == -1) {
        HC_LOG_ERROR("failed to set ttl: " << ttl << "! Error: " << strerror(errno));
        return false;
    } else {
        return true;
    }
}

bool mc_socket::join_group(const addr_storage& gaddr, int if_index)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " if_index: " << if_index);

    return generic_group_sockopt(gaddr, if_index, MCAST_JOIN_GROUP);
}

bool mc_socket::leave_group(const addr_storage& gaddr, int if_index)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " if_index: " << if_index);

    return generic_group_sockopt(gaddr, if_index, MCAST_LEAVE_GROUP);
}

bool mc_socket::block_source(const addr_storage& gaddr, const addr_storage& saddr, int if_index)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " saddr: " << saddr << " if_index: " << if_index << " optname: MCAST_BLOCK_SOURCE(" << MCAST_BLOCK_SOURCE << ")");

    return generic_source_sockopt(gaddr, saddr, if_index, MCAST_BLOCK_SOURCE);
}


bool mc_socket::unblock_source(const addr_storage& gaddr, const addr_storage& saddr, int if_index)
{
    HC_LOG_TRACE("gaddr: " << gaddr <<  " saddr: " << saddr << " if_index: " << if_index << " optname: MCAST_UNBLOCK_SOURCE(" << MCAST_UNBLOCK_SOURCE << ")");

    return generic_source_sockopt(gaddr, saddr, if_index, MCAST_UNBLOCK_SOURCE);
}


bool mc_socket::join_source_group(const addr_storage& gaddr, const addr_storage& saddr, int if_index)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " saddr: " << saddr << " if_index: " << if_index << " optname: MCAST_JOIN_SOURCE_GROUP(" << MCAST_JOIN_SOURCE_GROUP << ")");

    return generic_source_sockopt(gaddr, saddr, if_index, MCAST_JOIN_SOURCE_GROUP);
}

bool mc_socket::leave_source_group(const addr_storage& gaddr, const addr_storage& saddr, int if_index)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " saddr: " << saddr << " if_index: " << if_index << " optname: MCAST_LEAVE_SOURCE_GROUP(" << MCAST_LEAVE_SOURCE_GROUP << ")");

    return generic_source_sockopt(gaddr, saddr, if_index, MCAST_LEAVE_SOURCE_GROUP);
}


bool mc_socket::generic_group_sockopt(const addr_storage& gaddr, int if_index, int optname)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " if_index: " << if_index << " optname: " << optname);

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    } else {
        HC_LOG_DEBUG("use socket discriptor number: " << m_sock);
    }

    struct group_req req;
    int rc = 0;

    req.gr_group = gaddr.get_sockaddr_storage();
    req.gr_interface = if_index;

    rc = setsockopt (m_sock, family_to_level(gaddr.get_addr_family()), optname, &req, sizeof(req));

    if (rc == -1) {
        HC_LOG_ERROR("failed to set socket option! Error: " << strerror(errno) << " errno: " << errno);
        return false;
    } else {
        return true;
    }

}

bool mc_socket::generic_source_sockopt(const addr_storage& gaddr, const addr_storage& saddr, int if_index, int optname)
{
    HC_LOG_TRACE("gaddr: " << gaddr << " saddr: " << saddr << " if_index: " << if_index << " optname: " << optname);

    if (!is_udp_valid()) {
        HC_LOG_ERROR("udp_socket invalid");
        return false;
    } else {
        HC_LOG_DEBUG("use socket discriptor number: " << m_sock);
    }
    struct group_source_req req;
    int rc = 0;

    req.gsr_group = gaddr.get_sockaddr_storage();
    req.gsr_source = saddr.get_sockaddr_storage();
    req.gsr_interface = if_index;

    rc = setsockopt (m_sock, family_to_level(gaddr.get_addr_family()), optname, &req, sizeof(req));

    if (rc == -1) {
        HC_LOG_ERROR("failed to set socket option! Error: " << strerror(errno) << " errno: " << errno);
        return false;
    } else {
        return true;
    }

}
void mc_socket::test_mc_goup_functions(string ipversion, string msg, string interface, string gaddr, int port)
{
    HC_LOG_TRACE("");
    cout << "##-- Test multicast group managment functions --##" << endl;
    mc_socket m;
    int count = 0;
    int sleepTime = 1;

    cout << "--<" << count++ << "> Create an udp " << ipversion << " socket --" << endl;
    if (ipversion.compare("AF_INET") == 0) {
        if (m.create_udp_ipv4_socket()) {
            cout << "create socket OK!" << endl;
        } else {
            cout << "cerate socket FAILED!" << endl;
        }
    } else if (ipversion.compare("AF_INET6") == 0) {
        if (m.create_udp_ipv6_socket()) {
            cout << "create socket OK!" << endl;
        } else {
            cout << "cerate socket FAILED!" << endl;
        }
    } else {
        cout << "Unknown ip version: " << ipversion << endl;
        return;
    }

    cout << "--<" << count++ << "> Join and leave --" << endl;
    if (m.join_group(addr_storage(gaddr), if_nametoindex(interface.c_str()))) {
        cout << "join OK!" << endl;
    } else {
        cout << "join FAILED!" << endl;
    }
    sleep(sleepTime);
    if (m.leave_group(addr_storage(gaddr), if_nametoindex(interface.c_str()))) {
        cout << "leave OK!" << endl;
    } else {
        cout << "leave FAILED!" << endl;
    }
    sleep(sleepTime);

    cout << "--<" << count++ << "> Send Data --" << endl;
    if (m.choose_if(if_nametoindex(interface.c_str()))) {
        cout << "choose if (" << interface << ") OK! " << endl;
    } else {
        cout << "choose if (" << interface << ") FAILED! " << endl;
    }

    if (m.send_packet(addr_storage(gaddr).set_port(port),  msg)) {
        cout << "send OK! " << msg << " at addr:" << gaddr << " with port " << port << endl;
    } else {
        cout << "send FAILED!" << endl;
    }

}


void mc_socket::test_mc_source_functions(string ipversion, string interface, string gaddr, string saddr_a, string saddr_b)
{
    HC_LOG_TRACE("");

    cout << "##-- Test multicast source managment functions --##" << endl;
    mc_socket m;
    int count = 0;
    int sleepTime = 4;

    cout << "--<" << count++ << "> Create an udp " << ipversion << " socket --" << endl;
    if (ipversion.compare("AF_INET") == 0) {
        if (m.create_udp_ipv4_socket()) {
            cout << "create socket OK!" << endl;
        } else {
            cout << "cerate socket FAILED!" << endl;
        }
    } else if (ipversion.compare("AF_INET6") == 0) {
        if (m.create_udp_ipv6_socket()) {
            cout << "create socket OK!" << endl;
        } else {
            cout << "cerate socket FAILED!" << endl;
        }
    } else {
        cout << "Unknown ip version: " << ipversion << endl;
        return;
    }

    cout << "--<" << count++ << "> Join group " << gaddr << " --" << endl;
    if (m.join_group(addr_storage(gaddr), if_nametoindex(interface.c_str()))) {
        cout << "join OK!" << endl;
    } else {
        cout << "join FAILED!" << endl;
    }

    sleep(sleepTime);

    cout << "--<" << count++ << "> Block source " << saddr_a << " --" << endl;
    if (m.block_source(addr_storage(gaddr), addr_storage(saddr_a), if_nametoindex(interface.c_str()))) {
        cout << "block OK!" << endl;
    } else {
        cout << "block FAILED!" << endl;
    }

    sleep(sleepTime);

    cout << "--<" << count++ << "> Block source " << saddr_b << " --" << endl;
    if (m.block_source(addr_storage(gaddr), addr_storage(saddr_b), if_nametoindex(interface.c_str()))) {
        cout << "block OK!" << endl;
    } else {
        cout << "block FAILED!" << endl;
    }

    sleep(sleepTime);
    
    cout << "--<" << count++ << "> Unblock source " << saddr_a << " --" << endl;
    if (m.unblock_source(addr_storage(gaddr), addr_storage(saddr_a), if_nametoindex(interface.c_str()))) {
        cout << "unblock OK!" << endl;
    } else {
        cout << "unblock FAILED!" << endl;
    }
    sleep(sleepTime);
    
    cout << "--<" << count++ << "> Join source " << saddr_b << " --" << endl;
    if (m.join_source_group(addr_storage(gaddr), addr_storage(saddr_b), if_nametoindex(interface.c_str()))) {
        cout << "join OK!" << endl;
    } else {
        cout << "join FAILED!" << endl;
    }
    sleep(sleepTime);

    cout << "--<" << count++ << "> Leave source " << saddr_b << " --" << endl;
    if (m.leave_source_group(addr_storage(gaddr), addr_storage(saddr_b), if_nametoindex(interface.c_str()))) {
        cout << "leave OK!" << endl;
    } else {
        cout << "leave FAILED!" << endl;
    }

    sleep(sleepTime);
}

void mc_socket::test_all(){
    HC_LOG_TRACE("");

    addr_storage gaddr_v4("239.99.99.99");
    addr_storage gaddr_v6("FF02::99:99:99:99");
    addr_storage saddr_v4_a("141.22.0.1");
    addr_storage saddr_v4_b("141.22.0.2");
    addr_storage saddr_v6_a("FE80:5E26::2");
    addr_storage saddr_v6_b("FE80:5E26::3");
    string if_name("dummy0");
    int port = 9845; 

    mc_socket::test_mc_goup_functions("AF_INET", "Hallo", if_name, gaddr_v4.to_string(), port);
    mc_socket::test_mc_goup_functions("AF_INET6", "Hallo", if_name, gaddr_v6.to_string(),port);
    mc_socket::test_mc_source_functions("AF_INET", if_name, gaddr_v4.to_string(), saddr_v4_a.to_string(), saddr_v4_b.to_string());
    mc_socket::test_mc_source_functions("AF_INET6", if_name, gaddr_v6.to_string(), saddr_v6_a.to_string(), saddr_v6_b.to_string());
}
mc_socket::~mc_socket()
{
    HC_LOG_TRACE("");

    if (is_udp_valid() && m_own_socket) {
        close(m_sock);
    }
}
