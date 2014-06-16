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

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include "include/parser/token.hpp"
#include "include/parser/interface.hpp"
#include "include/proxy/def.hpp"
#include "include/proxy/interfaces.hpp"

#include <string>
#include <vector>
#include <memory>

#define CONFIGURATION_DEFAULT_CONIG_PATH "mcproxy.conf"

class configuration
{
private:
    const bool m_in_debug_testing_mode;

    bool m_reset_reverse_path_filter;
    group_mem_protocol m_gmp;
    std::shared_ptr<global_table_set> m_global_table_set;
    inst_def_set m_inst_def_set;

    //<line number (for a better error message output), command>
    std::vector<std::pair<unsigned int, std::string>> m_cmds;

    std::string load_file(const std::string& path);
    std::string delete_comments(std::string&& script_file);
    std::vector<std::pair<unsigned int, std::string>> separate_commands(std::string&& script_file);

    void run_parser();
    void initalize_interfaces();

    std::map<std::string, std::shared_ptr<interfaces>> m_interfaces_map;

public:
    configuration(const std::string& path, bool reset_reverse_path_filter, bool in_debug_testing_mode = false);

    const std::shared_ptr<const interfaces> get_interfaces_for_pinstance(const std::string& instance_name) const;
    group_mem_protocol get_group_mem_protocol() const;
    const inst_def_set& get_inst_def_set() const;

    std::string to_string() const;

    static void test_configuration();
    static void test_myProxy(const std::shared_ptr<instance_definition>& id);
};

#endif // CONFIGURATION_HPP
