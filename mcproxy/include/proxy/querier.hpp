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

#ifndef QUERIER_HPP
#define QUERIER_HPP

#include "include/proxy/membership_db.hpp"
#include "include/proxy/sender.hpp"
#include <string>
/**
 * @brief define the behaviour of a maulticast querier for a specific interface
 */
class querier 
{
private:
    int m_addr_family;
    int m_if_index;
    membership_db m_db;

    const sender& m_sender;

    bool init_db();
public:

    querier(int addr_family, int if_index, const sender& sender);
    bool init();
 

    /**
     * @brief Test the functionality of the querier.
     */
    static void test_querier(string if_name);
};

#endif // QUERIER_HPP
/** @} */
