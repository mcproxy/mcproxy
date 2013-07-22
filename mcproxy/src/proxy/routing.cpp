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
#include "include/proxy/routing.hpp"
#include "include/utils/addr_storage.hpp"

#include <net/if.h>
#include <linux/mroute.h>
#include <linux/mroute6.h>
#include <iostream>

bool routing::init(int addr_family, int version, mroute_socket* mrt_sock, bool single_instance, int table_number){
    HC_LOG_TRACE("");

    m_is_single_instance = single_instance;
    m_table_number = table_number;
    m_addr_family = addr_family;
    m_version = version;
    m_mrt_sock = mrt_sock;

    if(!init_if_prop()) return false;

    return false;
}

bool routing::init_if_prop(){
    HC_LOG_TRACE("");

    if(!m_if_prop.refresh_network_interfaces()) return false;

    return true;
}

bool routing::add_vif(int if_index, int vif){
    HC_LOG_TRACE("");

    char cstr[IF_NAMESIZE];
    const struct ifaddrs* item= nullptr;
    string if_name(if_indextoname(if_index,cstr));

    if(m_addr_family == AF_INET){
        if((item = m_if_prop.get_ip4_if(if_name))== nullptr){
            HC_LOG_ERROR("interface not found: " << if_name);
            return false;
        }
    }else if(m_addr_family == AF_INET6){
        if((item = m_if_prop.get_ip6_if(if_name)->front()) == nullptr){
            HC_LOG_ERROR("interface not found: " << if_name);
            return false;
        }
    }else{
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    if((item->ifa_flags & IFF_POINTOPOINT) && (item->ifa_dstaddr != nullptr)) { //tunnel

        addr_storage p2p_addr(*(item->ifa_dstaddr));

        if(!m_mrt_sock->add_vif(vif,if_name.c_str(),p2p_addr.to_string().c_str())){
            return false;
        }

    }else{ //phyint
        if(!m_mrt_sock->add_vif(vif,if_name.c_str(), nullptr)){
            return false;
        }

    }

    if(!m_is_single_instance){
        if(!m_mrt_sock->bind_vif_to_table(if_name.c_str(),m_table_number)){
            return false;
        }
    }

    HC_LOG_DEBUG("added interface: " << if_name << " to vif_table with vif number:" << vif);
    return true;
}

bool routing::add_route(int input_vif, const addr_storage& g_addr, const addr_storage& src_addr, const std::list<unsigned int>& output_vif){
    HC_LOG_TRACE("");

    if(m_addr_family == AF_INET){
        if(output_vif.size() > MAXVIFS) return false;
    }else if(m_addr_family == AF_INET6){
        if(output_vif.size() > MAXMIFS) return false;
    }else{
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    //list<int>::const_iterator iter_out;
    //unsigned int out_vif[output_vif.size()];

    //int i=0;
    //for(iter_out = output_vif.begin(); iter_out != output_vif.end(); iter_out++){
        //out_vif[i++] = *iter_out;
    //}

    if(!m_mrt_sock->add_mroute(input_vif, src_addr.to_string().c_str(), g_addr.to_string().c_str(), output_vif)){
        return false;
    }

    return true;
}

bool routing::del_route(int vif, const addr_storage& g_addr, const addr_storage& src_addr){
    HC_LOG_TRACE("");

    if(!m_mrt_sock->del_mroute(vif, src_addr.to_string().c_str(), g_addr.to_string().c_str())){
        return false;
    }

    return true;
}

bool routing::del_vif(int if_index, int vif){
    HC_LOG_TRACE("");

    if(!m_mrt_sock->del_vif(vif)){
        return false;
    }

    if(!m_is_single_instance){
        char cstr[IF_NAMESIZE];
        if_indextoname(if_index,cstr);

        if(!m_mrt_sock->unbind_vif_form_table(cstr,m_table_number)){
            return false;
        }
    }

    HC_LOG_DEBUG("removed interface with vif number: " << vif) ;
    return true;
}

