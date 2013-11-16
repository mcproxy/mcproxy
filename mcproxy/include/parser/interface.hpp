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

struct group_box {
    virtual bool match(const addr_storage& gaddr) = 0;
    virtual std::string to_string() = 0;
};

class source_box
{
    virtual bool match(const addr_storage& saddr) = 0;
    virtual std::string to_string() = 0;
};

struct rule_box {
    virtual bool match(const addr_storage& gaddr, const addr_storage& saddr) = 0;
    virtual std::string to_string() = 0;
};

class group : public group_box
{
    addr_storage m_gaddr;
public:
    group(const addr_storage& gaddr);
    bool match(const addr_storage& gaddr) override;
    std::string to_string() override;
};
????????????????? source_range
class group_range : public group_box
{
    addr_storage m_from;
    addr_storage m_to;
public:
    group_range(const addr_storage& from, const addr_storage& to);

    //uncluding from and to
    bool match(const addr_storage& gaddr) override;
    std::string to_string() override;
};

class source : public source_box
{
    addr_storage m_saddr;
    unsigned int m_netmask_prefix;
public:
    source(const addr_storage& saddr, unsigned int netmask_prefix);
    bool match(const addr_storage& saddr) override;
    std::string to_string() override;
};

class table : public rule_box
{
    std::string m_name;
    std::list<rule_box> m_rule_box_list;
public:
    table(const std::string& name, std::list<rule_box> rule_box_list);
    const std::string& get_name();
    bool match(const addr_storage& gaddr, const addr_storage& saddr) override;
    std::string to_string() override;
};

struct global_table_set {
    std::set<table> m_table_set;
    std::string to_string();
};

class rule_table : public rule_box
{
    table m_table;
public:
    rule_table(const table& t);
    bool match(const addr_storage& gaddr, const addr_storage& saddr) override;
    std::string to_string() override;
};

class rule_table_ref : public rule_box
{
    std::string m_table_name;
    std::shared_ptr<global_table_set> m_gloabal_table_set;
public:
    rule_table_ref(const std::string& table_name, const std::shared_ptr<global_table_set>& m_gloabal_table_set);
    bool match(const addr_storage& gaddr, const addr_storage& saddr) override;
    std::string to_string() override;
};

class rule : public rule_box
{
    std::unique_ptr<std::string> m_if_name;
    std::unique_ptr<group_box> m_group;
    std::unique_ptr<source_box> m_source;
public:
    rule(std::unique_ptr<std::string>&& if_name, std::unique_ptr<group_box>&& group, std::unique_ptr<source_box>&& source);
    bool match(const addr_storage& gaddr, const addr_storage& saddr) override;
    std::string to_string() override;
};


class interface
{
private:
public:
};



#endif // INTERFACE_HPP
