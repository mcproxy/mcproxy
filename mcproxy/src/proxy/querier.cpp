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
#include "include/proxy/querier.hpp"

#include <iostream>

querier::querier(int addr_family, int if_index, const sender& sender): m_addr_family(addr_family), m_if_index(if_index), m_sender(sender)
{
    HC_LOG_TRACE("");
}

bool querier::init()
{
    HC_LOG_TRACE("");

    return init_db();
}

bool querier::init_db()
{
    HC_LOG_TRACE("");

    if (m_addr_family == AF_INET) {
        m_db.compatibility_mode_variable = IGMPv3;
    } else if (m_addr_family == AF_INET6) {
        m_db.compatibility_mode_variable = MLDv2;
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    m_db.is_querier = true;




    return true;
}

void querier::test_querier(string if_name)
{
    cout << "##-- querier test on interface " << if_name << " --##" << endl;






}
