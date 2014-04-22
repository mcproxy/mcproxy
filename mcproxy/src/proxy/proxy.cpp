/*
 *
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
#include "include/proxy/proxy.hpp"
#include "include/proxy/check_kernel.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/proxy_instance.hpp"
//#include "include/proxy/proxy_configuration.hpp"
#include "include/parser/configuration.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>

#include <signal.h>
#include <unistd.h>

bool proxy::m_running = false;

proxy::proxy(int arg_count, char* args[])
    : m_verbose_lvl(0)
    , m_print_proxy_status(false)
    , m_reset_rp_filter(false)
    , m_config_path(CONFIGURATION_DEFAULT_CONIG_PATH)
    , m_configuration(nullptr)
    , m_timing(std::make_shared<timing>())
{
    HC_LOG_TRACE("");

    signal(SIGINT, proxy::signal_handler);
    signal(SIGTERM, proxy::signal_handler);

    prozess_commandline_args(arg_count, args);

    //admin test
    // Check root privilegis
    if (geteuid() != 0) {  //no root privilegis
        HC_LOG_ERROR("The mcproxy has to be started with root privileges!");
        throw "The mcproxy has to be started with root privileges!";
    }

    m_configuration.reset(new configuration(m_config_path, m_reset_rp_filter));

    start_proxy_instances();

    start();
}


proxy::~proxy()
{
    HC_LOG_TRACE("");
}

void proxy::help_output()
{
    using namespace std;
    HC_LOG_TRACE("");
    cout << "Mcproxy version 1.1.0" << endl;

#ifdef DEBUG_MODE
    cout << " - Compiled in debug mode." << endl;
#else
    cout << " - Compiled in release mode." << endl;
#endif

    cout << "Project page: http://mcproxy.realmv6.org/" << endl;
    cout << endl;
    cout << "Usage:" << endl;
    cout << "  mcproxy [-h]" << endl;
    cout << "  mcproxy [-c]" << endl;
    cout << "  mcproxy [-r] [-d] [-s] [-v [-v]] [-f <config file>]" << endl;
    cout << endl;
    cout << "\t-h" << endl;
    cout << "\t\tDisplay this help screen." << endl;

    cout << "\t-r" << endl;
    cout << "\t\tReset the reverse path filter flag, to accept data from" << endl;
    cout << "\t\tforeign subnets." << endl;

    cout << "\t-d" << endl;
    cout << "\t\tRun in debug mode if possible. Output all log messages" << endl;
    cout << "\t\tin thread[X] files." << endl;

    cout << "\t-s" << endl;
    cout << "\t\tPrint proxy status information repeatedly." << endl;

    cout << "\t-v" << endl;
    cout << "\t\tBe verbose. Give twice to see even more messages" << endl;

    cout << "\t-f" << endl;
    cout << "\t\tTo specify the configuration file." << endl;

    cout << "\t-c" << endl;
    cout << "\t\tCheck the currently available kernel features." << endl;
}

void proxy::prozess_commandline_args(int arg_count, char* args[])
{
    HC_LOG_TRACE("");

    bool is_logging = false;
    bool is_check_kernel = false;

    if (arg_count == 1) {

    } else {
        for (int c; (c = getopt(arg_count, args, "hrdsvcf:")) != -1;) {
            switch (c) {
            case 'h':
                help_output();
                throw "";
                break;
            case 'c':
                is_check_kernel = true;
                break;
            case 'r':
                m_reset_rp_filter = true;
                break;
            case 'd':
                is_logging = true;
                break;
            case 's':
                m_print_proxy_status = true;
                break;
            case 'v':
                m_verbose_lvl++;
                break;
            case 'f':
                m_config_path = std::string(optarg);
                //if (args[optind][0] != '-') {
                //m_config_path = string(args[optind]);
                //} else {
                //HC_LOG_ERROR("no config path defined");
                //throw "no config path defined";
                //}
                break;
            default:
                HC_LOG_ERROR("Unknown argument! See help (-h) for more information.");
                throw "Unknown argument! See help (-h) for more information.";
            }
        }
    }

    if (optind < arg_count) {
        HC_LOG_ERROR("Unknown option argument: " << args[optind]);
        throw "Unknown option argument";
    }

    if (!is_logging) {
        hc_set_default_log_fun(HC_LOG_ERROR_LVL); //no fatal logs defined
    } else {
        if (m_verbose_lvl == 0) {
            hc_set_default_log_fun(HC_LOG_DEBUG_LVL);
        } else if (m_verbose_lvl >= 1) {
            hc_set_default_log_fun(HC_LOG_TRACE_LVL);
        } else {
            HC_LOG_ERROR("Unknown verbose level: " << m_verbose_lvl);
            throw "Unknown verbose level";
        }
    }

    if (is_check_kernel) {
        check_kernel ck;
        ck.check_kernel_features();
        throw "";
    }
}

unsigned int proxy::get_default_priority_interval(){
    return 100;    
}

void proxy::start_proxy_instances()
{
    HC_LOG_TRACE("");

    int table_number = 0;
    auto inst_set = m_configuration->get_inst_def_set();
    for (auto & pinstance : inst_set) {

        const std::string& instance_name = pinstance->get_instance_name();


        if (!pinstance->get_user_selected_table_number()) {
            table_number++;
            if (inst_set.size() <= 1) {
                table_number = 0; //single instance
            }
        } else {
            table_number = pinstance->get_table_number();
        }

        auto& upstreams = pinstance->get_upstreams();
        auto& downstreams = pinstance->get_downstreams();

        auto& interfaces = m_configuration->get_interfaces_for_pinstance(instance_name);

        std::unique_ptr<proxy_instance> pr_i(new proxy_instance(m_configuration->get_group_mem_protocol(), instance_name, table_number, interfaces, m_timing));

        //global rule bindung      
        auto& global_settings = pinstance->get_global_settings();
        for (auto & r : global_settings) {
                pr_i->add_msg(std::make_shared<config_msg>(config_msg::SET_GLOBAL_RULE_BINDING, r));
        }

        //add upstream
        unsigned int upstream_priority = 0;
        for (auto & u : upstreams) {
            unsigned int if_index = interfaces::get_if_index(u->get_if_name());
            if (if_index == 0) {
                HC_LOG_ERROR("failed to map upstream interface " << u->get_if_name() << " interface index");
                throw "interface not found";
            }
            pr_i->add_msg(std::make_shared<config_msg>(config_msg::ADD_UPSTREAM, if_index, upstream_priority, u));
            upstream_priority+=get_default_priority_interval();
        }

        //add downstream
        for (auto & d : downstreams) {
            unsigned int if_index = interfaces::get_if_index(d->get_if_name());
            if (if_index == 0) {
                HC_LOG_ERROR("failed to map downstream interface " << d->get_if_name() << " interface index");
                throw "interface not found";
            }

            timers_values tv;
            //std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!here I mod the timers and values for debugging aim" << std::endl;
            //tv.set_query_interval(std::chrono::seconds(15));
            //tv.set_startup_query_interval(std::chrono::seconds(15));
            //tv.set_last_listener_query_count(2);
            //tv.set_last_listener_query_interval(std::chrono::seconds(4));

            pr_i->add_msg(std::make_shared<config_msg>(config_msg::ADD_DOWNSTREAM, if_index, d, tv));
        }

        m_proxy_instances.insert(std::pair<int, std::unique_ptr<proxy_instance>>(table_number, std::move(pr_i)));

    }


}

void proxy::start()
{
    using namespace std;
    HC_LOG_TRACE("");

    m_running = true;

    if (m_print_proxy_status) {
        cout << *this << endl;
        cout << endl;
    }

    while (m_running) {

        if (m_print_proxy_status) {
            for (auto & e : m_proxy_instances) {
                e.second->add_msg(std::make_shared<debug_msg>());
                sleep(2);
            }
        } else {
            sleep(2);
        }

    }


    //kill all proxy_instances
    std::for_each(begin(m_proxy_instances), end(m_proxy_instances), [](pair<const int, std::unique_ptr<proxy_instance>>& e) {
        e.second->add_msg(std::make_shared<exit_cmd>());
    });

    //for (pair<const int, std::unique_ptr<proxy_instance>>& e : m_proxy_instances) {
    //e.second->add_msg(std::make_shared<exit_cmd>());
    //}







    //vif_map::iterator it_vif;
    //interface_map::iterator it_proxy_numb;
    //proxy_msg msg;

    ////check_if init
    //check_if check_interface;
    //vector<int> if_list_tmp;
    //up_down_map::iterator it_up_down;

    //for (it_up_down = m_up_down_map.begin() ; it_up_down != m_up_down_map.end(); it_up_down++) {
    //down_vector tmp_down_vector = it_up_down->second;
    //for (unsigned int i = 0; i < tmp_down_vector.size(); i++) {
    //if_list_tmp.push_back(tmp_down_vector[i]);
    //}
    //}

    ////init status
    ////del all down interfaces
    //if_list_tmp = check_interface.init(if_list_tmp, m_addr_family);
    //for (vector<int>::iterator i = if_list_tmp.begin(); i != if_list_tmp.end(); i++) {
    //if ((it_vif = m_vif_map.find(*i)) == m_vif_map.end()) {
    //HC_LOG_ERROR("failed to find vif form if_index: " << *i);
    //return false;
    //}

    //if ((it_proxy_numb = m_interface_map.find(*i)) == m_interface_map.end()) {
    //HC_LOG_ERROR("failed to find proxy instance form if_index: " << *i);
    //return false;
    //}

    //msg.msg = new config_msg(config_msg::DEL_DOWNSTREAM, *i, it_vif->second);
    //m_proxy_instances[it_proxy_numb->second]->add_msg(msg);
    //}


    ////#################################
    //int alive_time = 0;
    //proxy::m_running = true;
    //while (proxy::m_running) {

    //usleep(4000000);
    //alive_time += 1;

    //if (m_print_status) {
    //debug_msg::lod lod = debug_msg::NORMAL;
    //if (m_verbose_lvl == 0) {
    //lod = debug_msg::NORMAL;
    //} else if (m_verbose_lvl == 1) {
    //lod = debug_msg::MORE;
    //} else if (m_verbose_lvl >= 2) {
    //lod = debug_msg::MORE_MORE;
    //}

    //cout << "alive time: " << alive_time << endl;

    //msg.type = proxy_msg::DEBUG_MSG;
    //msg.msg = new debug_msg(lod, m_proxy_instances.size(), PROXY_DEBUG_MSG_TIMEOUT);

    //for (unsigned int i = 0; i < m_proxy_instances.size(); i++) {
    //m_proxy_instances[i]->add_msg(msg);
    //}

    //debug_msg* dm = (debug_msg*)msg.msg.get();
    //dm->join_debug_msg();
    //cout << dm->get_debug_msg() << endl;
    //}

    //check_interface.check();
    ////calc swap_to_down interfaces
    //if_list_tmp = check_interface.swap_to_down();
    //for (vector<int>::iterator i = if_list_tmp.begin(); i < if_list_tmp.end(); i++) {
    //if ((it_vif = m_vif_map.find(*i)) == m_vif_map.end()) {
    //HC_LOG_ERROR("failed to find vif form if_index: " << *i);
    //return false;
    //}

    //if ((it_proxy_numb = m_interface_map.find(*i)) == m_interface_map.end()) {
    //HC_LOG_ERROR("failed to find proxy instance form if_index: " << *i);
    //return false;
    //}

    //msg.type = proxy_msg::CONFIG_MSG;
    //msg.msg = new config_msg(config_msg::DEL_DOWNSTREAM, *i, it_vif->second);
    //m_proxy_instances[it_proxy_numb->second]->add_msg(msg);
    //}

    ////calc swap_to_up interfaces
    //if_list_tmp = check_interface.swap_to_up();
    //for (vector<int>::iterator i = if_list_tmp.begin(); i < if_list_tmp.end(); i++) {
    //if ((it_vif = m_vif_map.find(*i)) == m_vif_map.end()) {
    //HC_LOG_ERROR("failed to find vif form if_index: " << *i);
    //return false;
    //}

    //if ((it_proxy_numb = m_interface_map.find(*i)) == m_interface_map.end()) {
    //HC_LOG_ERROR("failed to find proxy instance form if_index: " << *i);
    //return false;
    //}

    //msg.type = proxy_msg::CONFIG_MSG;
    //msg.msg = new config_msg(config_msg::ADD_DOWNSTREAM, *i, it_vif->second);
    //m_proxy_instances[it_proxy_numb->second]->add_msg(msg);
    //}
    //}

}

void proxy::signal_handler(int)
{
    proxy::m_running = false;
}

std::string proxy::to_string() const
{
    using namespace std;
    HC_LOG_TRACE("");
    ostringstream s;
    s << "##-- multicast proxy status --##" << endl;
    s << "is running: " << m_running << endl;
    s << "verbose level: " << m_verbose_lvl << endl;
    s << "print proxy_status information: " << m_print_proxy_status << endl;
    s << "reset all reverse path filter: " << m_reset_rp_filter << endl;
    s << "config path: " << m_config_path << endl;

    s << "-- proxy configuration --" << endl;
    s << m_configuration.get()->to_string() << endl;
    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const proxy& p)
{
    return stream << p.to_string();

}

