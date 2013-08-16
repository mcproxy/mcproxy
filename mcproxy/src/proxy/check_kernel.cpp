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
#include "include/proxy/check_kernel.hpp"
#include "include/utils/mroute_socket.hpp"


#include <iostream>
#include <unistd.h>

void check_kernel::check_kernel_features()
{
    using namespace std;
    HC_LOG_TRACE("");
    cout << "Check the currently available kernel features." << endl;

    //check root previleges
    if (geteuid() != 0) {
        HC_LOG_DEBUG(" - root privileges: Failed!");
        cout << " - root privileges: Failed" << endl;
    } else {
        HC_LOG_DEBUG(" - root privileges: Ok!");
        cout << " - root privileges: Ok!" << endl;
    }

    cout << endl;

    //ipv4
    mroute_socket ms;
    if (!ms.create_raw_ipv4_socket()) {
        HC_LOG_DEBUG(" - ipv4 multicast: Failed!");
        cout << " - ipv4 multicast: Failed" << endl;
    } else {
        HC_LOG_DEBUG(" - ipv4 multicast: Ok!");
        cout << " - ipv4 multicast: Ok!" << endl;
    }
    check_routing_tables(ms, "ipv4");

    cout << endl;

    if (!ms.create_raw_ipv6_socket()) {
        HC_LOG_DEBUG(" - ipv6 multicast: Failed!");
        cout << " - ipv6 multicast: Failed!" << endl;
    } else {
        HC_LOG_DEBUG(" - ipv6 multicast: Ok!");
        cout << " - ipv6 multicast: Ok!" << endl;
    }
    check_routing_tables(ms, "ipv6");

}

void check_kernel::check_routing_tables(mroute_socket& ms, std::string version)
{
    using namespace std;
    HC_LOG_TRACE("");

    if (!ms.set_kernel_table(1)) {
        HC_LOG_DEBUG(" - " << version << " multiple routing tables: Failed!");
        cout << " - " << version << " multiple routing tables: Failed!" << endl;
    } else {
        HC_LOG_DEBUG(" - " << version << " ipv4 multiple routing tables: Ok!");
        cout << " - " << version << " multiple routing tables: Ok!" << endl;
    }

    if (!ms.set_mrt_flag(true)) {
        HC_LOG_DEBUG(" - " << version << " routing tables: Failed!");
        cout << " - " << version << " routing tables: Failed!" << endl;
    } else {
        HC_LOG_DEBUG(" - " << version << " routing tables: Ok!");
        cout << " - " << version << " routing tables: Ok!" << endl;
    }

}


