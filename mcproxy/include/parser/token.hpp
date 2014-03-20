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

#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>
#include <vector>

enum token_type {
    TT_PROTOCOL,
    TT_MLDV1,
    TT_MLDV2,
    TT_IGMPV1,
    TT_IGMPV2,
    TT_IGMPV3,
    TT_PINSTANCE,
    TT_DOUBLE_DOT, //":"
    TT_DOT, // "."
    TT_ARROW, //"==>"
    TT_UPSTREAM,
    TT_DOWNSTREAM,
    TT_OUT,
    TT_IN,
    TT_BLACKLIST,
    TT_WHITELIST,
    TT_RULE_MATCHING,
    TT_TABLE,
    TT_ALL,
    TT_FIRST,
    TT_MUTEX,
    TT_DISABLE,
    //TT_PATH, //@path@
    TT_LEFT_BRACE, //"{"
    TT_RIGHT_BRACE, //"}"
    TT_LEFT_BRACKET, //"("
    TT_RIGHT_BRACKET, //")"
    TT_RANGE, //"-"
    TT_SLASH, //"/"
    TT_STAR, //"*"
    TT_PIPE, //"|"
    TT_STRING,
    TT_NIL
};

std::string get_token_type_name(token_type tt);

class token
{
private:
    token_type m_type;
    std::string m_string;

public:
    token(token_type type= TT_NIL, const std::string str= "");

    token_type get_type() const; 
    const std::string& get_string() const;
};

#endif //TOKEN_HPP
