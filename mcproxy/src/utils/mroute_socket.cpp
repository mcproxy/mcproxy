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
#include "include/utils/mroute_socket.hpp"
#include "include/utils/extended_mld_defines.hpp"

#include <netinet/icmp6.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdlib>

#include <cstring>
#include <iostream>
#include <sstream>

mroute_socket::mroute_socket()
{
    HC_LOG_TRACE("");
}

bool mroute_socket::create_raw_ipv4_socket()
{
    HC_LOG_TRACE("");

    if (is_udp_valid()) {
        close_socket();
    }

    //          IP-Protokollv4, UDP,    Protokoll
    m_sock = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
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

bool mroute_socket::create_raw_ipv6_socket()
{
    HC_LOG_TRACE("");

    if (is_udp_valid()) {
        close_socket();
    }

    //          IP-Protokollv6, UDP,    Protokoll
    m_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6); //SOCK_DGRAM //IPPROTO_IP
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

bool mroute_socket::set_kernel_table(int table) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
#ifdef MRT_TABLE
        if (setsockopt (m_sock, IPPROTO_IP, MRT_TABLE, &table, sizeof (table)) < 0) {
            HC_LOG_ERROR("failed to set kernel table! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        }
#else
        HC_LOG_ERROR("multiple ipv4 multicast routing table not supported: MRT_TABLE not defined");
        return false
#endif
    } else if (m_addrFamily == AF_INET6) {
#ifdef MRT6_TABLE
        if (setsockopt (m_sock, IPPROTO_IPV6, MRT6_TABLE, &table, sizeof (table)) < 0) {
            HC_LOG_ERROR("failed to set kernel table! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        }
#else
        HC_LOG_ERROR("multiple ipv6 multicast routing table not supported: MRT6_TABLE not defined");
        return false
#endif
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    return true;
}

bool mroute_socket::set_no_ip_hdr(bool enable) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int proto;
    if (m_addrFamily == AF_INET) {
        proto = IPPROTO_IP;
    } else if (m_addrFamily == AF_INET6) {
        proto = IPPROTO_IPV6;
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
    int one = enable ? 1 : 0;
    if (setsockopt (m_sock, proto, IP_HDRINCL, &one, sizeof (one)) < 0) {
        HC_LOG_ERROR("failed to set no ip header! Error: " << strerror(errno) << " errno: " << errno);
        return false;
    }

    return true;
}

u_int16_t mroute_socket::calc_checksum(const unsigned char* buf, int buf_size) const
{
    HC_LOG_TRACE("");

    u_int16_t* b = (u_int16_t*)buf;
    int sum = 0;

    for (int i = 0; i < buf_size / 2; i++) {
        ADD_SIGNED_NUM_U16(sum, b[i]);
        //sum +=b[i];
    }

    if (buf_size % 2 == 1) {
        //sum += buf[buf_size-1];
        ADD_SIGNED_NUM_U16(sum, buf[buf_size - 1]);
    }

    return ~sum;
}

bool mroute_socket::set_ipv6_auto_icmp6_checksum_calc(bool enable) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        HC_LOG_ERROR("this funktion is only available vor IPv6 sockets ");
        return false;
    } else if (m_addrFamily == AF_INET6) {
        int offset = enable ? 2 : -1;
        if (setsockopt (m_sock, IPPROTO_IPV6, IP_HDRINCL, &offset, sizeof (offset)) < 0) {
            HC_LOG_ERROR("failed to set default ICMP6 checksum! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        }

        return true;
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::add_ipv6_extension_header(const unsigned char* buf, unsigned int buf_size) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        HC_LOG_ERROR("this funktion is only available vor IPv6 sockets ");
        return false;
    } else if (m_addrFamily == AF_INET6) {
        int rc = setsockopt(m_sock, IPPROTO_IPV6, IPV6_HOPOPTS, buf, buf_size);

        if (rc == -1) {
            HC_LOG_ERROR("failed to add extension header! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::set_ipv4_receive_packets_with_router_alert_header(bool enable) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        int one = enable ? 1 : 0;
        int rc = setsockopt(m_sock, IPPROTO_IP, IP_ROUTER_ALERT, &one, sizeof(one));

        if (rc == -1) {
            HC_LOG_ERROR("failed to add router alert header! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::set_ipv6_recv_icmpv6_msg() const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        HC_LOG_ERROR("this funktion is only available vor IPv6 sockets ");
        return false;
    } else if (m_addrFamily == AF_INET6) {
        struct icmp6_filter myfilter;

        //ICMP6_FILTER_SETPASSALL(&myfilter);
        ICMP6_FILTER_SETBLOCKALL(&myfilter);
        ICMP6_FILTER_SETPASS(MLD_LISTENER_REPORT, &myfilter);
        ICMP6_FILTER_SETPASS(MLD_LISTENER_REDUCTION, &myfilter);
        ICMP6_FILTER_SETPASS(MLD_V2_LISTENER_REPORT, &myfilter);

        if (setsockopt(m_sock, IPPROTO_ICMPV6, ICMP6_FILTER, &myfilter, sizeof(myfilter)) < 0) {
            HC_LOG_ERROR("failed to set ICMP6 filter! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        }

        return true;
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::set_ipv6_recv_pkt_info() const
{
    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        HC_LOG_ERROR("this funktion is only available vor IPv6 sockets ");
        return false;
    } else if (m_addrFamily == AF_INET6) {
        int on = 1;

        if (setsockopt(m_sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) < 0) {
            HC_LOG_ERROR("failed to set IPV6_RECVPKTINFO! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        }

        return true;
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::set_ipv6_recv_hop_by_hop_msg() const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    if (m_addrFamily == AF_INET) {
        HC_LOG_ERROR("this funktion is only available vor IPv6 sockets ");
        return false;
    } else if (m_addrFamily == AF_INET6) {
        int on = 1;
        if (setsockopt(m_sock, IPPROTO_IPV6, IPV6_RECVHOPOPTS, &on, sizeof(on)) < 0) {
            HC_LOG_ERROR("failed to set IPV6_RECVHOPOPTS! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        }

        return true;
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::set_mrt_flag(bool enable) const
{
    HC_LOG_TRACE("enable: " << enable);

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;
    int proto;
    int mrt_cmd;

    if (enable) {
        if (m_addrFamily == AF_INET) {
            proto = IPPROTO_IP;
            mrt_cmd = MRT_INIT;
        } else if (m_addrFamily == AF_INET6) {
            proto = IPPROTO_IPV6;
            mrt_cmd = MRT6_INIT;
        } else {
            HC_LOG_ERROR("wrong address family");
            return false;
        }

        int val = 1;
        rc = setsockopt(m_sock, proto, mrt_cmd, (void*)&val, sizeof(val));

        if (rc == -1) {
            HC_LOG_ERROR("failed to set MRT flag! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        if (m_addrFamily == AF_INET) {
            proto = IPPROTO_IP;
            mrt_cmd = MRT_DONE;
        } else if (m_addrFamily == AF_INET6) {
            proto = IPPROTO_IPV6;
            mrt_cmd = MRT6_DONE;
        } else {
            HC_LOG_ERROR("wrong address family");
            return false;
        }

        rc = setsockopt(m_sock, proto, mrt_cmd, nullptr, 0);

        if (rc == -1) {
            HC_LOG_ERROR("failed to reset MRT flag! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    }
}

//vifNum musst the same uniqueName  on delVIF (0 > vifNum < MAXVIF ==32)
//iff_register = true if used for PIM Register encap/decap
bool mroute_socket::add_vif(int vifNum, uint32_t if_index, const addr_storage& ip_tunnel_remote_addr) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        struct vifctl vc;

        //VIFF_TUNNEL   /* vif represents a tunnel end-point */
        //VIFF_SRCRT    /* tunnel uses IP src routing */
        //VIFF_REGISTER /* used for PIM Register encap/decap */
        unsigned char flags;
        flags = VIFF_USE_IFINDEX;

        memset(&vc, 0, sizeof(vc));
        vc.vifc_vifi = vifNum;
        vc.vifc_flags = flags;
        vc.vifc_threshold = MROUTE_TTL_THRESHOLD;
        vc.vifc_rate_limit = MROUTE_RATE_LIMIT_ENDLESS;
        vc.vifc_lcl_ifindex = if_index;

        if (ip_tunnel_remote_addr.is_valid()) {
            vc.vifc_rmt_addr = ip_tunnel_remote_addr.get_in_addr();
        }

        rc = setsockopt(m_sock, IPPROTO_IP, MRT_ADD_VIF, (void *)&vc, sizeof(vc));
        if (rc == -1) {
            HC_LOG_ERROR("failed to add VIF! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else if (m_addrFamily == AF_INET6) {
        struct mif6ctl mc;

        unsigned char flags;
        flags = 0;

        memset(&mc, 0, sizeof(mc));
        mc.mif6c_mifi = vifNum;
        mc.mif6c_flags = flags;
        mc.vifc_rate_limit = MROUTE_RATE_LIMIT_ENDLESS;
        mc.vifc_threshold = MROUTE_TTL_THRESHOLD;
        mc.mif6c_pifi = if_index;

        rc = setsockopt(m_sock, IPPROTO_IPV6, MRT6_ADD_MIF, (void *)&mc, sizeof(mc));
        if (rc == -1) {
            HC_LOG_ERROR("failed to add VIF! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::del_vif(int vif_index) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        struct vifctl vifc;
        memset(&vifc, 0, sizeof(vifc));

        vifc.vifc_vifi = vif_index;
        rc = setsockopt(m_sock, IPPROTO_IP, MRT_DEL_VIF, (char *)&vifc, sizeof(vifc));
        if (rc == -1) {
            HC_LOG_ERROR("failed to del VIF! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else if (m_addrFamily == AF_INET6) {
        struct mif6ctl mc;
        memset(&mc, 0, sizeof(mc));

        mc.mif6c_mifi = vif_index;
        rc = setsockopt(m_sock, IPPROTO_IPV6, MRT6_DEL_MIF, (char *)&mc, sizeof(mc));
        if (rc == -1) {
            HC_LOG_ERROR("failed to del VIF! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
}

bool mroute_socket::bind_vif_to_table(uint32_t if_index, int table) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    std::ostringstream oss_in;
    std::ostringstream oss_out;

    std::string cmd;
    if (m_addrFamily == AF_INET) {
        cmd = "ip mrule";
    } else if (m_addrFamily == AF_INET6) {
        cmd = "ip -6 mrule";
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    char if_name[IF_NAMESIZE];
    if (if_indextoname(if_index, if_name) == nullptr) {
        HC_LOG_ERROR("failed to convert if_index to name! Error: " << strerror(errno) << " errno: " << errno);
    }

    oss_in << cmd << " add iif " << if_name << " lookup " << table;
    oss_out << cmd << " add oif " << if_name << " lookup " << table;

    if (system(oss_in.str().c_str()) != 0 ) {
        HC_LOG_ERROR("failed to bind vif to table! Error on cmd: " << oss_in.str());
        return false;
    }

    if (system(oss_out.str().c_str()) != 0 ) {
        HC_LOG_ERROR("failed to bind vif to table! Error on cmd: " << oss_out.str());
        return false;
    }

    return true;
}


bool mroute_socket::unbind_vif_form_table(uint32_t if_index, int table) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    std::ostringstream oss_in;
    std::ostringstream oss_out;

    std::string cmd;
    if (m_addrFamily == AF_INET) {
        cmd = "ip mrule";
    } else if (m_addrFamily == AF_INET6) {
        cmd = "ip -6 mrule";
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    char if_name[IF_NAMESIZE];
    if (if_indextoname(if_index, if_name) == nullptr) {
        HC_LOG_ERROR("failed to convert if_index to name! Error: " << strerror(errno) << " errno: " << errno);
    }

    oss_in << cmd << " del iif " << if_name << " lookup " << table;
    oss_out << cmd << " del oif " << if_name << " lookup " << table;

    if (system(oss_in.str().c_str()) != 0 ) {
        HC_LOG_ERROR("failed to unbind vif from table! Error on cmd: " << oss_in.str());
        return false;
    }

    if (system(oss_out.str().c_str()) != 0 ) {
        HC_LOG_ERROR("failed to unbind vif from table! Error on cmd: " << oss_out.str());
        return false;
    }

    return true;
}


//source_addr is the source address of the received multicast packet
//group_addr group address of the received multicast packet
bool mroute_socket::add_mroute(int vif_index, const addr_storage& source_addr, const addr_storage& group_addr, const std::list<int>& output_vif) const
{

//unsigned int* output_vifTTL, unsigned int output_vifTTL_Ncount){

    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        struct mfcctl mc;
        memset(&mc, 0, sizeof(mc));

        mc.mfcc_origin = source_addr.get_in_addr();
        mc.mfcc_mcastgrp = group_addr.get_in_addr();
        mc.mfcc_parent = vif_index;

        if (output_vif.size() > MAXVIFS) {
            HC_LOG_ERROR("output_vifNum_size to large: " << output_vif.size());
            return false;
        }

        for (auto e : output_vif) {
            mc.mfcc_ttls[e] = MROUTE_DEFAULT_TTL;
        }

        rc = setsockopt(m_sock, IPPROTO_IP, MRT_ADD_MFC, (void *)&mc, sizeof(mc));
        if (rc == -1) {
            HC_LOG_ERROR("failed to add multicast route! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else if (m_addrFamily == AF_INET6) {
        struct mf6cctl mc;
        memset(&mc, 0, sizeof(mc));

        mc.mf6cc_origin.sin6_addr = source_addr.get_in6_addr();
        mc.mf6cc_mcastgrp.sin6_addr = group_addr.get_in6_addr();
        mc.mf6cc_parent = vif_index;

        if (output_vif.size() > MAXMIFS) {
            HC_LOG_ERROR("output_vifNum_size to large: " << output_vif.size());
            return false;
        }

        for (auto e : output_vif) {
            IF_SET(e, &mc.mf6cc_ifset);
        }

        rc = setsockopt(m_sock, IPPROTO_IPV6, MRT6_ADD_MFC, (void*)&mc, sizeof(mc));
        if (rc == -1) {
            HC_LOG_ERROR("failed to add multicast route! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

}

bool mroute_socket::del_mroute(int vif_index, const addr_storage& source_addr, const addr_storage& group_addr) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        struct mfcctl mc;
        memset(&mc, 0, sizeof(mc));

        mc.mfcc_origin = source_addr.get_in_addr();
        mc.mfcc_mcastgrp = group_addr.get_in_addr();
        mc.mfcc_parent = vif_index;

        rc = setsockopt(m_sock, IPPROTO_IP, MRT_DEL_MFC, (void *)&mc, sizeof(mc));
        if (rc == -1) {
            HC_LOG_WARN("failed to delete multicast route! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else if (m_addrFamily == AF_INET6) {
        struct mf6cctl mc;
        memset(&mc, 0, sizeof(mc));

        mc.mf6cc_origin.sin6_addr = source_addr.get_in6_addr();
        mc.mf6cc_mcastgrp.sin6_addr = group_addr.get_in6_addr();
        mc.mf6cc_parent = vif_index;

        rc = setsockopt(m_sock, IPPROTO_IPV6, MRT6_DEL_MFC, (void *)&mc, sizeof(mc));
        if (rc == -1) {
            HC_LOG_WARN("failed to delete multicast route! Error: " << strerror(errno) << " errno: " << errno);
            return false;
        } else {
            return true;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
    return false;
}

bool mroute_socket::get_vif_stats(int vif_index, struct sioc_vif_req* req_v4, struct sioc_mif_req6* req_v6) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        if (req_v4 != nullptr) {

            req_v4->vifi = vif_index;

            rc = ioctl(m_sock, SIOCGETVIFCNT, req_v4);
            if (rc == -1) {
                HC_LOG_ERROR("failed to get vif stats! Error: " << strerror(errno) << " errno: " << errno);
                return false;
            } else {
                return true;
            }
        } else {
            HC_LOG_ERROR("failed to get vif stats! Error: claimed parameter req_v4 is null");
            return false;
        }
    } else if (m_addrFamily == AF_INET6) {
        if (req_v6 != nullptr) {

            req_v6->mifi = vif_index;

            rc = ioctl(m_sock, SIOCGETMIFCNT_IN6, req_v6);
            if (rc == -1) {
                HC_LOG_ERROR("failed to get vif stats! Error: " << strerror(errno) << " errno: " << errno);
                return false;
            } else {
                return true;
            }
        } else {
            HC_LOG_ERROR("failed to get vif stats! Error: claimed parameter req_v6 is null");
            return false;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
    return false;
}

bool mroute_socket::get_mroute_stats(const addr_storage& source_addr, const addr_storage& group_addr, struct sioc_sg_req* sgreq_v4, struct sioc_sg_req6* sgreq_v6) const
{
    HC_LOG_TRACE("");

    if (!is_udp_valid()) {
        HC_LOG_ERROR("raw_socket invalid");
        return false;
    }

    int rc;

    if (m_addrFamily == AF_INET) {
        if (sgreq_v4 != nullptr) {
            memset(sgreq_v4, 0, sizeof(struct sioc_sg_req));

            sgreq_v4->src = source_addr.get_in_addr();
            sgreq_v4->grp = group_addr.get_in_addr();

            rc = ioctl(m_sock, SIOCGETSGCNT, sgreq_v4);;
            if (rc == -1) {
                HC_LOG_ERROR("failed to get multicast route stats! Error: " << strerror(errno) << " errno: " << errno);
                return false;
            } else {
                return true;
            }

        } else {
            HC_LOG_ERROR("failed to get multicast route stats! Error: claimed parameter sgreq_v4 is null");
            return false;
        }
    } else if (m_addrFamily == AF_INET6) {
        if (sgreq_v6 != nullptr) {
            memset(sgreq_v6, 0, sizeof(struct sioc_sg_req6));

            sgreq_v6->src.sin6_addr = source_addr.get_in6_addr();
            sgreq_v6->grp.sin6_addr = group_addr.get_in6_addr();

            rc = ioctl(m_sock, SIOCGETSGCNT_IN6, sgreq_v6);
            if (rc == -1) {
                HC_LOG_ERROR("failed to get multicast route stats! Error: " << strerror(errno) << " errno: " << errno);
                return false;
            } else {
                return true;
            }
        } else {
            HC_LOG_ERROR("failed to get multicast route stats! Error: claimed parameter sgreq_v6 is null");
            return false;
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }
    return false;
}

#ifdef DEBUG_MODE
void mroute_socket::print_vif_stats(mroute_socket* m, int vif_index)
{
    using namespace std;
    HC_LOG_TRACE("");

    cout << "##-- vif stats --##" << endl;
    cout << " -vif_index:" << vif_index << endl;

    if (m->get_addr_family() == AF_INET) {
        sioc_vif_req req;
        if (!m->get_vif_stats(vif_index, &req, nullptr)) {
            cout << "failed to get vif stats" << endl;
            return;
        }

        cout << " -In packets[" << req.ibytes << " bytes]:" << req.icount << endl;
        cout << " -Out packets[" << req.obytes << " bytes]:" << req.ocount << endl;
    } else if (m->get_addr_family() == AF_INET6) {
        struct sioc_mif_req6 req;
        if (!m->get_vif_stats(vif_index, nullptr, &req)) {
            cout << "failed to get vif stats" << endl;
            return;
        }

        cout << " -In packets[" << req.ibytes << " bytes]:" << req.icount << endl;
        cout << " -Out packets[" << req.obytes << " bytes]:" << req.ocount << endl;
    } else {
        HC_LOG_ERROR("wrong address family");
        cout << "wrong address family" << endl;
    }
}

void mroute_socket::print_mroute_stats(mroute_socket* m, const addr_storage& source_addr, const addr_storage& group_addr)
{
    using namespace std;
    HC_LOG_TRACE("");

    cout << "##-- mroute states --##" << endl;
    cout << " -src: " << source_addr << " to grp: " << group_addr << endl;

    if (m->get_addr_family() == AF_INET) {
        struct sioc_sg_req req;
        if (!m->get_mroute_stats(source_addr, group_addr, &req, nullptr)) {
            cout << "failed to get mroute stats" << endl;
            return;
        }

        cout << " -packets[" << req.bytecnt << " bytes]:" << req.pktcnt << endl;
        cout << " -wrong packets:" << req.wrong_if << endl;
    } else if (m->get_addr_family() == AF_INET6) {
        struct sioc_sg_req6 req;
        if (!m->get_mroute_stats(source_addr, group_addr, nullptr, &req)) {
            cout << "failed to get mroute stats" << endl;
            return;
        }

        cout << " -packets[" << req.bytecnt << " bytes]:" << req.pktcnt << endl;
        cout << " -wrong packets:" << req.wrong_if << endl;
    } else {
        HC_LOG_ERROR("wrong address family");
        cout << "wrong address family" << endl;
    }
}

void mroute_socket::print_struct_mf6cctl(struct mf6cctl* mc)
{
    using namespace std;
    HC_LOG_TRACE("");

    char addressBuffer[INET6_ADDRSTRLEN];

    cout << "##-- mf6cctl --##" << endl;
    cout << " -mf6cc_parent: " << mc->mf6cc_parent << endl;
    cout << " -mcastgrp: " << inet_ntop(AF_INET6, &mc->mf6cc_mcastgrp.sin6_addr, addressBuffer,
                                        sizeof(addressBuffer)) << endl;
    cout << " -mcastorigin: " << inet_ntop(AF_INET6, &mc->mf6cc_origin.sin6_addr, addressBuffer,
                                           sizeof(addressBuffer)) << endl;

    cout << " -mf6cc_ifset: ";
    for (int i = 0; i < MAXMIFS; i++) {
        if (IF_ISSET(i, &mc->mf6cc_ifset)) {
            cout << i << "; ";
        }
    }
    cout << endl;
}

void mroute_socket::test_mcrouter_mrt_flag()
{
    HC_LOG_TRACE("");
    using namespace std;
    int sleepTime = 1;
    mroute_socket m;
    mroute_socket m1;

    cout << "--<1> set MRT flag ipv4 --" << endl;
    m.create_raw_ipv4_socket();
    if (m.set_mrt_flag(true)) {
        cout << "set OK!" << endl;
    } else {
        cout << "set FAILED!" << endl;
    }

    sleep(sleepTime);

    cout << "-- reset MRT flag ipv4 --" << endl;
    if (m.set_mrt_flag(false)) {
        cout << "reset OK!" << endl;
    } else {
        cout << "reset FAILED!" << endl;
    }

    sleep(sleepTime);

    cout << "-- set MRT flag ipv4 again --" << endl;
    m.create_raw_ipv4_socket();
    if (m.set_mrt_flag(true)) {
        cout << "set OK!" << endl;
    } else {
        cout << "set FAILED!" << endl;
    }

    cout << "--<2> set MRT flag ipv6 --" << endl;
    m.create_raw_ipv6_socket();
    if (m.set_mrt_flag(true)) {
        cout << "set OK!" << endl;
    } else {
        cout << "set FAILED!" << endl;
    }

    sleep(sleepTime);

    cout << "-- reset MRT flag ipv6 --" << endl;
    if (m.set_mrt_flag(false)) {
        cout << "reset OK!" << endl;
    } else {
        cout << "reset FAILED!" << endl;
    }

    sleep(sleepTime);

    cout << "--<3> error test set 2x MRT flag ipv4 --" << endl;
    m.create_raw_ipv4_socket();
    m1.create_raw_ipv4_socket();
    if (m.set_mrt_flag(true)) {
        cout << "set 1 OK!" << endl;
    } else {
        cout << "set 1 FAILED!" << endl;
    }

    if (m1.set_mrt_flag(true)) {
        cout << "set 2 OK! ==> FAILED!" << endl;
    } else {
        cout << "set 2 FAILED! ==> OK!" << endl;
    }

    sleep(sleepTime);

    if (m.set_mrt_flag(false)) {
        cout << "reset 1 OK!" << endl;
    } else {
        cout << "reset 1 FAILED!" << endl;
    }

    if (m1.set_mrt_flag(false)) {
        cout << "reset 2 OK! ==> FAILED!" << endl;
    } else {
        cout << "reset 2 FAILED! ==> OK!" << endl;
    }

    sleep(sleepTime);

    cout << "--<4> error test set 2x MRT flag ipv6 --" << endl;
    m.create_raw_ipv6_socket();
    m1.create_raw_ipv6_socket();
    if (m.set_mrt_flag(true)) {
        cout << "set 1 OK!" << endl;
    } else {
        cout << "set 1 FAILED!" << endl;
    }

    if (m1.set_mrt_flag(true)) {
        cout << "set 2 OK! ==> FAILED" << endl;
    } else {
        cout << "set 2 FAILED! ==> OK!" << endl;
    }

    sleep(sleepTime);

    if (m.set_mrt_flag(false)) {
        cout << "reset 1 OK!" << endl;
    } else {
        cout << "reset 1 FAILED!" << endl;
    }

    if (m1.set_mrt_flag(false)) {
        cout << "reset 2 OK! ==> FAILED!" << endl;
    } else {
        cout << "reset 2 FAILED! ==> OK!" << endl;
    }

    sleep(sleepTime);

    cout << "--<5> error test set 2x MRT flag ipv4&ipv6 --" << endl;
    m.create_raw_ipv4_socket();
    m1.create_raw_ipv6_socket();
    if (m.set_mrt_flag(true)) {
        cout << "set 1 OK!" << endl;
    } else {
        cout << "set 1 FAILED!" << endl;
    }

    if (m1.set_mrt_flag(true)) {
        cout << "set 2 OK!" << endl;
    } else {
        cout << "set 2 FAILED!" << endl;
    }

    sleep(sleepTime);

    if (m.set_mrt_flag(false)) {
        cout << "reset 1 OK!" << endl;
    } else {
        cout << "reset 1 FAILED!" << endl;
    }

    if (m1.set_mrt_flag(false)) {
        cout << "reset 2 OK!" << endl;
    } else {
        cout << "reset 2 FAILED!" << endl;
    }
}

void mroute_socket::test_add_vifs(mroute_socket* m)
{
    using namespace std;
    HC_LOG_TRACE("");

    int if_one = MROUTE_SOCKET_IF_NUM_ONE;
    string str_if_one = MROUTE_SOCKET_IF_STR_ONE;
    int if_two = MROUTE_SOCKET_IF_NUM_TWO;
    string str_if_two = MROUTE_SOCKET_IF_STR_TWO;

    cout << "-- addVIFs test --" << endl;
    if (m->add_vif(if_one, if_nametoindex(str_if_one.c_str()), addr_storage())) {
        cout << "addVIF " << str_if_one << " OK!" << endl;
    } else {
        cout << "addVIF " << str_if_one << " FAILED!" << endl;
    }

    if (m->add_vif(if_two, if_nametoindex(str_if_two.c_str()), addr_storage())) {
        cout << "addVIF " << str_if_two << " OK!" << endl;
    } else {
        //cout << "addVIF " << str_if_two << " FAILED!" << end;
    }

    /*if(m->addVIF(if_three, str_if_three.c_str(),false,false,false,NULL)){
          cout << "addVIF " << str_if_three << " OK!" << endl;
     }else{
          cout << "addVIF " << str_if_three << " FAILED!" << endl;
     }*/
}


void mroute_socket::test_del_vifs(mroute_socket* m)
{
    using namespace std;
    HC_LOG_TRACE("");

    int if_one = MROUTE_SOCKET_IF_NUM_ONE;
    int if_two = MROUTE_SOCKET_IF_NUM_TWO;

    cout << "-- delVIFs test--" << endl;
    if (m->del_vif(if_one)) {
        cout << "delVIF OK!" << endl;
    } else {
        cout << "delVIF FAILED!" << endl;
    }

    if (m->del_vif(if_two)) {
        cout << "delVIF OK!" << endl;
    } else {
        cout << "delVIF FAILED!" << endl;
    }
}

void mroute_socket::test_add_route(mroute_socket* m)
{
    using namespace std;
    HC_LOG_TRACE("");

    addr_storage src_addr;
    addr_storage g_addr;
    const int if_one = MROUTE_SOCKET_IF_NUM_ONE;
    const string str_if_one = MROUTE_SOCKET_IF_STR_ONE;
    const unsigned int if_two = MROUTE_SOCKET_IF_NUM_TWO;
    const string str_if_two = MROUTE_SOCKET_IF_STR_TWO;

    //int if_three = MROUTE_SOCKET_IF_NUM_THREE;

    if (m->get_addr_family() == AF_INET) {
        src_addr = MROUTE_SOCKET_SRC_ADDR_V4;
        g_addr = MROUTE_SOCKET_G_ADDR_V4;
    } else if (m->get_addr_family() == AF_INET6) {
        src_addr =  MROUTE_SOCKET_SRC_ADDR_V6;
        g_addr = MROUTE_SOCKET_G_ADDR_V6;
    } else {
        cout << "FAILED to start test wrong addrFamily: " << m->get_addr_family() << endl;
        return;
    }

    cout << "-- addRoute test --" << endl;
    //unsigned int output_vifs[]={[>if_three,<] if_two}; //if_two
    list<int> output_vifs = { if_two };
    if (m->add_mroute(if_one, src_addr, g_addr , output_vifs)) {
        cout << "addRoute (" << str_if_one << " ==> " << str_if_two << ") OK!" << endl;
    } else {
        cout << "addRoute (" << str_if_one << " ==> " << str_if_two << ") FAILED!" << endl;
    }
}

void mroute_socket::test_del_route(mroute_socket* m)
{
    using namespace std;
    HC_LOG_TRACE("");

    addr_storage src_addr;
    addr_storage g_addr;
    int if_one = MROUTE_SOCKET_IF_NUM_ONE;
    string str_if_one = MROUTE_SOCKET_IF_STR_ONE;
    string str_if_two = MROUTE_SOCKET_IF_STR_TWO;

    if (m->get_addr_family() == AF_INET) {
        src_addr = MROUTE_SOCKET_SRC_ADDR_V4;
        g_addr = MROUTE_SOCKET_G_ADDR_V4;
    } else if (m->get_addr_family() == AF_INET6) {
        src_addr =  MROUTE_SOCKET_SRC_ADDR_V6;
        g_addr = MROUTE_SOCKET_G_ADDR_V6;
    } else {
        cout << "FAILED to start test wrong addrFamily: " << m->get_addr_family() << endl;
        return;
    }

    cout << "-- delRoute test --" << endl;
    if (m->del_mroute(if_one, src_addr, g_addr)) {
        cout << "delMRoute (" << str_if_one << " ==> " << str_if_two << ") OK!" << endl;
    } else {
        cout << "delMRoute (" << str_if_one << " ==> " << str_if_two << ") FAILED!" << endl;
    }
}

void mroute_socket::test_mcrouter_vifs_routes(int addrFamily)
{
    using namespace std;
    HC_LOG_TRACE("");

    mroute_socket m;

    int sleepTime = 1;

    if (addrFamily == AF_INET) {
        m.create_raw_ipv4_socket();
    } else if (addrFamily == AF_INET6) {
        m.create_raw_ipv6_socket();
    } else {
        cout << "FAILED to start test wrong addrFamily: " << addrFamily << endl;
        return;
    }

    cout << "-- set mrt flag --" << endl;
    if (m.set_mrt_flag(true)) {
        cout << "set MRT flag OK!" << endl;
    } else {
        cout << "set MRT flag FAILED!" << endl;
    }

    m.test_add_vifs(&m);

    sleep(sleepTime);

    m.test_add_route(&m);

    m.test_del_route(&m);

    m.test_del_vifs(&m);

    cout << "-- reset mrt flag --" << endl;
    if (m.set_mrt_flag(false)) {
        cout << "reset MRT flag OK!" << endl;
    } else {
        cout << "reset MRT flag FAILED!" << endl;
    }

    sleep(sleepTime);
}

void mroute_socket::quick_test()
{
//https://github.com/torvalds/linux/blob/b3a3a9c441e2c8f6b6760de9331023a7906a4ac6/drivers/net/dummy.c
//
    auto assert = [](bool test, std::string s) {
        if (test) {
            std::cout << "process: " << s << std::endl;
        } else {
            std::cout << "error by: " << s << std::endl;
            exit(0);
        }
    };

    mroute_socket ma;
    mroute_socket mb;

    assert(ma.create_raw_ipv6_socket(), "ma.create_raw_ipv6_socket()");
    assert(mb.create_raw_ipv6_socket(), "mb.create_raw_ipv6_socket()");

    assert(ma.set_kernel_table(1), "ma.set_kernel_table(1)");
    assert(mb.set_kernel_table(2), "mb.set_kernel_table(2)");

    assert(ma.set_mrt_flag(true), "ma.set_mrt_flag(true)");
    assert(mb.set_mrt_flag(true), "mb.set_mrt_flag(true)");

    assert(ma.add_vif(1, if_nametoindex("dummy1"), addr_storage()), "mb.add_vif(1, if_nametoindex(dummy1), addr_storage())");
    assert(ma.add_vif(2, if_nametoindex("eth0"), addr_storage()), "mb.add_vif(2, if_nametoindex(eth0), addr_storage())");
    assert(ma.bind_vif_to_table(if_nametoindex("dummy1"), 1), "ma.bind_vif_to_table(if_nametoindex(dummy1), 1)");
    assert(ma.bind_vif_to_table(if_nametoindex("eth0"), 1), "ma.bind_vif_to_table(if_nametoindex(eth0), 1)");

    assert(mb.add_vif(1, if_nametoindex("eth4"), addr_storage()), "ma.add_vif(1, if_nametoindex(eth4), addr_storage())");
    assert(mb.add_vif(2, if_nametoindex("eth0"), addr_storage()), "ma.add_vif(2, if_nametoindex(eth0), addr_storage())");
    assert(ma.bind_vif_to_table(if_nametoindex("eth4"), 2), "ma.bind_vif_to_table(if_nametoindex(eth4), 2)");
    assert(ma.bind_vif_to_table(if_nametoindex("eth0"), 2), "ma.bind_vif_to_table(if_nametoindex(eth0), 2)");

    assert(ma.add_mroute(2, addr_storage("fd00::38"), addr_storage("ff05::99:99"), {1}), "mb.add_mroute(2, addr_storage(fd00::38), addr_storage(ff05::99:99), {1})");
    assert(mb.add_mroute(1, addr_storage("fd00::38"), addr_storage("ff05::99:99"), {2}), "ma.add_mroute(1, addr_storage(fd00::38), addr_storage(ff05::99:99), {2})");
    std::cout << "eth4(t1,v1) ==> eth0(t1,v2) ==> eth0(t2,v2) ==> dummy1(t2,v1)" << std::endl;

    while (true) {
        print_mroute_stats(&ma, addr_storage("fd00::38"), addr_storage("ff05::99:99"));
        print_mroute_stats(&mb, addr_storage("fd00::38"), addr_storage("ff05::99:99"));
        sleep(2);
    }

    //assert(ma.create_raw_ipv4_socket(), "ma.create_raw_ipv4_socket()");
    //assert(mb.create_raw_ipv4_socket(), "mb.create_raw_ipv4_socket()");

    //assert(ma.set_kernel_table(1), "ma.set_kernel_table(1)");
    //assert(mb.set_kernel_table(2), "mb.set_kernel_table(2)");

    //assert(ma.set_mrt_flag(true), "ma.set_mrt_flag(true)");
    //assert(mb.set_mrt_flag(true), "mb.set_mrt_flag(true)");

    //assert(ma.add_vif(1, if_nametoindex("eth4"), addr_storage()), "ma.add_vif(1, if_nametoindex(eth4), addr_storage())");
    //assert(ma.add_vif(2, if_nametoindex("eth0"), addr_storage()), "ma.add_vif(2, if_nametoindex(eth0), addr_storage())");

    //assert(mb.add_vif(1, if_nametoindex("dummy1"), addr_storage()), "mb.add_vif(1, if_nametoindex(dummy1), addr_storage())");
    //assert(mb.add_vif(2, if_nametoindex("eth0"), addr_storage()), "mb.add_vif(2, if_nametoindex(eth0), addr_storage())");

    //assert(ma.add_mroute(1, addr_storage("192.168.0.38"), addr_storage("239.99.99.99"), {2}), "ma.add_mroute(1, addr_storage(192.168.0.38), addr_storage(239.99.99.99), {2})");
    //assert(mb.add_mroute(2, addr_storage("192.168.0.38"), addr_storage("239.99.99.99"), {1}), "mb.add_mroute(1, addr_storage(192.168.0.38),  addr_storage(239.99.99.99), {2})");
    //std::cout << "eth4(t1,v1) ==> eth0(t1,v2) ==> eth0(t2,v2) ==> dummy1(t2,v1)" << std::endl;

    //while (true) {
    //ma.print_mroute_stats(addr_storage("192.168.0.38"), addr_storage("239.99.99.99"));
    //mb.print_mroute_stats(addr_storage("192.168.0.38"), addr_storage("239.99.99.99"));
    //sleep(2);
    //}
}
#endif /* DEBUG_MODE */

mroute_socket::~mroute_socket()
{
    HC_LOG_TRACE("");
}
