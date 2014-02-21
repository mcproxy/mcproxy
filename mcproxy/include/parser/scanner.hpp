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

#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>
#include <vector>

class token;

class scanner
{
private:
    unsigned int m_current_line;
    std::string m_cmd;
    unsigned int m_current_cmd_pos; 
    char m_current_cmd_char;
         
    std::vector<token> m_token_vec;
    unsigned int m_token_pos;

    void read_next_char();
    void skip_spaces();
    token read_next_token();    

    void fill_token_vec();

public:
    scanner(unsigned int current_line, const std::string& cmd);
    
    token get_next_token(bool peek = false, int token_count = 1);

    std::string to_string() const;
    friend std::ostream& operator<<(std::ostream& stream, const scanner& scan);
};

#endif // SCANNER_HPP
