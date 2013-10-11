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

#include "include/proxy/sender.hpp"
#include "include/proxy/igmp_sender.hpp"
#include "include/proxy/mld_sender.hpp"

#include <unistd.h>
#include <net/if.h>
#include <iostream>
#include <sstream>

querier::querier(proxy_instance* pr_i, int addr_family, int if_index, const std::shared_ptr<const sender>& sender, const std::shared_ptr<timing>& timing)
    : m_proxy_instance(pr_i)
    , m_addr_family(addr_family)
    , m_if_index(if_index)
    , m_sender(sender)
    , m_timing(timing)
{
    HC_LOG_TRACE("");

    //join all router groups
    if (!router_groups_function(&sender::send_report)) {
        HC_LOG_ERROR("failed to subscribe multicast router groups");
        throw "failed to subscribe multicast router groups";
    }

    if (!init_db()) {
        HC_LOG_ERROR("failed to initalize multicast membership database");
        throw "failed to initialze multicast membership database";
    }
}

bool querier::init_db()
{
    HC_LOG_TRACE("");

    if (m_addr_family == AF_INET) {
        m_db.compatibility_mode_variable = IGMPv3;
    } else if (m_addr_family == AF_INET6) {
        m_db.compatibility_mode_variable = MLDv2;
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }

    m_db.is_querier = true;

    return true;
}

bool querier::router_groups_function(std::function<bool(const sender&, int, addr_storage)> f) const
{
    HC_LOG_TRACE("");

//MLDv1 RFC 2710: Section 8. link-scope all-routers (FF02::2), link-scope all-routers (FF05::2)| IANA: site local scope all-routers
//MLDv2 RFC 3810: Section 7. all MLDv2-capable routers (FF02::16)
//IGMPv1
//IGMPv2 RFC 2236: Section 9. ALL-ROUTERS (224.0.0.2)
//IGMPv3 IANA: IGMP (224.0.0.22)

    bool rc = true;
    if (m_addr_family == AF_INET) {
        rc = rc && f(*m_sender.get(), m_if_index, addr_storage(IPV4_ALL_IGMP_ROUTERS_ADDR));
        rc = rc && f(*m_sender.get(), m_if_index, addr_storage(IPV4_IGMPV3_ADDR));
    } else if (m_addr_family == AF_INET6) {
        rc = rc && f(*m_sender.get(), m_if_index, addr_storage(IPV6_ALL_NODE_LOCAL_ROUTER));
        rc = rc && f(*m_sender.get(), m_if_index, addr_storage(IPV6_ALL_SITE_LOCAL_ROUTER));
        rc = rc && f(*m_sender.get(), m_if_index, addr_storage(IPV6_ALL_MLDv2_CAPABLE_ROUTERS));
    } else {
        HC_LOG_ERROR("wrong addr_family: " << m_addr_family);
        return false;
    }
    return rc;
}

void querier::receive_record(const std::shared_ptr<proxy_msg>& msg)
{
    HC_LOG_TRACE("");

    if(msg->get_type() != proxy_msg::GROUP_RECORD_MSG){
        HC_LOG_ERROR("wrong proxy message, it musst be be a GROUP_RECORD_MS");
        return; 
    }

    auto gr = std::static_pointer_cast<group_record_msg>(msg);


    auto db_info_it = m_db.group_info.find(gr->get_gaddr());

    if (db_info_it == end(m_db.group_info)) {
        //add an empty neutral record  to membership database
        HC_LOG_DEBUG("gaddr not found");
        db_info_it = m_db.group_info.insert(gaddr_pair(gr->get_gaddr(), gaddr_info())).first;
    }

    switch (db_info_it->second.filter_mode) {
    case  INCLUDE_MODE:
        receive_record_in_include_mode(gr->get_record_type(), gr->get_gaddr(), gr->get_slist(), gr->get_report_version(), db_info_it->second);
        break;
    case EXLCUDE_MODE:
        receive_record_in_exclude_mode(gr->get_record_type(), gr->get_gaddr(), gr->get_slist(), gr->get_report_version(), db_info_it->second);
        break;
    default :
        HC_LOG_ERROR("wrong filter mode: " << db_info_it->second.filter_mode);
        break;
    }

}

void querier::receive_record_in_include_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, int report_version, gaddr_info& ginfo)
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
    case ALLOW_NEW_SOURCES: //ALLOW(x)
        mali(gaddr, B);

        A += B;
        break;


        //INCLUDE (A)     BLOCK (B)      INCLUDE (A)          Send Q(MA,A*B)
    case BLOCK_OLD_SOURCES: //BLOCK(x)

        break;


        //INCLUDE (A)     TO_EX (B)      EXCLUDE (A*B,B-A)    (B-A)=0
        //                                                    Delete (A-B)
        //                                                    Send Q(MA,A*B)
        //                                                    Filter Timer=MALI
    case CHANGE_TO_EXCLUDE_MODE: //TO_EX(x)
        mali(gaddr, filter_timer);

        ginfo.filter_mode = EXLCUDE_MODE;
        ginfo.include_requested_list *= B;
        ginfo.exclude_list = B - A;
        break;


        //INCLUDE (A)     TO_IN (B)      INCLUDE (A+B)        (B)=MALI
        //                                                    Send Q(MA,A-B)
    case CHANGE_TO_INCLUDE_MODE: //TO_IN(x)
        mali(gaddr, B);

        A += B;
        break;


        //INCLUDE (A)       IS_EX (B)     EXCLUDE (A*B, B-A) (B-A)=0
        //                                                    Delete (A-B)
        //                                                    Filter Timer=MALI
    case  MODE_IS_EXCLUDE: //IS_EX(x)
        mali(gaddr, filter_timer);

        ginfo.filter_mode = EXLCUDE_MODE;
        ginfo.include_requested_list *= B;
        ginfo.exclude_list = B - A;
        break;


        //INCLUDE (A)       IS_IN (B)     INCLUDE (A+B)      (B)=MALI
    case MODE_IS_INCLUDE: //IS_IN(x)
        mali(gaddr, B);

        A += B;
        break;


    default:
        HC_LOG_ERROR("unknown multicast record type: " << record_type);
        return;
    }
}

void querier::receive_record_in_exclude_mode(mcast_addr_record_type record_type, const addr_storage& gaddr, source_list<source>& slist, int report_version, gaddr_info& ginfo)
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
    case ALLOW_NEW_SOURCES: //ALLOW(x)
        mali(gaddr, A);

        X += A;
        Y -= A;
        break;


        //EXCLUDE (X,Y)   BLOCK (A)      EXCLUDE (X+(A-Y),Y)  (A-X-Y) =
        //                                                          Filter Timer
        //                                                    Send Q(MA,A-Y)
    case BLOCK_OLD_SOURCES: //BLOCK(x)
        X += (A - Y);
        break;


        //EXCLUDE (X,Y)   TO_EX (A)      EXCLUDE (A-Y,Y*A)    (A-X-Y) =
        //                                                          Filter Timer
        //                                                    Delete (X-A)
        //                                                    Delete (Y-A)
        //                                                    Send Q(MA,A-Y)
        //                                                    Filter Timer=MALI
    case CHANGE_TO_EXCLUDE_MODE: //TO_EX(x)
        mali(gaddr, filter_timer);

        X = (A - Y);
        Y *= A;
        break;


        //EXCLUDE (X,Y)   TO_IN (A)      EXCLUDE (X+A,Y-A)    (A)=MALI
        //                                                    Send Q(MA,X-A)
        //                                                    Send Q(MA)
    case CHANGE_TO_INCLUDE_MODE: //TO_IN(x)
        mali(gaddr, A);

        X += A;
        Y -= A;
        break;


        //EXCLUDE (X,Y)     IS_EX (A)     EXCLUDE (A-Y, Y*A) (A-X-Y)=MALI
        //                                                   Delete (X-A)
        //                                                   Delete (Y-A)
        //                                                   Filter Timer=MALI
    case  MODE_IS_EXCLUDE: //IS_EX(x)
        mali(gaddr, A, A - X - Y);
        mali(gaddr, filter_timer);

        X = (A - Y);
        Y *= A;
        break;


        //EXCLUDE (X,Y)     IS_IN (A)     EXCLUDE (X+A, Y-A) (A)=MALI
    case MODE_IS_INCLUDE: //IS_IN(x)
        mali(gaddr, A);

        X += A;
        Y -= A;
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
    std::shared_ptr<filter_timer> ftimer;

    if (!msg.unique()) {
        switch (msg->get_type()) {
        case proxy_msg::FILTER_TIMER_MSG:
        case proxy_msg::SOURCE_TIMER_MSG: {

            ftimer = std::static_pointer_cast<filter_timer>(msg);

            db_info_it = m_db.group_info.find(ftimer->get_gaddr());

            if (db_info_it == end(m_db.group_info)) {
                HC_LOG_ERROR("filter_timer message is still in use but cannot found");
                return;
            }

        }
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
    case proxy_msg::FILTER_TIMER_MSG: {
        gaddr_info& ginfo = db_info_it->second;

        if (ginfo.shared_filter_timer.get() != ftimer.get() ) {
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

        if (ginfo.filter_mode == EXLCUDE_MODE) {
            if (ginfo.include_requested_list.empty()) {
                m_db.group_info.erase(db_info_it);
            } else {
                ginfo.filter_mode = INCLUDE_MODE;
                ginfo.shared_filter_timer.reset();
                ginfo.exclude_list.clear();
            }
        } else {
            HC_LOG_ERROR("filter_mode is not in expected mode EXCLUDE");
        }

    }
    break;
    case proxy_msg::SOURCE_TIMER_MSG: {
        std::shared_ptr<source_timer> stimer = std::static_pointer_cast<source_timer>(msg);
        gaddr_info& ginfo = db_info_it->second;

        switch (ginfo.filter_mode) {


            //7.2.3.  Definition of Source Timers
            //If the timer of a
            //source from the Include List expires, the source is deleted from the
            //Include List.  If there are no more source records left, the
            //multicast address record is deleted from the router.
        case INCLUDE_MODE:
            for (auto it = std::begin(ginfo.include_requested_list); it != std::end(ginfo.include_requested_list);) {
                if (it->shared_source_timer.get() == stimer.get()) {
                    it = ginfo.include_requested_list.erase(it);
                    continue;
                }
                ++it;
            }

            if (ginfo.include_requested_list.empty()) {
                m_db.group_info.erase(db_info_it);
            }
            break;


            //7.2.3.  Definition of Source Timers
            //If the timer
            //of a source from the Requested List expires, the source is moved to
            //the Exclude List.
        case EXLCUDE_MODE:
            for (auto it = std::begin(ginfo.include_requested_list); it != std::end(ginfo.include_requested_list);) {
                if (it->shared_source_timer.get() == stimer.get()) {
                    it->shared_source_timer.reset();
                    ginfo.exclude_list.insert(*it);

                    it = ginfo.include_requested_list.erase(it);
                    continue;
                }
                ++it;
            }

            break;
        default:
            HC_LOG_ERROR("unknown filter mode");
        }

    }
    break;
    default:
        HC_LOG_ERROR("unknown timer message format");
    }
}

void querier::mali(const addr_storage& gaddr, gaddr_info& db_info) const
{
    HC_LOG_TRACE("");
    auto ft = std::make_shared<filter_timer>(m_if_index, gaddr, m_timers_values.get_multicast_address_listening_interval());

    db_info.shared_filter_timer = ft;

    m_timing->add_time(m_timers_values.get_multicast_address_listening_interval(), m_proxy_instance, ft);
}

void querier::mali(const addr_storage& gaddr, source_list<source>& slist) const
{
    HC_LOG_TRACE("");
    std::shared_ptr<source_timer> st = std::make_shared<source_timer>(m_if_index, gaddr, m_timers_values.get_multicast_address_listening_interval());

    for (auto & e : slist) {
        e.shared_source_timer = st; //shard_source_timer is mutable
    }

    m_timing->add_time(m_timers_values.get_multicast_address_listening_interval(), m_proxy_instance, st);
}

void querier::mali(const addr_storage& gaddr, source_list<source>& slist, source_list<source>&&   tmp_slist) const
{
    HC_LOG_TRACE("");
    mali(gaddr, tmp_slist);

    for (auto & e : tmp_slist) {
        slist.find(e)->shared_source_timer = e.shared_source_timer;
    }
}

querier::~querier()
{
    HC_LOG_TRACE("");

    router_groups_function(&sender::send_leave);
}

timers_values& querier::get_timers_values()
{
    HC_LOG_TRACE("");
    return m_timers_values;
}

std::string querier::to_string() const
{
    std::ostringstream s;

    char cstr[IF_NAMESIZE];
    std::string if_name(if_indextoname(m_if_index, cstr));

    s << "##-- interface: " << if_name << " (index: " << m_if_index << ") --##" << std::endl;
    s << m_db;
    return s.str();
}

std::ostream& operator<<(std::ostream& stream, const querier& q)
{
    HC_LOG_TRACE("");
    return stream << q.to_string();

}
