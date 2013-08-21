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

/**
 * @addtogroup mod_proxy Proxy
 * @{
 */

#ifndef CHECK_IF_H
#define CHECK_IF_H

#include "include/utils/if_prop.hpp"
#include "vector"

/**
 * @brief Monitored the running state of the network interfaces.
 */
class check_if
{
private:
    int m_addr_family;

    if_prop m_if_property_a;
    if_prop m_if_property_b;
    if_prop* m_current_prop;

    std::vector<int> m_check_lst;
    std::vector<int> m_swap_to_up;
    std::vector<int> m_swap_to_down;

public:

    /**
     * @brief Create a check_if instance.
     */
    check_if();

    //return all dont running interfaces
    /**
      * @brief Initialize check_if for a specific IP version.
      * @param check_lst a list of monitored interface indexes
      * @param addr_family used IP version (AF_INET or AF_INET6)
      * @return Return all dont running interfaces to have an consistent start state.
     */
    std::vector<int> init(std::vector<int>& check_lst, int addr_family);

    /**
     * @brief Trigger the monitoring.
     * @return Return true on success.
     */
    bool check();

    /**
     * @brief Return the interface indexes who swap their running state to up after the last monitoring trigger.
     */
    std::vector<int> swap_to_up();

    /**
     * @brief Return the interface indexes who swap their running state to down after the last monitoring trigger.
     */
    std::vector<int> swap_to_down();

    /**
     * @brief Test the functionality of check_if.
     */
    static void test_check_if();
};

#endif // CHECK_IF_H
/** @} */
