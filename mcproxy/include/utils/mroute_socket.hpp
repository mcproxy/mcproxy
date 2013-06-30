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


#ifndef MROUTE_SOCKET_HPP
#define MROUTE_SOCKET_HPP

#include "include/utils/mc_socket.hpp"
#include <sys/types.h>

#define MROUTE_RATE_LIMIT_ENDLESS 0
#define MROUTE_TTL_THRESHOLD 1
#define MROUTE_DEFAULT_TTL 1

#define ADD_SIGNED_NUM_U16(r,a) (r)+= (a); (r)+= ((r)>>16)

/**
 * @brief Wrapper for a multicast socket with additional functions to manipulate Linux kernel tables.
 */
class mroute_socket: public mc_socket
{
private:
     mroute_socket(const mroute_socket &copy);

     //not used
     bool create_udp_ipv4_socket(){
          return false;
     }

     //not used
     bool create_udp_ipv6_socket(){
          return false;
     }

public:

     /**
      * @brief Create a mroute_socket.
      */
     mroute_socket();

     /**
     * @brief Close raw socket.
     */
     virtual ~mroute_socket();

     /**
     * @brief Create IPv4 raw socket.
     * @return Return true on success.
     */
     bool create_raw_ipv4_socket();

     /**
      * @brief Create IPv6 raw socket (RFC 3542 Section 3).
      * @return Return true on success.
      */
     bool create_raw_ipv6_socket();

     /**
      * @brief Create IPv6 raw socket (RFC 3542 Section 3).
      * @return Return true on success.
      */
     bool set_kernel_table(int table);

     /**
      * @brief The IPv4 layer generates an IP header when
      *        sending a packet unless the IP_HDRINCL socket option
      *        is enabled on the socket. When it is enabled, the
      *        packet must contain an IP header. For receiving the
      *        IP header is always included in the packet.
      * @return Return true on success.
      */
     bool set_no_ip_hdr();

     /**
      * @brief Calculate an internet checksum needed for IPv4 IGMP header.
      * @param buf header of the IGMP packet with a zero checksum field
      * @param buf_size size of the IGMP packet
      */
     u_int16_t calc_checksum(const unsigned char* buf, int buf_size);

     /**
      * @brief Calculate the ICMPv6 header checksum by sending an ICMPv6 packet.
      *        Per default the ICMP6 checksum (RFC 3542 Section 3.1) will be calculate.
      * @param enable if true the checsum will be calculate.
      * @return Return true on success.
      */
     bool set_default_icmp6_checksum_calc(bool enable);

     /**
      * @brief Add an extension header to a sending packet.
      * @param buf extension header
      * @param buf_size size of the extension header
      * @return Return true on success.
      */
     bool add_extension_header(const unsigned char* buf, unsigned int buf_size);

     /**
      * @brief Set to pass all icmpv6 packets to userpace.
      * @return Return true on success.
      */
     bool set_recv_icmpv6_msg();

     /**
      * @brief Set to pass the Hob-by-Hob header to userpace.
      * @return Return true on success
      */
     bool set_recv_hop_by_hop_msg();

     /**
      * @brief Set to pass the receive packet information to userpace.
      * @return Return true on success
      */
     bool set_recv_pkt_info();

     /**
      * @brief Enable or disable MRT flag to manipulate the multicast routing tables.
      *        - sysctl net.ipv4.conf.all.mc_forwarding will be set/reset
      * @return Return true on success.
      */
     bool set_mrt_flag(bool enable);

     /**
      * @brief Adds the virtual interface to the mrouted API
      *        - sysctl net.ipv4.conf.eth0.mc_forwarding will be set
      *        - /proc/net$ cat ip_mr_vif displays the added interface
      *
      * @param vifNum musst the same unique number as delVIF (0 > uniqueNumber < MAXVIF ==32)
      * @param ifName is the interface name e. g. "eth0"
      * @param ipTunnelRemoteAddr if the interface is a tunnel interface the remote address has to set else it has to be NULL
      * @return Return true on success.
      */
     bool add_vif(int vifNum, const char* ifName, const char* ipTunnelRemoteAddr);

     /**
      * @brief Bind the virtual interface to a spezific table as output and input interface
      * @param vifNum is the virtual interface index
      * @param table is the spezific table
      * @return Return true on success.
      */
     bool bind_vif_to_table(const char* ifName, int table);

     /**
      * @brief unbind the virtual interface from a spezific table as output and input interface
      * @param vifNum is the virtual interface index
      * @param table is the spezific table
      * @return Return true on success.
      */
     bool unbind_vif_form_table(const char* ifName, int table);


     /**
      * @brief Delete the virtual interface from the multicast routing table.
      * @param vifNum virtual index of the interface
      * @return Return true on success.
      */
     bool del_vif(int vifNum);

     /**
      * @brief Adds a multicast route to the kernel.
      *        /proc/net$ cat ip_mr_cache display the route
      *
      * @param input_vifNum have to be the same value as in addVIF set
      * @param source_addr from the receiving packet
      * @param group_addr from the receiving packet
      * @param output_vifNum forward to this virtual interface indexes
      * @param output_vifNum_size size of the interface indexes
      * @return Return true on success.
      */
     bool add_mroute(int input_vifNum, const char* source_addr, const char* group_addr, unsigned int* output_vifNum, unsigned int output_vifNum_size);

     /**
      * @brief Delete a multicast route.
      * @param input_vifNum have to be the same value as in addVIF set
      * @param source_addr from the receiving packet
      * @param group_addr from the receiving packet
      * @return Return true on success.
      */
     bool del_mroute(int input_vifNum, const char* source_addr, const char* group_addr);

     /**
      * @brief Get various statistics per interface.
      * @param vif_index is the virtual interface index for an interface
      * @param req_v4 musst point to a sioc_vif_req struct and will filled by this function when ipv4 is used
      * @param req_v6 musst point to a sioc_mif_req6 struct and will filled by this function when ipv6 is used
      * @return Return true on success.
      */
     bool get_vif_stats(int vif_index, struct sioc_vif_req* req_v4, struct sioc_mif_req6* req_v6);

     /**
      * @brief Get various statistics per multicast route.
      * @param source_addr is the defined source address of the requested multicast route
      * @param group_addr is the defined group address of the requested multicast route
      * @param sgreq_v4 musst point to a sioc_sg_req struct and will filled by this function when ipv4 is used
      * @param sgreq_v6 musst point to a sioc_sg_req6 struct and will filled by this function when ipv6 is used
      * @return Return true on success.
      */
     bool get_mroute_stats(const char* source_addr, const char* group_addr, struct sioc_sg_req* sgreq_v4, struct sioc_sg_req6* sgreq_v6);

     /**
      * @brief simple test outputs
      */
#define  MROUTE_SOCKET_SRC_ADDR_V4 "141.22.27.157"
#define  MROUTE_SOCKET_G_ADDR_V4 "238.99.99.99"
#define  MROUTE_SOCKET_SRC_ADDR_V6 "fe80::5e26:aff:fe23:8dc1"
#define  MROUTE_SOCKET_G_ADDR_V6 "FF02:0:0:0:99:99:99:99"

#define  MROUTE_SOCKET_IF_NUM_ONE 0
#define  MROUTE_SOCKET_IF_NUM_TWO 1
#define  MROUTE_SOCKET_IF_NUM_THREE 2
#define  MROUTE_SOCKET_IF_STR_ONE "eth0"
#define  MROUTE_SOCKET_IF_STR_TWO "wlan0"
#define  MROUTE_SOCKET_IF_STR_THREE "tun0"

     void print_vif_stats(int vif_index);
     void print_mroute_stats(const char* source_addr, const char* group_addr);

     static void print_struct_mf6cctl(struct mf6cctl* mc);
     static void test_mcrouter_mrt_flag();

     static void test_add_vifs(mroute_socket* m);
     static void test_del_vifs(mroute_socket* m);
     static void test_add_route(mroute_socket* m);
     static void test_del_route(mroute_socket* m);
     static void test_mcrouter_vifs_routes(int addrFamily);
};

#endif // MROUTE_SOCKET_HPP
