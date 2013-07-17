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
#include "include/utils/addr_storage.hpp"
#include "include/proxy/check_source.hpp"
#include <linux/mroute.h>
#include <linux/mroute6.h>

bool check_source::init(int addr_family, mroute_socket* sock ){
    HC_LOG_TRACE("");

    this->m_addr_family = addr_family;
    m_sock = sock;

    m_current_check = &m_check_src_a;

    return true;
}

bool check_source::check(){
    HC_LOG_TRACE("");

    if(m_current_check == &m_check_src_a){
        m_current_check = &m_check_src_b;
    }else{
        m_current_check = &m_check_src_a;
    }

    m_current_check->clear();

    return true;
}

bool check_source::is_src_unused(int vif, addr_storage src_addr, addr_storage g_addr){
    HC_LOG_TRACE("");

    // old table  |  new talbe  |  unused?
    //   entry    |    entry    |
    //------------------------------------
    //   empty    |   empty     | true  ==> Error
    //------------------------------------
    //   empty    |   exist     | false ==> save to current table
    //------------------------------------
    //   exist    |   empty     | true  ==> Error
    //------------------------------------
    //   exist    |   exist     | IF( old entry != new entry) ==> false and save ELSE true and save

    pkt_cnt_map* old_check= (m_current_check == &m_check_src_a)? &m_check_src_b : &m_check_src_a;

    int current_pkt_cnt = -1;
    int old_pkt_cnt = -1;

    //extract mroute information
    //--kernel
    if(m_addr_family == AF_INET){
        struct sioc_sg_req tmp_stat;
        if(m_sock->get_mroute_stats(src_addr.to_string().c_str(), g_addr.to_string().c_str(), &tmp_stat, nullptr)){
            current_pkt_cnt = tmp_stat.pktcnt;
            HC_LOG_DEBUG(" src_addr: " << src_addr << " g_addr: " << g_addr << " vif: " << vif);
            HC_LOG_DEBUG(" -packets[" << tmp_stat.bytecnt << " bytes]:" << tmp_stat.pktcnt);
            HC_LOG_DEBUG(" -wrong packets:" << tmp_stat.wrong_if);
        }else{
            return true;
        }
    }else if(m_addr_family == AF_INET6){
        struct sioc_sg_req6 tmp_stat;
        if(m_sock->get_mroute_stats(src_addr.to_string().c_str(), g_addr.to_string().c_str(), nullptr, &tmp_stat)){
            current_pkt_cnt = tmp_stat.pktcnt;
            HC_LOG_DEBUG(" src_addr: " << src_addr << " g_addr: " << g_addr << " vif: " << vif);
            HC_LOG_DEBUG(" -packets[" << tmp_stat.bytecnt << " bytes]:" << tmp_stat.pktcnt);
            HC_LOG_DEBUG(" -wrong packets:" << tmp_stat.wrong_if);
        }else{
            return true;
        }
    }else{
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return true;
    }

    //evaluate and save mroute information
    src_grp_pair current_sg_pair(src_addr,g_addr);
    pkt_cnt_map::iterator iter_pkt_cnt = old_check->find(current_sg_pair);

    if(iter_pkt_cnt == old_check->end()){
        m_current_check->insert(pkt_cnt_pair(current_sg_pair,current_pkt_cnt));
        return false;
    }else{
       old_pkt_cnt = iter_pkt_cnt->second;
    }

    if(current_pkt_cnt == old_pkt_cnt){
        m_current_check->insert(pkt_cnt_pair(current_sg_pair,current_pkt_cnt));
        return true;
    }else{
        m_current_check->insert(pkt_cnt_pair(current_sg_pair,current_pkt_cnt));
        return false;
    }

}
