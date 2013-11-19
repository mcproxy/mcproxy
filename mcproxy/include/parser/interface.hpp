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

#include "include/utils/addr_storage.hpp"

struct addr_match {
    virtual bool match(const addr_storage& addr) const = 0;
    virtual std::string to_string() const = 0;
};

struct rule_box {
    virtual bool match(const addr_storage& saddr, const addr_storage& gaddr) const = 0;
    virtual std::string to_string() const = 0;
};

class single_addr: public addr_match
{
    addr_storage m_addr;
public:
    single_addr(const addr_storage& addr);
    bool match(const addr_storage& addr) const override;
    std::string to_string() const override;
};

class addr_range : public addr_match
{
    addr_storage m_from;
    addr_storage m_to;
public:
    addr_range(const addr_storage& from, const addr_storage& to);

    //uncluding from and to
    bool match(const addr_storage& addr) const override;
    std::string to_string() const override;
};

class rule_addr : public rule_box
{
    std::string m_if_name;
    std::unique_ptr<addr_match> m_group;
    std::unique_ptr<addr_match> m_source;
public:
    rule_addr(const std::string& if_name, std::unique_ptr<addr_match>&& group, std::unique_ptr<addr_match>&& source);
    bool match(const addr_storage& gaddr, const addr_storage& saddr) const override;
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
    bool match(const addr_storage& gaddr, const addr_storage& saddr) const override;
    std::string to_string() const override;
    friend bool operator<(const table& t1, const table& t2);
};

class rule_table : public rule_box
{
    table m_table;
public:
    rule_table(table&& t);
    bool match(const addr_storage& gaddr, const addr_storage& saddr) const override;
    std::string to_string() const override;
};

class global_table_set
{
    std::set<table> m_table_set;
public :
    std::string to_string() const;
    bool add_table(table&& t);
    const table* get_table(const std::string& table_name) const;
};

class rule_table_ref : public rule_box
{
    std::string m_table_name;
    const std::shared_ptr<const global_table_set> m_global_table_set;
public:
    rule_table_ref(const std::string& table_name, const std::shared_ptr<const global_table_set>& global_table_set);
    bool match(const addr_storage& gaddr, const addr_storage& saddr) const override;
    std::string to_string() const override;
};

class interface
{
private:
public:
};



#endif // INTERFACE_HPP
