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

#ifndef CHECK_KERNEL_H
#define CHECK_KERNEL_H

#include <string>

class mroute_socket;

/**
 * @brief Check the currently available kernel features.
 */
class check_kernel
{
private:
    void check_routing_tables(mroute_socket& ms, std::string version);
    void check_kernel_limits(mroute_socket& ms, std::string version);
public:
    /**
     * @brief Check the currently available kernel features.
     */
    void check_kernel_features();
};

#endif // CHECK_KERNEL_H
/** @} */
