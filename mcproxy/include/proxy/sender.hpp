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
 * @defgroup mod_sender Sender
 * @brief The module Sender generates and sends Membership Queries to the downstreams and Membership Reports to the upstreams.
 * @{
 */

#ifndef SENDER_HPP
#define SENDER_HPP

#include "include/utils/mroute_socket.hpp"
#include "include/proxy/def.hpp"
#include "include/proxy/interfaces.hpp"

#include "memory"

class timers_values;
struct source;
class addr_storage;
/**
 * @brief Abstract basic sender class.
 */
class sender
{
protected:
    group_mem_protocol m_group_mem_protocol;
    const std::shared_ptr<const interfaces>& m_interfaces;

    mroute_socket m_sock;

public:

    sender(const std::shared_ptr<const interfaces>& interfaces, group_mem_protocol gmp);

    virtual bool send_record(unsigned int if_index, mc_filter filter_mode, const addr_storage& gaddr, const source_list<source>& slist) const;

    virtual bool send_general_query(unsigned int if_index, const timers_values& tv) const;

    virtual bool send_mc_addr_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, bool s_flag) const;

    virtual bool send_mc_addr_and_src_specific_query(unsigned int if_index, const timers_values& tv, const addr_storage& gaddr, source_list<source>& slist) const;

    virtual ~sender();
};

#endif // SENDER_HPP
/** @} */
