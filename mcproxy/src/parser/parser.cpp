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
 * * You should have received a copy of the GNU General Public License * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * written by Sebastian Woelke, in cooperation with:
 * INET group, Hamburg University of Applied Sciences,
 * Website: http://mcproxy.realmv6.org/
 */

#include "include/hamcast_logging.h"
#include "include/parser/parser.hpp"

#include <algorithm>

#include <stdexcept>
	
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
        if (cmp_token.get_type() == TT_DOUBLE_DOT || cmp_token.get_type() == TT_LEFT_BRACKET) {
            return PT_INSTANCE_DEFINITION;
        } else if (cmp_token.get_type() == TT_UPSTREAM || cmp_token.get_type() == TT_DOWNSTREAM) {
            return PT_INTERFACE_RULE_BINDING;
        } else {
            HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(cmp_token.get_type()) << " with value " << cmp_token.get_string() << ", expected \":\" or \"upstream\" or \"downstream\"");
            throw "failed to parse config file";
        }
    } else if(m_current_token.get_type() == TT_DISABLE) {
        throw "mcproxy is disabled";
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
    }else{
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << ", expected \"protocol\"");
        throw "failed to parse config file";
    }

    get_next_token();
    if (m_current_token.get_type() == TT_NIL) {
        return result;
    } else {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string());
        throw "failed to parse config file";
    }
}

void parser::parse_instance_definition(inst_def_set& ids)
{
    HC_LOG_TRACE("");

    //pinstance = "pinstance" @instance_name@ (instance_definition | interface_rule_binding);
    //instance_definition = ":" {@if_name@} "==>" @if_name@ {@if_name@};
    std::list<std::shared_ptr<interface>> upstreams;
    std::list<std::shared_ptr<interface>> downstreams;
    std::string instance_name;
    int table_number = 0;
    bool user_selected_table_number = false;

    if (get_parser_type() == PT_INSTANCE_DEFINITION) {
        get_next_token();
        if (m_current_token.get_type() == TT_STRING) {
            instance_name = m_current_token.get_string();
            get_next_token();

            if (m_current_token.get_type() == TT_LEFT_BRACKET) {
                get_next_token();
                if (m_current_token.get_type() == TT_STRING) {
                    try {
                        table_number = atoi(m_current_token.get_string().c_str());
                        user_selected_table_number = true;
                    } catch (std::logic_error e) {
                        HC_LOG_ERROR("failed to parse line " << m_current_line << " table number: " << table_number << " is not a number");
                        throw "failed to parse config file";
                    }

                    get_next_token();
                    if (m_current_token.get_type() == TT_RIGHT_BRACKET) {
                        get_next_token();
                    } else {
                        HC_LOG_ERROR("failed to parse line " << m_current_line << " instance " << instance_name << " with unknown table number");
                        throw "failed to parse config file";
                    }
                } else {
                    HC_LOG_ERROR("failed to parse line " << m_current_line << " instance " << instance_name << " with unknown table number");
                    throw "failed to parse config file";
                }
            }

            if (m_current_token.get_type() == TT_DOUBLE_DOT) {
                get_next_token();
                while (m_current_token.get_type() == TT_STRING) {
                    upstreams.push_back(std::make_shared<interface>(m_current_token.get_string()));
                    get_next_token();
                }

                if (m_current_token.get_type() == TT_ARROW) {

                    get_next_token();
                    while (m_current_token.get_type() == TT_STRING) {
                        downstreams.push_back(std::make_shared<interface>(m_current_token.get_string()));
                        get_next_token();
                    }

                    if (downstreams.size() > 0 && m_current_token.get_type() == TT_NIL) {
                        if (!ids.insert(std::make_shared<instance_definition>(instance_name, std::move(upstreams), std::move(downstreams), table_number, user_selected_table_number))) {
                            HC_LOG_ERROR("failed to parse line " << m_current_line << " instance " << instance_name << " already exists");
                            throw "failed to parse config file";
                        } else {
                            return;
                        }
                    }
                }
            }
        }
    }

    HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
    throw "failed to parse config file";
}

std::unique_ptr<table> parser::parse_table(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp, bool inside_rule_box)
{
    HC_LOG_TRACE("");
    std::string table_name;
    std::list<std::unique_ptr<rule_box>> rule_box_list;

    if (get_parser_type() == PT_TABLE) {
        get_next_token();
        if ((!inside_rule_box && m_scanner.get_next_token(true, 0).get_type() == TT_NIL) || (inside_rule_box && m_scanner.get_next_token(true, 0).get_type() == TT_RIGHT_BRACKET )) { //table reference
            if (m_current_token.get_type() == TT_STRING) {
                table_name = m_current_token.get_string();
                if (gts->get_table(table_name) != nullptr) {
                    get_next_token();

                    auto rb = std::unique_ptr<rule_box>(new rule_table_ref(table_name, gts));
                    rule_box_list.push_back(std::move(rb));
                    return std::unique_ptr<table>(new table(std::string(), std::move(rule_box_list)));
                } else {
                    HC_LOG_ERROR("failed to parse line " << m_current_line << " table " << table_name << " not found");
                    throw "failed to parse config file";
                }

            }
        } else if (m_current_token.get_type() == TT_STRING || m_current_token.get_type() == TT_LEFT_BRACE) {
            if (m_current_token.get_type() == TT_STRING) {
                table_name = m_current_token.get_string();

                get_next_token();
                if (m_current_token.get_type() != TT_LEFT_BRACE) {
                    HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
                    throw "failed to parse config file";
                }
            }

            get_next_token();
            auto tmp_rule = parse_rule(gts, gmp);

            while (tmp_rule != nullptr) {
                rule_box_list.push_back(std::move(tmp_rule));
                get_next_token();
                tmp_rule = parse_rule(gts, gmp);
            }

            if (m_current_token.get_type() == TT_RIGHT_BRACE) {
                get_next_token();
                if ((!inside_rule_box && m_current_token.get_type() == TT_NIL) || (inside_rule_box && m_current_token.get_type() == TT_RIGHT_BRACKET)) {
                    return std::unique_ptr<table>(new table(table_name, std::move(rule_box_list)));
                }
            }

        }
    }
    HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
    throw "failed to parse config file";
}

std::unique_ptr<rule_box> parser::parse_rule(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp)
{
    HC_LOG_TRACE("");
    std::string if_name;
    if (m_current_token.get_type() == TT_STRING || m_current_token.get_type() == TT_LEFT_BRACKET) {
        if (m_current_token.get_type() == TT_STRING) {
            if_name = m_current_token.get_string();
            get_next_token();
        }

        if (m_current_token.get_type() == TT_LEFT_BRACKET) {
            get_next_token();
            if (m_current_token.get_type() == TT_TABLE) {
                std::unique_ptr<rule_box> result(new rule_table(parse_table(gts, gmp, true)));
                return result;
            } else {
                std::unique_ptr<addr_match> group;
                std::unique_ptr<addr_match> source;
                group =  parse_rule_part(gmp);

                if (m_current_token.get_type() == TT_PIPE) {
                    get_next_token();
                    source = parse_rule_part(gmp);

                    if (m_current_token.get_type() == TT_RIGHT_BRACKET) {
                        std::unique_ptr<rule_box> result(new rule_addr(if_name, std::move(group), std::move(source)));
                        return result;
                    }
                }
            }
        }
    } else {
        return nullptr;
    }

    HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
    throw "failed to parse config file";
}

std::unique_ptr<table> parser::parse_table(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp)
{
    return parse_table(gts, gmp, false);
}

std::unique_ptr<addr_match> parser::parse_rule_part(group_mem_protocol gmp)
{
    HC_LOG_TRACE("");
    //TT_STAR
    //TT_STRING
    //single_addr
    //addr_range

    addr_storage addr_from(get_addr_family(gmp));
    addr_storage addr_to(get_addr_family(gmp));
    if (m_current_token.get_type() == TT_STRING || m_current_token.get_type() == TT_STAR) {
        if (m_current_token.get_type() == TT_STRING) {
            addr_from = get_addr(gmp);
        } else { //TT_STAR
            get_next_token();
        }

        if (m_current_token.get_type() == TT_SLASH) {
            get_next_token();
            if (m_current_token.get_type() == TT_STRING) {
                try {
                    unsigned int prefix = atoi(m_current_token.get_string().c_str());
                    if (prefix > 128) {
                        throw;
                    }

                    addr_to = addr_from;
                    addr_from.mask(prefix);
                    addr_to.broadcast_addr(prefix);

                    get_next_token();
                    if (m_current_token.get_type() == TT_RIGHT_BRACKET || m_current_token.get_type() == TT_PIPE) {
                        std::unique_ptr<addr_match> result(new addr_range(addr_from, addr_to));
                        return result;
                    }
                } catch (...) {
                    HC_LOG_ERROR("failed to parse line " << m_current_line << " token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " cant be converted to a prefix or subnet mask");
                    throw "failed to parse config file";
                }
            }
        } else if (m_current_token.get_type() == TT_RIGHT_BRACKET || m_current_token.get_type() == TT_PIPE) {
            std::unique_ptr<addr_match> result(new single_addr(addr_from));
            return result;
        } else if (m_current_token.get_type() == TT_RANGE) {
            get_next_token();
            if (m_current_token.get_type() == TT_STRING || m_current_token.get_type() == TT_STAR) {
                if (m_current_token.get_type() == TT_STRING) {
                    addr_to = get_addr(gmp);
                } else {
                    get_next_token();
                }

                if (m_current_token.get_type() == TT_RIGHT_BRACKET || m_current_token.get_type() == TT_PIPE) {
                    std::unique_ptr<addr_match> result(new addr_range(addr_from, addr_to));
                    return result;
                }
            }
        }
    }

    HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
    throw "failed to parse config file";
}

addr_storage parser::get_addr(group_mem_protocol gmp)
{
    HC_LOG_TRACE("");
    std::ostringstream s;

    while (true) {
        if (m_current_token.get_type() == TT_STRING) {
            s << m_current_token.get_string();
        } else if (m_current_token.get_type() == TT_DOT) {
            s << ".";
        } else if (m_current_token.get_type() == TT_DOUBLE_DOT) {
            s << ":";
        } else {
            break;
        }
        get_next_token();
    }

    addr_storage result(s.str());
    if (result.is_valid()) {
        if (result.get_addr_family() == get_addr_family(gmp)) {
            return result;
        } else {
            HC_LOG_ERROR("failed to parse line " << m_current_line << " ip address: " << s.str() << " has a wrong IP version");
            throw "failed to parse config file";
        }
    } else {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " ip address: " << s.str() << " is invalid");
        throw "failed to parse config file";
    }
}

void parser::parse_interface_rule_binding(const std::shared_ptr<const global_table_set>& gts, group_mem_protocol gmp, const inst_def_set& ids)
{
    HC_LOG_TRACE("");

    //pinstance split downstream tunD1 out whitelist table {tunU1(* | *)};

    std::string instance_name;
    rb_interface_type interface_type;
    std::string if_name;
    rb_interface_direction filter_direction;

    auto error_notification = [&]() {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
        throw "failed to parse config file";
    };

    if (get_parser_type() == PT_INTERFACE_RULE_BINDING) {
        get_next_token();
        if (m_current_token.get_type() == TT_STRING) {
            instance_name = m_current_token.get_string();
            auto it = ids.find(instance_name);
            if (it == ids.end()) {
                HC_LOG_ERROR("failed to parse line " << m_current_line << " proxy instance " << m_current_token.get_string() << " not defined");
                throw "failed to parse config file";
            }
        } else {
            error_notification();
        }

        get_next_token();
        if (m_current_token.get_type() == TT_UPSTREAM) {
            interface_type = IT_UPSTREAM;
        } else if (m_current_token.get_type() == TT_DOWNSTREAM) {
            interface_type = IT_DOWNSTREAM;
        } else {
            error_notification();
        }

        get_next_token();
        if (m_current_token.get_type() == TT_STRING) {
            if_name = m_current_token.get_string();
        } else if (m_current_token.get_type() == TT_STAR) {
            if_name = "*";
        } else {
            error_notification();
        }

        get_next_token();
        if (m_current_token.get_type() == TT_IN) {
            filter_direction = ID_IN;
        } else if (m_current_token.get_type() == TT_OUT) {
            filter_direction = ID_OUT;
        } else {
            error_notification();
        }

        get_next_token();
        if (m_current_token.get_type() == TT_WHITELIST || m_current_token.get_type() == TT_BLACKLIST) {
            return parse_interface_table_binding(std::move(instance_name), interface_type, std::move(if_name), filter_direction, gts, gmp, ids);
        } else if (m_current_token.get_type() == TT_RULE_MATCHING) {
            return parse_interface_rule_match_binding(std::move(instance_name), interface_type, std::move(if_name), filter_direction, ids);
        } else {
            error_notification();
        }
    } else {
        error_notification();
    }

    HC_LOG_ERROR("this code should never reached");
    throw "this code should never reached";
}

void parser::parse_interface_table_binding(
    std::string && instance_name
    , rb_interface_type interface_type
    , std::string && if_name
    , rb_interface_direction filter_direction
    , const std::shared_ptr<const global_table_set>& gts
    , group_mem_protocol gmp
    , const inst_def_set& ids)
{
    HC_LOG_TRACE("");
    auto error_notification = [&]() {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
        throw "failed to parse config file";
    };

    rb_filter_type filter_type = FT_UNDEFINED;
    std::unique_ptr<table> filter_table;

    //pinstance split downstream tunD1 out whitelist table {tunU1(* | *)};
    if (m_current_token.get_type() == TT_WHITELIST) {
        filter_type = FT_WHITELIST;
    } else if (m_current_token.get_type() == TT_BLACKLIST) {
        filter_type = FT_BLACKLIST;
    } else {
        error_notification();
    }

    get_next_token();
    filter_table = parse_table(gts, gmp);

    if (m_current_token.get_type() != TT_NIL) {
        error_notification();
    }

    auto instance_it = ids.find(instance_name);
    if (instance_it != ids.end()) {
        std::unique_ptr<rule_binding> rb(new rule_binding(instance_name, interface_type, if_name, filter_direction, filter_type, std::move(filter_table)));

        std::shared_ptr<interface> interf = nullptr;

        if (interface_type == IT_UPSTREAM) {
            auto interface_it = std::find((*instance_it)->m_upstreams.begin(), (*instance_it)->m_upstreams.end(), std::make_shared<interface>(if_name));
            if (interface_it != (*instance_it)->m_upstreams.end()) {
                interf = *interface_it;
            } else {
                HC_LOG_ERROR("failed to parse line " << m_current_line << " upstream interface " << if_name << " not defined");
                throw "failed to parse config file";
            }
        } else if (interface_type == IT_DOWNSTREAM) {
            auto interface_it = std::find((*instance_it)->m_downstreams.begin(), (*instance_it)->m_downstreams.end(), std::make_shared<interface>(if_name));
            if (interface_it != (*instance_it)->m_downstreams.end()) {
                interf = *interface_it;
            } else {
                HC_LOG_ERROR("failed to parse line " << m_current_line << " downstream interface " << if_name << " not not defined");
                throw "failed to parse config file";
            }
        } else {
            HC_LOG_ERROR("failed to parse line " << m_current_line << " interface tpye not defined");
            throw "failed to parse config file";
        }

        if (filter_direction == ID_IN) {
            if (interf->m_input_filter == nullptr) {
                interf->m_input_filter = std::move(rb);
                return;
            } else {
                HC_LOG_ERROR("failed to parse line " << m_current_line << " input filter for interface " << if_name << " already defined");
                throw "failed to parse config file";
            }
        } else if (filter_direction == ID_OUT) {
            if (interf->m_output_filter == nullptr) {
                interf->m_output_filter = std::move(rb);
                return;
            } else {
                HC_LOG_ERROR("failed to parse line " << m_current_line << " output filter for interface " << if_name << " already defined");
                throw "failed to parse config file";
            }
        } else {
            HC_LOG_ERROR("failed to parse line " << m_current_line << " rule direction not defined");
            throw "failed to parse config file";
        }
    } else {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " proxy instance " << m_current_token.get_string() << " not defined");
        throw "failed to parse config file";
    }
}

void parser::parse_interface_rule_match_binding(
    std::string && instance_name
    , rb_interface_type interface_type
    , std::string && if_name
    , rb_interface_direction filter_direction
    , const inst_def_set& ids)
{
    HC_LOG_TRACE("");
    auto error_notification = [&]() {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " unknown token " << get_token_type_name(m_current_token.get_type()) << " with value " << m_current_token.get_string() << " in this context");
        throw "failed to parse config file";
    };

    rb_rule_matching_type rule_matching_type = RMT_UNDEFINED;
    std::chrono::milliseconds timeout(0);
    //pinstance A upstream ap in rulematching mutex 10000;
    if (m_current_token.get_type() == TT_RULE_MATCHING) {
        get_next_token();
        if (m_current_token.get_type() == TT_ALL) {
            rule_matching_type = RMT_ALL;
        } else if (m_current_token.get_type() == TT_FIRST) {
            rule_matching_type = RMT_FIRST;
        } else if (m_current_token.get_type() == TT_MUTEX) {
            rule_matching_type = RMT_MUTEX;

            get_next_token();
            if (m_current_token.get_type() == TT_STRING) {
                try {
                    int tmp_timeout = atoi(m_current_token.get_string().c_str());
                    timeout = std::chrono::milliseconds(tmp_timeout);
                } catch (...) {
                    error_notification();
                }
            } else {
                error_notification();
            }

        } else {
            error_notification();
        }
    } else {
        error_notification();
    }

    get_next_token();
    if (m_current_token.get_type() != TT_NIL) {
        error_notification();
    }

    auto instance_it = ids.find(instance_name);
    if (instance_it != ids.end()) {
        auto rb = std::make_shared<rule_binding>(instance_name, interface_type, if_name, filter_direction, rule_matching_type, timeout);
        (*instance_it)->m_global_settings.push_back(rb);
        return;
    } else {
        HC_LOG_ERROR("failed to parse line " << m_current_line << " proxy instance " << m_current_token.get_string() << " not defined");
        throw "failed to parse config file";
    }

    //auto it = m_inst_def_set.find(rb->get_instance_name());
    //if (it != m_inst_def_set.end()) {
    //if (rb->get_rule_binding_type() == RBT_FILTER) {
    //if (rb->get_interface_type() == IT_UPSTREAM) {
    //auto upstream_list = (*it)->get_upstreams();
    //auto interface_it = std::find(upstream_list.begin(), upstream_list.end(),rb->get_if_name());

    //interface_it->
    //} else if (rb->get_interface_type() == IT_DOWNSTREAM) {

    //} else {

    //}
    //} else if (rb->get_rule_binding_type() == RBT_RULE_MATCHING) {

    //} else {

    //}
    //}
}

void parser::get_next_token()
{
    m_current_token = m_scanner.get_next_token();
}
