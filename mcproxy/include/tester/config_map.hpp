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
 * written by Dominik Charousset <dominik.charousset@haw-hamburg.de>
 *
 */

#ifndef CONFIG_MAP_HPP
#define CONFIG_MAP_HPP

#include <map>
#include <string>
#include <stdexcept>

class config_map
{
public:
    typedef std::string                    key_type;
    typedef std::map<key_type,key_type>    mapped_type;
    typedef std::map<key_type,mapped_type> container_type;
    typedef container_type::const_iterator const_iterator;

    /**
     * @throws runtime_error
     */
    void read_ini(const key_type& filename);

    inline bool has_group(const key_type& group) const {
        return m_data.count(group) > 0;
    }

    inline const_iterator begin() const { return m_data.begin(); }

    inline const_iterator end() const { return m_data.end(); }

    inline const_iterator find(const key_type& group) const {
        return m_data.find(group);
    }

    inline const key_type& get(const_iterator group, const key_type& key) const {
        mapped_type::const_iterator j = group->second.find(key);
        if (j != group->second.end()) return j->second;
        return m_empty;
    }

    inline const key_type& get(const key_type& group, const key_type& key) const {
        container_type::const_iterator i = m_data.find(group);
        if (i != m_data.end()) return get(i, key);
        return m_empty;
    }

    /**
     * @throws range_error if @p group is unknown
     */
    inline const mapped_type& operator[](const key_type& group) const {
        container_type::const_iterator i = m_data.find(group);
        if (i == m_data.end()) throw std::range_error("unknown group: " + group);
        return i->second;
    }

    inline unsigned int size() const{
        return m_data.size();         
    }

private:
    key_type m_empty;
    container_type m_data;
};

#endif // CONFIG_MAP_HPP
