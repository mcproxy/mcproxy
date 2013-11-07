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

#include <fstream>

scanner::scanner(const std::string& path)
{
    HC_LOG_TRACE("");
    m_input = separate_commands(delete_comments(load_file(path)));
}

std::string scanner::load_file(const std::string& path)
{
    HC_LOG_TRACE("");
    std::ifstream file;
    std::ostringstream result;
    std::string line;
    file.open (path, std::ifstream::in);
    if (!file) {
        HC_LOG_ERROR("failed to open config file: " << path);
        throw "failed to open config file";
    }

    while (std::getline(file, line)) {
        result << delete_comments(line);
    }

    return result.str();
}

std::vector<std::string> scanner::separate_commands(const std::string& script_file)
{
    HC_LOG_TRACE("");

}

std::string scanner::delete_comments(const std::string& script_file)
{
    HC_LOG_TRACE("");


}

void scanner::test_scanner()
{
    scanner s("../references/parser/config_script_example");


}
