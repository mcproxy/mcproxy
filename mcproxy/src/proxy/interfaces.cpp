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
#include "include/proxy/interfaces.hpp"

#include <net/if.h>
#include <vector>

interfaces::interfaces(int addr_family): m_addr_family(addr_family)
{
    HC_LOG_TRACE("");
    if(!m_if_prop.refresh_network_interfaces()){
        throw "failed to refresh network interfaces";        
    }
}

interfaces::~interfaces()
{
    HC_LOG_TRACE("");

}

bool interfaces::add_interface(const std::string& if_name)
{
    HC_LOG_TRACE("");
    return add_interface(get_if_index(if_name));
}

bool interfaces::add_interface(unsigned int if_index)
{
    HC_LOG_TRACE("");
    int free_vif =  get_free_vif_number();    
    if(free_vif > INTERFACES_UNKOWN_VIF_INDEX){
        auto rc_vif_if = m_vif_if.insert(std::pair<int, int>(free_vif, if_index)); 
        if(!rc_vif_if.second){
            return false; 
        }

        auto rc_if_vif = m_if_vif.insert(std::pair<int, int>(if_index, free_vif)); 
        if(!rc_if_vif.second){
            HC_LOG_ERROR("inconsistent database");
            m_vif_if.erase(rc_vif_if.first);
            return false;
        }
        return true;
    }else{
        return false;
    }
}

bool interfaces::del_interface(const std::string& if_name)
{
    HC_LOG_TRACE("");
    return del_interface(get_if_index(if_name));
}

bool interfaces::del_interface(unsigned int if_index)
{
    HC_LOG_TRACE("");
    if(if_index != INTERFACES_UNKOWN_IF_INDEX){
       int vif = get_virtual_if_index(if_index);                       
       m_vif_if.erase(vif);
       m_if_vif.erase(if_index);
       return true;
    }else{
        return false; 
    }
}

bool interfaces::refresh_network_interfaces()
{
    HC_LOG_TRACE("");
    return m_if_prop.refresh_network_interfaces();
}

unsigned int interfaces::get_if_index(std::string if_name) const
{
    HC_LOG_TRACE("");
    return if_nametoindex(if_name.c_str());
}

unsigned int interfaces::get_if_index(int virtual_if_index) const
{
    HC_LOG_TRACE("");
    auto rc = m_vif_if.find(virtual_if_index);      
    if(rc != end(m_vif_if)){
        return rc->second; 
    }else{
        return INTERFACES_UNKOWN_IF_INDEX; 
    }
}

int interfaces::get_virtual_if_index(unsigned int if_index) const
{
    auto rc = m_if_vif.find(if_index);
    if(rc != end(m_if_vif)){
        return rc->second;
    }else{
        return INTERFACES_UNKOWN_VIF_INDEX; 
    }
    HC_LOG_TRACE("");

}

std::string interfaces::get_if_name(unsigned int if_index) const
{
    HC_LOG_TRACE("");
    char tmp[IF_NAMESIZE];
    const char* if_name = if_indextoname(if_index, tmp);
    if(if_name == nullptr){
        return std::string();
    } else{
        return std::string(if_name); 
    }
     
}

unsigned int interfaces::get_if_index(const addr_storage& addr) const
{
    HC_LOG_TRACE("");

    addr_storage tmp_mask;
    addr_storage own_addr;

    const if_prop_map* prop_map; 

    if (addr.get_addr_family() == AF_INET) {
        prop_map = m_if_prop.get_if_props();
        for(auto& e: *prop_map){
            if(e.second.ip4_addr->ifa_netmask != nullptr && e.second.ip4_addr->ifa_addr != nullptr){
            tmp_mask = *e.second.ip4_addr->ifa_netmask;
            own_addr = *e.second.ip4_addr->ifa_addr; 
            own_addr.mask_ipv4(tmp_mask);
            if( own_addr == tmp_mask.mask_ipv4(addr)){
               return get_if_index(e)              
            }
        }
    }       










        for (it = m_if_proxy_map.begin(); it != m_if_proxy_map.end(); it++) {
            //maks own ip
            string if_name(if_indextoname(it->first, cstr));

            item = m_if_property.get_ip4_if(if_name);
            if (item == nullptr) {
                return 0;
            }

            own_addr = *(item->ifa_addr);
            tmp_mask = *(item->ifa_netmask);

            comp_addr = own_addr.mask_ipv4(tmp_mask);

            if (comp_addr == tmp_mask.mask_ipv4(src_addr)) {
                return it->first;
            }

        }
    } else {
        HC_LOG_ERROR("cannot map IPv6 addr to interface index:" << addr);
        return INTERFACES_UNKOWN_IF_INDEX;
    }

    return 0; //no interface found
}

int interfaces::get_free_vif_number() const
{
    HC_LOG_TRACE("");

    int vifs_elements;

    if (m_addr_family == AF_INET) {
        vifs_elements = MAXVIFS;
    } else if (m_addr_family == AF_INET6) {
        vifs_elements = MAXMIFS;
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return INTERFACES_UNKOWN_VIF_INDEX;
    }

    std::vector<int> vifs(vifs_elements, INTERFACES_UNKOWN_IF_INDEX); 

    //fill vif list
    for (auto iter = begin(m_if_vif); iter != end(m_if_vif); ++iter) {
        if (iter->second >= vifs_elements) {
            HC_LOG_ERROR("wrong vif index");
            return INTERFACES_UNKOWN_VIF_INDEX;
        }
        vifs[iter->second] = iter->first;
    }

    for (int i = 0; i < vifs_elements; i++) {
        if (vifs[i] == INTERFACES_UNKOWN_IF_INDEX ) {
            return i;
        }
    }

    HC_LOG_ERROR("no free vif number");
    return -1;
}
