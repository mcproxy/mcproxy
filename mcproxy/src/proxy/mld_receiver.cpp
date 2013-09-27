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

#include <linux/mroute6.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <sys/socket.h>

//DEBUG
#include <net/if.h>

mld_receiver::mld_receiver(int addr_family, std::shared_ptr<mroute_socket> mrt_sock): receiver(addr_family, mrt_sock)
{
    HC_LOG_TRACE("");
    if (!m_mrt_sock->set_recv_icmpv6_msg()) {
        throw "failed to set receive icmpv6 message";
    }

    if (!m_mrt_sock->set_recv_pkt_info()) {
        throw "faield to set receive paket info";
    }
}

int mld_receiver::get_iov_min_size()
{
    HC_LOG_TRACE("");
    int size_ip = sizeof(struct mld_hdr);
    int size_kernel = sizeof(struct mrt6msg);
    return (size_ip < size_kernel) ? size_kernel : size_ip;
}

int mld_receiver::get_ctrl_min_size()
{
    HC_LOG_TRACE("");

    return sizeof(struct cmsghdr) + sizeof(struct in6_pktinfo);
    //return 400;
}

void mld_receiver::analyse_packet(struct msghdr* msg, int)
{
    HC_LOG_TRACE("");


    struct mld_hdr* hdr = (struct mld_hdr*)msg->msg_iov->iov_base;
    proxy_instance* pr_i;
    addr_storage g_addr;

    if (hdr->mld_type == MLD_RECEIVER_KERNEL_MSG) { //kernel
        struct mrt6msg* mldctl = (struct mrt6msg*)msg->msg_iov->iov_base;

        switch (mldctl->im6_msgtype) {
        case MRT6MSG_NOCACHE: {
            addr_storage src_addr(mldctl->im6_src);
            g_addr = mldctl->im6_dst;

            int if_index;
            if ((if_index = get_if_index(mldctl->im6_mif)) == 0) {
                return;
            }

            if ((pr_i = get_proxy_instance(if_index)) == nullptr) {
                return;
            }

            //proxy_msg m;
            //m.type = proxy_msg::RECEIVER_MSG;
            //m.msg = new struct receiver_msg(receiver_msg::CACHE_MISS, if_index, src_addr, g_addr);
            //pr_i->add_msg(m);
            break;
        }
        default:
            HC_LOG_WARN("unknown kernel message");
        }
    } else if (hdr->mld_type == MLD_LISTENER_REPORT || hdr->mld_type == MLD_LISTENER_REDUCTION) { //join
        struct in6_pktinfo* packet_info = nullptr;

        for (struct cmsghdr* cmsgptr = CMSG_FIRSTHDR(msg); cmsgptr != nullptr; cmsgptr = CMSG_NXTHDR(msg, cmsgptr)) {
            if (cmsgptr->cmsg_len > 0 && cmsgptr->cmsg_level == IPPROTO_IPV6 && cmsgptr->cmsg_type == IPV6_PKTINFO ) {
                packet_info = (struct in6_pktinfo*)CMSG_DATA(cmsgptr);
            }
        }
        if (packet_info == nullptr) {
            return;
        }

        if ((pr_i = this->get_proxy_instance(packet_info->ipi6_ifindex)) == nullptr) {
            return;    //?is ifindex registratet
        }
        g_addr = hdr->mld_addr;

        //proxy_msg m;
        //m.type = proxy_msg::RECEIVER_MSG;

        //if (hdr->mld_type == MLD_LISTENER_REPORT) {
            //m.msg = new struct receiver_msg(receiver_msg::JOIN, packet_info->ipi6_ifindex, g_addr);
        //} else if (hdr->mld_type == MLD_LISTENER_REDUCTION) {
            //m.msg = new struct receiver_msg(receiver_msg::LEAVE, packet_info->ipi6_ifindex, g_addr);
        //} else {
            //HC_LOG_ERROR("wrong mld type");
        //}

        //pr_i->add_msg(m);
    } else {
        HC_LOG_DEBUG("unknown MLD-packet: " << (int)(hdr->mld_type));
    }
}
