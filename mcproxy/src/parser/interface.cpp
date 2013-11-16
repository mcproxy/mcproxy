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
single_addr::single_addr(const addr_storage& addr)
    : m_addr(addr)
{
}

bool single_addr::match(const addr_storage& addr) const
{
    return addr == m_addr;
}

std::string single_addr::to_string() const
{
    addr_storage(m_addr.get_addr_family());
    if (m_addr == addr_storage(m_addr.get_addr_family())) {
        return "*";
    } else {
        return m_addr.to_string();
    }
}
//-----------------------------------------------------
addr_range::addr_range(const addr_storage& from, const addr_storage& to)
    : m_from(from)
    , m_to(to)
{
}

bool addr_range::match(const addr_storage& addr) const
{
    return addr >= m_from && addr <= m_to;
}

std::string addr_range::to_string() const
{
    std::ostringstream s;
    s << m_from << " - " << m_to;
    return s.str();
}
//-----------------------------------------------------
addr_rule::addr_rule(const std::string& if_name, std::unique_ptr<addr_match>&& group, std::unique_ptr<addr_match>&& source)
    : m_if_name(if_name)
    , m_group(std::move(group))
    , m_source(std::move(source))
{
}

bool addr_rule::match(const addr_storage& gaddr, const addr_storage& saddr) const
{
    return m_group->match(gaddr) && m_source->match(saddr);
}

std::string addr_rule::to_string() const
{
    std::ostringstream s;
    s << "(" << m_group->to_string() << " | " << m_source->to_string() << ")";
    return s.str();
}
//-----------------------------------------------------
table::table(const std::string& name, std::list<rule_box>&& rule_box_list)
    : m_name(name)
    , m_rule_box_list(std::move(rule_box_list))
{
}

const std::string& table::get_name() const
{
    return m_name;
}

bool table::match(const addr_storage& gaddr, const addr_storage& saddr) const
{
    for (auto & e : m_rule_box_list) {
        if (e.match(gaddr, saddr)) {
            return true;
        }
    }

    return false;
}

std::string table::to_string() const
{
    std::ostringstream s;
    s << "table " << m_name << " {" << std::endl;
    for (auto & e : m_rule_box_list) {
        s << e.to_string() << std::endl;

    }
    s << "}";
    return s.str();
}
bool operator<(const table& t1, const table& t2)
{
    return t1.m_name.compare(t2.m_name) < 0;

}
//-----------------------------------------------------
std::string global_table_set::to_string() const
{
    std::ostringstream s;
    s << "##-- global table set --##";
    for (auto & e : m_table_set) {
        s << std::endl << e.to_string();

    }
    return s.str();
}

bool global_table_set::add_table(table&& t)
{
    return m_table_set.insert(std::move(t)).second;
}

const table* global_table_set::get_table(const std::string& table_name)
{
    auto it = m_table_set.find(table_name);
    if()
}
//-----------------------------------------------------
rule_table::rule_table(const table& t)
    : m_table(t)
{
}

bool rule_table::match(const addr_storage& gaddr, const addr_storage& saddr) const
{
    return m_table.match(gaddr, saddr);
}

std::string rule_table::to_string() const
{
    return m_table.to_string();
}
//-----------------------------------------------------
rule_table_ref::rule_table_ref(const std::string& table_name, const std::shared_ptr<global_table_set>& global_table_set)
    : m_table_name(table_name)
    , m_global_table_set(global_table_set)
{
}

bool rule_table_ref::match(const addr_storage& gaddr, const addr_storage& saddr) const
{
    return
}

std::string rule_table_ref::to_string() const
{

}
