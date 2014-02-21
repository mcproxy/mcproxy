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

#ifndef REVERSE_PATH_FILTER_HPP
#define REVERSE_PATH_FILTER_HPP

#include <string>
#include <set>
#include <sstream>

#define REVERSE_PATH_FILTER_PATH "/proc/sys/net/ipv4/conf/"
#define REVERSE_PATH_FILTER_THE_ALL_INTERFACE "all"

class reverse_path_filter
{
private:
    bool m_used_earlier;
    //save interfaces what musst set to true before terminating
    std::set<std::string> m_restore_if_state; 

    bool get_rp_filter(const std::string& if_name) const;
    bool set_rp_filter(const std::string& if_name, bool set_to) const;
    void restore_rp_filter();
public:
    reverse_path_filter();
    virtual ~reverse_path_filter();

    void reset_rp_filter(const std::string& if_name);
    void restore_rp_filter(const std::string& if_name);

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const reverse_path_filter& r);
};

#endif // REVERSE_PATH_FILTER_HPP
