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
 * @addtogroup mod_receiver Receiver
 * @{
 */

#ifndef MLD_RECEIVER_HPP
#define MLD_RECEIVER_HPP

#include "include/proxy/receiver.hpp"

/**
 * @brief Cache Miss message received form the Linux Kernel identified by this ip verion.
 */
#define MLD_RECEIVER_KERNEL_MSG 0

/**
 * @brief Receive MLD messages.
 */
class mld_receiver : public receiver
{
private:
    int get_ctrl_min_size(); //size in byte
    int get_iov_min_size(); //size in byte
    void analyse_packet(struct msghdr* msg, int info_size);

public:
    mld_receiver(proxy_instance* pr_i, std::shared_ptr<const mroute_socket> mrt_sock, std::shared_ptr<const interfaces> interfaces, bool in_debug_testing_mode);
};

#endif // MLD_RECEIVER_HPP
/** @} */
