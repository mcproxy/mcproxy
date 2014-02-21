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
#include "include/parser/token.hpp"

#include <map>

std::string get_token_type_name(token_type tt)
{
    std::map<token_type, std::string> name_map = {
        {TT_PROTOCOL, "TT_PROTOCOL"},
        {TT_MLDV1, "TT_MLDV1"},
        {TT_MLDV2, "TT_MLDV2"},
        {TT_IGMPV1, "TT_IGMPV1"},
        {TT_IGMPV2, "TT_IGMPV2"},
        {TT_IGMPV3, "TT_IGMPV3"},
        {TT_PINSTANCE, "TT_PINSTANCE"},
        //{TT_INSTANCE_NAME, "TT_INSTANCE_NAME"},
        {TT_DOUBLE_DOT, "TT_DOUBLE_DOT"},
        {TT_DOT, "TT_DOT"},
        //{TT_IF_NAME, "TT_IF_NAME"},
        {TT_ARROW, "TT_ARROW"},
        {TT_UPSTREAM, "TT_UPSTREAM"},
        {TT_DOWNSTREAM, "TT_DOWNSTREAM"},
        {TT_OUT, "TT_OUT"},
        {TT_IN, "TT_IN"},
        {TT_BLACKLIST, "TT_BLACKLIST"},
        {TT_WHITELIST, "TT_WHITELIST"},
        {TT_RULE_MATCHING, "TT_RULE_MATCHING"},
        {TT_TABLE, "TT_TABLE"},
        {TT_ALL, "TT_ALL"},
        {TT_FIRST, "TT_FIRST"},
        {TT_MUTEX, "TT_MUTEX"},
        //{TT_MILLISECONDS, "TT_MILLISECONDS"},
        //{TT_TABLE_NAME, "TT_TABLE_NAME"},
        //{TT_PATH, "TT_PATH"},
        {TT_LEFT_BRACE, "TT_LEFT_BRACE"},
        {TT_RIGHT_BRACE, "TT_RIGHT_BRACE"},
        {TT_LEFT_BRACKET, "TT_LEFT_BRACKET"},
        {TT_RIGHT_BRACKET, "TT_RIGHT_BRACKET"},
        {TT_RANGE, "TT_RANGE"},
        {TT_SLASH, "TT_SLASH"},
        {TT_STAR,  "TT_STAR"},
        {TT_PIPE,  "TT_PIPE"},
        //{TT_IPV4_ADDRESS, "TT_IPV4_ADDRESS"},
        //{TT_IPV6_ADDRESS, "TT_IPV6_ADDRESS"},
        {TT_STRING, "TT_STRING"},
        {TT_NIL, "TT_NIL"}
    };

    return name_map[tt];
}

token_type token::get_type() const
{
    return m_type;
}

const std::string& token::get_string() const
{
    HC_LOG_TRACE("");
    return m_string;
}

token::token(token_type type, const std::string str)
    : m_type(type)
    , m_string(str)
{
}
