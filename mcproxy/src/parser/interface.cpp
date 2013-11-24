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
#include "include/proxy/def.hpp"

#include <sstream>

//-----------------------------------------------------
single_addr::single_addr(const addr_storage& addr)
    : m_addr(addr)
{
    HC_LOG_TRACE("");
}

bool single_addr::match(const addr_storage& addr) const
{
    return addr == m_addr;
}

std::string single_addr::to_string() const
{
    return m_addr.to_string();
}
//-----------------------------------------------------
addr_range::addr_range(const addr_storage& from, const addr_storage& to)
    : m_from(from)
    , m_to(to)
{
    HC_LOG_TRACE("");
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
rule_addr::rule_addr(const std::string& if_name, std::unique_ptr<addr_match>&& group, std::unique_ptr<addr_match>&& source)
    : m_if_name(if_name)
    , m_group(std::move(group))
    , m_source(std::move(source))
{
    HC_LOG_TRACE("");
}

bool rule_addr::match(const std::string& if_name, const addr_storage& gaddr, const addr_storage& saddr) const
{
    return m_if_name.compare(if_name) == 0 && m_group->match(gaddr) && m_source->match(saddr);
}

std::string rule_addr::to_string() const
{
    std::ostringstream s;
    s << m_if_name << "(" << m_group->to_string() << " | " << m_source->to_string() << ")";
    return s.str();
}
//-----------------------------------------------------
table::table(const std::string& name)
    : m_name(name)
{
    HC_LOG_TRACE("");
}

table::table(const std::string& name, std::list<std::unique_ptr<rule_box>>&& rule_box_list)
    : m_name(name)
    , m_rule_box_list(std::move(rule_box_list))
{
    HC_LOG_TRACE("");
}

const std::string& table::get_name() const
{
    return m_name;
}

bool table::match(const std::string& if_name, const addr_storage& gaddr, const addr_storage& saddr) const
{
    for (auto & e : m_rule_box_list) {
        if (e->match(if_name, gaddr, saddr)) {
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
        s << indention(e->to_string()) << std::endl;

    }
    s << "}";
    return s.str();
}

bool operator<(const table& t1, const table& t2)
{
    return t1.m_name.compare(t2.m_name) < 0;
}
//-----------------------------------------------------
global_table_set::global_table_set()
    : m_table_set(comp_table_pointer())
{
    HC_LOG_TRACE("");    
}

std::string global_table_set::to_string() const
{
    HC_LOG_TRACE("");    
    std::ostringstream s;
    s << "##-- global table set --##";
    for (auto & e : m_table_set) {
        s << std::endl << e->to_string();

    }
    return s.str();
}

bool global_table_set::add_table(std::unique_ptr<table> t)
{
    HC_LOG_TRACE("");    
    return m_table_set.insert(std::move(t)).second;
}

const table* global_table_set::get_table(const std::string& table_name) const
{
    auto it = m_table_set.find(std::unique_ptr<table>(new table(table_name)));
    if (it != std::end(m_table_set)) {
        return it->get();
    } else {
        return nullptr;
    }
}
//-----------------------------------------------------
rule_table::rule_table(std::unique_ptr<table> t)
    : m_table(std::move(t))
{
    HC_LOG_TRACE("");
}

bool rule_table::match(const std::string& if_name, const addr_storage& gaddr, const addr_storage& saddr) const
{
    return m_table->match(if_name, gaddr, saddr);
}

std::string rule_table::to_string() const
{
    return m_table->to_string();
}
//-----------------------------------------------------
rule_table_ref::rule_table_ref(const std::string& table_name, const std::shared_ptr<const global_table_set>& global_table_set)
    : m_table_name(table_name)
    , m_global_table_set(global_table_set)
{
    HC_LOG_TRACE("");
}

bool rule_table_ref::match(const std::string& if_name, const addr_storage& gaddr, const addr_storage& saddr) const
{
    auto t = m_global_table_set->get_table(m_table_name);
    if (t == nullptr) {
        return false;
    } else {
        return t->match(if_name, gaddr, saddr);
    }
}

std::string rule_table_ref::to_string() const
{
    std::ostringstream s;
    s << "(table " << m_table_name << ")";
    return s.str();
}
//-----------------------------------------------------
instance_definition::instance_definition(const std::string& instance_name)
    : m_instance_name(instance_name)
{
    HC_LOG_TRACE("");
}

instance_definition::instance_definition(std::string&& instance_name, std::list<std::string>&& upstreams, std::list<std::string>&& downstreams)
    : m_instance_name(std::move(instance_name))
    , m_upstreams(std::move(upstreams))
    , m_downstreams(std::move(downstreams))
{
    HC_LOG_TRACE("");
}

const std::list<std::string>& instance_definition::get_upstreams() const
{
    HC_LOG_TRACE("");
    return m_upstreams;
}

const std::list<std::string>& instance_definition::get_downstreams() const
{
    HC_LOG_TRACE("");
    return m_downstreams;
}

bool operator<(const instance_definition& i1, const instance_definition& i2)
{
    return i1.m_instance_name.compare(i2.m_instance_name) < 0;
}

std::string instance_definition::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;
    s << "pinstance " << m_instance_name << ":";
    for (auto & e : m_upstreams) {
        s << " " << e;
    }
    s << " ==>";
    for (auto & e : m_downstreams) {
        s << " " << e;
    }
    return s.str();
}
//-----------------------------------------------------
rule_binding::rule_binding(std::string&& instance_name, rb_interface_type interface_type, std::string&& if_name, rb_interface_direction filter_direction, rb_filter_type filter_type, std::unique_ptr<table> filter_table)
    : m_rule_binding_type(RBT_FILTER)
    , m_instance_name(std::move(instance_name))
    , m_interface_type(interface_type)
    , m_if_name(std::move(if_name))
    , m_filter_direction(filter_direction)
    , m_filter_type(filter_type)
    , m_table(std::move(filter_table))
    , m_rule_matching_type(RMT_UNDEFINED)
    , m_timeout(std::chrono::milliseconds(0))
{
    HC_LOG_TRACE("");
}

rule_binding::rule_binding(std::string&& instance_name, rb_interface_type interface_type, std::string&& if_name, rb_interface_direction filter_direction, rb_rule_matching_type rule_matching_type, std::chrono::milliseconds&& timeout)
    : m_rule_binding_type(RBT_RULE_MATCHING)
    , m_instance_name(std::move(instance_name))
    , m_interface_type(interface_type)
    , m_if_name(std::move(if_name))
    , m_filter_direction(filter_direction)
    , m_filter_type(FT_UNDEFINED)
    , m_table(nullptr)
    , m_rule_matching_type(rule_matching_type)
    , m_timeout(timeout)
{
    HC_LOG_TRACE("");
}

rb_type rule_binding::get_rule_binding_type() const
{
    HC_LOG_TRACE("");
    return m_rule_binding_type;
}

const std::string& rule_binding::get_instance_name() const
{
    HC_LOG_TRACE("");
    return m_instance_name;
}

rb_interface_type rule_binding::get_interface_type() const
{
    HC_LOG_TRACE("");
    return m_interface_type;
}

const std::string& rule_binding::get_if_name() const
{
    HC_LOG_TRACE("");
    return m_if_name;
}

rb_interface_direction rule_binding::get_interface_direction() const
{
    HC_LOG_TRACE("");
    return m_filter_direction;
}

rb_filter_type rule_binding::get_filter_type() const
{
    HC_LOG_TRACE("");
    return m_filter_type;
}

const table& rule_binding::get_table() const
{
    HC_LOG_TRACE("");
    return *m_table;
}

rb_rule_matching_type rule_binding::get_rule_matching_type() const
{
    HC_LOG_TRACE("");
    return m_rule_matching_type;
}

std::chrono::milliseconds rule_binding::get_timeout() const
{
    HC_LOG_TRACE("");
    return m_timeout;
}

bool rule_binding::match(const std::string& if_name, const addr_storage& saddr, const addr_storage& gaddr) const
{
    HC_LOG_TRACE("");
    if (m_table != nullptr) {
        if (m_filter_type == FT_BLACKLIST) {
            return !m_table->match(if_name, saddr, gaddr); 
        } else if (m_filter_type == FT_WHITELIST){
            return m_table->match(if_name, saddr, gaddr); 
        }
    }

    return false;
}

