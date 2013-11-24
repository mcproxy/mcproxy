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
#include "include/parser/configuration.hpp"
#include "include/parser/parser.hpp"

#include <algorithm>
#include <fstream>

configuration::configuration(const std::string& path)
{
    HC_LOG_TRACE("");
    m_cmds = separate_commands(delete_comments(load_file(path)));
}

// trim from start
inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
inline std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

inline unsigned int count_chars(const std::string& s, const char comp)
{
    unsigned int count = 0;
    std::for_each(s.begin(), s.end(), [&count, &comp](const char c) {
        if (c == comp) {
            ++count;
        }
    });
    return count;
}

std::string configuration::load_file(const std::string& path)
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
        result << line << std::endl;
    }

    return result.str();
}

std::string configuration::delete_comments(std::string&& script_file)
{
    HC_LOG_TRACE("");

    const char comment_char = '#';
    const char end_of_comment = '\n';
    std::string::size_type cc = script_file.find(comment_char);
    std::string::size_type eoc;

    while (cc != std::string::npos) {
        eoc = script_file.find(end_of_comment, cc + 1);
        if (eoc == std::string::npos) {
            return script_file.substr(0, cc);
        } else {
            script_file.erase(cc, eoc - cc);
        }

        cc = script_file.find(comment_char, cc);
    }
    return script_file;
}

std::vector<std::pair<unsigned int, std::string>> configuration::separate_commands(std::string&& script_file)
{
    HC_LOG_TRACE("");
    std::stringstream ss(script_file);

    std::vector<std::pair<unsigned int, std::string>> result;
    const char cmd_separator = ';';
    std::string item;

    unsigned int current_line = 1;


    while (std::getline(ss, item, cmd_separator)) {
        unsigned int line_count_before = count_chars(item, '\n');
        ltrim(item);
        unsigned int line_count_after = count_chars(item, '\n');
        rtrim(item);
        if (!item.empty()) {
            result.push_back(std::pair<unsigned int, std::string>((current_line + (line_count_before - line_count_after)), item));
        }
        current_line += line_count_before;
    }

    return result;
}

void configuration::test_configuration()
{
    using namespace std;
    cout << "start programm" << endl;

    configuration conf("../references/parser/config_script_example");
    //configuration conf("../references/parser/test_script_1");
    //configuration conf("../references/parser/test_script");


    group_mem_protocol gmp = IGMPv3;
    auto gts = std::make_shared<global_table_set>();
    set<instance_definition> instance_def_set;


    for (auto e : conf.m_cmds) {
        parser p(e.first, e.second);
        try {
            switch (p.get_parser_type()) {
            case PT_PROTOCOL: {
                cout << "cmd: " << e.second << endl;
                gmp = p.parse_group_mem_proto();
                cout << "PT_PROTOCOL: " << get_group_mem_protocol_name(gmp) << endl;
                cout << endl;
                break;
            }
            case PT_INSTANCE_DEFINITION: {
                cout << "cmd: " << e.second << endl;
                cout << "PT_INSTANCE_DEFINITION: ";
                auto tmp = p.parse_instance_definition();
                cout << tmp.to_string() << endl;
                cout << endl;
                break;
            }
            case PT_TABLE: {
                cout << "cmd: " << e.second << endl;
                auto t = p.parse_table(gts, gmp);
                cout << "PT_TABLE: " << t->to_string() << endl;
                gts->add_table(std::move(t));
                cout << endl;
                break;
            }
            case PT_INTERFACE_RULE_BINDING:
                cout << "cmd: " << e.second << endl;
                cout << "PT_INTERFACE_RULE_BINDING" << endl;
                cout << endl;
                break;
            }
        } catch (const char* c) {
            cout << c << endl;
        }
    }




    //cout << "1<" << s.delete_comments("#1234\n1234") << ">" << endl;
    //cout << "2<" << s.delete_comments("1234\n#1234") << ">" << endl;
    //cout << "3<" << s.delete_comments("#\n1234\n#1234") << ">" << endl;
    //cout << "4<" << s.delete_comments("1234#1234\n") << ">" << endl;
    //cout << "5<" << s.delete_comments("1234") << ">" << endl;
    //cout << "6<" << s.delete_comments("") << ">" << endl;
    //cout << "7<" << s.delete_comments("##\n1234") << ">" << endl;
    //cout << "8<" << s.delete_comments("\n1234#") << ">" << endl;
    //cout << "9<" << s.delete_comments("#12#34\n#56#78#910\n\n\n1234#") << ">" << endl;
    //cout << "10<" << s.delete_comments("#12#34\n#56#78#910\n1\n2\n3\n4#") << ">" << endl;
    //cout << "11<" << s.delete_comments("1234#\n") << ">" << endl;

    //auto tmp  = s.separate_commands(";asd; ad; xx;;;");
    //std::for_each(tmp.begin(), tmp.end(), [](std::string & n) {
    //std::cout << n << std::endl;
    //});

    //std::for_each(s.m_cmds.begin(), s.m_cmds.end(), [&](std::pair<unsigned int, std::string>& n) {
    //std::cout << n.first << ":=" << n.second << std::endl;
    //});
    //

    cout << "end of programm" << endl;
}


