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


#ifndef MCSOCKET_H_
#define MCSOCKET_H_

#include "include/utils/addr_storage.hpp"
#include <time.h>
#include <string>
using namespace std;

///@author Sebastian Woelke
///@brief socket for multicast applications

/*
 * ##--Multicast Addresses--##
 * 224.0.0.0 - 224.255.255.255 ==> für Routingprotokolle reserviert
 * 239.0.0.0 - 239.255.255.255 ==> für scoping reserviert
 * 225.x.x.x bis 238.x.x.x     ==> frei verfügbar
 */

//IPv4 addresses that are reserved for IP multicasting (http://en.wikipedia.org/wiki/Multicast_address)
#define IPV4_ALL_HOST_ADDR                  "224.0.0.1"     //The All Hosts multicast group that contains all systems on the same network segment
#define IPV4_ALL_IGMP_ROUTERS_ADDR                "224.0.0.2"   //The All Routers multicast group that contains all routers on the same network segment
#define IPV4_ALL_SPF_ROUTER_ADDR                  "224.0.0.5"   //The Open Shortest Path First (OSPF) AllSPFRouters address. Used to send Hello packets to all OSPF routers on a network segment
#define IPV4_ALL_D_Routers_ADDR                   "224.0.0.6"   //The OSPF AllDRouters address. Used to send OSPF routing information to OSPF designated routers on a network segment
#define IPV4_RIPV2_ADDR                     "224.0.0.9" //The RIP version 2 group address. Used to send routing information using the RIP protocol to all RIP v2-aware routers on a network segment
#define IPV4_EIGRP_ADDR                     "224.0.0.10"    //EIGRP group address. Used to send EIGRP routing information to all EIGRP routers on a network segment
#define IPV4_PIMv2_ADDR                     "224.0.0.13"    //PIM Version 2 (Protocol Independent Multicast)
#define IPV4_VRR_ADDR                       "224.0.0.18"    //Virtual Router Redundancy Protocol
#define IPV4_IS_IS_OVER_IP_19_ADDR                "224.0.0.19"  //IS-IS over IP
#define IPV4_IS_IS_OVER_IP_20_ADDR                "224.0.0.20"  //IS-IS over IP
#define IPV4_IS_IS_OVER_IP_21_ADDR                "224.0.0.21"  //IS-IS over IP
#define IPV4_IGMPV3_ADDR                          "224.0.0.22"  //IGMP Version 3 (Internet Group Management Protocol)
#define IPV4_HOT_STANDBY_ROUTERV2_ADDR            "224.0.0.102" //Hot Standby Router Protocol Version 2
#define IPV4_MCAST_DNS_ADDR                 "224.0.0.251"   //Multicast DNS address
#define IPV4_LINK_LOCAL_MCAST_NAME_RES_ADDR       "224.0.0.252" //Link-local Multicast Name Resolution address
#define IPV4_NTP_ADDR                       "224.0.1.1" //Network Time Protocol address
#define IPV4_CISCO_AUTO_RP_ANNOUNCE_ADDR          "224.0.1.39"  //Cisco Auto-RP-Announce address
#define IPV4_CISCO_AUTO_RP_DISCOVERY_ADDR         "224.0.1.40"  //Cisco Auto-RP-Discovery address
#define IPV4_H_323_GETEKEEPER_DISC_ADDR           "224.0.1.41"  //H.323 Gatekeeper discovery address

//IPv6 addresses that are reserved for IP multicasting (http://www.iana.org/assignments/ipv6-multicast-addresses/)
#define IPV6_ALL_NODES_ADDR                       "ff02::1"   //All nodes on the local network segment (equivalent to the IPv4 link-local broadcast address, 169.254.255.255)
#define IPV6_ALL_NODE_LOCAL_ROUTER                "ff01::2"       //RFC 4291 ==> All Routers Addresses
#define IPV6_ALL_LINK_LOCAL_ROUTER                "ff02::2"   //All routers on the link local network segment
#define IPV6_ALL_SITE_LOCAL_ROUTER                "ff05::2"       //All routers on the site local network segment [RFC4291]
#define IPV6_ALL_MLDv2_CAPABLE_ROUTERS            "ff02::16"      //All MLDv2-capable routers [RFC3810]
#define IPV6_ALL_PIM_ROUTERS                      "ff02::d"       //All PIM Routers

string ipAddrResolver(string ipAddr);

typedef void (*free_fun) (struct addrinfo*);

template< typename F, typename V>
struct save_free {
private:
    F m_fun;
    V m_val;
public:
    save_free(F fun, V val): m_fun(fun), m_val(val) {
    }

    virtual ~save_free() {
        (*m_fun)(m_val);
    }
};

//Multicast Socket
/**
 * @brief Wrapper for a multicast socket.
 */
class mc_socket
{
private:
    bool generic_source_sockopt(const addr_storage& gaddr, const addr_storage& saddr, int if_index, int optname);
    bool generic_group_sockopt(const addr_storage& gaddr, int if_index, int optname);
protected:
    /**
     * @brief Used socket descriptor
     */
    int m_sock;

    /**
     * @brief Used IP version (AF_INET or AF_INET6).
     */
    int m_addrFamily;

    /**
     * @brief Save whether the socket is created not in this class.
     */
    bool m_own_socket;

public:
    /**
     * @brief Create a multicast socket.
     */
    mc_socket();

    /**
     * @brief Mc_socket mustnt be copied because its contain an unique socket.
     */
    mc_socket(const mc_socket& copy) = delete;

    /**
     * @brief Mc_socket mustnt be assigned because its contain an unique socket.
     */
    mc_socket& operator=(const mc_socket& copy) = delete;

    /**
     * @brief Close the internal datagram socket.
     */
    virtual ~mc_socket();

    /**
     * @brief Create an IPv4 datagram socket.
     */
    virtual bool create_udp_ipv4_socket();

    /**
     * @brief Create IPv6 datagram socket.
     */
    virtual bool create_udp_ipv6_socket();

    /**
     * @brief Set an extern socket.
     * @param socket socket descriptor
     * @param addr_family used address family (AF_INET or AF_INET6)
     * @return Return true on success.
     */
    bool set_own_socket(int socket, int addr_family);

    /**
     * @return Get the address family (AF_INET | AF_INET6).
     */
    int get_addr_family();

    /**
     * @brief Bind IPv4 or IPv6 socket to a specific port.
     * @return Return true on success.
     */
    bool bind_udp_socket(int port);

    /**
     * @brief Enable or disable multicast loopback.
     * @return Return true on success.
     */
    bool set_loop_back(bool enable);

    /**
     * @brief Send a string to a specific ip address and to a specific port.
     * @param addr destination address of packet in clear text
     * @param port destination port
     * @param data message to send
     * @return Return true on success.
     */
    bool send_packet(const char* addr, int port, string data);

    /**
     * @brief Send data to a specific ip address and to a specific port.
     * @param addr destination address of the packet in clear text
     * @param port destination port
     * @param data data to send
     * @param data_size size of the data
     * @return Return true on success.
     */
    bool send_packet(const char* addr, int port, const unsigned char* data, unsigned int data_size);

    /**
     * @brief Receive a datagram
     * @param[out] buf read N bytes into buf from socket
     * @param[in] sizeOfBuf size of buf
     * @param[out] sizeOfInfo filled with the effective packet length less then sizeOfBuf
     * @return Return true on success.
     */
    bool receive_packet(unsigned char* buf, int sizeOfBuf, int& sizeOfInfo);

    /**
     * @brief Receive a message with the kernel function recvmsg().
     * @param[out] msg received message
     * @param[out] sizeOfInfo size of the received message
     * @return Return true on success.
     */
    bool receive_msg(struct msghdr* msg, int& sizeOfInfo);

    /**
     * @brief Set a receive timeout.
     * @param msec timeout in millisecond
     * @return Return true on success.
     */
    bool set_receive_timeout(long msec);

    /**
     * @brief Choose a specific network interface
     * @return Return true on success.
     */
    bool choose_if(int if_index);

    /**
     * @brief set the ttl
     * @return Return true on success.
     */
    bool set_ttl(int ttl);

    /**
     * @brief Join a multicast group on a specific network interface.
     * @param addr clear text multicast group 
     * @param if_index define related interface 
     * @return Return true on success.
     */
    bool join_group(const addr_storage& gaddr, int if_index);

    /**
     * @brief Leave a multicast group on a specific network interface.
     * @param addr clear text multicast group 
     * @param if_index define related interface 
     * @return Return true on success.
     */
    bool leave_group(const addr_storage& gaddr, int if_index);

    /**
     * @brief Block a source on a specific network interface.
     * @param addr clear text multicast group 
     * @param if_index define related interface 
     * @return Return true on success.
     */
    bool block_source(const addr_storage& gaddr, const addr_storage& saddr, int if_index);

    /**
     * @brief Unblock a source on a specific network interface.
     * @param addr clear text multicast group 
     * @param if_index define related interface 
     * @return Return true on success.
     */
    bool unblock_source(const addr_storage& gaddr, const addr_storage& saddr, int if_index);

    /**
     * @brief Join a source on a specific network interface.
     * @param addr clear text multicast group 
     * @param if_index define related interface 
     * @return Return true on success.
     */
    bool join_source_group(const addr_storage& gaddr, const addr_storage& saddr, int if_index);

    /**
     * @brief Leave a source on a specific network interface.
     * @param addr clear text multicast group 
     * @param if_index define related interface 
     * @return Return true on success.
     */
    bool leave_source_group(const addr_storage& gaddr, const addr_storage& saddr, int if_index);

    /**
     * @brief Check for valid socket descriptor.
     */
    bool is_udp_valid() {
        return m_sock > 0;
    }

    /**
     * @brief Test a part of the class mc_socket.
     * @param ipverion "AF_INET" or "AF_INET6"
     * @param msg any message is allowed
     * @param interface for example "eth0" or "lo"
     * @param gaddr multicast address for example "239.99.99.99" or "FF02:0:0:0:99:99:99:99"
     * @param port define a port 
     */
    static void test_mc_goup_functions(string ipversion, string msg, string interface, string gaddr, int port);

    /**
     * @brief Test a part of the class mc_socket.
     */
    static void test_mc_source_functions(string ipversion, string interface, string gaddr, string saddr_a, string saddr_b);

};

#endif /* MCSOCKET_H_ */
