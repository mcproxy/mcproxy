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

#ifndef EXTENDED_IGMP_DEFINES_HPP
#define EXTENDED_IGMP_DEFINES_HPP

#include <stdint.h>
#include <sys/types.h>
#include <netinet/igmp.h>

#ifndef IGMP_V3_MEMBERSHIP_REPORT
#define IGMP_V3_MEMBERSHIP_REPORT 0x22
#endif

//struct igmp {
//u_int8_t igmp_type;             [> IGMP type <]
//u_int8_t igmp_code;             [> routing code <]
//u_int16_t igmp_cksum;           [> checksum <]
//struct in_addr igmp_group;      [> group address <]
//};

//RFC 2113
struct router_alert_option {
    uint8_t type; //Ccopied flag: 1, Option class: 0, Option number: 20
    uint8_t length;
    uint16_t value;
    router_alert_option()
        : type(0x94)
        , length(0x4)
        , value(0) {};
};

struct igmpv3_query: igmp {
#if BYTE_ORDER == LITTLE_ENDIAN //is this right?????????????????ßß 
    uint8_t qrv : 3, suppress : 1, resv2 : 4;
#elif BYTE_ORDER == BIG_ENDIAN //is this right??????????????????????
    uint8_t resv2 : 4, suppress : 1, qrv : 3;
#else
#   error "unkown byte order"
#endif
    uint8_t qqic;
    uint16_t num_of_srcs; //ntohs
} __attribute__ ((packed));

struct igmpv3_mc_record {
    uint8_t type;
    uint8_t aux_data_len;
    uint16_t num_of_srcs; //ntohs
    in_addr gaddr;
} __attribute__ ((packed));

struct igmpv3_mc_report {
    uint8_t type;
    uint8_t reservedA;
    uint16_t checksum;
    uint16_t reservedB;
    uint16_t num_of_mc_records; //ntohs
} __attribute__ ((packed));

#endif
