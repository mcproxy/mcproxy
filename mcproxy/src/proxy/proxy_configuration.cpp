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


#include "include/proxy/proxy_configuration.hpp"
#include "include/hamcast_logging.h"

#include <fstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>

proxy_configuration::proxy_configuration(const std::string& path, bool reset_reverse_path_filter)
    : m_interfaces(nullptr)
    , m_group_mem_protocol(MLDv2)
{
    HC_LOG_TRACE("");
    upstream_downstream_map rc = parse_config(path);
    m_interfaces.reset(new interfaces(get_addr_family(m_group_mem_protocol), reset_reverse_path_filter));
    if (!rc.empty()) {
        for (auto & e : rc) {
            if (!add_upstream(e.first)) {
                throw  "failed to add upstream";
            }

            for (auto f : e.second) {
                if (!add_downstream(e.first, f)) {
                    throw  "failed to add downstream";
                }
            }
        }
    } else {
        HC_LOG_ERROR("failed to parse config file: " << path);
        throw "failed to parse config file";
    }

    m_upstream_downstream_map = move(rc);
}

// trim from start
std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

upstream_downstream_map proxy_configuration::parse_config(const std::string& path)
{
    HC_LOG_TRACE("");

    //0= look for upstream, 1= look for "==>" 2= look for the first downstream 3= look for additional downstreams
    enum state_machine {
        LOOK_FOR_UPSTREAM = 0,
        LOOK_FOR_ARROW = 1,
        LOOK_FOR_FIRST_DOWNSTREAM = 2,
        LOOK_FOR_ADDITIONAL_DOWNSTREAM = 3
    };

    std::ifstream file;
    std::string line;
    state_machine state = LOOK_FOR_UPSTREAM;
    int linecount = 0;
    upstream_downstream_map rc;

    file.open(path.c_str());
    if (!file) {
        HC_LOG_ERROR("can't open file: " << path);
        return upstream_downstream_map();
    } else {
        std::getline(file, line);
        linecount++;
        while (!file.eof()) {
            std::stringstream strline;
            std::string comp_str;
            strline << trim(line);

            int tmp_upstream_if = -1;
            int tmp_downstream_if = -1;
            downstream_set tmp_downstream_set;

            state = LOOK_FOR_UPSTREAM;
            if (!strline.eof()) {
                do {
                    strline >> comp_str;
                    if (!comp_str.empty() && comp_str.at(0) != '#') {

                        if (comp_str.compare("protocol") == 0) {
                            if (!strline.eof()) {
                                strline >> comp_str;
                                if (comp_str.compare("IGMPv1") == 0) {
                                    HC_LOG_DEBUG("protocol IGMPv1");
                                    m_group_mem_protocol = IGMPv1;
                                } else if (comp_str.compare("IGMPv2") == 0) {
                                    HC_LOG_DEBUG("protocol IGMPv2");
                                    m_group_mem_protocol = IGMPv2;
                                } else if (comp_str.compare("IGMPv3") == 0) {
                                    HC_LOG_DEBUG("protocol IGMPv3");
                                    m_group_mem_protocol = IGMPv3;
                                } else if (comp_str.compare("MLDv1") == 0) {
                                    HC_LOG_DEBUG("protocol MLDv1");
                                    m_group_mem_protocol = MLDv1;
                                } else if (comp_str.compare("MLDv2") == 0) {
                                    HC_LOG_DEBUG("protocol MLDv2");
                                    m_group_mem_protocol = MLDv2;
                                } else {
                                    HC_LOG_ERROR("unknown protocol: " << comp_str << " <line " << linecount << ">");
                                    return upstream_downstream_map();
                                }
                            } else {
                                HC_LOG_ERROR("unknown protocol " << " <line " << linecount << ">");
                                return upstream_downstream_map();
                            }

                            if (!strline.eof()) {
                                strline >> comp_str;
                                if (comp_str.at(0) == '#') {
                                    while (!strline.eof()) {
                                        strline >> comp_str;
                                    }
                                } else {
                                    HC_LOG_ERROR("to much arguments" << " <line " << linecount << ">");
                                    return upstream_downstream_map();
                                }
                            }
                        } else if (state == LOOK_FOR_UPSTREAM) {
                            tmp_upstream_if = interfaces::get_if_index(comp_str);

                            if (tmp_upstream_if > 0) {
                                state = LOOK_FOR_ARROW;
                                HC_LOG_DEBUG("upstream_if: " << comp_str << " (if_index=" << tmp_upstream_if << ")");
                            } else {
                                HC_LOG_ERROR("upstream interface not found: " << comp_str << " <line " << linecount << ">");
                                return upstream_downstream_map();
                            }

                        } else if (state == 1) {
                            if (comp_str.compare("==>") == 0) {
                                state = LOOK_FOR_FIRST_DOWNSTREAM;
                            } else {
                                HC_LOG_ERROR("syntax error: " << comp_str << " expected: " << " ==>" << " <line " << linecount << ">");
                                return upstream_downstream_map();
                            }
                        } else if (state == LOOK_FOR_FIRST_DOWNSTREAM || state == LOOK_FOR_ADDITIONAL_DOWNSTREAM) {
                            tmp_downstream_if = interfaces::get_if_index(comp_str);
                            if (tmp_downstream_if > 0) {
                                state = LOOK_FOR_ADDITIONAL_DOWNSTREAM;
                                HC_LOG_DEBUG("downstream_if: " << comp_str << " (if_index=" << tmp_downstream_if << ")");
                                if (!tmp_downstream_set.insert(tmp_downstream_if).second) {
                                    HC_LOG_ERROR("downstream interface " << interfaces::get_if_name(tmp_downstream_if) << " already in use <line " << linecount << ">");
                                    return upstream_downstream_map();
                                }
                            } else {
                                HC_LOG_ERROR("downstream interface not found: " << comp_str << " <line " << linecount << ">");
                                return upstream_downstream_map();
                            }
                        } else {
                            HC_LOG_ERROR("wrong state: " << state);
                            return upstream_downstream_map();
                        }

                    } else {
                        break;
                    }

                } while (!strline.eof());
            }

            if (state == LOOK_FOR_ADDITIONAL_DOWNSTREAM) {
                if (!rc.insert(upstream_downsteram_pair(tmp_upstream_if, tmp_downstream_set)).second) {
                    HC_LOG_ERROR("upstream interface " << interfaces::get_if_name(tmp_upstream_if) << " already in use <line " << linecount << ">");
                    return upstream_downstream_map();
                }
            } else if (state == LOOK_FOR_FIRST_DOWNSTREAM) {
                HC_LOG_ERROR("line incomplete: " << line << " in line " << linecount << ">");
                return upstream_downstream_map();
            }

            std::getline(file, line);
            linecount++;
        }
    }

    HC_LOG_DEBUG("group membership protocol: " << m_group_mem_protocol);
    return rc; 
}


bool proxy_configuration::add_downstream(unsigned int if_index_upstream, unsigned int if_index_downstream)
{
    HC_LOG_TRACE("");
    auto it = m_upstream_downstream_map.find(if_index_upstream);
    if (it != end(m_upstream_downstream_map)) {
        if (m_interfaces->add_interface(if_index_downstream)) {
            return it->second.insert(if_index_downstream).second;
        }
    } else {
        HC_LOG_ERROR("failed to add downstream: " << m_interfaces->get_if_name(if_index_downstream) << " upstream: " << m_interfaces->get_if_name(if_index_upstream) << " not found");
    }
    return false;
}

bool proxy_configuration::add_upstream(unsigned int if_index_upstream)
{
    HC_LOG_TRACE("");
    if (m_interfaces->add_interface(if_index_upstream)) {
        return m_upstream_downstream_map.insert(upstream_downsteram_pair(if_index_upstream, downstream_set())).second;
    }
    return false;
}

bool proxy_configuration::del_downstream(unsigned int if_index_upstream, unsigned int if_index_downstream)
{
    HC_LOG_TRACE("");
    auto ita = m_upstream_downstream_map.find(if_index_upstream);
    if (ita != end(m_upstream_downstream_map)) {
        auto itb = ita->second.find(if_index_downstream);
        if (itb != end(ita->second)) {
            ita->second.erase(itb);
            m_interfaces->del_interface(if_index_downstream);
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool proxy_configuration::del_upstream(unsigned int if_index_upstream)
{
    HC_LOG_TRACE("");
    auto ita = m_upstream_downstream_map.find(if_index_upstream);
    if (ita != end(m_upstream_downstream_map)) {
        for (auto it = std::begin(ita->second); it != std::end(ita->second); ++it ) {
            m_interfaces->del_interface(*it) ;
        }

        m_interfaces->del_interface(if_index_upstream);
        m_upstream_downstream_map.erase(ita);
        return true;
    } else {
        return false;
    }
}

const upstream_downstream_map& proxy_configuration::get_upstream_downstream_map() const
{
    HC_LOG_TRACE("");
    return m_upstream_downstream_map;
}

const std::shared_ptr<const interfaces> proxy_configuration::get_interfaces() const
{
    HC_LOG_TRACE("");
    return m_interfaces;
}

group_mem_protocol proxy_configuration::get_group_mem_protocol() const
{
    HC_LOG_TRACE("");
    return m_group_mem_protocol;
}

proxy_configuration::~proxy_configuration()
{
    HC_LOG_TRACE("");
}

std::string proxy_configuration::to_string() const
{
    using namespace std;
    HC_LOG_TRACE("");

    stringstream str;

    str << "protocol: " << endl;
    str << "\t" << get_group_mem_protocol_name(m_group_mem_protocol) << endl;

    str << endl;

    if (m_upstream_downstream_map.empty()) {
        return "state table is empty";
    }

    for (auto & e : m_upstream_downstream_map) {
        str << m_interfaces->get_if_name(e.first) << " (#" << e.first << ")" << " ==>" << endl;

        for (auto & f : e.second) {
            str << "\t" << m_interfaces->get_if_name(f)  << " (#" << f << ")" << endl;
        }
    }

    str << endl;
    str << m_interfaces->to_string();
    return str.str();
}

std::ostream& operator<<(std::ostream& stream, const proxy_configuration& pc)
{
    HC_LOG_TRACE("");
    stream << pc.to_string();
    return stream;
}

void proxy_configuration::test_proxy_configuration()
{
    HC_LOG_TRACE("");
    try {
        proxy_configuration p(PROXY_CONFIGURATION_DEFAULT_CONIG_PATH, true);
        std::cout << p << std::endl;
    } catch (const char* e) {
        std::cout << e << std::endl;
    }
}
