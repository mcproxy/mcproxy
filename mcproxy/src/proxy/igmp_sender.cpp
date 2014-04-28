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

igmp_sender::igmp_sender(const std::shared_ptr<const interfaces>& interfaces): sender(interfaces, IGMPv3)
{
    HC_LOG_TRACE("");

    if (is_IPv4(m_group_mem_protocol)) {
        if (!m_sock.set_no_ip_hdr(true)) {
            throw "failed to set no ip hdr";
        }
    } else {
        HC_LOG_ERROR("wrong address family: " << get_group_mem_protocol_name(m_group_mem_protocol));
        throw "wrong address family";
    }
}

bool igmp_sender::send_record(unsigned int if_index, mc_filter filter_mode, const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    if (filter_mode == INCLUDE_MODE && slist.empty() ) {
        m_sock.leave_group(gaddr, if_index);
        return true;
    } else if (filter_mode == INCLUDE_MODE) {
        for (auto & e : slist) {
            if (! m_sock.join_source_group(gaddr, e.saddr, if_index)) {
                return false;
            }
        }

        return true;
    } else if (filter_mode == EXCLUDE_MODE) {
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

bool igmp_sender::send_general_query(unsigned int if_index, const timers_values& tv) const
{
    HC_LOG_TRACE("");

    return send_igmpv3_query(if_index, tv, addr_storage(AF_INET), false, source_list<source>());
}

bool igmp_sender::send_mc_addr_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag) const
{
    HC_LOG_TRACE("");

    return send_igmpv3_query(if_index, tv, gaddr, s_flag, source_list<source>());
}

bool igmp_sender::send_mc_addr_and_src_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, source_list<source>& slist) const
{
    HC_LOG_TRACE("");

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

bool igmp_sender::send_igmpv3_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    std::unique_ptr<unsigned char[]> packet;
    unsigned int size;

    if (slist.empty()) {
        size = sizeof(ip) + sizeof(router_alert_option) + sizeof(igmpv3_query);
        packet.reset(new unsigned char[size]);
    } else { //all other types of queries
        size = sizeof(ip) + sizeof(router_alert_option) + sizeof(igmpv3_query) + (slist.size() * sizeof(in_addr));
        packet.reset(new unsigned char[size]);
    }

    addr_storage dst_addr;

    if (gaddr == addr_storage(AF_INET)) { //is general query
        dst_addr = IPV4_ALL_HOST_ADDR;
    } else {
        dst_addr = gaddr;
    }

    //-------------------------------------------------------------------
    //fill ip header
    ip* ip_hdr = reinterpret_cast<ip*>(packet.get());

    ip_hdr->ip_v = 4;
    ip_hdr->ip_hl = (sizeof(ip) + sizeof(router_alert_option)) / 4;
    ip_hdr->ip_tos = 0;
    ip_hdr->ip_len = htons(size);
    ip_hdr->ip_id = 0;
    ip_hdr->ip_off = htons(0 | IP_DF); //dont fragment flag
    ip_hdr->ip_ttl = 1;
    ip_hdr->ip_p = IPPROTO_IGMP;
    ip_hdr->ip_sum = 0;
    ip_hdr->ip_src = m_interfaces->get_saddr(interfaces::get_if_name(if_index)).get_in_addr();
    ip_hdr->ip_dst = dst_addr.get_in_addr();

    //-------------------------------------------------------------------
    //fill router_alert_option header
    router_alert_option* ra_hdr = reinterpret_cast<router_alert_option*>(reinterpret_cast<unsigned char*>(ip_hdr) + sizeof(ip));
    *ra_hdr = router_alert_option();

    ip_hdr->ip_sum = m_sock.calc_checksum(reinterpret_cast<unsigned char*>(ip_hdr), sizeof(ip) + sizeof(router_alert_option));

    //-------------------------------------------------------------------
    //fill igmpv3 query
    igmpv3_query* query = reinterpret_cast<igmpv3_query*>(reinterpret_cast<unsigned char*>(ra_hdr) + sizeof(router_alert_option));

    query->igmp_type = IGMP_MEMBERSHIP_QUERY;

    if (gaddr == addr_storage(AF_INET)) { //general query
        query->igmp_code = tv.maxrespi_to_maxrespc_igmpv3(tv.get_query_response_interval());
    } else {
        query->igmp_code = tv.maxrespi_to_maxrespc_igmpv3(tv.get_last_listener_query_time());
    }

    query->igmp_cksum = 0;
    query->igmp_group = gaddr.get_in_addr();
    query->resv2 = 0;
    query->suppress = s_flag;

    if (tv.get_robustness_variable() <= 7) {
        query->qrv = tv.get_robustness_variable();
    } else {
        query->qrv = 0;
    }

    query->qqic = tv.qqi_to_qqic(tv.get_query_interval());
    query->num_of_srcs = ntohs(slist.size());

    //-------------------------------------------------------------------
    //add sources
    if (!slist.empty()) {
        in_addr* source_ptr = reinterpret_cast<in_addr*>(reinterpret_cast<unsigned char*>(query) + sizeof(igmpv3_query));
        for (auto & e : slist) {
            *source_ptr = e.saddr.get_in_addr();
            source_ptr++;
        }
    }

    query->igmp_cksum = m_sock.calc_checksum(reinterpret_cast<unsigned char*>(query), (sizeof(igmpv3_query) + (slist.size() * sizeof(in_addr))));

    if (!m_sock.choose_if(if_index)) {
        return false;
    }

    return m_sock.send_packet(dst_addr, reinterpret_cast<unsigned char*>(ip_hdr), size);
}

