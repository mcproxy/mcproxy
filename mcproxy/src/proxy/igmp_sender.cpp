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
#include "include/proxy/igmp_sender.hpp"
#include "include/proxy/message_format.hpp"
#include "include/utils/extended_igmp_defines.hpp"

#include <netinet/igmp.h>
#include <netinet/ip.h>
#include <net/if.h>

#include <memory>

igmp_sender::igmp_sender(): sender(IGMPv3)
{
    HC_LOG_TRACE("");

    if (is_IPv4(m_group_mem_protocol)) {
        if (!m_sock.set_ipv4_router_alert_header(true)) {
            throw "failed to add router alert header";
        }
    } else {
        HC_LOG_ERROR("wrong address family: " << get_group_mem_protocol_name(m_group_mem_protocol));
        throw "wrong address family";
    }
}

bool igmp_sender::send_report(unsigned int if_index, mc_filter filter_mode, const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    if (filter_mode == INCLUDE_MODE && slist.empty() ) {
        m_sock.leave_group(gaddr, if_index);
        return true;
    } else if (filter_mode == EXLCUDE_MODE) {
        m_sock.join_group(gaddr, if_index);
        std::list<addr_storage> src_list;
        for (auto & e : slist) {
            src_list.push_back(e.saddr);
        }

        return m_sock.set_source_filter(if_index, gaddr, filter_mode, src_list);
    } else {
        HC_LOG_ERROR("unknown filter mode");
        return false;
    }
}

bool igmp_sender::send_general_query(unsigned int if_index, const timers_values& tv, group_mem_protocol gmp) const
{
    HC_LOG_TRACE("");

    switch (gmp) {
    case IGMPv1:
        HC_LOG_ERROR("igmpv1 not supported");
        return false;
    case IGMPv2:
        HC_LOG_ERROR("igmpv2 not supported");
        return false;
    case IGMPv3: {
        return send_igmpv3_query(if_index, tv, addr_storage(AF_INET), false, source_list<source>());
    }
    default:
        HC_LOG_ERROR("unknown group membership protocol");
        return false;
    }
}

bool igmp_sender::send_mc_addr_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag, group_mem_protocol gmp) const
{
    HC_LOG_TRACE("");

    switch (gmp) {
    case IGMPv1:
        HC_LOG_ERROR("igmpv1 not supported");
        return false;
    case IGMPv2:
        HC_LOG_ERROR("igmpv2 not supported");
        return false;
    case IGMPv3: {
        return send_igmpv3_query(if_index, tv, gaddr, s_flag, source_list<source>());
    }
    default:
        HC_LOG_ERROR("unknown group membership protocol");
        return false;
    }
}

bool igmp_sender::send_mc_addr_and_src_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, source_list<source>& slist, group_mem_protocol gmp) const
{
    HC_LOG_TRACE("");

    switch (gmp) {
    case IGMPv1:
        HC_LOG_ERROR("igmpv1 not supported");
        return false;
    case IGMPv2:
        HC_LOG_ERROR("igmpv2 not supported");
        return false;
    case IGMPv3: {
        source_list<source> slist_lower;
        source_list<source> slist_higher;
        bool rc = false;
        for (auto & e : slist) {
            if (e.retransmission_count > 0) {
                e.retransmission_count--;

                if (e.retransmission_count > 0 ) {
                    rc = true;
                }

                if (e.shared_source_timer.get() != nullptr) {
                    if (e.shared_source_timer->is_remaining_time_greater_than(tv.get_last_listener_query_time())) {
                        slist_higher.insert(e);
                    } else {
                        slist_lower.insert(e);
                    }
                } else {
                    HC_LOG_ERROR("the shared source timer shouldnt be null");
                }
            }
        }

        if (!slist_higher.empty()) {
            send_igmpv3_query(if_index, tv, gaddr, true, slist_higher);
        }

        if (!slist_lower.empty()) {
            send_igmpv3_query(if_index, tv, gaddr, false, slist_lower);
        }

        return rc;
    }
    default:
        HC_LOG_ERROR("unknown group membership protocol");
        return false;
    }
}

bool igmp_sender::send_igmpv3_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    std::unique_ptr<igmpv3_query> q;
    unsigned int size;

    if (slist.empty()) {
        size = sizeof(igmpv3_query);
        q.reset(new igmpv3_query);
    } else { //all other types of queries
        size = sizeof(igmpv3_query) + (slist.size() * sizeof(in_addr));
        q.reset(reinterpret_cast<igmpv3_query*>(new unsigned char[size]));
    }

    q->igmp_type = IGMP_MEMBERSHIP_QUERY;

    addr_storage dst_addr;

    if (gaddr == addr_storage(AF_INET)) { //general query
        dst_addr = IPV4_ALL_HOST_ADDR;
        q->igmp_code = tv.maxrespi_to_maxrespc_igmpv3(tv.get_query_response_interval());
    } else {
        dst_addr = gaddr;
        q->igmp_code = tv.maxrespi_to_maxrespc_igmpv3(tv.get_last_listener_query_time());
    }

    q->igmp_cksum = 0;
    q->igmp_group = gaddr.get_in_addr();
    q->resv2 = 0;
    q->suppress = s_flag;

    if (tv.get_robustness_variable() <= 7) {
        q->qrv = tv.get_robustness_variable();
    } else {
        q->qrv = 0;
    }

    q->qqic = tv.qqi_to_qqic(tv.get_query_interval());
    q->num_of_srcs = ntohs(slist.size());

    if (!slist.empty()) {
        in_addr* source_ptr = reinterpret_cast<in_addr*>(reinterpret_cast<unsigned char*>(q.get()) + sizeof(igmpv3_query));
        for (auto & e : slist) {
            *source_ptr = e.saddr.get_in_addr();
            source_ptr++;
        }
    }

    q->igmp_cksum = m_sock.calc_checksum(reinterpret_cast<unsigned char*>(q.get()), size);

    if (!m_sock.choose_if(if_index)) {
        return false;
    }

    return m_sock.send_packet(dst_addr, reinterpret_cast<unsigned char*>(q.get()), size);
}


//------------------------------------------------------------------------------------------

//bool igmp_sender::send_general_query(int if_index) const
//{
//HC_LOG_TRACE("");

//int size = get_msg_min_size();
//if (size < 0) {
//return false;
//}

//std::unique_ptr<unsigned char[]> buf { new unsigned char[size] };
////unsigned char buf[size];

//if (!m_sock.choose_if(if_index)) {
//return false;
//}
//if (!create_mc_query(GENERAL_QUERY, buf.get())) {
//return false;
//}

//return m_sock.send_packet(addr_storage(IPV4_ALL_HOST_ADDR), buf.get(), size);
//}

//bool igmp_sender::send_group_specific_query(int if_index, const addr_storage& g_addr) const
//{
//HC_LOG_TRACE("");

//int size = get_msg_min_size();
//if (size < 0) {
//return false;
//}

//std::unique_ptr<unsigned char[]> buf { new unsigned char[size] };
////unsigned char buf[size];

//if (!m_sock.choose_if(if_index)) {
//return false;
//}
//if (!create_mc_query(GROUP_SPECIFIC_QUERY, buf.get(), &g_addr)) {
//return false;
//}

//return m_sock.send_packet(addr_storage(g_addr), buf.get(), size);
//}

//bool igmp_sender::send_report(int if_index, const addr_storage& g_addr) const
//{
//HC_LOG_TRACE("");

//return m_sock.join_group(g_addr, if_index);
//}

//bool igmp_sender::send_leave(int if_index, const addr_storage& g_addr) const
//{
//HC_LOG_TRACE("");

//return m_sock.leave_group(g_addr, if_index);
//}

//int igmp_sender::get_msg_min_size() const
//{
//HC_LOG_TRACE("");

////if (m_version == 2) {
////return sizeof(struct igmp);
////} else {
////HC_LOG_ERROR("IPv4 version: " << m_version << " not supported");
////return -1;
////}
//return -1;
//}


//bool igmp_sender::create_mc_query(msg_type type, unsigned char* buf, const addr_storage* g_addr) const
//{
//HC_LOG_TRACE("");

//if (m_version == 2) {
//struct igmp* igmp_Hdr = (struct igmp*)(buf);

//igmp_Hdr->igmp_type = IGMP_MEMBERSHIP_QUERY;
//igmp_Hdr->igmp_cksum = 0;

//if (type == GENERAL_QUERY) {
//igmp_Hdr->igmp_code = MC_TV_QUERY_RESPONSE_INTERVAL * MC_TV_MAX_RESPONSE_TIME_UNIT;
//igmp_Hdr->igmp_group = addr_storage(m_addr_family).get_in_addr(); //0.0.0.0
//} else if (type == GROUP_SPECIFIC_QUERY) {
//if (!g_addr) {
//HC_LOG_ERROR("g_addr is NULL");
//return false;
//}

//igmp_Hdr->igmp_code = MC_TV_LAST_MEMBER_QUERY_INTEVAL * MC_TV_MAX_RESPONSE_TIME_UNIT;
//igmp_Hdr->igmp_group = g_addr->get_in_addr();
//} else {
//HC_LOG_ERROR("wrong type: " << type);
//return false;
//}

//igmp_Hdr->igmp_cksum = m_sock.calc_checksum((unsigned char*)igmp_Hdr, sizeof(struct igmp));

//return true;
//} else {
//HC_LOG_ERROR("wrong verson: " << m_version);
//return false;
//}
//return true;
//}

