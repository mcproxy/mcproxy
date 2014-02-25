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

#ifndef PARSER_HPP
#define PARSER_HPP

#include "include/parser/scanner.hpp"
#include "include/proxy/def.hpp"
#include "include/parser/token.hpp"
#include "include/parser/interface.hpp"

#include <string>
#include <list>
#include <tuple>
#include <memory>

enum parser_type {
    PT_PROTOCOL, PT_INSTANCE_DEFINITION, PT_TABLE, PT_INTERFACE_RULE_BINDING
};

class parser
{
private:
    scanner m_scanner;
    token m_current_token;
    int m_current_line;

    void get_next_token();

    std::unique_ptr<rule_box> parse_rule(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp);
    std::unique_ptr<addr_match> parse_rule_part(group_mem_protocol gmp);
    addr_storage get_addr(group_mem_protocol gmp);

    std::unique_ptr<table> parse_table(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp, bool inside_rule_box);

    void parse_interface_table_binding(std::string&& instance_name, rb_interface_type interface_type, std::string&& if_name, rb_interface_direction filter_direction, const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp, const inst_def_set& ids);

    void parse_interface_rule_match_binding(std::string&& instance_name, rb_interface_type interface_type, std::string&& if_name, rb_interface_direction filter_direction, const inst_def_set& ids);

public:
    parser(unsigned int current_line, const std::string& cmd);
    parser_type get_parser_type();

    group_mem_protocol parse_group_mem_proto();
    void parse_instance_definition(inst_def_set& ids);
    std::unique_ptr<table> parse_table(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp);
    void parse_interface_rule_binding(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp, const inst_def_set& ids);

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const parser& scan);
};

#endif // PARSER_HPP
