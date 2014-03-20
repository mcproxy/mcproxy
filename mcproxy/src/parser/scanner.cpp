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
#include "include/parser/scanner.hpp"
#include "include/parser/token.hpp"

#include <algorithm>
#include <fstream>

scanner::scanner(unsigned int current_line, const std::string& cmd)
    : m_current_line(current_line)
    , m_cmd(cmd)
    , m_current_cmd_pos(0)
    , m_token_pos(0)
{
    HC_LOG_TRACE("");

    read_next_char();
    fill_token_vec();
}

token scanner::get_next_token(bool peek, int token_count)
{
    if (peek && m_token_pos + token_count < m_token_vec.size()) {
        return m_token_vec[m_token_pos + token_count];
    } else if (!peek && m_token_pos < m_token_vec.size()) {
        return m_token_vec[m_token_pos++];
    } else {
        return token(TT_NIL);
    }
}

void scanner::read_next_char()
{
    if (m_current_cmd_pos < m_cmd.length()) {
        m_current_cmd_char = m_cmd[m_current_cmd_pos++];
    } else {
        m_current_cmd_char = 0;
    }
}

void scanner::skip_spaces()
{
    while (std::isspace(m_current_cmd_char)) {
        read_next_char();
    }
}

void scanner::fill_token_vec()
{
    token tok = read_next_token();

    while (tok.get_type() != TT_NIL) {
        m_token_vec.push_back(tok);
        tok = read_next_token();
    }
}

token scanner::read_next_token()
{
    auto is_string = [] (char cmp) {
        return std::isalpha(cmp) || std::isdigit(cmp) || cmp == '_';
    };

    skip_spaces();

    switch (m_current_cmd_char) {
    case '\0':
        break;
    case ':':
        read_next_char();
        return token(TT_DOUBLE_DOT);
    case '.':
        read_next_char();
        return token(TT_DOT);
    case '=':
        read_next_char();
        if (m_current_cmd_char == '=') {
            read_next_char();
            if (m_current_cmd_char == '>') {
                read_next_char();
                return token(TT_ARROW);
            }
        }
        break;
    case '{':
        read_next_char();
        return token(TT_LEFT_BRACE);
    case '}':
        read_next_char();
        return token(TT_RIGHT_BRACE);
    case '(':
        read_next_char();
        return token(TT_LEFT_BRACKET);
    case ')':
        read_next_char();
        return token(TT_RIGHT_BRACKET);
    case '-':
        read_next_char();
        return  token(TT_RANGE);
    case '/':
        read_next_char();
        return token(TT_SLASH);
    case '*':
        read_next_char();
        return token(TT_STAR);
    case '|':
        read_next_char();
        return token(TT_PIPE);
    case '"': {
        std::ostringstream s;

        read_next_char();
        while (m_current_cmd_char != 0 && m_current_cmd_char != '"') {
            s << m_current_cmd_char;
            read_next_char();
        }
        read_next_char();

        return token(TT_STRING, s.str());
    }
    default:
        if (is_string(m_current_cmd_char)) {
            std::ostringstream s;
            s << m_current_cmd_char;

            read_next_char();
            while (is_string(m_current_cmd_char)) {
                s << m_current_cmd_char;
                read_next_char();
            }

            std::string cmp_str = s.str();
            std::transform(cmp_str.begin(), cmp_str.end(), cmp_str.begin(), ::tolower);
            if (cmp_str.compare("protocol") == 0) {
                return TT_PROTOCOL;
            } else if (cmp_str.compare("mldv1") == 0) {
                return TT_MLDV1;
            } else if (cmp_str.compare("mldv2") == 0) {
                return TT_MLDV2;
            } else if (cmp_str.compare("igmpv1") == 0) {
                return TT_IGMPV1;
            } else if (cmp_str.compare("igmpv2") == 0) {
                return TT_IGMPV2;
            } else if (cmp_str.compare("igmpv3") == 0) {
                return TT_IGMPV3;
            } else if (cmp_str.compare("pinstance") == 0) {
                return TT_PINSTANCE;
            } else if (cmp_str.compare("upstream") == 0) {
                return TT_UPSTREAM;
            } else if (cmp_str.compare("downstream") == 0) {
                return TT_DOWNSTREAM;
            } else if (cmp_str.compare("rulematching") == 0) {
                return TT_RULE_MATCHING;
            } else if (cmp_str.compare("out") == 0) {
                return TT_OUT;
            } else if (cmp_str.compare("in") == 0) {
                return TT_IN;
            } else if (cmp_str.compare("blacklist") == 0) {
                return TT_BLACKLIST;
            } else if (cmp_str.compare("whitelist") == 0) {
                return TT_WHITELIST;
            } else if (cmp_str.compare("table") == 0) {
                return TT_TABLE;
            } else if (cmp_str.compare("all") == 0) {
                return TT_ALL;
            } else if (cmp_str.compare("first") == 0) {
                return TT_FIRST;
            } else if (cmp_str.compare("mutex") == 0) {
                return TT_MUTEX;
            } else if (cmp_str.compare("disable") == 0) {
                return TT_DISABLE;
            } else {
                return token(TT_STRING, s.str());
            }
        } else {
            HC_LOG_ERROR("failed to scan config file. Unsupported char <" << m_current_cmd_char << "> in line " << m_current_line << " and postion " << m_current_cmd_pos);
            throw "failed to scan config file. Unsupported char";
        }
        break;
    }

    return token(TT_NIL);
}

std::string scanner::to_string() const
{
    HC_LOG_TRACE("");
    using namespace std;

    ostringstream s;
    s << "##-- scanner --##" << endl;
    s << m_current_line << ": " << m_cmd << endl;
    s << " ==> ";
    int i = 1;
    for (auto e : m_token_vec) {
        if (i % 5 == 0) {
            s << endl;
        }

        s << get_token_type_name(e.get_type());
        if (!e.get_string().empty()) {
            s << "(" << e.get_string() << ") ";
        } else {
            s << " ";
        }

        ++i;
    }

    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const scanner& scan)
{
    return stream << scan.to_string();
}
