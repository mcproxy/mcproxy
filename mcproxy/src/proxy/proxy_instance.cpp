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
#include "include/proxy/proxy_instance.hpp"
#include "include/utils/mc_timers_values.hpp"
#include "include/proxy/igmp_receiver.hpp"
#include "include/proxy/mld_receiver.hpp"

#include <net/if.h>
#include <sstream>

proxy_instance::proxy_instance():
    worker(PROXY_INSTANCE_MSG_QUEUE_SIZE), m_is_single_instance(true), m_table_number(-1), m_upstream(0), m_addr_family(-1), m_version(-1)
{
    HC_LOG_TRACE("");

}

proxy_instance::~proxy_instance(){
    HC_LOG_TRACE("");
    close();
}

bool proxy_instance::init(int addr_family, int version, int upstream_index, int upstream_vif, int downstream_index, int downstram_vif, bool single_instance){
    HC_LOG_TRACE("");

    m_is_single_instance = single_instance;
    m_addr_family =  addr_family;
    m_version = version;

    m_upstream = upstream_index;
    m_table_number = upstream_index;
    m_vif_map.insert(vif_pair(upstream_index,upstream_vif));

    m_state_table.insert(state_tabel_pair(downstream_index, g_state_map()));
    m_vif_map.insert(vif_pair(downstream_index, downstram_vif));

    if(!init_mrt_socket()) return false;

    m_check_source.init(m_addr_family, &m_mrt_sock);

    if(!init_receiver()) return false;

    if(!init_sender()) return false;

    m_routing.init(m_addr_family,m_version, &m_mrt_sock, m_is_single_instance, m_table_number);

    m_timing = timing::getInstance();

    return true;
}

bool proxy_instance::init_receiver(){
    HC_LOG_TRACE("");

    if(m_addr_family == AF_INET){
        m_receiver = new igmp_receiver();
    }else if(m_addr_family == AF_INET6){
        m_receiver = new mld_receiver();
    }else{
        HC_LOG_ERROR("wrong address family");
        return false;
    }

    if(!m_receiver->init(m_addr_family,m_version, &m_mrt_sock)) return false;
    m_receiver->start();

    return true;
}

bool proxy_instance::init_mrt_socket(){
    HC_LOG_TRACE("");

    if(m_addr_family == AF_INET){
        m_mrt_sock.create_raw_ipv4_socket();
    }else if(m_addr_family == AF_INET6){
        m_mrt_sock.create_raw_ipv6_socket();
    }else{
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    if(!m_is_single_instance){
        if(!m_mrt_sock.set_kernel_table(m_upstream)){
            return false;
        }else{
            HC_LOG_DEBUG("proxy instance: " << m_upstream);
        }
    }

    if(!m_mrt_sock.set_mrt_flag(true)) return false;

    return true;
}

bool proxy_instance::init_sender(){
    HC_LOG_TRACE("");

    if(m_addr_family == AF_INET){
        m_sender = new igmp_sender;
    }else if(m_addr_family == AF_INET6){
        m_sender = new mld_sender;
    }else{
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    if(!m_sender->init(m_addr_family,m_version)) return false;

    return true;
}

void proxy_instance::worker_thread(){
    HC_LOG_TRACE("");

    proxy_msg m;
    state_table_map::iterator it_state_table;

    //##-- add all interfaces --##
    //upsteam
    registrate_if(m_upstream);
    //downsteams (only one)
    for ( it_state_table=m_state_table.begin() ; it_state_table != m_state_table.end(); it_state_table++ ){
        registrate_if(it_state_table->first);
    }

    //##-- general query --##
    //nach registrate_if ausgelagert
    //send_gq_to_all();

    //##-- initiate GQ timer --##
    m.type = proxy_msg::CLOCK_MSG;
    m.msg = new struct clock_msg(clock_msg::SEND_GQ_TO_ALL);
    m_timing->add_time(MC_TV_QUERY_INTERVAL*1000 /*msec*/,this,m);

    //##-- thread working loop --##
    while(m_running){
        m = m_job_queue.dequeue();
        HC_LOG_DEBUG("received new job. type: " << m.msg_type_to_string());
        switch(m.type){
        case proxy_msg::TEST_MSG: {
            struct test_msg* t= (struct test_msg*) m.msg.get();
            t->test();
            break;
        }
        case proxy_msg::RECEIVER_MSG: {
            struct receiver_msg* t= (struct receiver_msg*) m.msg.get();
            handle_igmp(t);
            break;
        }
        case proxy_msg::CLOCK_MSG: {
            struct clock_msg* t = (struct clock_msg*) m.msg.get();
            handle_clock(t);
            break;
        }
        case proxy_msg::CONFIG_MSG: {
            struct config_msg* t = (struct config_msg*) m.msg.get();
            handle_config(t);
            break;
        }
        case proxy_msg::DEBUG_MSG: {
            struct debug_msg* t = (struct debug_msg*) m.msg.get();
            handle_debug_msg(t);
            break;
        }

        case proxy_msg::EXIT_CMD: m_running = false; break;
        default: HC_LOG_ERROR("unknown message format");
        }
    }

    //##-- timing --##
    //remove all running times
    m_timing->stop_all_time(this);

    //##-- del all interfaces --##
    //upsteam
    unregistrate_if(m_upstream);
    //downsteams
    for ( it_state_table=m_state_table.begin() ; it_state_table != m_state_table.end(); it_state_table++ ){
        unregistrate_if(it_state_table->first);
    }

    HC_LOG_DEBUG("worker thread proxy_instance end");
}

void proxy_instance::handle_igmp(struct receiver_msg* r){
    HC_LOG_TRACE("");

    state_table_map::iterator iter_table;
    g_state_map::iterator iter_state;
    src_state_map::iterator iter_src;
    src_group_state_pair* sgs_pair;
    proxy_msg msg;

    switch(r->type){
    case receiver_msg::JOIN: {
        //upstream joins are uninteresting
        if(r->if_index == m_upstream) return;

        iter_table =m_state_table.find(r->if_index);
        if(iter_table == m_state_table.end()) return;

        iter_state = iter_table->second.find(r->g_addr);

        if(iter_state == iter_table->second.end()){ //add group
            struct src_state tmp_state(MC_TV_ROBUSTNESS_VARIABLE, src_state::RUNNING);
            iter_table->second.insert(g_state_pair(r->g_addr,src_group_state_pair(src_state_map(), tmp_state)));

            //--refresh upstream
            if(!is_group_joined(r->if_index,r->g_addr)){
                if(!m_sender->send_report(m_upstream, r->g_addr)){
                    HC_LOG_ERROR("failed to join on upstream group: " << r->g_addr);
                    return;
                }
            }

            //--refresh routing
            refresh_all_traffic(r->if_index, r->g_addr);

        }else{ //refresh group
            sgs_pair = &iter_state->second;
            sgs_pair->second.robustness_counter = MC_TV_ROBUSTNESS_VARIABLE;
            sgs_pair->second.flag = src_state::RUNNING;
        }

        break;
    }
    case receiver_msg::LEAVE:{
        //cout << "leave an if: " << r->if_index << " für gruppe:" << r->g_addr << " empfangen" << endl;
        //upstream leaves are uninteresting
        if(r->if_index == m_upstream) return;

        iter_table =m_state_table.find(r->if_index);
        if(iter_table == m_state_table.end()) return;

        iter_state = iter_table->second.find(r->g_addr);
        if(iter_state == iter_table->second.end()) return;
        sgs_pair = &iter_state->second;

        sgs_pair->second.flag = src_state::RESPONSE_STATE;

        msg.type = proxy_msg::CLOCK_MSG;
        msg.msg = new struct clock_msg(clock_msg::SEND_GSQ, iter_table->first, iter_state->first);

        if(m_addr_family == AF_INET){
            sgs_pair->second.robustness_counter = MC_TV_LAST_MEMBER_QUERY_COUNT;

            m_timing->add_time(MC_TV_LAST_MEMBER_QUERY_INTEVAL*1000 /*msec*/,this,msg);
        }else if(m_addr_family== AF_INET6){
            sgs_pair->second.robustness_counter = MC_TV_LAST_LISTENER_QUERY_COUNT;

            m_timing->add_time(MC_TV_LAST_LISTENER_QUERY_INTERVAL*1000 /*msec*/,this,msg);
        }else{
            HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
            return;
        }

        break;
    }
    case receiver_msg::CACHE_MISS: {
        if(r->if_index == m_upstream){
            upstream_src_state_map::iterator it_gss = m_upstream_state.find(r->g_addr);
            if(it_gss == m_upstream_state.end()){ //new group found
                struct src_state tmp_src_state(MC_TV_ROBUSTNESS_VARIABLE, src_state::UNUSED_SRC);
                src_state_map tmp_src_state_map;
                tmp_src_state_map.insert(src_state_pair(r->src_addr,tmp_src_state));
                m_upstream_state.insert(upstream_src_state_pair(r->g_addr,tmp_src_state_map));
            }else{ // insert in existing group
                iter_src = it_gss->second.find(r->src_addr);
                if(iter_src == it_gss->second.end()){ //new src addr
                    struct src_state tmp_src_state(MC_TV_ROBUSTNESS_VARIABLE, src_state::UNUSED_SRC);
                    it_gss->second.insert(src_state_pair(r->src_addr,tmp_src_state));
                }else{ //src exist
                    HC_LOG_DEBUG("kernel msg with src: " << r->src_addr << " received, this source address exist for if_index: " << r->if_index << " and group:" << r->g_addr << " because it's normaly not joined on a downstream interface");
                    return;
                }
            }

            //refresh routing
            if(split_traffic(r->if_index, r->g_addr, r->src_addr)){
                // versetzt nach split traffic
                //                    upstream_src_state_map::iterator it_gss = m_upstream_state.find(r->g_addr);
                //                    if(it_gss == m_upstream_state.end()){
                //                         HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find upstream g_addr:" << r->g_addr);
                //                         return;
                //                    }
                //                    iter_src = it_gss->second.find(r->src_addr);
                //                    if(iter_src == it_gss->second.end()){
                //                         HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find to g_addr:" << r->g_addr << " the source:"  << r->src_addr);
                //                         return;
                //                    }
                //                    iter_src->second.flag = src_state::CACHED_SRC;
            }

        }else{ //downstream cache miss
            iter_table =m_state_table.find(r->if_index);
            if(iter_table == m_state_table.end()) return;

            iter_state = iter_table->second.find(r->g_addr);
            if(iter_state == iter_table->second.end()) { //new group found
                struct src_state tmp_state(MC_TV_ROBUSTNESS_VARIABLE, src_state::UNUSED_SRC);
                struct src_state tmp_state_group;
                src_state_map tmp_src_state_map;
                tmp_src_state_map.insert(src_state_pair(r->src_addr,tmp_state));
                iter_table->second.insert(g_state_pair(r->g_addr, src_group_state_pair(tmp_src_state_map, tmp_state_group)));
            }else{ //insert in existing group
                sgs_pair = &iter_state->second;

                iter_src = sgs_pair->first.find(r->src_addr);
                if(iter_src == sgs_pair->first.end()){ //new src found, add to list
                    struct src_state tmp_state(MC_TV_ROBUSTNESS_VARIABLE, src_state::UNUSED_SRC);
                    sgs_pair->first.insert(src_state_pair(r->src_addr,tmp_state));
                }else{ //error old src found
                    HC_LOG_ERROR("kernel msg with src: " << r->src_addr << " received, this source address exist for if_index: " << r->if_index << " and group:" << r->g_addr);
                    return;
                }
            }

            //refresh routing
            if(split_traffic(r->if_index, r->g_addr, r->src_addr)){
                // versetzt nach split traffic
                //                    iter_state = iter_table->second.find(r->g_addr);
                //                    if(iter_state == iter_table->second.end()) {
                //                         HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find downstream g_addr:" << r->g_addr);
                //                         return;
                //                    }
                //                    sgs_pair = &iter_state->second;
                //                    iter_src = sgs_pair->first.find(r->src_addr);
                //                    if(iter_state == iter_table->second.end()) {
                //                         HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find to g_addr:" << r->g_addr << " the source:"  << r->src_addr);
                //                         return;
                //                    }
                //                    iter_src->second.flag = src_state::CACHED_SRC;
            }

        }
        break;
    }
    default: HC_LOG_ERROR("unknown receiver messge format");
    }
}

void proxy_instance::handle_clock(struct clock_msg* c){
    HC_LOG_TRACE("");

    proxy_msg msg;
    state_table_map::iterator iter_table;
    g_state_map::iterator iter_state;
    src_state_map::iterator iter_src;
    src_group_state_pair* sgs_pair = nullptr;


    switch(c->type){
    case clock_msg::SEND_GQ_TO_ALL: {
        //send GQ
        send_gq_to_all();

        m_check_source.check(); //reloade routing table

        //##-- dekrement all counter of all groups on all downstream interfaces in RUNNING state --##
        //##-- and dekrement all counter of all groups source addresses --##
        vector<addr_storage> tmp_erase_group_vector; //if group not joined and all sources are deleted
        for(iter_table= m_state_table.begin(); iter_table != m_state_table.end(); iter_table++){

            for(iter_state= iter_table->second.begin(); iter_state != iter_table->second.end(); iter_state++){
                sgs_pair = &iter_state->second;

                //-- process groups in RUNNING state --
                if( sgs_pair->second.flag == src_state::RUNNING){
                    //if counter == 0 delete this group after query response
                    if(--sgs_pair->second.robustness_counter == 0){
                        sgs_pair->second.flag= src_state::WAIT_FOR_DEL;
                        sgs_pair->second.robustness_counter = PROXY_INSTANCE_DEL_IMMEDIATELY;

                        msg.type = proxy_msg::CLOCK_MSG;
                        msg.msg = new struct clock_msg(clock_msg::DEL_GROUP, iter_table->first, iter_state->first);
                        m_timing->add_time(MC_TV_QUERY_RESPONSE_INTERVAL*1000 /*msec*/,this,msg);
                    }
                }


                //-- process sources in FOREIGN_SRC state -downstream- --
                vector<addr_storage> tmp_erase_source_vector;
                for(iter_src = sgs_pair->first.begin(); iter_src != sgs_pair->first.end(); iter_src++){
                    if(iter_src->second.flag == src_state::UNUSED_SRC || iter_src->second.flag == src_state::CACHED_SRC){
                        //del unused sources
                        vif_map::iterator it_vif_map;
                        if((it_vif_map = m_vif_map.find(iter_table->first)) == m_vif_map.end()){
                            HC_LOG_ERROR("cant find vif to if_index:" << iter_table->first);
                        }

                        if(m_check_source.is_src_unused(it_vif_map->second, iter_src->first, iter_state->first)){
                            iter_src->second.robustness_counter--;
                        }else{
                            iter_src->second.robustness_counter=MC_TV_ROBUSTNESS_VARIABLE;
                        }

                        if(iter_src->second.robustness_counter == 0){
                            //save invalid sources
                            tmp_erase_source_vector.push_back(iter_src->first);

                            //refresh routing
                            if(iter_src->second.flag == src_state::CACHED_SRC){
                                del_route(iter_table->first, iter_state->first, iter_src->first);
                            }

                        }
                    }else{
                        HC_LOG_ERROR("downstream source is in unknown state: " << iter_src->second.state_type_to_string());
                    }
                }
                //erase invalid sources
                for(unsigned int i=0; i< tmp_erase_source_vector.size(); i++){
                    if((iter_src = sgs_pair->first.find(tmp_erase_source_vector[i]))!= sgs_pair->first.end()){
                        sgs_pair->first.erase(iter_src);
                    }else{
                        HC_LOG_ERROR("cant find invalid downstream soruce: " << tmp_erase_source_vector[i]);
                    }
                }

                //if group has no sources and is not joined (flag=INIT) remove the group
                if(sgs_pair->first.size()  == 0){
                    if(sgs_pair->second.flag == src_state::INIT){
                        tmp_erase_group_vector.push_back(iter_state->first);
                    }
                }


            }
            //erase unused groups
            for(unsigned int i=0; i< tmp_erase_group_vector.size(); i++){
                if((iter_state = iter_table->second.find(tmp_erase_group_vector[i]))!= iter_table->second.end()){
                    iter_table->second.erase(iter_state);

                }else{
                    HC_LOG_ERROR("cant find unused groups: " << tmp_erase_group_vector[i]);
                }
            }
            tmp_erase_group_vector.clear();

        }



        //-- process sources in FOREIGN_SRC state -upstream- --
        upstream_src_state_map::iterator tmp_it_up_ss_map;
        src_state_map* tmp_ss_map;
        tmp_erase_group_vector.clear(); //unused groups
        for(tmp_it_up_ss_map = m_upstream_state.begin(); tmp_it_up_ss_map != m_upstream_state.end(); tmp_it_up_ss_map++){

            tmp_ss_map = &tmp_it_up_ss_map->second;
            vector<addr_storage> tmp_erase_source_vector;
            for(iter_src = tmp_ss_map->begin(); iter_src != tmp_ss_map->end(); iter_src++){
                //                if(iter_src->second.flag == src_state::UNUSED_SRC || iter_src->second.flag == src_state::CACHED_SRC){

                //                    //del old sources
                //                    if(--iter_src->second.robustness_counter == 0){
                //                        //save invalid sources
                //                        tmp_erase_source_vector.push_back(iter_src->first);

                //                        //refresh routing
                //                        if(iter_src->second.flag == src_state::CACHED_SRC){
                //                            del_route(m_upstream, tmp_it_up_ss_map->first, iter_src->first);
                //                        }

                //                    }
                //                }else{
                //                    HC_LOG_ERROR("upstream source is in unknown state: " << iter_src->second.state_type_to_string());
                //                }

                if(iter_src->second.flag == src_state::UNUSED_SRC){

                    //del old sources
                    if(--iter_src->second.robustness_counter == 0){
                        //save invalid sources
                        tmp_erase_source_vector.push_back(iter_src->first);
                    }
                }else if(iter_src->second.flag == src_state::CACHED_SRC){

                    if(m_check_source.is_src_unused(m_upstream, iter_src->first, tmp_it_up_ss_map->first)){
                        if(--iter_src->second.robustness_counter == 0){
                            //save invalid sources
                            tmp_erase_source_vector.push_back(iter_src->first);

                            del_route(m_upstream, tmp_it_up_ss_map->first, iter_src->first);
                        }

                    }else{
                        iter_src->second.robustness_counter=MC_TV_ROBUSTNESS_VARIABLE;
                    }


                }else{
                    HC_LOG_ERROR("upstream source is in unknown state: " << iter_src->second.state_type_to_string());
                }


            }
            //erase invalid sources
            for(unsigned int i=0; i< tmp_erase_source_vector.size(); i++){
                if((iter_src = tmp_ss_map->find(tmp_erase_source_vector[i])) != tmp_ss_map->end()){
                    tmp_ss_map->erase(iter_src);
                }else{
                    HC_LOG_ERROR("cant find invalid upstream soruce: " << tmp_erase_source_vector[i]);
                }
            }

            //if group has no sources remove the group
            if(tmp_it_up_ss_map->second.size() == 0){
                tmp_erase_group_vector.push_back(tmp_it_up_ss_map->first);
            }

        }
        //erase unused groups
        for(unsigned int i=0; i < tmp_erase_group_vector.size(); i++){
            tmp_it_up_ss_map = m_upstream_state.find(tmp_erase_group_vector[i]);
            if(tmp_it_up_ss_map != m_upstream_state.end()){
                m_upstream_state.erase(tmp_it_up_ss_map);
            }else{
                HC_LOG_ERROR("cant find unused groups: " << tmp_erase_group_vector[i]);
            }
        }

        //initiate new GQ
        msg.type = proxy_msg::CLOCK_MSG;
        msg.msg = new struct clock_msg(clock_msg::SEND_GQ_TO_ALL);
        m_timing->add_time(MC_TV_QUERY_INTERVAL*1000 /*msec*/,this,msg);
        break;
    }
    case clock_msg::SEND_GSQ: {
        iter_table =m_state_table.find(c->if_index);
        if(iter_table == m_state_table.end()) return;

        iter_state = iter_table->second.find(c->g_addr);
        if(iter_state == iter_table->second.end()) return;

        sgs_pair = &iter_state->second;
        if(sgs_pair->second.flag == src_state::RESPONSE_STATE){
            m_sender->send_group_specific_query(c->if_index,c->g_addr);

            if(--sgs_pair->second.robustness_counter == PROXY_INSTANCE_DEL_IMMEDIATELY){
                sgs_pair->second.flag = src_state::WAIT_FOR_DEL;

                msg.type = proxy_msg::CLOCK_MSG;
                msg.msg = new struct clock_msg(clock_msg::DEL_GROUP, iter_table->first, iter_state->first);
                if(m_addr_family == AF_INET){
                    m_timing->add_time(MC_TV_LAST_MEMBER_QUERY_INTEVAL*1000 /*msec*/,this,msg);
                }else if(m_addr_family == AF_INET6){
                    m_timing->add_time(MC_TV_LAST_LISTENER_QUERY_INTERVAL*1000 /*msec*/,this,msg);
                }else{
                    HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
                    return;
                }
            }else{
                msg.type = proxy_msg::CLOCK_MSG;
                msg.msg = new struct clock_msg(clock_msg::SEND_GSQ, iter_table->first, iter_state->first);

                if(m_addr_family == AF_INET){
                    m_timing->add_time(MC_TV_LAST_MEMBER_QUERY_INTEVAL*1000 /*msec*/,this,msg);
                }else if(m_addr_family== AF_INET6){
                    m_timing->add_time(MC_TV_LAST_LISTENER_QUERY_INTERVAL*1000 /*msec*/,this,msg);
                }else{
                    HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
                    return;
                }
            }
        }
        break;
    }
    case clock_msg::DEL_GROUP:{
        iter_table =m_state_table.find(c->if_index);
        if(iter_table == m_state_table.end()) return;

        iter_state = iter_table->second.find(c->g_addr);
        if(iter_state == iter_table->second.end()) return;

        sgs_pair = &iter_state->second;
        if(sgs_pair->second.flag == src_state::WAIT_FOR_DEL){
            HC_LOG_DEBUG("DEL_GROUP if_index: " << c->if_index << " group: " << c->g_addr);

            //refresh upstream
            if(!is_group_joined(c->if_index,c->g_addr)){
                if(!m_sender->send_leave(m_upstream, c->g_addr)){
                    HC_LOG_ERROR("failed to leave on upstream group: " << c->g_addr);
                }
            }

            //del only if no FOREIGN_SRC available
            if(sgs_pair->first.size() == 0){
                iter_table->second.erase(iter_state);
            }else{ //set groupstate to INIT
                sgs_pair->second.flag = src_state::INIT;
            }

            //refresh routing
            //cout << "in del group: refresh_all_traffic()..." << endl;
            refresh_all_traffic(c->if_index, c->g_addr);
        }

        break;
    }
    case clock_msg::SEND_GQ: break; //start up Query Interval vor new interfaces
    default: HC_LOG_ERROR("unknown clock message foramt");
    }
}

void proxy_instance::handle_config(struct config_msg* c){
    HC_LOG_TRACE("");

    switch(c->type){
    case config_msg::ADD_DOWNSTREAM: {

        //if interface exist
        if(m_state_table.find(c->if_index) != m_state_table.end()){
            HC_LOG_ERROR("failed to add downstream, interface " << c->if_index << " allready exist");
        }

        m_state_table.insert(state_tabel_pair(c->if_index,g_state_map()));
        m_vif_map.insert(vif_pair(c->if_index,c->vif));
        registrate_if(c->if_index);
        break;
    }
    case config_msg::DEL_DOWNSTREAM: {
        //if interface exist
        if(m_state_table.find(c->if_index) == m_state_table.end()){
            HC_LOG_ERROR("failed to del downstream, interface " << c->if_index << " not exist");
        }

        unregistrate_if(c->if_index);

        state_table_map::iterator iter_table;
        g_state_map::iterator iter_state;
        src_group_state_pair* sgs_pair = nullptr;
        src_state_map::iterator iter_src;

        iter_table =m_state_table.find(c->if_index);
        if(iter_table == m_state_table.end()) {
            HC_LOG_ERROR("faild to del downstream: cant find if_index: " << c->if_index);
            return;
        }

        vector<addr_storage> tmp_erase_group_vector;
        for(iter_state= iter_table->second.begin(); iter_state != iter_table->second.end(); iter_state++){
            sgs_pair = &iter_state->second;

            //remove all own sources (routes)
            for(iter_src = sgs_pair->first.begin(); iter_src != sgs_pair->first.end(); iter_src++){
                if(iter_src->second.flag == src_state::CACHED_SRC){
                    del_route(iter_table->first, iter_state->first, iter_src->first);
                }
            }

            //save all groups for remove
            tmp_erase_group_vector.push_back(iter_state->first);

            //refresh upstream
            if(!is_group_joined(c->if_index,iter_state->first)){
                if(!m_sender->send_leave(m_upstream, iter_state->first)){
                    HC_LOG_ERROR("failed to leave on upstream group: " << iter_state->first);
                }
            }
        }

        //erase all groups
        for(unsigned int i=0; i< tmp_erase_group_vector.size(); i++){
            if((iter_state = iter_table->second.find(tmp_erase_group_vector[i]))!= iter_table->second.end()){
                iter_table->second.erase(iter_state);

                //calculate the joined group roles
                refresh_all_traffic(c->if_index, tmp_erase_group_vector[i]);
            }else{
                HC_LOG_ERROR("cant find downstream group: " << tmp_erase_group_vector[i]);}

        }

        //clean state table
        m_state_table.erase(iter_table);

        //clean vif map
        vif_map::iterator it_vif_map = m_vif_map.find(c->if_index);
        if(it_vif_map == m_vif_map.end()) {
            HC_LOG_ERROR("faild to del downstream: cant find vif to if_index: " << c->if_index);
            return;
        }
        m_vif_map.erase(it_vif_map);

        //HC_LOG_ERROR("del downstream not implementeted");
        break;
    }
    case config_msg::SET_UPSTREAM: {

        //remove current upstream
        unregistrate_if(c->if_index);

        vif_map::iterator it_vif_map = m_vif_map.find(c->if_index);
        if(it_vif_map == m_vif_map.end()) {
            HC_LOG_ERROR("faild to del downstream: cant find if_index: " << c->if_index);
            return;
        }
        m_vif_map.erase(it_vif_map);

        //ToDo
        //refresh routes?????????????????????????????????
        HC_LOG_ERROR("set upstream not implementeted");


        //set new upstream
        m_upstream = c->if_index;
        m_vif_map.insert(vif_pair(c->if_index,c->vif));
        registrate_if(c->if_index);

        break;
    }
    default: HC_LOG_ERROR("unknown config message format");
    }
}

void proxy_instance::handle_debug_msg(struct debug_msg* db){
    HC_LOG_TRACE("");
    std::stringstream str;
    state_table_map::iterator iter_table;
    g_state_map::iterator iter_state;
    src_state_map::iterator iter_src_state;
    vif_map::iterator iter_vif;

    upstream_src_state_map::iterator iter_up_src_state;


    char cstr[IF_NAMESIZE];
    string if_name(if_indextoname(m_upstream,cstr));

    if((iter_vif = m_vif_map.find(m_upstream))== m_vif_map.end()){
        HC_LOG_ERROR("failed to find vif to upstream if_index:" << m_upstream);
        return;
    }
    str << "##-- instance upstream " << if_name << " [vif=" << iter_vif->second<< "] --##" << endl;

    if(db->get_level_of_detail() > debug_msg::LESS){

        if(db->get_level_of_detail() > debug_msg::NORMAL){
            //upstream output
            str << "\tgroup addr" << endl;

            int tmp_g_counter=0;
            for(iter_up_src_state= m_upstream_state.begin(); iter_up_src_state !=m_upstream_state.end(); iter_up_src_state++){
                src_state_map* tmp_src_state_map = &iter_up_src_state->second;
                str << "\t[" << tmp_g_counter++ << "] " <<iter_up_src_state->first << "\t" << endl;

                int tmp_s_counter=0;
                if(db->get_level_of_detail() > debug_msg::MORE){
                    if(tmp_src_state_map->size()>0 ){
                        str << "\t\tsrc addr | robustness_counter | flag" << endl;
                        for(iter_src_state = tmp_src_state_map->begin(); iter_src_state != tmp_src_state_map->end(); iter_src_state++){
                            str << "\t\t[" << tmp_s_counter++ << "] " << iter_src_state->first << "\t"  << iter_src_state->second.robustness_counter << "\t" << iter_src_state->second.state_type_to_string() << endl;
                        }
                    }
                }
            }
            str << endl;

        }

        //downstream output
        for(iter_table= m_state_table.begin(); iter_table != m_state_table.end(); iter_table++){
            if_name = if_indextoname(iter_table->first,cstr);

            if((iter_vif = m_vif_map.find(iter_table->first))== m_vif_map.end()){
                HC_LOG_ERROR("failed to find vif to downstream if_index:" << iter_table->first);
                return;
            }

            str << "\t-- downstream " << if_name << " [vif=" << iter_vif->second << "] --" << endl;

            if(db->get_level_of_detail() > debug_msg::NORMAL){

                str << "\t\tgroup addr | robustness_counter | flag" << endl;

                int tmp_g_counter=0;
                for(iter_state= iter_table->second.begin(); iter_state != iter_table->second.end(); iter_state++){
                    src_group_state_pair* tmp_gsp = &iter_state->second;
                    str << "\t\t[" << tmp_g_counter++ << "] " <<iter_state->first << "\t"  << tmp_gsp->second.robustness_counter << "\t" << tmp_gsp->second.state_type_to_string() << endl;

                    int tmp_s_counter=0;
                    if(db->get_level_of_detail() > debug_msg::MORE){
                        if(tmp_gsp->first.size()>0 ){
                            str << "\t\t\tsrc addr | robustness_counter | flag" << endl;
                            for(iter_src_state = tmp_gsp->first.begin(); iter_src_state != tmp_gsp->first.end(); iter_src_state++){
                                str << "\t\t\t[" << tmp_s_counter++ << "] " << iter_src_state->first << "\t"  << iter_src_state->second.robustness_counter << "\t" << iter_src_state->second.state_type_to_string() << endl;
                            }
                        }
                    }
                }
                str << endl;

            }
        }
    }

    db->add_debug_msg(str.str());
}

bool proxy_instance::send_gq_to_all(){
    HC_LOG_TRACE("");
    HC_LOG_DEBUG("send general query to all");
    bool correct= true;

    state_table_map::iterator it_state_table;
    for(it_state_table= m_state_table.begin(); it_state_table != m_state_table.end(); it_state_table++){
        if(!m_sender->send_general_query(it_state_table->first)){
            HC_LOG_ERROR("failed to send general to if_index: " << it_state_table->first);
            correct = false;
        }
    }

    return correct;
}

bool proxy_instance::split_traffic(int if_index, const addr_storage& g_addr, const addr_storage& src_addr){
    HC_LOG_TRACE("");
    proxy_msg msg;
    std::list<unsigned int> vif_list;

    vif_map::iterator it_vif_map = m_vif_map.find(if_index);
    if(it_vif_map == m_vif_map.end()){
        HC_LOG_ERROR("cant find vif to if_index:" << if_index);
        return false;
    }
    int vif = it_vif_map->second;
    //cout << "vif vom source interface: " << vif << endl;

    //find all downstream interaces who join this group and if if_index is not a upstream add upstream vif
    add_all_group_vifs_to_list(vif_list, if_index, g_addr);
    //cout << "split_traffic: haben wieviele vifs gefunden: " << vif_list.size() << endl;
    if(vif_list.size() == 0) return false; //if nobody join this group ignore

    m_routing.add_route(vif, g_addr, src_addr, vif_list);

    //set source to flag CACHED_SRC
    if(if_index == m_upstream){ //upstream source
        upstream_src_state_map::iterator it_gss = m_upstream_state.find(g_addr);
        if(it_gss == m_upstream_state.end()){
            HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find upstream g_addr:" << g_addr);
            return false;
        }
        src_state_map::iterator iter_src = it_gss->second.find(src_addr);
        if(iter_src == it_gss->second.end()){
            HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find to g_addr:" << g_addr << " the source:"  << src_addr);
            return false;
        }
        iter_src->second.flag = src_state::CACHED_SRC;
    }else{ //downstream source
        state_table_map::iterator iter_table =m_state_table.find(if_index);
        if(iter_table == m_state_table.end()) return false;

        g_state_map::iterator iter_state = iter_table->second.find(g_addr);
        if(iter_state == iter_table->second.end()) {
            HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find downstream g_addr:" << g_addr);
            return false;
        }
        src_group_state_pair* sgs_pair = &iter_state->second;
        src_state_map::iterator iter_src = sgs_pair->first.find(src_addr);
        if(iter_state == iter_table->second.end()) {
            HC_LOG_ERROR("CACHE_MISS refresh routing: failed to find to g_addr:" << g_addr << " the source:"  << src_addr);
            return false;
        }
        iter_src->second.flag = src_state::CACHED_SRC;
    }

    return true;
}

bool proxy_instance::del_route(int if_index, const addr_storage& g_addr, const addr_storage& src_addr){
    HC_LOG_TRACE("");

    vif_map::iterator it_vif_map = m_vif_map.find(if_index);
    if(it_vif_map == m_vif_map.end()){
        HC_LOG_ERROR("cant find vif to if_index:" << if_index);
        return false;
    }
    int vif = it_vif_map->second;

    m_routing.del_route(vif, g_addr, src_addr);

    return true;
}

void proxy_instance::refresh_all_traffic(int if_index, const addr_storage& g_addr){
    HC_LOG_TRACE("");

    upstream_src_state_map::iterator iter_uss;
    src_state_map::iterator iter_src;
    state_table_map::iterator iter_table;
    g_state_map::iterator iter_state;
    src_group_state_pair* sgs_pair = 0;

    if(if_index == m_upstream){
        HC_LOG_ERROR("the if_index:" << if_index << " mussnt be the upstream, upstream have no joined groups");
    }

    //process upstream
    iter_uss = m_upstream_state.find(g_addr);
    if(iter_uss != m_upstream_state.end()){ //g_addr found
        for(iter_src = iter_uss->second.begin(); iter_src != iter_uss->second.end(); iter_src++){
            if(!split_traffic(m_upstream, g_addr, iter_src->first)){
                //have to check old upstream source because the have no default stream so they dont refresh themselve like downstreams
                iter_src->second.flag = src_state::UNUSED_SRC;
                del_route(if_index,g_addr,iter_src->first);
            }
        }
    }

    //process downstream
    for(iter_table = m_state_table.begin(); iter_table != m_state_table.end(); iter_table++){
        if(if_index != iter_table->first){

            iter_state = iter_table->second.find(g_addr);

            if(iter_state != iter_table->second.end()){
                sgs_pair = &iter_state->second;

                for(iter_src = sgs_pair->first.begin(); iter_src != sgs_pair->first.end(); iter_src++){
                    split_traffic(iter_table->first, g_addr, iter_src->first);
                }
            }

        }
    }
}

void proxy_instance::add_all_group_vifs_to_list(std::list<unsigned int>& vif_list, int without_if_index, addr_storage g_addr){

    vif_map::iterator it_vif_map;

    state_table_map::iterator iter_table;
    g_state_map::iterator iter_state;
    src_group_state_pair* sgs_pair = 0;

    //all downstream traffic musst be forward to upstream
    if(without_if_index != m_upstream){
        it_vif_map = m_vif_map.find(m_upstream);
        if(it_vif_map == m_vif_map.end()){
            HC_LOG_ERROR("cant find vif to if_index:" << m_upstream);
            return;
        }
        vif_list.push_back(it_vif_map->second);
        //cout << "add_all_group_vifs_to_list: upstream gefunden und gesetzt. if_index: " << m_upstream << " vif: " << it_vif_map->second << endl;
    }

    //all downstream and upstream traffic musste be forward to the downstream who joined the same group
    for(iter_table = m_state_table.begin(); iter_table != m_state_table.end(); iter_table++){
        if(without_if_index != iter_table->first){

            iter_state = iter_table->second.find(g_addr);

            if(iter_state != iter_table->second.end()){
                sgs_pair = &iter_state->second;

                //if the groupe is in use
                if(sgs_pair->second.flag == src_state::RUNNING || sgs_pair->second.flag == src_state::RESPONSE_STATE || sgs_pair->second.flag == src_state::WAIT_FOR_DEL){
                    it_vif_map = m_vif_map.find(iter_table->first);
                    if(it_vif_map == m_vif_map.end()){
                        HC_LOG_ERROR("cant find vif to if_index:" << iter_table->first);
                        return;
                    }
                    vif_list.push_back(it_vif_map->second);
                }
            }

        }
    }
}

void proxy_instance::registrate_if(int if_index){
    HC_LOG_TRACE("");

    vif_map::iterator it_vif_map;
    if((it_vif_map = m_vif_map.find(if_index))== m_vif_map.end()){
        HC_LOG_ERROR("failed to find vif from if_index:" << if_index);
        return;
    }
    int vif = it_vif_map->second;

    //##-- routing --##
    m_routing.add_vif( if_index, vif);

    //##-- receiver --##
    m_receiver->registrate_interface(if_index, vif, this);

    if(if_index != m_upstream){

        //##-- sender --##
        //join all_router_addr at all downstreams
        if(m_addr_family == AF_INET){
            addr_storage mc_router_addr(IPV4_ALL_IGMP_ROUTERS_ADDR);
            if(!m_sender->send_report(if_index, mc_router_addr)){
                HC_LOG_ERROR("failed to join: ==> " << IPV4_ALL_IGMP_ROUTERS_ADDR);
            }
        }else if(m_addr_family == AF_INET6){
            addr_storage mc_router_addr(IPV6_ALL_LINK_LOCAL_ROUTER);
            if(!m_sender->send_report(if_index, mc_router_addr )){
                HC_LOG_ERROR("failed to join: ==> " << mc_router_addr);
            }

            mc_router_addr= IPV6_ALL_SITE_LOCAL_ROUTER;
            if(!m_sender->send_report(if_index, mc_router_addr )){
                HC_LOG_ERROR("failed to join: ==> " << mc_router_addr);
            }

            HC_LOG_WARN("untesteted");
        }else{
            HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
            return;
        }

        //send the first GQ
        if(!m_sender->send_general_query(if_index)){
            HC_LOG_ERROR("failed to send general to if_index: " << if_index);
            return;
        }
    }
}

void proxy_instance::unregistrate_if(int if_index){
    HC_LOG_TRACE("");

    vif_map::iterator it_vif_map;
    if((it_vif_map = m_vif_map.find(if_index))== m_vif_map.end()){
        HC_LOG_ERROR("failed to find vif from if_index:" << if_index);
        return;
    }
    int vif = it_vif_map->second;

    //##-- routing --##
    m_routing.del_vif(if_index, vif);

    //##-- receiver --##
    m_receiver->del_interface(if_index, vif);

    //##-- timing --##
    //remove all running times
    //m_timing->stop_all_time(this);

    if(if_index != m_upstream){

        //##-- sender --##
        //leave all router addr at all downstreams
        if(m_addr_family == AF_INET){
            addr_storage mc_router_addr(IPV4_ALL_IGMP_ROUTERS_ADDR);
            if(!m_sender->send_leave(if_index, mc_router_addr )){
                HC_LOG_ERROR("failed to leave: ==> " << mc_router_addr);
            }
        }else if(m_addr_family == AF_INET6){
            addr_storage mc_router_addr(IPV6_ALL_LINK_LOCAL_ROUTER);
            if(!m_sender->send_leave(if_index, mc_router_addr )){
                HC_LOG_ERROR("failed to leave: ==> " << mc_router_addr);
            }

            mc_router_addr=IPV6_ALL_SITE_LOCAL_ROUTER;
            if(!m_sender->send_leave(if_index, mc_router_addr )){
                HC_LOG_ERROR("failed to leave: ==> " << mc_router_addr);
            }

        }else{
            HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
            return;
        }


    }

}

bool proxy_instance::is_group_joined(int without_if_index, const addr_storage& g_addr){
    HC_LOG_TRACE("");

    state_table_map::iterator iter_table;
    g_state_map::iterator iter_state;
    src_group_state_pair* sgs_pair = 0;

    //process downstream
    for(iter_table = m_state_table.begin(); iter_table != m_state_table.end(); iter_table++){
        if(without_if_index != iter_table->first){

            iter_state = iter_table->second.find(g_addr);

            if(iter_state != iter_table->second.end()){
                sgs_pair = &iter_state->second;
                if(sgs_pair->second.flag != src_state::INIT){
                    return true;
                }
            }

        }
    }

    return false;
}

void proxy_instance::close(){
    HC_LOG_TRACE("");

    delete m_sender;

    m_receiver->stop();
    m_receiver->join();
    delete m_receiver;



}
