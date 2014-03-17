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
#include "include/proxy/message_format.hpp"
#include "include/utils/extended_mld_defines.hpp"

#include <net/if.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include <memory>

mld_sender::mld_sender(const std::shared_ptr<const interfaces>& interfaces): sender(interfaces, MLDv2)
{
    HC_LOG_TRACE("");

    if (is_IPv6(m_group_mem_protocol)) {
        if (!m_sock.set_ipv6_auto_icmp6_checksum_calc(true)) {
            throw "failed to set default icmmpv6 checksum";
        }
        if (!add_hbh_opt_header()) {
            throw "failed to add router alert header";
        }
    } else {
        HC_LOG_ERROR("wrong address family: " << get_group_mem_protocol_name(m_group_mem_protocol));
        throw "wrong address family";
    }
}

bool mld_sender::send_record(unsigned int if_index, mc_filter filter_mode, const addr_storage& gaddr, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    if (filter_mode == INCLUDE_MODE && slist.empty() ) {
        m_sock.leave_group(gaddr, if_index);
        return true;
    } else if (filter_mode == EXCLUDE_MODE || filter_mode == EXCLUDE_MODE) {
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

bool mld_sender::send_general_query(unsigned int if_index, const timers_values& tv) const
{
    HC_LOG_TRACE("");

    return send_mldv2_query(if_index, tv, addr_storage(AF_INET6), false, source_list<source>());
}

bool mld_sender::send_mc_addr_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag) const
{
    HC_LOG_TRACE("");

    return send_mldv2_query(if_index, tv, gaddr, s_flag, source_list<source>());
}

bool mld_sender::send_mc_addr_and_src_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, source_list<source>& slist) const
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
        send_mldv2_query(if_index, tv, gaddr, true, slist_higher);
    }

    if (!slist_lower.empty()) {
        send_mldv2_query(if_index, tv, gaddr, false, slist_lower);
    }

    return rc;
}

bool mld_sender::send_mldv2_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag, const source_list<source>& slist) const
{
    HC_LOG_TRACE("");

    std::unique_ptr<mldv2_query> q;
    unsigned int size;

    if (slist.empty()) {
        size = sizeof(mldv2_query);
        q.reset(new mldv2_query);
    } else {
        size = sizeof(mldv2_query) + (slist.size() * sizeof(in6_addr));
        q.reset(reinterpret_cast<mldv2_query*>(new unsigned char[size]));
    }

    q->type = MLD_LISTENER_QUERY;
    q->code = 0;
    q->checksum = MC_MASSAGES_AUTO_FILL;

    addr_storage dst_addr;

    if (gaddr == addr_storage(AF_INET6)) { //general query
        dst_addr = IPV6_ALL_NODES_ADDR;
        q->max_resp_delay = htons(tv.maxrespi_to_maxrespc_mldv2(tv.get_query_response_interval()));
    } else { //all other types of queries
        dst_addr = gaddr;
        q->max_resp_delay = htons(tv.maxrespi_to_maxrespc_mldv2(tv.get_last_listener_query_time()));
    }

    q->reserved = 0;
    q->gaddr = gaddr.get_in6_addr();
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
        in6_addr* source_ptr = reinterpret_cast<in6_addr*>(reinterpret_cast<unsigned char*>(q.get()) + sizeof(mldv2_query));
        for (auto & e : slist) {
            *source_ptr = e.saddr.get_in6_addr();
            source_ptr++;
        }
    }

    if (!m_sock.choose_if(if_index)) {
        return false;
    }

    return m_sock.send_packet(dst_addr, reinterpret_cast<unsigned char*>(q.get()), size);
}

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

    if (!m_sock.add_ipv6_extension_header((unsigned char*)hbh_Hdr, sizeof(struct ip6_hbh) + sizeof(struct ip6_opt_router) + sizeof(pad2))) {
        return false;
    }

    return true;
}
