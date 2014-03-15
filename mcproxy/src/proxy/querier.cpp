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
#include "include/proxy/querier.hpp"
#include "include/utils/addr_storage.hpp"
#include "include/proxy/timing.hpp"
#include "include/proxy/interfaces.hpp"
#include "include/proxy/def.hpp"

#include "include/proxy/sender.hpp"
#include "include/proxy/igmp_sender.hpp"
#include "include/proxy/mld_sender.hpp"

#include <unistd.h>
#include <iostream>
#include <sstream>

querier::querier(worker* msg_worker, group_mem_protocol querier_version_mode, int if_index, const std::shared_ptr<const sender>& sender, const std::shared_ptr<timing>& timing, const timers_values& tv, callback_querier_state_change cb_state_change)
    : m_msg_worker(msg_worker)
    , m_if_index(if_index)
    , m_db(querier_version_mode)
    , m_timers_values(tv)
    , m_cb_state_change(cb_state_change)
    , m_sender(sender)
    , m_timing(timing)
{
    HC_LOG_TRACE("");

    //join all router groups
    if (!router_groups_function(true)) {
        HC_LOG_ERROR("failed to subscribe multicast router groups");
        throw "failed to subscribe multicast router groups";
    }

    if (!send_general_query()) {
        HC_LOG_ERROR("failed to initialise query startup");
        throw "failed to initialise query startup";
    }
}

//unsigned int if_index, mc_filter filter_mode, const addr_storage& gaddr, const source_list<source>& slist

bool querier::router_groups_function(bool subscribe) const
{
    HC_LOG_TRACE("");

//MLDv1 RFC 2710: Section 8. link-scope all-routers (FF02::2), link-scope all-routers (FF05::2)| IANA: site local scope all-routers
//MLDv2 RFC 3810: Section 7. all MLDv2-capable routers (FF02::16)
//IGMPv1
//IGMPv2 RFC 2236: Section 9. ALL-ROUTERS (224.0.0.2)
//IGMPv3 IANA: IGMP (224.0.0.22)

    mc_filter mf;

    if (subscribe) {
        mf = EXCLUDE_MODE;
    } else {
        mf = INCLUDE_MODE;
    }

    bool rc = true;
    if (is_IPv4(m_db.querier_version_mode)) {
        rc = rc && m_sender->send_record(m_if_index, mf, addr_storage(IPV4_ALL_IGMP_ROUTERS_ADDR), source_list<source>());
        rc = rc && m_sender->send_record(m_if_index, mf, addr_storage(IPV4_IGMPV3_ADDR), source_list<source>());
    } else if (is_IPv6(m_db.querier_version_mode)) {
        rc = rc && m_sender->send_record(m_if_index, mf, addr_storage(IPV6_ALL_NODE_LOCAL_ROUTER), source_list<source>());
        rc = rc && m_sender->send_record(m_if_index, mf, addr_storage(IPV6_ALL_SITE_LOCAL_ROUTER), source_list<source>());
        rc = rc && m_sender->send_record(m_if_index, mf, addr_storage(IPV6_ALL_MLDv2_CAPABLE_ROUTERS), source_list<source>());
    } else {
        HC_LOG_ERROR("unknown ip version");
        return false;
    }
    return rc;
}
bool querier::send_general_query()
{
    HC_LOG_TRACE("");
    if (m_db.general_query_timer.get() == nullptr) {
        m_db.startup_query_count = m_timers_values.get_startup_query_count() - 1;
    }

    std::chrono::seconds t;
    if (m_db.startup_query_count > 0) {
        m_db.startup_query_count--;
        t  = m_timers_values.get_startup_query_interval();
    } else {
        t = m_timers_values.get_query_interval();
    }

    auto gqt = std::make_shared<general_query_timer_msg>(m_if_index, t);
    m_db.general_query_timer = gqt;

    m_timing->add_time(t, m_msg_worker, gqt);
    return m_sender->send_general_query(m_if_index, m_timers_values);
}

void querier::receive_record(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    if (msg->get_type() != proxy_msg::GROUP_RECORD_MSG) {
        HC_LOG_ERROR("wrong proxy message, it musst be be a GROUP_RECORD_MS");
        return;
    }

    auto gr = std::static_pointer_cast<group_record_msg>(msg);

    auto db_info_it = m_db.group_info.find(gr->get_gaddr());

    if (db_info_it == end(m_db.group_info)) {
        //add an empty neutral record  to membership database
        HC_LOG_DEBUG("gaddr not found");
        db_info_it = m_db.group_info.insert(gaddr_pair(gr->get_gaddr(), gaddr_info(m_db.querier_version_mode))).first;
    }

    //backwards compatibility coordination
    if (!is_newest_version(gr->get_grp_mem_proto()) && is_older_or_equal_version(gr->get_grp_mem_proto(), m_db.querier_version_mode) ) {
        db_info_it->second.compatibility_mode_variable = gr->get_grp_mem_proto();
        auto ohpt = std::make_shared<older_host_present_timer_msg>(m_if_index, db_info_it->first, m_timers_values.get_older_host_present_interval());
        db_info_it->second.older_host_present_timer = ohpt;
        m_timing->add_time(m_timers_values.get_older_host_present_interval(), m_msg_worker, ohpt);
    }

    //section 8.3.2. In the Presence of MLDv1 Multicast Address Listeners
    //MLDv2 BLOCK messages are ignored, as are source-lists in TO_EX()
    //messages (i.e., any TO_EX() message is treated as TO_EX( {} )).
    if (db_info_it->second.is_in_backward_compatibility_mode()) {
        if (gr->get_record_type() == CHANGE_TO_EXCLUDE_MODE) {
            gr->get_slist() = {};
        } else if (gr->get_record_type() == BLOCK_OLD_SOURCES){
            return;     
        }
    }

    switch (db_info_it->second.filter_mode) {
    case  INCLUDE_MODE:
        receive_record_in_include_mode(gr->get_record_type(), gr->get_gaddr(), gr->get_slist(), db_info_it->second);

        //if the new created group is not used delete it
        if (db_info_it->second.filter_mode == INCLUDE_MODE && db_info_it->second.include_requested_list.empty()) {
            m_db.group_info.erase(db_info_it);
        }

        break;
    case EXCLUDE_MODE:
        receive_record_in_exclude_mode(gr->get_record_type(), gr->get_gaddr(), gr->get_slist(), db_info_it->second);
        break;
    default :
        HC_LOG_ERROR("wrong filter mode: " << db_info_it->second.filter_mode);
        break;
    }

}

void querier::receive_record_in_include_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, gaddr_info& ginfo)
{
    HC_LOG_TRACE("record type: " << record_type);
    //7.4.1.  Reception of Current State Records
    //7.4.2.  Reception of Filter Mode Change and Source List Change Records

    source_list<source>& A = ginfo.include_requested_list;
    source_list<source>& B = slist;

    gaddr_info& filter_timer = ginfo;

    switch (record_type) {

        //Router State  Report Received  New Router State     Actions
        //------------  ---------------  ----------------     -------
        //INCLUDE (A)     ALLOW (B)      INCLUDE (A+B)        (B)=MALI
    case ALLOW_NEW_SOURCES: {//ALLOW(x)
        A += B;

        mali(gaddr, A, std::move(B));

        state_change_notification(gaddr);
    }
    break;


    //INCLUDE (A)     BLOCK (B)      INCLUDE (A)          Send Q(MA,A*B)
    case BLOCK_OLD_SOURCES: {//BLOCK(x)
        send_Q(gaddr, ginfo, A, (A * B));
    }
    break;


    //INCLUDE (A)     TO_EX (B)      EXCLUDE (A*B,B-A)    (B-A)=0
    //                                                    Delete (A-B)
    //                                                    Send Q(MA,A*B)
    //                                                    Filter Timer=MALI
    case CHANGE_TO_EXCLUDE_MODE: {//TO_EX(x)
        ginfo.filter_mode = EXCLUDE_MODE;
        ginfo.include_requested_list *= B;
        ginfo.exclude_list = B - A;

        send_Q(gaddr, ginfo, ginfo.include_requested_list, (A * B)),
               mali(gaddr, filter_timer);


        state_change_notification(gaddr); //all sources
    }
    break;


    //INCLUDE (A)     TO_IN (B)      INCLUDE (A+B)        (B)=MALI
    //                                                    Send Q(MA,A-B)
    case CHANGE_TO_INCLUDE_MODE: {//TO_IN(x)
        A += B;

        send_Q(gaddr, ginfo, A, (A - B));
        mali(gaddr, A, std::move(B));

        state_change_notification(gaddr);
    }
    break;


    //INCLUDE (A)       IS_EX (B)     EXCLUDE (A*B, B-A) (B-A)=0
    //                                                    Delete (A-B)
    //                                                    Filter Timer=MALI
    case  MODE_IS_EXCLUDE: {//IS_EX(x)
        ginfo.filter_mode = EXCLUDE_MODE;
        ginfo.include_requested_list *= B;
        ginfo.exclude_list = B - A;

        mali(gaddr, filter_timer);

        state_change_notification(gaddr); //all sources
    }
    break;


    //INCLUDE (A)       IS_IN (B)     INCLUDE (A+B)      (B)=MALI
    case MODE_IS_INCLUDE: {//IS_IN(x)
        A += B;

        mali(gaddr, A, move(B));

        state_change_notification(gaddr);
    }
    break;


    default:
        HC_LOG_ERROR("unknown multicast record type: " << record_type);
        return;
    }

}

void querier::receive_record_in_exclude_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, gaddr_info& ginfo)
{
    HC_LOG_TRACE("record type: " << record_type);
    //7.4.1.  Reception of Current State Records
    //7.4.2.  Reception of Filter Mode Change and Source List Change Records

    source_list<source>& X = ginfo.include_requested_list;
    source_list<source>& Y = ginfo.exclude_list;

    source_list<source>& A = slist;
    gaddr_info& filter_timer = ginfo;

    switch (record_type) {


        //Router State  Report Received  New Router State     Actions
        //------------  ---------------  ----------------     -------
        //EXCLUDE (X,Y)   ALLOW (A)      EXCLUDE (X+A,Y-A)    (A)=MALI
    case ALLOW_NEW_SOURCES: {//ALLOW(x)
        X += A;
        Y -= A;

        mali(gaddr, X, std::move(A));

        state_change_notification(gaddr);
    }
    break;


    //EXCLUDE (X,Y)   BLOCK (A)      EXCLUDE (X+(A-Y),Y)  (A-X-Y) =
    //                                                          Filter Timer
    //                                                    Send Q(MA,A-Y)
    case BLOCK_OLD_SOURCES: { //BLOCK(x)
        //auto tmpX = X;
        X += (A - Y);

        //filter_time(ginfo, X, (A - tmpX) - Y); this is useless the source timer will be update again in send_Q()??????????????????
        send_Q(gaddr, ginfo, X, (A - Y));
    }
    break;


    //EXCLUDE (X,Y)   TO_EX (A)      EXCLUDE (A-Y,Y*A)    (A-X-Y) =
    //                                                          Filter Timer
    //                                                    Delete (X-A)
    //                                                    Delete (Y-A)
    //                                                    Send Q(MA,A-Y)
    //                                                    Filter Timer=MALI
    case CHANGE_TO_EXCLUDE_MODE: {//TO_EX(x)
        //filter_time(ginfo, A, (A - X) - Y); this is useless the source timer will be update again in send_Q()??????????????????


        //X = (A - Y);
        //this is bad!! if in request_list is IP 1.1.1.1 and in A 1.1.1.1 then you create a zombie in X (without a running timer)???????????????
        X *= A;
        X += (A - Y);

        Y *= A;

        auto tmpXa = X;
        send_Q(gaddr, ginfo, X, move(tmpXa)); //bad style, but i haven't a better solution right now ???????????
        mali(gaddr, filter_timer);

        state_change_notification(gaddr);
    }
    break;


    //EXCLUDE (X,Y)   TO_IN (A)      EXCLUDE (X+A,Y-A)    (A)=MALI
    //                                                    Send Q(MA,X-A)
    //                                                    Send Q(MA)
    case CHANGE_TO_INCLUDE_MODE: {//TO_IN(x)
        X += A;
        Y -= A;

        send_Q(gaddr, ginfo, X, (X - A));
        send_Q(gaddr, ginfo);
        mali(gaddr, X, std::move(A));

        state_change_notification(gaddr);
    }
    break;


    //EXCLUDE (X,Y)     IS_EX (A)     EXCLUDE (A-Y, Y*A) (A-X-Y)=MALI
    //                                                   Delete (X-A)
    //                                                   Delete (Y-A)
    //                                                   Filter Timer=MALI
    case  MODE_IS_EXCLUDE: {//IS_EX(x)
        mali(gaddr, A, (A - X) - Y);

        //X = (A - Y);
        //this is bad!! if in request_list is IP 1.1.1.1 and in A 1.1.1.1 then you create a zombie in X (without a running timer)?????????????????????
        X *= A;
        X += (A - Y);

        Y *= A;

        mali(gaddr, filter_timer);

        state_change_notification(gaddr);
    }
    break;


    //EXCLUDE (X,Y)     IS_IN (A)     EXCLUDE (X+A, Y-A) (A)=MALI
    case MODE_IS_INCLUDE: {//IS_IN(x)
        X += A;
        Y -= A;

        mali(gaddr, X, std::move(A));

        state_change_notification(gaddr);
    }
    break;
    default:
        HC_LOG_ERROR("unknown multicast record type: " << record_type);
        return;
    }
}

void querier::timer_triggerd(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");
    gaddr_map::iterator db_info_it;
    std::shared_ptr<timer_msg> tm;

    if (!msg.unique()) {
        switch (msg->get_type()) {
        case proxy_msg::FILTER_TIMER_MSG:
        case proxy_msg::SOURCE_TIMER_MSG:
        case proxy_msg::RET_GROUP_TIMER_MSG:
        case proxy_msg::RET_SOURCE_TIMER_MSG:
        case proxy_msg::OLDER_HOST_PRESENT_TIMER_MSG: {
            tm = std::static_pointer_cast<timer_msg>(msg);

            db_info_it = m_db.group_info.find(tm->get_gaddr());

            if (db_info_it == end(m_db.group_info)) {
                HC_LOG_ERROR("filter_timer message is still in use but cannot found");
                return;
            }
        }
        break;
        case proxy_msg::GENERAL_QUERY_TIMER_MSG:
            tm = std::static_pointer_cast<timer_msg>(msg);
            break;
        default:
            HC_LOG_ERROR("unknown timer message format");
            return;
        }
    } else {
        HC_LOG_DEBUG("filter_timer is outdate");
        return;
    }

    switch (msg->get_type()) {
    case proxy_msg::FILTER_TIMER_MSG:
        timer_triggerd_filter_timer(db_info_it, tm);
        break;
    case proxy_msg::SOURCE_TIMER_MSG:
        timer_triggerd_source_timer(db_info_it, tm);
        break;
    case proxy_msg::RET_GROUP_TIMER_MSG:
        timer_triggerd_ret_group_timer(db_info_it, tm);
        break;
    case proxy_msg::RET_SOURCE_TIMER_MSG:
        timer_triggerd_ret_source_timer(db_info_it, tm);
        break;
    case proxy_msg::GENERAL_QUERY_TIMER_MSG:
        timer_triggerd_general_query_timer(tm);
        break;
    case proxy_msg::OLDER_HOST_PRESENT_TIMER_MSG:
        timer_triggerd_older_host_present_timer(db_info_it, tm);
        break;
    default:
        HC_LOG_ERROR("unknown timer message format");
        return;
    }
}

void querier::timer_triggerd_filter_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg)
{
    HC_LOG_TRACE("");

    gaddr_info& ginfo = db_info_it->second;

    auto ftimer = std::static_pointer_cast<filter_timer_msg>(msg);

    if (ftimer->is_used_as_source_timer()) {
        timer_triggerd_source_timer(db_info_it, msg);
    }

    if (ginfo.shared_filter_timer.get() != ftimer.get()) {
        HC_LOG_ERROR("found filter_timer differs from processing filter_timer");
        return;
    }
    //7.2.2.  Definition of Filter Timers
    //Router               Filter
    //Filter Mode          Timer Value        Actions/Comments
    //-----------       -----------------     ----------------
    //INCLUDE             Not Used            All listeners in
    //                                        INCLUDE mode.
    //
    //EXCLUDE             Timer > 0           At least one listener
    //                                        in EXCLUDE mode.
    //
    //EXCLUDE             Timer == 0          No more listeners in
    //                                        EXCLUDE mode for the
    //                                        multicast address.
    //                                        If the Requested List
    //                                        is empty, delete
    //                                        Multicast Address
    //                                        Record.  If not, switch
    //                                        to INCLUDE filter mode;
    //                                        the sources in the
    //                                        Requested List are
    //                                        moved to the Include
    //                                        List, and the Exclude
    //                                        List is deleted.

    if (ginfo.filter_mode == EXCLUDE_MODE) {
        if (ginfo.include_requested_list.empty()) {
            addr_storage notify_gaddr = db_info_it->first;

            m_db.group_info.erase(db_info_it);

            state_change_notification(notify_gaddr); //only A
        } else {
            addr_storage notify_gaddr = db_info_it->first;

            ginfo.filter_mode = INCLUDE_MODE;
            ginfo.shared_filter_timer.reset();
            ginfo.exclude_list.clear();

            state_change_notification(notify_gaddr); //only A
        }
    } else {
        HC_LOG_ERROR("filter_mode is not in expected mode EXCLUDE");
    }

}

void querier::timer_triggerd_source_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg)
{
    HC_LOG_TRACE("");

    gaddr_info& ginfo = db_info_it->second;

    switch (ginfo.filter_mode) {


        //7.2.3.  Definition of Source Timers
        //If the timer of a
        //source from the Include List expires, the source is deleted from the
        //Include List.  If there are no more source records left, the
        //multicast address record is deleted from the router.
    case INCLUDE_MODE: {
        addr_storage notify_gaddr = db_info_it->first;

        for (auto it = std::begin(ginfo.include_requested_list); it != std::end(ginfo.include_requested_list);) {
            if (it->shared_source_timer.get() == msg.get()) {

                it = ginfo.include_requested_list.erase(it);
                continue;
            }
            ++it;
        }

        if (ginfo.include_requested_list.empty()) {
            m_db.group_info.erase(db_info_it);
        }


        state_change_notification(notify_gaddr); //only A
        break;
    }

    //7.2.3.  Definition of Source Timers
    //If the timer
    //of a source from the Requested List expires, the source is moved to
    //the Exclude List.
    case EXCLUDE_MODE: {
        addr_storage notify_gaddr = db_info_it->first;

        for (auto it = std::begin(ginfo.include_requested_list); it != std::end(ginfo.include_requested_list);) {
            if (it->shared_source_timer.get() == msg.get()) {
                it->shared_source_timer.reset();
                ginfo.exclude_list.insert(*it);

                it = ginfo.include_requested_list.erase(it);
                continue;
            }
            ++it;
        }

        state_change_notification(notify_gaddr); //only A
        break;
    }
    default:
        HC_LOG_ERROR("unknown filter mode");
    }
}

void querier::timer_triggerd_ret_group_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg)
{
    HC_LOG_TRACE("");

    gaddr_info& ginfo = db_info_it->second;

    if (ginfo.group_retransmission_timer.get() == msg.get()) { //msg is an retransmit filter timer message
        send_Q(msg->get_gaddr(), ginfo);
    } else { //msg is an retransmit source timer message
        HC_LOG_ERROR("retransmission timer not found");
    }
}

void querier::timer_triggerd_ret_source_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg)
{
    HC_LOG_TRACE("");

    gaddr_info& ginfo = db_info_it->second;

    if (ginfo.source_retransmission_timer.get() == msg.get()) { //msg is an retransmit filter timer message
        send_Q(msg->get_gaddr(), ginfo, ginfo.include_requested_list , source_list<source>(), true);
    } else { //msg is an retransmit source timer message
        HC_LOG_ERROR("retransmission timer not found");
    }
}

void querier::timer_triggerd_general_query_timer(const std::shared_ptr<timer_msg>& msg)
{
    HC_LOG_TRACE("");

    if (m_db.general_query_timer.get() == msg.get()) {
        send_general_query();
    } else {
        HC_LOG_ERROR("general query timer not found");
    }
}

void querier::timer_triggerd_older_host_present_timer(gaddr_map::iterator db_info_it, const std::shared_ptr<timer_msg>& msg)
{
    HC_LOG_TRACE("");

    gaddr_info& ginfo = db_info_it->second;

    if (ginfo.older_host_present_timer.get() == msg.get()) {
        if (is_newest_version(ginfo.compatibility_mode_variable)) {
            ginfo.older_host_present_timer = nullptr;
            state_change_notification(db_info_it->first);
        } else {
            ginfo.compatibility_mode_variable = get_next_newer_version(ginfo.compatibility_mode_variable);

            std::chrono::milliseconds delay;
            if (is_newest_version(ginfo.compatibility_mode_variable)) {
                //RFC 3810 8.3.2. In the Presence of MLDv1 Multicast Address Listeners:
                //Source-specific information will be learned during the next General Query,
                //but sources that should be blocked will not be blocked until [Multicast Address Listening Interval]
                //after that.
                delay = m_timers_values.get_multicast_address_listening_interval();
            } else {
                delay = m_timers_values.get_older_host_present_interval();
            }

            auto ohpt = std::make_shared<older_host_present_timer_msg>(m_if_index, db_info_it->first, delay);
            ginfo.older_host_present_timer = ohpt;
            m_timing->add_time(delay, m_msg_worker, ohpt);
        }
    }
}

void querier::mali(const addr_storage& gaddr, gaddr_info& ginfo) const
{
    HC_LOG_TRACE("");
    auto ft = std::make_shared<filter_timer_msg>(m_if_index, gaddr, m_timers_values.get_multicast_address_listening_interval());

    ginfo.shared_filter_timer = ft;

    m_timing->add_time(m_timers_values.get_multicast_address_listening_interval(), m_msg_worker, ft);
}

void querier::mali(const addr_storage& gaddr, source_list<source>& slist) const
{
    HC_LOG_TRACE("");
    auto st = std::make_shared<source_timer_msg>(m_if_index, gaddr, m_timers_values.get_multicast_address_listening_interval());

    for (auto & e : slist) {
        e.shared_source_timer = st; //shard_source_timer is mutable
        e.retransmission_count = -1;
    }

    if (!slist.empty()) {
        m_timing->add_time(m_timers_values.get_multicast_address_listening_interval(), m_msg_worker, st);
    }
}

void querier::mali(const addr_storage& gaddr, source_list<source>& slist, source_list<source>&& tmp_slist) const
{
    HC_LOG_TRACE("");
    mali(gaddr, tmp_slist);

    for (auto & e : tmp_slist) {
        auto it = slist.find(e);
        if (it != std::end(slist)) {
            it->shared_source_timer = e.shared_source_timer;
            it->retransmission_count = -1;
        }
    }
}

void querier::filter_time(gaddr_info& ginfo, source_list<source>& slist, source_list<source>&&  tmp_slist)
{
    HC_LOG_TRACE("");

    for (auto & e : tmp_slist) {
        auto it = slist.find(e);
        if (it != std::end(slist)) {
            std::static_pointer_cast<filter_timer_msg>(ginfo.shared_filter_timer)->set_as_source_timer();
            it->shared_source_timer = ginfo.shared_filter_timer;
        }
    }
}

void querier::send_Q(const addr_storage& gaddr, gaddr_info& ginfo)
{
    HC_LOG_TRACE("");

//7.6.3.1.  Building and Sending Multicast Address Specific Queries
    //When a table action "Send Q(MA)" is encountered, the Filter Timer
    //must be lowered to LLQT.  The Querier must then immediately send a
    //Multicast Address Specific query as well as schedule [Last Listener
    //Query Count - 1] query retransmissions to be sent every [Last
    //Listener Query Interval], over [Last Listener Query Time].

    //When transmitting a Multicast Address Specific Query, if the Filter
    //Timer is larger than LLQT, the "Suppress Router-Side Processing" bit
    //is set in the query message.

    if (ginfo.group_retransmission_timer == nullptr) {
        ginfo.group_retransmission_count = m_timers_values.get_last_listener_query_count();
        auto llqt = m_timers_values.get_last_listener_query_time();
        auto ftimer = std::make_shared<filter_timer_msg>(m_if_index, gaddr, llqt);

        ginfo.shared_filter_timer = ftimer;

        m_timing->add_time(llqt, m_msg_worker, ftimer);
    }

    if (ginfo.group_retransmission_count > 0) {
        ginfo.group_retransmission_count--;

        if (ginfo.group_retransmission_count > 0) {
            auto llqi = m_timers_values.get_last_listener_query_interval();
            auto rtimer = std::make_shared<retransmit_group_timer_msg>(m_if_index, gaddr, llqi);
            ginfo.group_retransmission_timer = rtimer;
            m_timing->add_time(llqi, m_msg_worker, rtimer);
        }

        m_sender->send_mc_addr_specific_query(m_if_index, m_timers_values, gaddr, ginfo.shared_filter_timer->is_remaining_time_greater_than(m_timers_values.get_last_listener_query_time()));

    } else { //reset itself
        ginfo.group_retransmission_timer = nullptr;
        ginfo.group_retransmission_count = -1;
    }
}


void querier::send_Q(const addr_storage& gaddr, gaddr_info& ginfo, source_list<source>& slist, source_list<source>&& tmp_list, bool in_retransmission_state)
{
    HC_LOG_TRACE("");

    bool is_used = false;

    auto llqt = m_timers_values.get_last_listener_query_time();
    auto st = std::make_shared<source_timer_msg>(m_if_index, gaddr, llqt);

    for (auto & e : tmp_list) {
        auto it = slist.find(e);
        if (it != std::end(slist)) {
            if (it->retransmission_count < 1) {
                is_used = true;

                it->shared_source_timer = st;
                it->retransmission_count = m_timers_values.get_last_listener_query_count();
            }
        }
    }

    if (is_used) {
        m_timing->add_time(llqt, m_msg_worker, st);
    }

    if (is_used  || in_retransmission_state) {
        if (m_sender->send_mc_addr_and_src_specific_query(m_if_index, m_timers_values, gaddr, slist)) {
            auto llqi = m_timers_values.get_last_listener_query_interval();
            auto rst = std::make_shared<retransmit_source_timer_msg>(m_if_index, gaddr, llqi);
            ginfo.source_retransmission_timer = rst;
            m_timing->add_time(llqi, m_msg_worker, rst);
        }
    }
}

void querier::state_change_notification(const addr_storage& gaddr)
{
    HC_LOG_TRACE("");
    m_cb_state_change(m_if_index, gaddr);
}

querier::~querier()
{
    HC_LOG_TRACE("");
    router_groups_function(false);
}

timers_values& querier::get_timers_values()
{
    HC_LOG_TRACE("");
    return m_timers_values;
}

//interface_filter_fun is very useless, please overwork ???????????????
void querier::suggest_to_forward_traffic(const addr_storage& gaddr, std::list<std::pair<source, std::list<unsigned int>>>& rt_slist, std::function<bool(const addr_storage&)> interface_filter_fun) const
{
    HC_LOG_TRACE("");

    if (m_db.is_querier == true) {
        auto db_info_it = m_db.group_info.find(gaddr);
        if (db_info_it != std::end(m_db.group_info)) {
            if (db_info_it->second.is_under_bakcward_compatibility_effects()) {

                //accept all sources
                for (auto & e : rt_slist) {
                    if (interface_filter_fun(e.first.saddr)) {
                        e.second.push_back(m_if_index);
                    }
                }

            } else {

                if (db_info_it->second.filter_mode == INCLUDE_MODE) {
                    for (auto & e : rt_slist) {
                        auto irl_it = db_info_it->second.include_requested_list.find(e.first);
                        if (irl_it != std::end(db_info_it->second.include_requested_list) ) {
                            if (interface_filter_fun(e.first.saddr)) {
                                e.second.push_back(m_if_index);
                            }
                        }
                    }
                } else if (db_info_it->second.filter_mode == EXCLUDE_MODE) {
                    for (auto & e : rt_slist) {
                        auto el_it = db_info_it->second.exclude_list.find(e.first);
                        if (el_it == std::end(db_info_it->second.exclude_list) ) {
                            if (interface_filter_fun(e.first.saddr)) {
                                e.second.push_back(m_if_index);
                            }
                        }
                    }
                } else {
                    HC_LOG_ERROR("unknown filter mode");
                }

            }
        }
    }

}

std::pair<mc_filter, source_list<source>> querier::get_group_membership_infos(const addr_storage& gaddr)
{
    HC_LOG_TRACE("");
    std::pair<mc_filter, source_list<source>> rt_pair;
    rt_pair.first = INCLUDE_MODE;
    rt_pair.second = {};

    auto db_info_it = m_db.group_info.find(gaddr);
    if (db_info_it != std::end(m_db.group_info)) {
        if (db_info_it->second.is_under_bakcward_compatibility_effects()) {

            rt_pair.first = EXCLUDE_MODE;
            rt_pair.second = {};

        } else {

            rt_pair.first = db_info_it->second.filter_mode;
            if (db_info_it->second.filter_mode == INCLUDE_MODE) {
                rt_pair.second = db_info_it->second.include_requested_list;
            } else if (db_info_it->second.filter_mode == EXCLUDE_MODE) {
                rt_pair.second = db_info_it->second.exclude_list;
            }

        }
    }

    return rt_pair;
}

std::string querier::to_string() const
{
    std::ostringstream s;
    s << "##-- downstream interface: " << interfaces::get_if_name(m_if_index) << " (index:" << m_if_index << ") --##" << std::endl;
    s << m_db;
    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const querier& q)
{
    HC_LOG_TRACE("");
    return stream << q.to_string();

}
