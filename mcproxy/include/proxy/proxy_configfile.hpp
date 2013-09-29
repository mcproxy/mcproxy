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

#ifndef PROXY_CONFIGFILE_HPP
#define PROXY_CONFIGFILE_HPP

#include "include/proxy/interfaces.hpp"

#include <memory>


/**
 * @brief Default path to find the config file.
 */
#define PROXY_CONFIGFILE_DEFAULT_CONIG_PATH "mcproxy.conf"

/**
 * @brief Maximum length of a line in the config file.
 */
#define PROXY_CONFIGFILE_LINE_LENGTH 200

class proxy_configfile
{
private:
    const shared_ptr<interfaces> m_interfaces;
public:
    proxy_configfile(const shared_ptr<interfaces> interfaces);
    ~proxy_configfile();

    bool load_config(std::string path);
    std::string get_parsed_state();
};


#endif // PROXY_CONFIGFILE_HPP
