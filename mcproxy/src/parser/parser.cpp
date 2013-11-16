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
        if (cmp_token.get_type() == TT_DOUBLE_DOT) {
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

group_mem_protocol parser::parse_group_mem_proto()
{
    HC_LOG_TRACE("");

    //protocol =  "protocol" potocol_type;
    //protocol_type = "MLDv1" | "MLDv2" | "IGMPv1" | "IGMPv2" | "IGMPv3";
    group_mem_protocol result;

    if (get_parser_type() == PT_PROTOCOL) {
        get_next_token();

        switch (m_current_token.get_type()) {
        case TT_MLDV1:
            result = MLDv1;
            break;
        case TT_MLDV2:
            result = MLDv2;
            break;
        case TT_IGMPV1:
            result = IGMPv1;
            break;
        case TT_IGMPV2:
            result = IGMPv2;
            break;
        case TT_IGMPV3:
            result = IGMPv3;
            break;
        default:
            HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << ", expected \"MLDV1\" or \"MLDv2\" or \"IGMPv1\" or \"IGMPv2\" or \"IGMPv3\"");
            throw "failed to parse config file";
            break;
        }
    }

    get_next_token();
    if (m_current_token.get_type() == TT_NIL) {
        return result;
    } else {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string());
        throw "failed to parse config file";
    }
}


std::tuple<std::string, std::list<std::string>, std::list<std::string>> parser::parse_instance_definition()
{
    HC_LOG_TRACE("");

    //pinstance = "pinstance" @instance_name@ (instance_definition | interface_rule_binding);
    //instance_definition = ":" {@if_name@} "==>" @if_name@ {@if_name@};
    std::list<std::string> upstreams;
    std::list<std::string> downstreams;
    std::string instance_name;

    if (get_parser_type() == PT_INSTANCE_DEFINITION) {
        get_next_token();
        if (m_current_token.get_type() == TT_STRING) {
            instance_name = m_current_token.get_string();
            get_next_token();
            if (m_current_token.get_type() == TT_DOUBLE_DOT) {

                get_next_token();
                while (m_current_token.get_type() == TT_STRING) {
                    upstreams.push_back(m_current_token.get_string());
                    get_next_token();
                }

                if (m_current_token.get_type() == TT_ARROW) {

                    get_next_token();
                    while (m_current_token.get_type() == TT_STRING) {
                        downstreams.push_back(m_current_token.get_string());
                        get_next_token();
                    }

                    if (downstreams.size() > 0 && m_current_token.get_type() == TT_NIL) {
                        return std::make_tuple(instance_name, upstreams, downstreams);

                    }
                }
            }
        }
    }

    HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
    throw "failed to parse config file";

}

void parser::get_next_token()
{
    m_current_token = m_scanner.get_next_token();
}
