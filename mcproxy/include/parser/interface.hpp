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

#ifndef INTERFACE_HPP
#define INTERFACE_HPP
#include <list>
#include <set>
#include <string>
#include <memory>
#include <chrono>

#include "include/utils/addr_storage.hpp"

struct addr_box {
    virtual const std::set<addr_storage>& get_addr_set() const = 0;
    virtual bool is_addr_contained(const addr_storage& addr) const = 0;
    virtual std::string to_string() const = 0;
};

struct rule_box {
    virtual const std::set<addr_storage>& get_addr_set(const std::string& if_name, const addr_storage& gaddr, bool explicit_if_name) const = 0;
    virtual std::string to_string() const = 0;
};

class single_addr : public addr_box
{
    std::set<addr_storage> m_addr_set;
public:
    single_addr(const addr_storage& addr);
    bool is_addr_contained(const addr_storage& addr) const override;
    const std::set<addr_storage>& get_addr_set() const override;
    std::string to_string() const override;
};

class addr_range : public addr_box
{
    std::set<addr_storage> m_addr_set;
public:
    addr_range(const addr_storage& from, const addr_storage& to);

    //including from and to
    bool is_addr_contained(const addr_storage& addr) const override;
    const std::set<addr_storage>& get_addr_set() const override;
    std::string to_string() const override;
};

class rule_addr : public rule_box
{
    std::string m_if_name;
    std::unique_ptr<addr_box> m_group;
    std::unique_ptr<addr_box> m_source;
public:
    rule_addr(const std::string& if_name, std::unique_ptr<addr_box> group, std::unique_ptr<addr_box> source);
    const std::set<addr_storage>& get_addr_set(const std::string& if_name, const addr_storage& gaddr, bool explicit_if_name) const override;
    std::string to_string() const override;
};

class table : public rule_box
{
    std::string m_name;
    std::list<std::unique_ptr<rule_box>> m_rule_box_list;
public:
    table(const std::string& name);
    table(const std::string& name, std::list<std::unique_ptr<rule_box>>&& rule_box_list);
    const std::string& get_name() const;
    const std::set<addr_storage>& get_addr_set(const std::string& if_name, const addr_storage& gaddr, bool explicit_if_name) const override;
    std::string to_string() const override;
    friend bool operator<(const table& t1, const table& t2);
};

struct comp_table_pointer {
    bool operator()(const std::unique_ptr<table>& l, const std::unique_ptr<table>& r) const {
        return *l < *r;
    }
};

class rule_table : public rule_box
{
    std::unique_ptr<table> m_table;
public:
    rule_table(std::unique_ptr<table> t);
    const std::set<addr_storage>& get_addr_set(const std::string& if_name, const addr_storage& gaddr, bool explicit_if_name) const override;
    std::string to_string() const override;
};

class global_table_set
{
    std::set<std::unique_ptr<table>, comp_table_pointer> m_table_set;
public :
    global_table_set();
    std::string to_string() const;
    bool insert(std::unique_ptr<table> t);
    const table* get_table(const std::string& table_name) const;
};

class rule_table_ref : public rule_box
{
    std::string m_table_name;
    const std::shared_ptr<const global_table_set> m_global_table_set;
public:
    rule_table_ref(const std::string& table_name, const std::shared_ptr<const global_table_set>& global_table_set);
    const std::set<addr_storage>& get_addr_set(const std::string& if_name, const addr_storage& gaddr, bool explicit_if_name) const override;
    std::string to_string() const override;
};

enum rb_type {
    RBT_FILTER, RBT_RULE_MATCHING, RBT_TIMER_VALUE
};

enum rb_interface_type {
    IT_UPSTREAM, IT_DOWNSTREAM
};

enum rb_interface_direction {
    ID_IN, ID_OUT, ID_WILDCARD
};

enum rb_filter_type {
    //this values musst be different form mc_filter!!!
    FT_BLACKLIST = 100, FT_WHITELIST = 101, FT_UNDEFINED = 102
};

std::string get_rb_filter_type_name(rb_filter_type ft);

enum rb_rule_matching_type {
    RMT_ALL, RMT_FIRST, RMT_MUTEX, RMT_UNDEFINED
};

class rule_binding
{
private:
    rb_type m_rule_binding_type;
    std::string m_instance_name;
    rb_interface_type m_interface_type;
    std::string m_if_name;
    rb_interface_direction m_filter_direction;

    //RBT_FILTER
    rb_filter_type m_filter_type;
    std::unique_ptr<table> m_table;

    //RBT_RULE_MATCHING
    rb_rule_matching_type m_rule_matching_type;
    std::chrono::milliseconds m_timeout;

    std::string to_string_table_filter() const;
    std::string to_string_rule_matching() const;

public:
    rule_binding(const std::string& instance_name, rb_interface_type interface_type, const std::string& if_name, rb_interface_direction filter_direction, rb_filter_type filter_type, std::unique_ptr<table> filter_table);
    rule_binding(const std::string& instance_name, rb_interface_type interface_type, const std::string& if_name, rb_interface_direction filter_direction, rb_rule_matching_type rule_matching_type, const std::chrono::milliseconds& timeout);

    rb_type get_rule_binding_type() const;
    const std::string& get_instance_name() const;
    rb_interface_type get_interface_type() const;
    const std::string& get_if_name() const;
    rb_interface_direction get_interface_direction() const;

    //RBT_FILTER
    rb_filter_type get_filter_type() const;
    const table& get_table() const;

    //RBT_RULE_MATCHING
    rb_rule_matching_type get_rule_matching_type() const;
    std::chrono::milliseconds get_timeout() const;

    std::string to_string() const;
};

class interface
{
    std::string m_if_name;
    std::unique_ptr<rule_binding> m_output_filter;
    std::unique_ptr<rule_binding> m_input_filter;
    rb_filter_type get_filter_type(const std::unique_ptr<rule_binding>& filter) const;
    const std::set<addr_storage>& get_saddr_set(const std::string& if_name, const addr_storage& gaddr, const std::unique_ptr<rule_binding>& filter, bool explicit_if_name) const;
    bool is_source_allowed(const std::string& input_if_name, const addr_storage& gaddr, const addr_storage& saddr, const std::unique_ptr<rule_binding>& filter, bool excplicit_if_name) const;

public:
    interface(const std::string& if_name);
    std::string get_if_name() const;

    rb_filter_type get_filter_type(rb_interface_direction direction) const;

    const std::set<addr_storage>& get_saddr_set(rb_interface_direction direction, const std::string& if_name, const addr_storage& gaddr, bool explicit_if_name = false) const;

    bool is_source_allowed(rb_interface_direction direction, const std::string& input_if_name, const addr_storage& gaddr, const addr_storage& saddr, bool excplecit_if_name = false) const;


    std::string to_string_rule_binding() const;
    std::string to_string_interface() const;
    friend class parser;
    friend bool operator<(const interface& i1, const interface& i2);
    friend bool operator==(const interface& i1, const interface& i2);
    friend bool operator==(const std::shared_ptr<interface>& i1, const std::shared_ptr<interface>& i2);
};

class instance_definition
{
    std::string m_instance_name;
    int m_table_number;
    bool m_user_selected_table_number;
    std::list<std::shared_ptr<interface>> m_upstreams;
    std::list<std::shared_ptr<interface>> m_downstreams;

    std::list<std::shared_ptr<rule_binding>> m_global_settings;

public:
    instance_definition(const std::string& instance_name);
    instance_definition(const std::string& instance_name, std::list<std::shared_ptr<interface>>&& upstreams, std::list<std::shared_ptr<interface>>&& downstreams, int table_number, bool user_selected_table_number);
    const std::string& get_instance_name() const;
    const std::list<std::shared_ptr<interface>>& get_upstreams() const;
    const std::list<std::shared_ptr<interface>>& get_downstreams() const;
    const std::list<std::shared_ptr<rule_binding>>& get_global_settings() const;
    int get_table_number() const;
    bool get_user_selected_table_number() const;
    friend bool operator<(const instance_definition& i1, const instance_definition& i2);
    friend class parser;
    std::string to_string_instance() const;
    std::string to_string_rule_binding() const;
};

struct comp_instance_definition_pointer {
    bool operator()(const std::shared_ptr<instance_definition>& l, const std::shared_ptr<instance_definition>& r) const {
        return *l < *r;
    }
};

class inst_def_set
{
private:
    using instance_definition_set = std::set<std::shared_ptr<instance_definition>, comp_instance_definition_pointer>;
    using const_iterator = instance_definition_set::const_iterator;

    instance_definition_set m_instance_def_set;

public:
    inst_def_set();
    bool insert(const std::shared_ptr<instance_definition>& id);

    const_iterator find(const std::string& instance_name) const {
        return m_instance_def_set.find(std::make_shared<instance_definition>(instance_name));
    }

    const_iterator begin() const {
        return m_instance_def_set.cbegin();
    };

    const_iterator end() const {
        return m_instance_def_set.cend();
    };

    unsigned int size() const;

    std::string to_string() const;
};

#endif // INTERFACE_HPP
