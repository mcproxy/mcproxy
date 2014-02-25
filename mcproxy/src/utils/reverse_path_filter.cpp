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

#include "include/utils/reverse_path_filter.hpp"
#include "include/hamcast_logging.h"

#include <fstream>

reverse_path_filter::reverse_path_filter()
    : m_used_earlier(false)
{
    HC_LOG_TRACE("");
}

reverse_path_filter::~reverse_path_filter()
{
    HC_LOG_TRACE("");
    restore_rp_filter();
}

void reverse_path_filter::restore_rp_filter()
{
    HC_LOG_TRACE("");

    for (auto & e : m_restore_if_state) {
        set_rp_filter(e, true);
    }
}

bool reverse_path_filter::get_rp_filter(const std::string& if_name) const
{
    HC_LOG_TRACE("");
    std::stringstream path;
    bool state;
    path << REVERSE_PATH_FILTER_PATH << if_name << "/rp_filter";

    std::ifstream is(path.str().c_str(), std::ios::binary | std::ios::in);
    if (!is) {
        HC_LOG_ERROR("failed to open file:" << path.str());
        return false;
    } else {
        state = is.get();
    }
    is.close();

    return state;
}

bool reverse_path_filter::set_rp_filter(const std::string& if_name, bool set_to) const
{
    HC_LOG_TRACE("");
    std::stringstream path;
    path << REVERSE_PATH_FILTER_PATH << if_name << "/rp_filter";

    std::ofstream os(path.str().c_str(), std::ios::binary | std::ios::out);
    if (!os) {
        HC_LOG_ERROR("failed to open file:" << path.str() << " and set rp_filter to " << set_to);
        return false;
    } else {
        if (set_to) {
            os.put('1');
        } else {
            os.put('0');
        }
    }
    os.flush();
    os.close();
    return true;
}

void reverse_path_filter::reset_rp_filter(const std::string& if_name)
{
    HC_LOG_TRACE("");
    if (get_rp_filter(if_name)) {
        if (set_rp_filter(if_name, false)) {
            m_restore_if_state.insert(if_name);

            if (!m_used_earlier) {
                m_used_earlier = true;
                reset_rp_filter(REVERSE_PATH_FILTER_THE_ALL_INTERFACE);
            }
        }
    }
}

void reverse_path_filter::restore_rp_filter(const std::string& if_name)
{
    HC_LOG_TRACE("");
    auto it = m_restore_if_state.find(if_name);
    if (it != end(m_restore_if_state)) {
        m_restore_if_state.erase(it);
        set_rp_filter(if_name, true);
    }
}

std::string reverse_path_filter::to_string() const
{
    HC_LOG_TRACE("");
    std::ostringstream s;

    s << "disabled reverse path filter on following interfaces:";
    for (auto & e : m_restore_if_state) {
        s << std::endl << "\t-" <<  e;
    }
    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const reverse_path_filter& r)
{
    HC_LOG_TRACE("");
    stream << r.to_string();
    return stream;
}
