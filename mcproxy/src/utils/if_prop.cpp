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
#include "include/utils/if_prop.hpp"
#include "include/utils/addr_storage.hpp"

#include <cstring>
#include <errno.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <iostream>

if_prop::if_prop():
    m_if_addrs(0)
{
    HC_LOG_TRACE("");
}

bool if_prop::refresh_network_interfaces()
{
    HC_LOG_TRACE("");

    //clean
    if (is_getaddrs_valid()) {
        freeifaddrs(m_if_addrs);
    }

    m_if_map.clear();

    //create
    if (getifaddrs(&m_if_addrs) < 0) {
        HC_LOG_ERROR("getifaddrs failed! Error: " << strerror(errno) );
        return false;
    }

    struct ifaddrs* ifEntry = nullptr;
    for (ifEntry = m_if_addrs; ifEntry != nullptr; ifEntry = ifEntry->ifa_next) {
        if (ifEntry->ifa_addr == nullptr || ifEntry->ifa_addr->sa_data == nullptr) {
            continue;
        }

        if (ifEntry->ifa_addr->sa_family == AF_INET) {
            if_prop_map::iterator iter = m_if_map.find(ifEntry->ifa_name);
            if (iter != m_if_map.end()) { //existing interface
                if (iter->second.ip4_addr != nullptr) {
                    HC_LOG_WARN("more than one ipv4 address for one interface configurated! used:" << addr_storage(* (iter->second.ip4_addr->ifa_addr)) << "; don't used:" << addr_storage(*(ifEntry->ifa_addr)) << ";");
                    //return false;
                } else {
                    iter->second.ip4_addr = ifEntry;
                }
            } else { //new interface
                m_if_map.insert(if_prop_pair(ifEntry->ifa_name, ipv4_6_pair(ifEntry, std::list<const struct ifaddrs*>())));
            }
        } else if (ifEntry->ifa_addr->sa_family == AF_INET6) {
            if_prop_map::iterator iter = m_if_map.find(ifEntry->ifa_name);
            if (iter != m_if_map.end()) { //existing interface
                std::list<const struct ifaddrs*>* l = &iter->second.ip6_addr;
                l->push_back(ifEntry);
            } else { //new interface
                std::list<const struct ifaddrs*> l;
                l.push_back(ifEntry);
                m_if_map.insert(if_prop_pair(ifEntry->ifa_name, ipv4_6_pair(nullptr, l)));
            }
        } else {
            //It isn't IPv4 or IPv6
            continue;
        }
    }
    return true;
}

const if_prop_map* if_prop::get_if_props() const
{
    HC_LOG_TRACE("");

    if (!is_getaddrs_valid()) {
        HC_LOG_ERROR("data invalid");
        return nullptr;
    }

    return &m_if_map;
}

const struct ifaddrs* if_prop::get_ip4_if(const std::string& if_name) const {
    HC_LOG_TRACE("");

    if (!is_getaddrs_valid()) {
        HC_LOG_ERROR("data invalid");
        return nullptr;
    }

    if_prop_map::const_iterator if_prop_iter = m_if_map.find(if_name);
    if (if_prop_iter == m_if_map.end()) {
        return nullptr;
    }

    return if_prop_iter->second.ip4_addr;
}

const std::list<const struct ifaddrs*>* if_prop::get_ip6_if(const std::string& if_name) const
{
    HC_LOG_TRACE("");

    if (!is_getaddrs_valid()) {
        HC_LOG_ERROR("data invalid");
        return nullptr;
    }

    if_prop_map::const_iterator if_prop_iter = m_if_map.find(if_name);
    if (if_prop_iter == m_if_map.end()) {
        return nullptr;
    }

    return &(if_prop_iter->second.ip6_addr);
}

if_prop::~if_prop()
{
    HC_LOG_TRACE("");

    if (is_getaddrs_valid()) {
        freeifaddrs(m_if_addrs);
    }
}

#ifdef DEBUG_MODE
void if_prop::print_if_info(if_prop* p)
{
    using namespace std;
    HC_LOG_TRACE("");

    if (!p->is_getaddrs_valid()) {
        HC_LOG_ERROR("data invalid");
        return;
    }

    const if_prop_map* prop = p->get_if_props();
    if (prop == nullptr) {
        HC_LOG_ERROR("data struct not found");
        return;
    }

    const struct ifaddrs* if_p;
    const list<const struct ifaddrs*>* if_p_list;

    cout << "##-- IPv4 [count:" << prop->size() << "]--##" << endl;
    for (if_prop_map::const_iterator iter = prop->begin(); iter != prop->end(); iter++) {
        if_p = p->get_ip4_if(iter->first);
        if (if_p == nullptr) {
            HC_LOG_ERROR("interface name not found: " << iter->first);
            continue;
        }

        print_if_addr(if_p);
    }

    cout << "##-- IPv6 [count:" << prop->size() << "]--##" << endl;
    for (if_prop_map::const_iterator iter = prop->begin(); iter != prop->end(); iter++) {
        if_p_list = p->get_ip6_if(iter->first);

        if (if_p_list == nullptr) {
            HC_LOG_ERROR("interface name not found: " << iter->first);
            continue;
        }

        for (list<const struct ifaddrs*>::const_iterator itera = if_p_list->begin(); itera != if_p_list->end(); itera++) {
            print_if_addr(*itera);
        }
    }
}

void if_prop::print_if_addr(const struct ifaddrs* if_p)
{
    using namespace std;
    cout << "\tif name(#" << if_nametoindex(if_p->ifa_name) << "): " << if_p->ifa_name << endl;
    cout << "\t- addr: " << addr_storage(*if_p->ifa_addr) << endl;
    cout << "\t- netmask: " << addr_storage(*if_p->ifa_netmask) << endl;

    cout << "\t- flags:";
    if (if_p->ifa_flags & IFF_UP) {
        cout << "IFF_UP ";
    }
    if (if_p->ifa_flags & IFF_RUNNING) {
        cout << "IFF_RUNNING ";
    }
    if (if_p->ifa_flags & IFF_LOOPBACK) {
        cout << "IFF_LOOPBACK ";
    }
    if (if_p->ifa_flags & IFF_BROADCAST) {
        cout << "IFF_BROADCAST ";
    }
    if (if_p->ifa_flags & IFF_ALLMULTI) {
        cout << "IFF_ALLMULTI ";
    }
    if (if_p->ifa_flags & IFF_MULTICAST) {
        cout << "IFF_MULTICAST ";
    }
    if (if_p->ifa_flags & IFF_PROMISC) {
        cout << "IFF_PROMISCIFF_PROMISC ";
    }
    if (if_p->ifa_flags & IFF_POINTOPOINT) {
        cout << "IFF_POINTOPOINT ";
    }

    cout << endl;

    if (if_p->ifa_flags & IFF_POINTOPOINT) {
        if (if_p->ifa_dstaddr != nullptr) {
            cout << "\t- dstaddr: " << addr_storage(*if_p->ifa_dstaddr) << endl;
        }
    } else if (if_p->ifa_addr->sa_family == AF_INET) { //broadcast addr
        cout << "\t- broadaddr: " << addr_storage(*if_p->ifa_broadaddr) << endl;
    }
}

void if_prop::test_if_prop()
{
    using namespace std;
    HC_LOG_TRACE("");

    if_prop p;
    cout << "##-- refresh --##" << endl;
    if (!p.refresh_network_interfaces()) {
        cout << "refresh faild" << endl;
        return;
    }
    print_if_info(&p);
    cout << "##-- refresh --##" << endl;
    sleep(1);
    if (!p.refresh_network_interfaces()) {
        cout << "refresh faild" << endl;
        return;
    }
    print_if_info(&p);
}
#endif /* DEBUG_MODE */
