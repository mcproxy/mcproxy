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


#include "include/proxy/proxy_configfile.hpp"
#include "include/hamcast_logging.h"

proxy_configfile::proxy_configfile(const shared_ptr<interfaces> interfaces)
    : m_interfaces(interfaces)
{
    HC_LOG_TRACE("");


}

bool proxy_configfile::load_config(std::string path)
{
    HC_LOG_TRACE("");

    ifstream file;
    //IF_NAMESIZE
    //net/if.h, line 74
    //item ifreq*
    char cstr[PROXY_CONFIG_LINE_LENGTH];
    int state =
        0; //0= look für upstream, 1= look for "==>" 2= look for the first downstream 3= look for additional downstreams
    int linecount = 0;

    file.open(path.c_str());
    if (!file) {
        HC_LOG_ERROR("can't open file: " << path);
        return false;
    } else {
        file.getline(cstr, sizeof(cstr));
        linecount++;
        while (!file.eof()) {
            stringstream strline;
            string comp_str;
            strline << string(cstr).erase(string(cstr).find_last_not_of(' ') + 1); //trim all spaces at the end of the string

            int tmp_upstream_if = -1;
            int tmp_downstream_if = -1;
            down_vector tmp_down_vector;

            state = 0;
            if (!strline.eof()) {
                do {
                    strline >> comp_str;
                    if (!comp_str.empty() && comp_str.at(0) != '#') {

                        if (comp_str.compare("protocol") == 0) {
                            if (!strline.eof()) {
                                strline >> comp_str;
                                if (comp_str.compare("IGMPv1") == 0) {
                                    m_addr_family = AF_INET;
                                    m_version = 1;
                                } else if (comp_str.compare("IGMPv2") == 0) {
                                    m_addr_family = AF_INET;
                                    m_version = 2;
                                } else if (comp_str.compare("IGMPv3") == 0) {
                                    m_addr_family = AF_INET;
                                    m_version = 3;
                                } else if (comp_str.compare("MLDv1") == 0) {
                                    m_addr_family = AF_INET6;
                                    m_version = 1;
                                } else if (comp_str.compare("MLDv2") == 0) {
                                    m_addr_family = AF_INET6;
                                    m_version = 2;
                                } else {
                                    HC_LOG_ERROR("unknown protocol: " << comp_str << " <line " << linecount << ">");
                                    return false;
                                }
                            } else {
                                HC_LOG_ERROR("unknown protocol " << " <line " << linecount << ">");
                                return false;
                            }

                            if (!strline.eof()) {
                                strline >> comp_str;
                                if (comp_str.at(0) == '#') {
                                    while (!strline.eof()) {
                                        strline >> comp_str;
                                    }
                                } else {
                                    HC_LOG_ERROR("to much arguments" << " <line " << linecount << ">");
                                    return false;
                                }
                            }
                        } else if (state == 0) {
                            tmp_upstream_if = if_nametoindex(comp_str.c_str());

                            if (tmp_upstream_if > 0) {
                                state = 1;
                                HC_LOG_DEBUG("upstream_if: " << comp_str << " (if_index=" << tmp_upstream_if << ")");
                            } else {
                                HC_LOG_ERROR("upstream interface not found: " << comp_str << " <line " << linecount << ">");
                                return false;
                            }

                        } else if (state == 1) {
                            if (comp_str.compare("==>") == 0) {
                                state = 2;
                            } else {
                                HC_LOG_ERROR("syntax error: " << comp_str << " expected: " << " ==>" << " <line " << linecount << ">");
                                return false;
                            }
                        } else if (state == 2 || state == 3) {
                            tmp_downstream_if = if_nametoindex(comp_str.c_str());
                            if (tmp_downstream_if > 0) {
                                state = 3;
                                tmp_down_vector.push_back(tmp_downstream_if);
                            } else {
                                HC_LOG_ERROR("downstream interface not found: " << comp_str << " <line " << linecount << ">");
                                return false;
                            }
                        } else {
                            HC_LOG_ERROR("wrong state: " << state);
                            return false;
                        }


                    } else {
                        break;
                    }

                } while (!strline.eof());
            }

            if (state == 3) {
                m_up_down_map.insert(up_down_pair(tmp_upstream_if, tmp_down_vector));
            } else if (state == 2) {
                HC_LOG_ERROR("line incomplete: " << cstr << " in line " << linecount << ">");
                return false;
            }
            file.getline(cstr, sizeof(cstr));
        }
    }

    if (m_up_down_map.size() > 1) {
        m_is_single_instance = false;
    } else {
        m_is_single_instance = true;
    }

    HC_LOG_DEBUG("m_addr_family: " << m_addr_family << ";  m_version: " << m_version << "; m_is_single_instance: " <<
                 m_is_single_instance << ";");
    return true;
}

std::string proxy_configfile::get_parsed_state()
{
    HC_LOG_TRACE("");

    up_down_map::iterator it_up_down;
    stringstream str;
    char cstr[IF_NAMESIZE];

    str << "protocol: " << endl;

    if (m_addr_family == AF_INET) {
        switch (m_version) {
        case 1:
            str << "\tIGMPv1" << endl;
            break;
        case 2:
            str << "\tIGMPv2" << endl;
            break;
        case 3:
            str << "\tIGMPv3" << endl;
            break;
        default:
            str << "\tIPv4 but unknown addr family" << endl;
        }
    } else if (m_addr_family == AF_INET6) {
        switch (m_version) {
        case 1:
            str << "\tMLDv1" << endl;
            break;
        case 2:
            str << "\tMLDv2" << endl;
            break;
        default:
            str << "\tIPv6 but unknown addr family" << endl;
        }
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        str << "\tunknown IP version" << endl;
    }

    str << endl;

    if (m_up_down_map.empty()) {
        return "state table is empty";
    }

    for ( it_up_down = m_up_down_map.begin() ; it_up_down != m_up_down_map.end(); it_up_down++ ) {
        str << if_indextoname(it_up_down->first, cstr) << " (#" << it_up_down->first << ")" << " ==>" << endl;

        down_vector tmp_down_vector = it_up_down->second;
        for (unsigned int i = 0; i < tmp_down_vector.size(); i++) {
            str << "\t" << if_indextoname(tmp_down_vector[i], cstr)  << " (#" << tmp_down_vector[i] << ")" << endl;
        }
    }
    return str.str();
}
