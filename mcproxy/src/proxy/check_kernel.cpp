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
#include "include/utils/addr_storage.hpp"

#include <iostream>
#include <unistd.h>
#include <netinet/in.h>

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
    check_kernel_limits(ms, "ipv4");

    cout << endl;

    if (!ms.create_raw_ipv6_socket()) {
        HC_LOG_DEBUG(" - ipv6 multicast: Failed!");
        cout << " - ipv6 multicast: Failed!" << endl;
    } else {
        HC_LOG_DEBUG(" - ipv6 multicast: Ok!");
        cout << " - ipv6 multicast: Ok!" << endl;
    }
    check_routing_tables(ms, "ipv6");

    cout << endl;
    check_kernel_limits(ms, "ipv6");
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

void check_kernel::check_kernel_limits(mroute_socket& ms, std::string version)
{
    using namespace std;
    HC_LOG_TRACE("");

    bool running = true;
    int join_count = 0;
    int filter_count = 0;
    int secure_limit = 40;
    addr_storage gaddr;
    addr_storage saddr;

    if (version.compare("ipv4") == 0) {
        gaddr = "225.0.0.0";
        saddr = "1.1.1.1";
    } else if (version.compare("ipv6") == 0) {
        gaddr = "FF05:1::0";
        saddr = "2005::1";
    } else {
        HC_LOG_ERROR("unknown ip version");
        return;
    }

    //count how many groups can be joined
    //its looks like that IPv6 hasnt a limit at the moment ...
    //so we stop after 50 tries otherwise the system will die instantly
    while (running && join_count < secure_limit) {
        if (ms.join_group(gaddr, 1)) {
            ++join_count;
            ++gaddr;
        } else {
            running = false;
        }
    }

    //count how many filter rules can be set
    running = true;
    --gaddr;
    while (running && filter_count < secure_limit) {
        if (ms.set_source_filter(1, gaddr, MCAST_INCLUDE, {saddr})) {
            ++filter_count;
            ++saddr;
        }
        else {
            running = false;
        }
    }


    cout << " - " << version << " mcproxy was able to join " << join_count;
    if (join_count >= secure_limit) {
        cout << "+ groups successfully (no limit found)" << endl;
    } else {
        cout << " groups successfully" << endl;
    }

    cout << " - " << version << " mcproxy was able to set " << filter_count;
    if (filter_count >= secure_limit) {
        cout << "+ filters successfully (no limit found)" << endl;
    } else {
        cout << " filters successfully" << endl;
    }
}

