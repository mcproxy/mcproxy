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
#include "include/parser/interface.hpp"

#include <sstream>

//-----------------------------------------------------
group::group(const addr_storage& gaddr)
    : m_gaddr(gaddr)
{
}

bool group::match(const addr_storage& gaddr)
{
    return gaddr == m_gaddr;
}

std::string group::to_string(){
    addr_storage(m_gaddr.get_addr_family());
    if (m_gaddr == addr_storage(m_gaddr.get_addr_family())){
        return "*";
    }else{
        return m_gaddr.to_string(); 
    }
}
//-----------------------------------------------------
group_range::group_range(const addr_storage& from, const addr_storage& to)
    : m_from(from)
    , m_to(to)
{
}

bool group_range::match(const addr_storage& gaddr){
    return gaddr >= m_from && gaddr <= m_to; 
}

std::string group_range::to_string(){
    std::ostringstream s; 
    s << m_from << " - " << m_to; 
    return s.str();
}
//-----------------------------------------------------
source::source(const addr_storage& saddr, unsigned int netmask_prefix)
    : m_saddr(saddr)
    , m_netmask_prefix(netmask_prefix)
{
}

bool source::match(const addr_storage& saddr) override;
    std::string to_string() override;
//-----------------------------------------------------
