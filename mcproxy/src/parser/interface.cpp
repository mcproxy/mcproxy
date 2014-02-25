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
#include "include/proxy/interfaces.hpp"

#include <sstream>

//-----------------------------------------------------
bool addr_match::is_wildcard(const addr_storage& addr, int addr_family) const
{
    return addr == addr_storage(addr_family);
}
//-----------------------------------------------------
single_addr::single_addr(const addr_storage& addr)
    : m_addr(addr)
{
    HC_LOG_TRACE("");
}

bool single_addr::match(const addr_storage& addr) const
{
    return addr == m_addr || is_wildcard(m_addr, addr.get_addr_family());
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
    return (addr >= m_from || is_wildcard(m_from, addr.get_addr_family())) && (addr <= m_to || is_wildcard(m_to, addr.get_addr_family()) );
}

std::string addr_range::to_string() const
{
    std::ostringstream s;
    s << m_from << " - " << m_to;
    return s.str();
}
//-----------------------------------------------------
rule_addr::rule_addr(const std::string& if_name, std::unique_ptr<addr_match> group, std::unique_ptr<addr_match> source)
    : m_if_name(if_name)
    , m_group(std::move(group))
    , m_source(std::move(source))
{
    HC_LOG_TRACE("");
}

bool rule_addr::match(const std::string& if_name, const addr_storage& gaddr, const addr_storage& saddr) const
{
    if (m_if_name.empty()) {
        return m_group->match(gaddr) &&  m_source->match(saddr);
    } else {
        return m_if_name.compare(if_name) == 0 && m_group->match(gaddr) &&  m_source->match(saddr);
    }
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
    bool is_first_use = true;
    for (auto & e : m_table_set) {
        if (is_first_use) {
            is_first_use = false;
        } else {
            s << std::endl;
        }
        s << e->to_string();
    }
    return s.str();
}

bool global_table_set::insert(std::unique_ptr<table> t)
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
rule_binding::rule_binding(const std::string& instance_name, rb_interface_type interface_type, const std::string& if_name, rb_interface_direction filter_direction, rb_filter_type filter_type, std::unique_ptr<table> filter_table)
    : m_rule_binding_type(RBT_FILTER)
    , m_instance_name(instance_name)
    , m_interface_type(interface_type)
    , m_if_name(if_name)
    , m_filter_direction(filter_direction)
    , m_filter_type(filter_type)
    , m_table(std::move(filter_table))
    , m_rule_matching_type(RMT_UNDEFINED)
    , m_timeout(std::chrono::milliseconds(0))
{
    HC_LOG_TRACE("");
}

rule_binding::rule_binding(const std::string& instance_name, rb_interface_type interface_type, const std::string& if_name, rb_interface_direction filter_direction, rb_rule_matching_type rule_matching_type, const std::chrono::milliseconds& timeout)
    : m_rule_binding_type(RBT_RULE_MATCHING)
    , m_instance_name(instance_name)
    , m_interface_type(interface_type)
    , m_if_name(if_name)
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
        } else if (m_filter_type == FT_WHITELIST) {
            return m_table->match(if_name, saddr, gaddr);
        }
    }

    return false;
}

std::string rule_binding::to_string() const
{
    HC_LOG_TRACE("");
    using namespace std;
    ostringstream s;
    //pinstance A upstream ap in rulematching mutex 10000;

    s << "pinstance " << m_instance_name << " ";
    if (m_interface_type == IT_UPSTREAM) {
        s << "upstream ";
    } else if (m_interface_type == IT_DOWNSTREAM) {
        s << "downstream ";
    } else {
        HC_LOG_ERROR("unkown interface type");
        s << "??? ";
    }

    s << m_if_name << " ";

    if (m_filter_direction == ID_IN) {
        s << "in ";
    } else if (m_filter_direction == ID_OUT) {
        s << "out ";
    } else if (m_filter_direction == ID_WILDCARD) {
        s << "* ";
    } else {
        HC_LOG_ERROR("unkown interface direction");
        s << "??? ";
    }

    if (m_rule_binding_type == RBT_FILTER) {
        s << to_string_table_filter();
    } else if (m_rule_binding_type == RBT_RULE_MATCHING) {
        s << to_string_rule_matching();
    } else {
        HC_LOG_ERROR("unkown rule binding type");
        s << "??? ";
    }

    return s.str();
}

std::string rule_binding::to_string_table_filter() const
{
    HC_LOG_TRACE("");
    using namespace std;
    ostringstream s;

    if (m_filter_type == FT_BLACKLIST) {
        s << "blacklist ";
    } else if (m_filter_type == FT_WHITELIST) {
        s << "whitelist ";
    } else if (m_filter_type == FT_UNDEFINED) {
        s << "* ";
    } else {
        HC_LOG_ERROR("unkown filter type");
        s << "??? ";
    }

    if (m_table != nullptr) {
        s << m_table->to_string();
    } else {
        HC_LOG_ERROR("undefined table");
        s << "table ???";
    }

    return s.str();
}

std::string rule_binding::to_string_rule_matching() const
{
    HC_LOG_TRACE("");
    using namespace std;
    ostringstream s;

    s << "rulematching ";
    if (m_rule_matching_type == RMT_ALL) {
        s << "all ";
    } else if (m_rule_matching_type == RMT_FIRST) {
        s << "first ";
    } else if (m_rule_matching_type == RMT_MUTEX) {
        s << "mutex " << m_timeout.count();
    } else {
        HC_LOG_ERROR("unkown rule matching type");
        s << "???";
    }

    return s.str();
}
//-----------------------------------------------------
interface::interface(const std::string& if_name)
    : m_if_name(if_name)
    , m_output_filter(nullptr)
    , m_input_filter(nullptr)
{
    HC_LOG_TRACE("");
    //unsigned int if_index = interfaces::get_if_index(if_name);
    //if (if_index <= 0) {
    //HC_LOG_ERROR("interface " << if_name << " not found");
    //throw "interface not found";

}

std::string interface::get_if_name() const
{
    return m_if_name;
}

bool interface::match_output_filter(const std::string& input_if_name, const addr_storage& saddr, const addr_storage& gaddr) const
{
    HC_LOG_TRACE("");
    return match_filter(input_if_name, saddr, gaddr, m_output_filter);
}

bool interface::match_input_filter(const std::string& input_if_name, const addr_storage& saddr, const addr_storage& gaddr) const
{
    HC_LOG_TRACE("");
    return match_filter(input_if_name, saddr, gaddr, m_input_filter);
}

std::string interface::to_string_rule_binding() const
{
    HC_LOG_TRACE("");
    using namespace std;
    ostringstream s;

    if (m_output_filter != nullptr) {
        s << m_output_filter->to_string();

        if (m_input_filter != nullptr) {
            s << endl << m_input_filter->to_string();
        }
    } else {
        if (m_input_filter != nullptr) {
            s << m_input_filter->to_string();
        }
    }
    return s.str();
}

std::string interface::to_string_interface() const
{
    HC_LOG_TRACE("");
    return m_if_name;
}

bool interface::match_filter(const std::string& input_if_name, const addr_storage& saddr, const addr_storage& gaddr, const std::unique_ptr<rule_binding>& filter) const
{
    if (filter != nullptr) {
        return filter->match(input_if_name, saddr, gaddr);
    } else {
        return true; //default behaviour
    }
}

bool operator<(const interface& i1, const interface& i2)
{
    return i1.m_if_name.compare(i2.m_if_name) < 0;
}

bool operator==(const interface& i1, const interface& i2)
{
    return i1.m_if_name.compare(i2.m_if_name) == 0;
}
bool operator==(const std::shared_ptr<interface>& i1, const std::shared_ptr<interface>& i2)
{
    return *i1 == *i2;
}
//-----------------------------------------------------
instance_definition::instance_definition(const std::string& instance_name)
    : m_instance_name(instance_name)
    , m_table_number(0)
    , m_user_selected_table_number(false)
{
    HC_LOG_TRACE("");
}

instance_definition::instance_definition(const std::string& instance_name, std::list<std::shared_ptr<interface>>&& upstreams, std::list<std::shared_ptr<interface>>&& downstreams, int table_number, bool user_selected_table_number)
    : m_instance_name(instance_name)
    , m_table_number(table_number)
    , m_user_selected_table_number(user_selected_table_number)
    , m_upstreams(std::move(upstreams))
    , m_downstreams(std::move(downstreams))
{
    HC_LOG_TRACE("");
}

const std::string& instance_definition::get_instance_name() const
{
    HC_LOG_TRACE("");
    return m_instance_name;
}

const std::list<std::shared_ptr<interface>>& instance_definition::get_upstreams() const
{
    HC_LOG_TRACE("");
    return m_upstreams;
}

const std::list<std::shared_ptr<interface>>& instance_definition::get_downstreams() const
{
    HC_LOG_TRACE("");
    return m_downstreams;
}

const std::list<std::shared_ptr<rule_binding>>& instance_definition::get_global_settings() const
{
    HC_LOG_TRACE("");
    return m_global_settings;
}

int instance_definition::get_table_number() const
{
    HC_LOG_TRACE("");
    return m_table_number;
}

bool instance_definition::get_user_selected_table_number() const
{
    HC_LOG_TRACE("");
    return m_user_selected_table_number;
}

bool operator<(const instance_definition& i1, const instance_definition& i2)
{
    return i1.m_instance_name.compare(i2.m_instance_name) < 0;
}

std::string instance_definition::to_string_instance() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;
    s << "pinstance " << m_instance_name << ": ";
    for (auto & e : m_upstreams) {
        s << e->to_string_interface() << " ";
    }
    s << "==> ";
    for (auto & e : m_downstreams) {
        s << e->to_string_interface() << " ";
    }
    return s.str();
}

std::string instance_definition::to_string_rule_binding() const
{
    HC_LOG_TRACE("");
    using namespace std;
    ostringstream s;

    bool first_touch = true;
    for (auto & e : m_upstreams) {
        if (first_touch) {
            s << e->to_string_rule_binding();
            first_touch = false;
        } else {
            s << endl << e->to_string_rule_binding();
        }
    }

    for (auto & e : m_downstreams) {
        s << endl << e->to_string_rule_binding();
    }

    for (auto & e : m_global_settings) {
        s << endl << e->to_string();
    }

    return s.str();
}

//-----------------------------------------------------
inst_def_set::inst_def_set()
    : m_instance_def_set(comp_instance_definition_pointer())
{
    HC_LOG_TRACE("");
}

bool inst_def_set::insert(const std::shared_ptr<instance_definition>& id)
{
    HC_LOG_TRACE("");
    return m_instance_def_set.insert(id).second;
}

unsigned int inst_def_set::size() const
{
    HC_LOG_TRACE("");
    return m_instance_def_set.size();
}

std::string inst_def_set::to_string() const
{
    HC_LOG_TRACE("");
    using namespace std;
    ostringstream s;
    bool first_touch = true;
    for (auto & e : m_instance_def_set) {
        if (first_touch) {
            s << e->to_string_instance();
            first_touch = false;
        } else {
            s << endl << e->to_string_instance();
        }
        s << endl << e->to_string_rule_binding();
    }
    return s.str();
}
