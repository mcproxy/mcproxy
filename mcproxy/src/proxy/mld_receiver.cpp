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
#include "include/proxy/mld_receiver.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/utils/extended_mld_defines.hpp"

#include <linux/mroute6.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <sys/socket.h>

//DEBUG
#include <net/if.h>

mld_receiver::mld_receiver(proxy_instance* pr_i, const std::shared_ptr<const mroute_socket> mrt_sock, const std::shared_ptr<const interfaces> interfaces, bool in_debug_testing_mode)
    : receiver(pr_i, AF_INET6, mrt_sock, interfaces, in_debug_testing_mode)
{
    HC_LOG_TRACE("");
    if (!m_mrt_sock->set_ipv6_recv_icmpv6_msg()) {
        throw "failed to set receive icmpv6 message";
    }

    if (!m_mrt_sock->set_ipv6_recv_pkt_info()) {
        throw "faield to set receive paket info";
    }

    start();
}

int mld_receiver::get_iov_min_size()
{
    HC_LOG_TRACE("");
    int size_ip = sizeof(struct mld_hdr) + 10000; //random number plz fix it??????????????????????????
    int size_kernel = sizeof(struct mrt6msg);
    return (size_ip < size_kernel) ? size_kernel : size_ip;
}

int mld_receiver::get_ctrl_min_size()
{
    HC_LOG_TRACE("");

    return sizeof(struct cmsghdr) + sizeof(struct in6_pktinfo);
}

void mld_receiver::analyse_packet(struct msghdr* msg, int)
{
    HC_LOG_TRACE("");


    struct mld_hdr* hdr = (struct mld_hdr*)msg->msg_iov->iov_base;
    unsigned int if_index = 0;
    addr_storage gaddr;
    addr_storage saddr;

    HC_LOG_DEBUG("received packet XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

    /* packets sent up from kernel to daemon have ip->ip_p = 0 */
    if (hdr->mld_type == MLD_RECEIVER_KERNEL_MSG) { //kernel
        HC_LOG_DEBUG("kernel msg received");
        struct mrt6msg* mldctl = (struct mrt6msg*)msg->msg_iov->iov_base;

        switch (mldctl->im6_msgtype) {
        case MRT6MSG_NOCACHE: {
            saddr = mldctl->im6_src;
            HC_LOG_DEBUG("\tsaddr: " << saddr);

            gaddr = mldctl->im6_dst;
            HC_LOG_DEBUG("\tgaddr: " << gaddr);

            HC_LOG_DEBUG("\tvif: " << (int)mldctl->im6_mif);
            if ((if_index = m_interfaces->get_if_index(mldctl->im6_mif)) == 0) {
                return;
            }
            HC_LOG_DEBUG("\treceived on interface:" << interfaces::get_if_name(if_index));

            if (!is_if_index_relevant(if_index)) {
                return;
            }

            m_proxy_instance->add_msg(std::make_shared<new_source_msg>(if_index, gaddr, saddr));
            break;
        }
        default:
            HC_LOG_WARN("unknown kernel message");
        }
    } else if (hdr->mld_type == MLD_LISTENER_REPORT || hdr->mld_type == MLD_LISTENER_REDUCTION) {
        HC_LOG_DEBUG("MLD_LISTENER_REPORT or MLD_LISTENER_REDUCTION received");

        struct in6_pktinfo* packet_info = nullptr;

        for (struct cmsghdr* cmsgptr = CMSG_FIRSTHDR(msg); cmsgptr != nullptr; cmsgptr = CMSG_NXTHDR(msg, cmsgptr)) {
            if (cmsgptr->cmsg_len > 0 && cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_PKTINFO ) {
                packet_info = (struct in6_pktinfo*)CMSG_DATA(cmsgptr);
            }
        }
        if (packet_info == nullptr) {
            return;
        }

        HC_LOG_DEBUG("\tsaddr: " << addr_storage(packet_info->ipi6_addr));
        if_index = packet_info->ipi6_ifindex;
        HC_LOG_DEBUG("\treceived on interface:" << interfaces::get_if_name(if_index));

        if (!is_if_index_relevant(if_index)) {
            HC_LOG_DEBUG("interface is not relevant");
            return;
        }

        gaddr = hdr->mld_addr;
        HC_LOG_DEBUG("\tgroup: " << gaddr);

        if (hdr->mld_type == MLD_LISTENER_REPORT) {
            HC_LOG_DEBUG("\treport received");
            m_proxy_instance->add_msg(std::make_shared<group_record_msg>(if_index, MODE_IS_EXCLUDE, gaddr, source_list<source>(), MLDv1));
        } else if (hdr->mld_type == MLD_LISTENER_REDUCTION) {
            HC_LOG_DEBUG("\tlistener reduction received");
            m_proxy_instance->add_msg(std::make_shared<group_record_msg>(if_index, CHANGE_TO_INCLUDE_MODE, gaddr, source_list<source>(), MLDv1));
        } else {
            HC_LOG_ERROR("unkown mld type: " << hdr->mld_type);
        }
    } else if (hdr->mld_type == MLD_V2_LISTENER_REPORT) {
        HC_LOG_DEBUG("MLD_V2_LISTENER_REPORT received");

        struct in6_pktinfo* packet_info = nullptr;

        for (struct cmsghdr* cmsgptr = CMSG_FIRSTHDR(msg); cmsgptr != nullptr; cmsgptr = CMSG_NXTHDR(msg, cmsgptr)) {
            if (cmsgptr->cmsg_len > 0 && cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_PKTINFO ) {
                packet_info = (struct in6_pktinfo*)CMSG_DATA(cmsgptr);
            }
        }
        if (packet_info == nullptr) {
            return;
        }

        mldv2_mc_report* v3_report = reinterpret_cast<mldv2_mc_report*>(hdr);
        mldv2_mc_record* rec = reinterpret_cast<mldv2_mc_record*>(reinterpret_cast<unsigned char*>(v3_report) + sizeof(mldv2_mc_report));

        int num_records = ntohs(v3_report->num_of_mc_records);
        HC_LOG_DEBUG("\tnum of multicast records: " << num_records);

        if_index = packet_info->ipi6_ifindex;
        HC_LOG_DEBUG("\treceived on interface:" << interfaces::get_if_name(if_index));

        if (!is_if_index_relevant(if_index)) {
            HC_LOG_DEBUG("interface is not relevant");
            return;
        }

        for (int i = 0; i < num_records; ++i) {
            mcast_addr_record_type rec_type = static_cast<mcast_addr_record_type>(rec->type);
            unsigned int aux_size = rec->aux_data_len * 4; //RFC 3810 Section 5.2.6 Aux Data Len
            int nos = ntohs(rec->num_of_srcs);

            gaddr = addr_storage(rec->gaddr);
            source_list<source> slist;

            in6_addr* src = reinterpret_cast<in6_addr*>(reinterpret_cast<unsigned char*>(rec) + sizeof(mldv2_mc_record));
            for (int j = 0; j < nos; ++j) {
                slist.insert(addr_storage(*src));
                ++src;
            }

            HC_LOG_DEBUG("\trecord type: " << get_mcast_addr_record_type_name(rec_type));
            HC_LOG_DEBUG("\tgaddr: " << gaddr);
            HC_LOG_DEBUG("\tnumber of sources: " << slist.size());
            HC_LOG_DEBUG("\tsource_list: " << slist);
            m_proxy_instance->add_msg(std::make_shared<group_record_msg>(if_index, rec_type, gaddr, move(slist), MLDv2));

            rec = reinterpret_cast<mldv2_mc_record*>(reinterpret_cast<unsigned char*>(rec) + sizeof(mldv2_mc_record) + nos * sizeof(in6_addr) + aux_size);
        }
    } else if (hdr->mld_type == MLD_LISTENER_QUERY) {
        HC_LOG_DEBUG("MLD_LISTENER_QUERY received");
        HC_LOG_WARN("querier election is not implemented");
    } else {
        HC_LOG_DEBUG("unknown MLD-packet: " << (int)(hdr->mld_type));
    }
}
