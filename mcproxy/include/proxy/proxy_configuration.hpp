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

#ifndef PROXY_CONFIGURATION_HPP
#define PROXY_CONFIGURATION_HPP

#include "include/proxy/interfaces.hpp"
#include "include/proxy/def.hpp"

#include <memory>
#include <set>


/**
 * @brief Default path of the configuration file.
 */
#define PROXY_CONFIGURATION_DEFAULT_CONIG_PATH "mcproxy.conf"

using downstream_set = std::set<unsigned int>;
using upstream_downstream_map = std::map<unsigned int, downstream_set>; //map<upstream, downstreams>
using upstream_downsteram_pair = std::pair<unsigned int, downstream_set>;

/**
 * @brief load and maintain the configuration of all proxies
 */
class proxy_configuration
{
private:
    std::shared_ptr<interfaces> m_interfaces;
    group_mem_protocol m_group_mem_protocol;
    upstream_downstream_map m_upstream_downstream_map;

    upstream_downstream_map parse_config(const std::string& path);
public:
    proxy_configuration(const std::string& path, bool reset_reverse_path_filter);
    virtual ~proxy_configuration();


    bool add_downstream(unsigned int if_index_upstream, unsigned int if_index_downstream);
    bool add_upstream(unsigned int if_index_upsteram);
    bool del_downstream(unsigned int if_index_upstream, unsigned int if_index_downstream);
    bool del_upstream(unsigned int if_index_upstream);

    const upstream_downstream_map& get_upstream_downstream_map() const;
    const std::shared_ptr<const interfaces> get_interfaces() const;

    group_mem_protocol get_group_mem_protocol() const;

    std::string to_string() const;
    
    friend std::ostream& operator<<(std::ostream& stream, const proxy_configuration& pc);

    static void test_proxy_configuration();
};

#endif // PROXY_CONFIGURATION_HPP
