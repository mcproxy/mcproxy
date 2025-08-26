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

#ifndef IGMP_RECEIVER_HPP
#define IGMP_RECEIVER_HPP

#include <net/if.h>
#include <netinet/igmp.h>
#include <netinet/ip.h>

#include <linux/mroute.h>

#include "include/hamcast_logging.h"

#include "include/proxy/receiver.hpp"
#include "include/proxy/proxy_instance.hpp"
#include "include/proxy/message_format.hpp"

#include "include/utils/extended_igmp_defines.hpp"


/**
 * @brief Size of the router alert option.
 */
#define IGMP_RECEIVER_IPV4_ROUTER_ALERT_OPT_SIZE 4  //RFC 2711

/**
 * @brief Cache Miss message received form the Linux Kernel identified by this ip verion.
 */
#define IGMP_RECEIVER_KERNEL_MSG 0

/**
 * @brief Receive IGMP messages.
 */
class igmp_receiver : public receiver
{
private:

    int get_ctrl_min_size() override;
    int get_iov_min_size() override;
    void analyse_packet(struct msghdr* msg, int info_size) override;

public:
    /**
     * @brief Create an igmp_receiver.
     */
    igmp_receiver(proxy_instance* pr_i, const std::shared_ptr<const mroute_socket> mrt_sock,const std::shared_ptr<const interfaces> interfaces, bool in_debug_testing_mode);
};

#endif // IGMP_RECEIVER_HPP
/** @} */
