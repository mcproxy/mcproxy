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

/**
 * @defgroup mod_proxy Proxy
 * @brief The module Proxy reads the configuration file on startup.
 *        It then configures and starts all other modules, acquire the mrt-flag
 *        on a socket for manipulating the multicast routing tables of the Linux kernel.
 * @{
 */

#ifndef PROXY_HPP
#define PROXY_HPP

#include <vector>
#include <string>
#include <memory>
#include <map>

class configuration;
class timing;
class proxy_instance;

/**
  * @brief start and maintain all proxy instances.
  */
class proxy
{
private:
    static bool m_running;
    int m_verbose_lvl;
    bool m_print_proxy_status;
    bool m_reset_rp_filter;
    std::string m_config_path;

    std::unique_ptr<configuration> m_configuration;
    std::shared_ptr<timing> m_timing;

    //table (= interface index), proxy_instance
    std::map<int, std::unique_ptr<proxy_instance>> m_proxy_instances;

    void prozess_commandline_args(int arg_count, char* args[]);
    void help_output();

    void start_proxy_instances();


    static void signal_handler(int sig);

    void start();

    unsigned int get_default_priority_interval();
public:
    /**
     * @brief Set default values of the class members and add signal handlers for the signal SIGINT and SIGTERM.
     */
    proxy(int arg_count, char* args[]);

    /**
     * @brief Release all resources and restore reverse path flags if changed.
     */
    virtual ~proxy();

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const proxy& p);
};

#endif // PROXY_HPP
/** @}*/
