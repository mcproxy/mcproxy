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


#ifndef MC_TIMERS_VALUES_HPP
#define MC_TIMERS_VALUES_HPP

//-- igmp and mld --
#define MC_TV_ROBUSTNESS_VARIABLE                 2//2
#define MC_TV_QUERY_INTERVAL                      30//125 //sec
#define MC_TV_QUERY_RESPONSE_INTERVAL             1//10 //sec
#define MC_TV_OTHER_QUERIER_PRESENT_INTERVAL      (MC_TV_ROBUSTNESS_VARIABLE*MC_TV_QUERY_RESPONSE_INTERVAL) //sec
#define MC_TV_STARTUP_QUERY_INTERVAL              (MC_TV_QUERY_INTERVAL/4)
#define MC_TV_STARTUP_QUERY_COUNT                 MC_TV_ROBUSTNESS_VARIABLE
#define MC_TV_UNSOLICITED_REPORT_INTERVAL         10 //sec

//-- igmp --
#define MC_TV_GROUP_MEMBERSHIP_INTERVAL           (MC_TV_ROBUSTNESS_VARIABLE*MC_TV_QUERY_INTERVAL + MC_TV_QUERY_RESPONSE_INTERVAL) //sec
#define MC_TV_LAST_MEMBER_QUERY_INTEVAL           1 //sec
#define MC_TV_LAST_MEMBER_QUERY_COUNT             MC_TV_ROBUSTNESS_VARIABLE
#define MC_TV_VERION_1_ROUTER_PRESNET_TIMEOUT     400 //sec
#define MC_TV_MAX_RESPONSE_TIME_UNIT              10 //1/10 sec

//-- mld --
#define MC_TV_MC_LISTENER_INTERVAL                MC_TV_GROUP_MEMBERSHIP_INTERVAL
#define MC_TV_LAST_LISTENER_QUERY_INTERVAL        MC_TV_LAST_MEMBER_QUERY_INTEVAL
#define MC_TV_LAST_LISTENER_QUERY_COUNT           MC_TV_LAST_MEMBER_QUERY_COUNT
#define MC_TV_MAX_RESPONSE_DELAY_UNIT             1000 //1/1000 sec ==> 1 msec

#endif // MC_TIMERS_VALUES_HPP
