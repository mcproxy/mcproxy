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
#include "include/proxy/mld_sender.hpp"

#include <net/if.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include <memory>

mld_sender::mld_sender(): sender(MLDv2)
{
    HC_LOG_TRACE("");

    if (is_IPv6(m_group_mem_protocol)) {
        if (!m_sock.set_default_icmp6_checksum_calc(true)) {
            throw "failed to set default icmmpv6 checksum";
        }
        if (!add_hbh_opt_header()) {
            throw "failed to add hop by hop header";
        }
    } else {
        HC_LOG_ERROR("wrong address family: " <<get_group_mem_protocol_name(m_group_mem_protocol));
        throw "wrong address family";
    }
}

//bool mld_sender::send_general_query(int if_index) const
//{
    //HC_LOG_TRACE("");

    //int size = get_msg_min_size();
    //if (size < 0) {
        //return false;
    //}

    //if (!m_sock.choose_if(if_index)) {
        //return false;
    //}

    //std::unique_ptr<unsigned char[]> buf {new unsigned char[size] };
    ////unsigned char buf[size];
    //if (!create_mc_query(GENERAL_QUERY, buf.get())) {
        //return false;
    //}

    //return m_sock.send_packet(addr_storage(IPV6_ALL_NODES_ADDR), buf.get(), size);
//}

//bool mld_sender::send_group_specific_query(int if_index, const addr_storage& g_addr) const
//{
    //HC_LOG_TRACE("");

    //int size = get_msg_min_size();
    //if (size < 0) {
        //return false;
    //}

    //if (!m_sock.choose_if(if_index)) {
        //return false;
    //}

    //std::unique_ptr<unsigned char[]> buf {new unsigned char[size] };
    ////unsigned char buf[size];
    //if (!create_mc_query(MC_ADDR_SPECIFIC_QUERY, buf.get(), &g_addr)) {
        //return false;
    //}

    //return m_sock.send_packet(g_addr, buf.get(), size);
//}

//int mld_sender::get_msg_min_size() const
//{
    //HC_LOG_TRACE("");

    ////if (m_version == 1) {
    ////return sizeof(struct mld_hdr);
    ////} else {
    ////HC_LOG_ERROR("IPv6 version: " << m_version << " not supported");
    ////return -1;
    ////}
    //return -1;
//}

//bool mld_sender::send_report(int if_index, const addr_storage& g_addr) const
//{
    //HC_LOG_TRACE("");

    //return m_sock.join_group(g_addr, if_index);
//}

//bool mld_sender::send_leave(int if_index, const addr_storage& g_addr) const
//{
    //HC_LOG_TRACE("");

    //return m_sock.leave_group(g_addr, if_index);
//}

//bool mld_sender::create_mc_query(msg_type type, unsigned char* buf, const addr_storage* g_addr) const
//{
    //HC_LOG_TRACE("");

    ////if (m_version == 1) {
    ////struct mld_hdr* mld_Hdr = (struct mld_hdr*)buf;

    ////mld_Hdr->mld_type = MLD_LISTENER_QUERY;
    ////mld_Hdr->mld_code = 0;
    ////mld_Hdr->mld_cksum = MC_MASSAGES_AUTO_FILL;
    ////mld_Hdr->mld_reserved = 0;

    ////if (type == GENERAL_QUERY) {
    ////mld_Hdr->mld_maxdelay = htons(MC_TV_QUERY_RESPONSE_INTERVAL * MC_TV_MAX_RESPONSE_DELAY_UNIT);
    ////mld_Hdr->mld_addr = addr_storage(m_addr_family).get_in6_addr(); //0.0.0.0
    ////} else if (type == MC_ADDR_SPECIFIC_QUERY) {
    ////if (!g_addr) {
    ////HC_LOG_ERROR("g_addr is NULL");
    ////return false;
    ////}

    ////mld_Hdr->mld_maxdelay = htons(MC_TV_LAST_LISTENER_QUERY_INTERVAL * MC_TV_MAX_RESPONSE_DELAY_UNIT);
    ////mld_Hdr->mld_addr = g_addr->get_in6_addr();
    ////} else {
    ////HC_LOG_ERROR("wrong type: " << type);
    ////return false;
    ////}

    ////return true;
    ////} else {
    ////HC_LOG_ERROR("wrong verson: " << m_version);
    ////return false;
    ////}
    //return true;
//}

bool mld_sender::add_hbh_opt_header() const
{
    HC_LOG_TRACE("");

    unsigned char extbuf[sizeof(struct ip6_hdr) + sizeof(struct ip6_hbh) + sizeof(struct ip6_opt_router) + sizeof(pad2)];

    struct ip6_hbh* hbh_Hdr = (struct ip6_hbh*)extbuf;
    struct ip6_opt_router* opt_Hdr = (struct ip6_opt_router*)(extbuf + sizeof(struct ip6_hbh));
    pad2* pad_Hdr = (pad2*)(extbuf + sizeof(struct ip6_hbh) + sizeof(struct ip6_opt_router));

    hbh_Hdr->ip6h_nxt = IPPROTO_ICMPV6;
    hbh_Hdr->ip6h_len =  MC_MASSAGES_IPV6_ROUTER_ALERT_OPT_SIZE; //=> 8 Bytes

    opt_Hdr->ip6or_type = IP6OPT_ROUTER_ALERT;
    opt_Hdr->ip6or_len = sizeof(opt_Hdr->ip6or_value);
    *(u_int16_t*)&opt_Hdr->ip6or_value[0] = IP6_ALERT_MLD;

    *pad_Hdr = IP6OPT_PADN;

    if (!m_sock.add_extension_header((unsigned char*)hbh_Hdr, sizeof(struct ip6_hbh) + sizeof(struct ip6_opt_router) + sizeof(pad2))) {
        return false;
    }

    return true;
}
