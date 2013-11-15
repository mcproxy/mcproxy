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
#include "include/parser/parser.hpp"

parser::parser(unsigned int current_line, const std::string& cmd)
    : m_scanner(current_line, cmd)
    , m_current_line(current_line)
{
    HC_LOG_TRACE("");

    get_next_token();
}

parser_type parser::get_parser_type()
{
    if (m_current_token.get_type() == TT_PROTOCOL) {
        return PT_PROTOCOL;
    } else if (m_current_token.get_type() == TT_TABLE) {
        return PT_TABLE;
    } else if (m_current_token.get_type() == TT_PINSTANCE) {
        auto cmp_token = m_scanner.get_next_token(true, 1);
        if (cmp_token.get_type() == TT_DEFINITION) {
            return PT_INSTANCE_DEFINITION;
        } else if (cmp_token.get_type() == TT_UPSTREAM || cmp_token.get_type() == TT_DOWNSTREAM) {
            return PT_INTERFACE_RULE_BINDING;
        } else {
            HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(cmp_token.get_type()) << " with value " << cmp_token.get_string() << ", expected \":\" or \"upstream\" or \"downstream\"");
            throw "failed to parse config file";
        }
    } else {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string());
        throw "failed to parse config file";
    }
}

void parser::parse_group_mem_proto(group_mem_protocol& gmp)
{
    HC_LOG_TRACE("");




}

//void parser::accept(token_type type)
//{
//HC_LOG_TRACE("");

//if (m_current_token.get_type() == type){


//}

////a
//get_next_token();
//}

void parser::get_next_token()
{
    HC_LOG_TRACE("");

    m_current_token = m_scanner.get_next_token();
}
