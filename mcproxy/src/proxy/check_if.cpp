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
#include "include/proxy/check_if.hpp"
#include <net/if.h>
#include <unistd.h>
#include <iostream>

check_if::check_if()
{
    HC_LOG_TRACE("");
}

std::vector<int> check_if::init(std::vector<int>& check_lst, int addr_family)
{
    HC_LOG_TRACE("");

    this->m_check_lst = check_lst;
    this->m_addr_family = addr_family;

    std::vector<int> result;
    m_if_property_a.refresh_network_interfaces();
    m_current_prop = &m_if_property_a;

    for (std::vector<int>::iterator i = m_check_lst.begin(); i != m_check_lst.end(); i++) {
        char cstr[IF_NAMESIZE];
        std::string if_name(if_indextoname(*i, cstr)); //fehler wenn null?????????????????????

        const struct ifaddrs* prop;
        if (m_addr_family == AF_INET) {
            prop = m_current_prop->get_ip4_if(if_name);
        } else if (m_addr_family == AF_INET6) {
            const std::list<const struct ifaddrs*>* ipv6_if_list = m_current_prop->get_ip6_if(if_name);
            prop = *(ipv6_if_list->begin());
        } else {
            HC_LOG_ERROR("wrong address family: " << addr_family);
            return result;
        }

        if (!(prop->ifa_flags & IFF_RUNNING)) { //down
            result.push_back(*i);
        }
    }

    return result;
}

bool check_if::check()
{
    HC_LOG_TRACE("");

    m_swap_to_up.clear();
    m_swap_to_down.clear();

    if_prop* if_property_old = m_current_prop;

    if (m_current_prop == &m_if_property_a) {
        m_current_prop = &m_if_property_b;
    } else if (m_current_prop == &m_if_property_b) {
        m_current_prop = &m_if_property_a;
    } else {
        HC_LOG_ERROR("unknown pointer");
        return false;
    }

    m_current_prop->refresh_network_interfaces();

    for (std::vector<int>::iterator i = m_check_lst.begin(); i != m_check_lst.end(); i++) {
        char cstr[IF_NAMESIZE];
        std::string if_name(if_indextoname(*i, cstr)); //fehler wenn null??????????????????

        const struct ifaddrs* prop_new = m_current_prop->get_ip4_if(if_name);
        const struct ifaddrs* prop_old = if_property_old->get_ip4_if(if_name);

        if (m_addr_family == AF_INET) {
            prop_new = m_current_prop->get_ip4_if(if_name);
            prop_old = if_property_old->get_ip4_if(if_name);
        } else if (m_addr_family == AF_INET6) {
            const std::list<const struct ifaddrs*>* ipv6_if_list_new = m_current_prop->get_ip6_if(if_name);
            const std::list<const struct ifaddrs*>* ipv6_if_list_old = if_property_old->get_ip6_if(if_name);

            prop_new = *(ipv6_if_list_new->begin());
            prop_old = *(ipv6_if_list_old->begin());
        }

        if (((prop_new->ifa_flags ^ prop_old->ifa_flags) & IFF_RUNNING)) { //IFF_RUNNING changed
            if (prop_new->ifa_flags & IFF_RUNNING) { //up
                m_swap_to_up.push_back(*i);
            } else { //down
                m_swap_to_down.push_back(*i);
            }
        }
    }

    return true;
}

std::vector<int> check_if::swap_to_up()
{
    HC_LOG_TRACE("");
    return m_swap_to_up;

}

std::vector<int> check_if::swap_to_down()
{
    HC_LOG_TRACE("");
    return m_swap_to_down;
}

#ifdef DEBUG_MODE
void check_if::test_check_if()
{
    using namespace std;
    HC_LOG_TRACE("");

    check_if c;
    if_prop prop;
    vector<int> if_list_tmp;
    char cstr[IF_NAMESIZE];
    int sleeptime = 0;

    //fill if_list_tmp
    prop.refresh_network_interfaces();
    const if_prop_map* if_prop_map_p = prop.get_if_props();
    cout << "available interfaces under test:" << endl;
    for (if_prop_map::const_iterator i = if_prop_map_p->begin(); i != if_prop_map_p->end(); i++) {
        cout << i->first << " ";
        if_list_tmp.push_back(if_nametoindex(i->first.c_str()));
    }
    cout << endl;

    //init status
    if_list_tmp = c.init(if_list_tmp, AF_INET);
    cout << "this interfaces are down:" << endl;
    for (vector<int>::iterator i = if_list_tmp.begin(); i != if_list_tmp.end(); i++) {
        cout << if_indextoname(*i, cstr) << " ";
    }
    cout << endl;

    while (sleeptime < 1000) {
        usleep(1000000);
        cout << "sleeptime: " << sleeptime << endl;
        c.check();
        if_list_tmp = c.swap_to_down();
        if (if_list_tmp.size() > 0) {
            cout << "this interfaces switch to down: " << endl;
            for (vector<int>::iterator i = if_list_tmp.begin(); i < if_list_tmp.end(); i++) {
                cout << if_indextoname(*i, cstr) << " ";
            }
            cout << endl;
        }

        if_list_tmp = c.swap_to_up();
        if (if_list_tmp.size() > 0) {
            cout << "this interfaces switch to up: " << endl;
            for (vector<int>::iterator i = if_list_tmp.begin(); i < if_list_tmp.end(); i++) {
                cout << if_indextoname(*i, cstr) << " ";
            }
            cout << endl;
        }

        sleeptime++;
    }


}
#endif /* DEBUG_MODE */
