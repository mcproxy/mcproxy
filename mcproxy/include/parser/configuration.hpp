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

#include <string>
#include <vector>


class configuration 
{
private:
    //<line number (for a better error message output), command>
    std::vector<std::pair<unsigned int, std::string>> m_cmds; 

    std::string load_file(const std::string& path);
    std::string delete_comments(std::string&& script_file);
    std::vector<std::pair<unsigned int, std::string>> separate_commands(std::string&& script_file);
public:
    configuration(const std::string& path);

    static void test_configuration();
};


#endif // CONFIGURATION_HPP

