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
#include "include/proxy/routing.hpp"
#include "include/proxy/igmp_receiver.hpp"
#include "include/proxy/mld_receiver.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/check_if.hpp"
#include "include/proxy/check_kernel.hpp"

#include <linux/mroute.h>
#include <linux/mroute6.h>
#include <sstream>
#include <net/if.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <signal.h>

#include <unistd.h>

using namespace std;

bool proxy::m_running = false;

proxy::proxy():
    m_is_single_instance(true), m_verbose_lvl(0), m_print_status(false), m_rest_rp_filter(false),
    m_config_path(PROXY_DEFAULT_CONIG_PATH) , m_addr_family(AF_INET), m_version(2)
{
    HC_LOG_TRACE("");

    signal(SIGINT, proxy::signal_handler);
    signal(SIGTERM, proxy::signal_handler);
}

proxy::~proxy()
{
    HC_LOG_TRACE("");

    close();
}



vector<int> proxy::all_if_to_list()
{
    HC_LOG_TRACE("");

    vector<int> interface_list;

    up_down_map::iterator it_up_down;

    for ( it_up_down = m_up_down_map.begin() ; it_up_down != m_up_down_map.end(); it_up_down++ ) {
        interface_list.push_back(it_up_down->first);

        down_vector tmp_down_vector = it_up_down->second;
        for (unsigned int i = 0; i < tmp_down_vector.size(); i++) {
            interface_list.push_back(tmp_down_vector[i]);
        }
    }

    return interface_list;
}


bool proxy::check_double_used_if(const vector<int>* new_interfaces)
{
    HC_LOG_TRACE("");

    vector<int> interface_list = all_if_to_list();

    //add new if_list to interface_list
    if (new_interfaces != nullptr) {
        for (unsigned int i = 0; i < new_interfaces->size(); i++) {
            interface_list.push_back((*new_interfaces)[i]);
        }
    }

    while (!interface_list.empty()) {
        int tmp_if = interface_list.back();
        interface_list.pop_back();
        for (unsigned int i = 0; i < interface_list.size(); i++) {
            if (interface_list[i] == tmp_if) {
                return false;
            }
        }
    }


    return true;
}


bool proxy::init(int arg_count, char* args[])
{
    HC_LOG_TRACE("");

    if (!prozess_commandline_args(arg_count, args)) {
        return false;
    }

    //admin test
    // Check root privilegis
    if (geteuid() != 0) {  //no root privilegis
        HC_LOG_ERROR("The mcproxy has to be started with root privileges!");
        cout << "The mcproxy has to be started with root privileges!" << endl;
        return false;
    }

    if (!load_config(m_config_path)) {
        return false;
    }

    if (!check_double_used_if(nullptr)) {
        HC_LOG_ERROR("found double used interface");
        return false;
    }

    if (!init_vif_map()) {
        return false;
    }

    if (!init_if_prop()) {
        return false;
    }

    vector<int> if_list = all_if_to_list();
    if (!check_and_set_flags(if_list)) {
        return false;
    }

    //start timing
    timing* tim = new timing();

    if (!start_proxy_instances()) {
        return false;
    }

    return true;
}

void proxy::help_output()
{
    HC_LOG_TRACE("");
    cout << "Mcproxy version 0.1.5" << endl;

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

bool proxy::prozess_commandline_args(int arg_count, char* args[])
{
    HC_LOG_TRACE("");

    bool is_logging = false;
    bool is_check_kernel = false;

    if (arg_count == 1) {

    } else {
        for (int c; (c = getopt(arg_count, args, "hrdsvcf")) != -1;) {
            switch (c) {
            case 'h':
                help_output();
                return false;
                break;
            case 'c':
                is_check_kernel = true;
                break;
            case 'r':
                m_rest_rp_filter = true;
                break;
            case 'd':
                is_logging = true;
                break;
            case 's':
                m_print_status = true;
                break;
            case 'v':
                m_verbose_lvl++;
                break;
            case 'f':
                if (args[optind][0] != '-') {
                    m_config_path = string(args[optind]);
                } else {
                    HC_LOG_ERROR("no config path defined");
                    cout << "no config path defined" << endl;
                    return false;
                }
                break;
            default:
                HC_LOG_ERROR("Unknown argument! See help (-h) for more information.");
                cout << "See help (-h) for more information." << endl;
                return false;
                break;
            }
        }
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
            return false;
        }
    }

    if (is_check_kernel) {
        check_kernel ck;
        ck.check_kernel_features();
        return false;
    }

    return true;

}

bool proxy::start_proxy_instances()
{
    HC_LOG_TRACE("");

    //proxy_msg msg;
    //up_down_map::iterator it_up_down;
    //vif_map::iterator it_vif;
    //int upstream_vif;
    //int downstream_vif;


    //for ( it_up_down = m_up_down_map.begin() ; it_up_down != m_up_down_map.end(); it_up_down++ ) {
        //down_vector tmp_down_vector = it_up_down->second;

        //proxy_instance* p = new proxy_instance();
        //m_proxy_instances.push_back(p);

        //if ((it_vif = m_vif_map.find(it_up_down->first)) == m_vif_map.end()) {
            //HC_LOG_ERROR("failed to find vif form if_index: " << it_up_down->first);
            //return false;
        //}
        //upstream_vif = it_vif->second;

        //if ((it_vif = m_vif_map.find(tmp_down_vector[0])) == m_vif_map.end()) {
            //HC_LOG_ERROR("failed to find vif form if_index: " << tmp_down_vector[0]);
            //return false;
        //}
        //downstream_vif = it_vif->second;

        ////start proxy instance
        //if (!p->init(m_addr_family, it_up_down->first, upstream_vif, tmp_down_vector[0], downstream_vif,
                     //m_is_single_instance)) {
            //return false;
        //}

        //p->start();


        ////add upstream and first downstream
        //m_interface_map.insert(interface_pair(it_up_down->first, m_proxy_instances.size() - 1));
        //m_interface_map.insert(interface_pair(tmp_down_vector[0], m_proxy_instances.size() - 1));

        ////add downstream
        //for (unsigned int i = 1; i < tmp_down_vector.size(); i++) {
            //msg.type = proxy_msg::CONFIG_MSG;

            //if ((it_vif = m_vif_map.find(tmp_down_vector[i])) == m_vif_map.end()) {
                //HC_LOG_ERROR("failed to find vif form if_index: " << tmp_down_vector[0]);
                //return false;
            //}
            //downstream_vif = it_vif->second;
            //msg.msg = new config_msg(config_msg::ADD_DOWNSTREAM, tmp_down_vector[i], downstream_vif);

            //HC_LOG_DEBUG("add a new downstream interface; pointer: " << msg.msg);
            //cout << "add a new downstream interface" << endl;
            //cout << "proxy.cpp: msg.msg: " << msg.msg << " msg.msg.get(): " << msg.msg.get() << endl;
            //p->add_msg(msg);
            //m_interface_map.insert(interface_pair(tmp_down_vector[i], m_proxy_instances.size() - 1));
        //}
    //}

    return false;
}

bool proxy::check_and_set_flags(vector<int>& interface_list)
{
    HC_LOG_TRACE("");

    char cstr[IF_NAMESIZE];
    const if_prop_map* prop_map = m_if_prop.get_if_props();

    //##debug
    //m_if_prop.print_if_info();

    if_prop_map::const_iterator iter;
    const struct ifaddrs* ifaddr;

    for (unsigned int i = 0; i < interface_list.size(); i++) {
        string if_name(if_indextoname(interface_list[i], cstr));

        iter = prop_map->find(if_name);

        if (iter == prop_map->end()) {
            HC_LOG_ERROR("interface " << if_name << " not found");
            return false;
        }

        if (m_addr_family == AF_INET) {
            if (iter->second.ip4_addr != nullptr) {
                ifaddr = iter->second.ip4_addr;
            } else {
                HC_LOG_ERROR("interface " << if_name << " don't support ipv4");
                return false;
            }
        } else if (m_addr_family == AF_INET6) {
            const list<const struct ifaddrs*>* ipv6_if_list = &(iter->second.ip6_addr);
            if (ipv6_if_list->size() != 0) {
                ifaddr = *(ipv6_if_list->begin());
            } else {
                HC_LOG_ERROR("interface " << if_name << " don't support ipv6");
                return false;
            }
        } else {
            HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
            return false;
        }

        if (!(ifaddr->ifa_flags & IFF_UP)) {
            HC_LOG_ERROR("wrong interface status: interface " << if_name << " is not up");
            return false;
        }

        //reset rp_filter for every used interface
        //if (m_rest_rp_filter) {
            //if (get_rp_filter(if_name)) {
                //if (set_rp_filter(if_name, false)) {
                    //m_restore_rp_filter_vector.push_back(if_name);
                //} else {
                    //return false;
                //}
            //}
        //}
    //}

    ////reset global rp_filter
    //if (m_rest_rp_filter) {
        //string global_if = "all";
        //if (get_rp_filter(global_if)) {
            //if (set_rp_filter(global_if, false)) {
                //m_restore_rp_filter_vector.push_back(global_if);
            //} else {
                //return false;
            //}
        //}
    //}

    return true;
}


bool proxy::start()
{
    HC_LOG_TRACE("");

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



    return true;
}

void proxy::signal_handler(int)
{
    HC_LOG_TRACE("");

    //HC_LOG_DEBUG("Signale: " << sys_siglist[sig] << " received");
    proxy::m_running = false;
}


void proxy::stop()
{
    HC_LOG_TRACE("");

    //proxy_msg m;
    //m.type = proxy_msg::EXIT_CMD;

    //HC_LOG_DEBUG("kill worker thread proxy_instance");
    //for (unsigned int i = 0; i < m_proxy_instances.size(); i++) {
        //m_proxy_instances[i]->add_msg(m);
    //}

    //for (unsigned int i = 0; i < m_proxy_instances.size(); i++) {
        //HC_LOG_DEBUG("join worker thread proxy_instance.");
        //m_proxy_instances[i]->join();
        //HC_LOG_DEBUG("joined.");
        //delete m_proxy_instances[i];
    //}

    //timing* tim = timing::getInstance();
    //tim->stop();
    //tim->join();

}

void proxy::close()
{
    HC_LOG_TRACE("");

    restore_rp_filter();
}
