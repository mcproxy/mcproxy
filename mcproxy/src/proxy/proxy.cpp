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

#include <iostream>
#include <signal.h>
#include <unistd.h>

bool proxy::m_running = false;

proxy::proxy(int arg_count, char* args[])
    : m_verbose_lvl(0)
    , m_print_proxy_status(false)
    , m_rest_rp_filter(false)
    , m_config_path(PROXY_CONFIGURATION_DEFAULT_CONIG_PATH)
    , m_proxy_configuration(nullptr)
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

    m_proxy_configuration.reset(new proxy_configuration(m_config_path, m_rest_rp_filter));

    if (!start_proxy_instances()) {
        throw "failed to start a proxy instance";
    }

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
                m_rest_rp_filter = true;
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
                m_config_path = string(optarg);
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

    //untestestd ???????????????????????
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

bool proxy::start_proxy_instances()
{
    HC_LOG_TRACE("");

    auto& db =m_proxy_configuration->get_upstream_downstream_map();
    for(auto& e: db){
        int table = e.first;
        unsigned int upstream = e.first;
        auto& downstreams = e.second;         
        //std::unique_ptr<proxy_instance> instance = new proxy_instance(m_proxy_configuration->get_addr_family(), m_proxy_configuration->get_interfaces(), m_timing);
        
    //proxy_instance(int addr_family, int table_number, const std::shared_ptr<const interfaces> interfaces, const std::shared_ptr<timing> shared_timing);
        

    }

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
    proxy::m_running = false;
}

