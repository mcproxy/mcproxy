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

#ifndef ROUTING_MANAGEMENT_HPP
#define ROUTING_MANAGEMENT_HPP

#include "include/proxy/def.hpp"

#include <memory>
#include <string>
#include <sstream>

struct proxy_msg;
struct source;
class proxy_instance;
class addr_storage;

/**
 * @brief abstract interface of a summary of routing events 
 */
class routing_management
{
protected:
    const proxy_instance* const m_p;

public:
    routing_management(const proxy_instance* p): m_p(p) {}

    virtual void event_new_source(const std::shared_ptr<proxy_msg>& msg) = 0;
    virtual void event_querier_state_change(unsigned int if_index, const addr_storage& gaddr) = 0;
    virtual void timer_triggerd_maintain_routing_table(const std::shared_ptr<proxy_msg>& msg) = 0;

    virtual std::string to_string() const {return std::string();}

    friend std::ostream& operator<<(std::ostream& stream, const routing_management& rm) {
        return stream << rm.to_string();
    }

    virtual ~routing_management() {};
};

#endif // ROUTING_MANAGEMENT_HPP
