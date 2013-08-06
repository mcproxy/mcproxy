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
#include "include/proxy/igmp_receiver.hpp"
#include "include/proxy/proxy_instance.hpp"

#include <net/if.h>
#include <linux/mroute.h>
#include <netinet/igmp.h>
#include <netinet/ip.h>



igmp_receiver::igmp_receiver(){
     HC_LOG_TRACE("");
}

int igmp_receiver::get_iov_min_size(){
     HC_LOG_TRACE("");
     int size_ip = sizeof(struct ip) + sizeof(struct igmp) + IGMP_RECEIVER_IPV4_ROUTER_ALERT_OPT_SIZE;
     int size_kernel = sizeof(struct igmpmsg);
     return (size_ip < size_kernel)? size_kernel : size_ip;
}

int igmp_receiver::get_ctrl_min_size(){
     HC_LOG_TRACE("");
     //useless
     return 0;
}

void igmp_receiver::analyse_packet(struct msghdr* msg, int){
     HC_LOG_TRACE("");

     struct ip* ip_hdr = (struct ip*)msg->msg_iov->iov_base;
     struct igmp* igmp_hdr = (struct igmp*) ((char*)msg->msg_iov->iov_base + ip_hdr->ip_hl*4);

     int if_index = 0;
     addr_storage g_addr;
     addr_storage src_addr;
     proxy_instance* pr_i;

     //test_output::printPacket_IPv4_Infos(buf, msg_size);
     HC_LOG_DEBUG("received packet XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");

     /* packets sent up from kernel to daemon have ip->ip_p = 0 */
     if(ip_hdr->ip_p == IGMP_RECEIVER_KERNEL_MSG){
          HC_LOG_DEBUG("kernel msg");
          struct igmpmsg *igmpctl;
          igmpctl = (struct igmpmsg *) msg->msg_iov->iov_base;

          switch (igmpctl->im_msgtype) {
          case IGMPMSG_NOCACHE: {
               src_addr = igmpctl->im_src;
               HC_LOG_DEBUG("\tsrc: " << src_addr);

               g_addr = igmpctl->im_dst;
               HC_LOG_DEBUG("\tgroup: " << g_addr);

               HC_LOG_DEBUG("\tvif: " << (int)igmpctl->im_vif);
               if((if_index = get_if_index(igmpctl->im_vif)) == 0) return;
               HC_LOG_DEBUG("\tif_index: " << if_index);

               if((pr_i = get_proxy_instance(if_index)) == nullptr) return;

               proxy_msg m;
               m.type = proxy_msg::RECEIVER_MSG;
               m.msg = new struct receiver_msg(receiver_msg::CACHE_MISS, if_index, src_addr, g_addr);
               pr_i->add_msg(m);
               break;
          }
          default:
               HC_LOG_WARN("unknown kernel message");
          }
     }else if(ip_hdr->ip_p == IPPROTO_IGMP && ntohs(ip_hdr->ip_len)==get_iov_min_size()){
          //test_output::printPaket_IPv4_IgmpInfos(buf);
          if(igmp_hdr->igmp_type == IGMP_V2_MEMBERSHIP_REPORT){
               HC_LOG_DEBUG("\tjoin");

               src_addr = ip_hdr->ip_src;
               HC_LOG_DEBUG("\tsrc: " << src_addr);

               g_addr = igmp_hdr->igmp_group;
               HC_LOG_DEBUG("\tgroup: " << g_addr);

               if((if_index = this->map_ip2if_index(src_addr)) == 0) return;
               HC_LOG_DEBUG("\tif_index: " << if_index);

               if((pr_i= this->get_proxy_instance(if_index))== nullptr) return;

               proxy_msg m;
               m.type = proxy_msg::RECEIVER_MSG;
               m.msg = new struct receiver_msg(receiver_msg::JOIN, if_index, g_addr);
               pr_i->add_msg(m);
          }else if(igmp_hdr->igmp_type == IGMP_V2_LEAVE_GROUP){
               HC_LOG_DEBUG("\tleave");

               src_addr = ip_hdr->ip_src;
               HC_LOG_DEBUG("\tsrc: " << src_addr);

               g_addr = igmp_hdr->igmp_group;
               HC_LOG_DEBUG("\tgroup: " << g_addr);

               if((if_index = this->map_ip2if_index(src_addr))== 0) return ;
               HC_LOG_DEBUG("\tif_index: " << if_index);

               if((pr_i=this->get_proxy_instance(if_index)) == nullptr) return;

               proxy_msg m;
               m.type = proxy_msg::RECEIVER_MSG;
               m.msg = new struct receiver_msg(receiver_msg::LEAVE, if_index, g_addr);
               pr_i->add_msg(m);
          }else{
               HC_LOG_DEBUG("unknown IGMP-packet");
               HC_LOG_DEBUG("type: " << igmp_hdr->igmp_type);
          }
     }else{
          HC_LOG_DEBUG("unknown IP-packet: " << ip_hdr->ip_p);
     }
}

int igmp_receiver::map_ip2if_index(const addr_storage& src_addr){
     HC_LOG_TRACE("");

     char cstr[IF_NAMESIZE];
     addr_storage tmp_mask;

     addr_storage own_addr;
     const struct ifaddrs* item;

     addr_storage comp_addr;

     if(src_addr.get_addr_family() == AF_INET){
          if_poxy_instance_map::iterator it;
          for(it= m_if_proxy_map.begin(); it != m_if_proxy_map.end(); it++){
               //maks own ip
               string if_name(if_indextoname(it->first, cstr));

               item = m_if_property.get_ip4_if(if_name);
               if(item == nullptr) return 0;

               own_addr = *(item->ifa_addr);
               tmp_mask = *(item->ifa_netmask);

               comp_addr = own_addr.mask(tmp_mask);

               if(comp_addr == tmp_mask.mask(src_addr)){
                    return it->first;
               }

          }
     }else{
          HC_LOG_ERROR("cannot map IPv6 addr to interface index:" << src_addr);
          return 0;
     }

     return 0; //no interface found
}

