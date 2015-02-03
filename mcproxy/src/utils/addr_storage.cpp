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

#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <iostream>

void addr_storage::clean()
{
    reinterpret_cast<sockaddr_in6*>(&m_addr)->sin6_family = AF_UNSPEC;
    reinterpret_cast<sockaddr_in6*>(&m_addr)->sin6_port = 0;
    reinterpret_cast<sockaddr_in6*>(&m_addr)->sin6_flowinfo = 0;
    reinterpret_cast<sockaddr_in6*>(&m_addr)->sin6_scope_id = 0;
}

socklen_t addr_storage::get_addr_len(int addr_family) const
{
    switch (addr_family) {
    case AF_INET:
        return sizeof(sockaddr_in);
    case AF_INET6:
        return sizeof(sockaddr_in6);
    default:
        HC_LOG_ERROR("Unknown address family");
        return 0;
    }
}

void addr_storage::set_addr_family(int addr_family)
{
    m_addr.ss_family = addr_family;
}

in_addr& addr_storage::get_in_addr_mutable()
{
    return reinterpret_cast<sockaddr_in*>(&m_addr)->sin_addr;
}

in6_addr& addr_storage::get_in6_addr_mutable()
{
    return reinterpret_cast<sockaddr_in6*>(&m_addr)->sin6_addr;
}

addr_storage::addr_storage()
{
    clean();
}

addr_storage::addr_storage(int addr_family)
{
    memset(&m_addr, 0, sizeof(m_addr));
    set_addr_family(addr_family);
}

addr_storage::addr_storage(const std::string& addr)
{
    *this = addr;
}

addr_storage::addr_storage(const sockaddr_storage& addr)
{
    *this = addr;
}

addr_storage::addr_storage(const in_addr& addr)
{
    *this = addr;
}

addr_storage::addr_storage(const in6_addr& addr)
{
    *this = addr;
}

addr_storage::addr_storage(const sockaddr& addr)
{
    *this = addr;
}

addr_storage::addr_storage(const sockaddr_in6& addr)
{
    *this = addr;
}

addr_storage::addr_storage(const sockaddr_in& addr)
{
    *this = addr;
}

std::ostream& operator <<(std::ostream& s, const addr_storage a)
{
    s << a.to_string();
    return s;
}

addr_storage& addr_storage::operator=(const sockaddr_storage& s)
{
    this->m_addr = s;
    return *this;
}

addr_storage& addr_storage::operator=(const std::string& s)
{
    clean();

    if (s.find_first_of(':') == std::string::npos) { //==> IPv4
        set_addr_family(AF_INET);
        if (inet_pton(get_addr_family(), s.c_str(), reinterpret_cast<void*>(&get_in_addr_mutable())) < 1) {
            HC_LOG_ERROR("failed to convert string to sockaddr_storage:" << s);
            set_invalid();
        }
    } else { //==> IPv6
        set_addr_family(AF_INET6);
        if (inet_pton(get_addr_family(), s.c_str(), reinterpret_cast<void*>(&get_in6_addr_mutable())) < 1) {
            HC_LOG_ERROR("failed to convert string to sockaddr_storage:" << s);
            set_invalid();
        }
    }

    return *this;
}

addr_storage& addr_storage::operator=(const in_addr& s)
{
    clean();

    set_addr_family(AF_INET);
    get_in_addr_mutable() = s;
    return *this;
}

addr_storage& addr_storage::operator=(const in6_addr& s)
{
    clean();

    set_addr_family(AF_INET6);
    get_in6_addr_mutable() =  s;
    return *this;
}

addr_storage& addr_storage::operator=(const sockaddr& s)
{
    memcpy(&m_addr, &s, get_addr_len(s.sa_family));
    return *this;
}

addr_storage& addr_storage::operator=(const sockaddr_in& s)
{
    *this = *reinterpret_cast<const sockaddr*>(&s);
    return *this;
}

addr_storage& addr_storage::operator=(const sockaddr_in6& s)
{
    *this = *reinterpret_cast<const sockaddr_in*>(&s);
    return *this;
}

bool addr_storage::operator==(const addr_storage& addr) const
{
    if (this->get_addr_family() == AF_INET && addr.get_addr_family() == AF_INET) {
        return this->get_in_addr().s_addr  == addr.get_in_addr().s_addr;
    } else if (this->get_addr_family() == AF_INET6 && addr.get_addr_family() == AF_INET6) {
        return IN6_ARE_ADDR_EQUAL(&this->get_in6_addr(), &addr.get_in6_addr());
    } else {
        return false;
    }
}

bool addr_storage::operator!=(const addr_storage& addr) const
{
    return !(*this == addr);
}

bool operator<(const addr_storage& addr1, const addr_storage& addr2)
{
    if (addr1.get_addr_family() == AF_INET && addr2.get_addr_family() == AF_INET) {
        return  ntohl(addr1.get_in_addr().s_addr) < ntohl(addr2.get_in_addr().s_addr);
    } else if (addr1.get_addr_family() == AF_INET6 && addr2.get_addr_family() == AF_INET6) {
        const uint8_t* a1 = addr1.get_in6_addr().s6_addr;
        const uint8_t* a2 = addr2.get_in6_addr().s6_addr;

        for (unsigned int i = 0; i < sizeof(in6_addr) / sizeof(uint8_t); ++i) {
            if (a1[i] > a2[i]) {
                return false;
            } else if (a1[i] < a2[i]) {
                return true;
            }
        }
        return false;
    } else {
        HC_LOG_ERROR("incompatible ip versions");
        return false;
    }
}

bool operator>(const addr_storage& addr1, const addr_storage& addr2)
{
    return !(addr1 < addr2) &&  !(addr1 == addr2);
}

bool operator<=(const addr_storage& addr1, const addr_storage& addr2)
{
    return (addr1 < addr2) || (addr1 == addr2);
}

bool operator>=(const addr_storage& addr1, const addr_storage& addr2)
{
    return !(addr1 < addr2);
}

addr_storage& addr_storage::operator++() //prefix ++
{
    if (get_addr_family() == AF_INET) {
        get_in_addr_mutable().s_addr = htonl(ntohl(get_in_addr().s_addr) + 1);
    } else if (get_addr_family() == AF_INET6) {
        for (int i = 3; i >= 0; --i) {
            uint32_t& tmp = get_in6_addr_mutable().s6_addr32[i];
            tmp = htonl(ntohl(tmp) + 1);
            if (tmp != 0) {
                break;
            }
        }
    } else {
        HC_LOG_ERROR("unknown ip version");
    }
    return *this;
}

addr_storage addr_storage::operator++(int) //postfix
{
    return ++(*this);
}

addr_storage& addr_storage::operator--() //prefix --
{
    if (get_addr_family() == AF_INET) {
        get_in_addr_mutable().s_addr = htonl(ntohl(get_in_addr().s_addr) - 1);
    } else if (get_addr_family() == AF_INET6) {
        for (int i = 3; i >= 0; --i) {
            uint32_t& tmp = get_in6_addr_mutable().s6_addr32[i];
            tmp = htonl(ntohl(tmp) - 1);
            if (tmp != static_cast<uint32_t>(-1)) {
                break;
            }
        }
    } else {
        HC_LOG_ERROR("unknown ip version");
    }
    return *this;
}

addr_storage addr_storage::operator--(int) //postfix --
{
    return ++(*this);
}

int addr_storage::get_addr_family() const
{
    return this->m_addr.ss_family;
}

in_port_t addr_storage::get_port() const
{
    return ntohs(reinterpret_cast<const sockaddr_in*>(&m_addr)->sin_port);
}

addr_storage& addr_storage::set_port(uint16_t port)
{
    reinterpret_cast<sockaddr_in*>(&m_addr)->sin_port = htons(port);
    return *this;
}

addr_storage& addr_storage::set_port(const std::string& port)
{
    set_port(std::stoi(port.c_str()));
    return *this;
}

socklen_t addr_storage::get_addr_len() const
{
    return get_addr_len(get_addr_family());
}

const sockaddr_storage& addr_storage::get_sockaddr_storage() const
{
    return m_addr;
}

const in_addr& addr_storage::get_in_addr() const
{
    return reinterpret_cast<const sockaddr_in*>(&m_addr)->sin_addr;
}

const in6_addr& addr_storage::get_in6_addr() const
{
    return reinterpret_cast<const sockaddr_in6*>(&m_addr)->sin6_addr;
}

const sockaddr& addr_storage::get_sockaddr() const
{
    return *reinterpret_cast<const sockaddr*>(&m_addr);
}

const sockaddr_in& addr_storage::get_sockaddr_in() const
{
    return *reinterpret_cast<const sockaddr_in*>(&m_addr);
}

const sockaddr_in6& addr_storage::get_sockaddr_in6() const
{
    return *reinterpret_cast<const sockaddr_in6*>(&m_addr);
}

std::string addr_storage::to_string() const
{
    if (get_addr_family() == AF_INET) {
        char addressBuffer[INET_ADDRSTRLEN];

        if (inet_ntop(get_addr_family(), reinterpret_cast<const void*>(&get_in_addr()), addressBuffer, sizeof(addressBuffer)) == nullptr) {
            HC_LOG_ERROR("failed to convert sockaddr_storage");
            return std::string();
        } else {
            return std::string(addressBuffer);
        }
    } else if (get_addr_family() == AF_INET6) {
        char addressBuffer[INET6_ADDRSTRLEN];
        if (inet_ntop(get_addr_family(), reinterpret_cast<const void*>(&get_in6_addr()), addressBuffer, sizeof(addressBuffer)) == nullptr) {
            HC_LOG_ERROR("failed to convert sockaddr_storage");
            return std::string();
        } else {
            return std::string(addressBuffer);
        }
    } else {
        HC_LOG_ERROR("wrong address family");
        return std::string();
    }
}

addr_storage& addr_storage::mask_ipv4(const addr_storage& subnet_mask)
{
    if (this->get_addr_family() == AF_INET && subnet_mask.get_addr_family() == AF_INET) {
        get_in_addr_mutable().s_addr &= subnet_mask.get_in_addr().s_addr;
        return *this;
    } else {
        HC_LOG_ERROR("incompatible ip versions");
    }

    return *this;
}

addr_storage& addr_storage::mask(unsigned int suffix)
{
    if (get_addr_family() == AF_INET) {
        get_in_addr_mutable().s_addr &= htonl(~(static_cast<unsigned int>(-1) >> suffix));
    } else if (get_addr_family() == AF_INET6) {
        for (int i = 3; i >= 0; --i) {
            uint32_t& tmp_addr = get_in6_addr_mutable().s6_addr32[i];
            int tmp_suffix = suffix - (i * 32 /*Bit*/) ;
            if (tmp_suffix <= 0) {
                tmp_addr = 0;
            } else if (tmp_suffix < 32) {
                tmp_addr &= htonl(~(static_cast<unsigned int>(-1) >> suffix));
            } else if (tmp_suffix >= 32) {
                break;
            }
        }
    } else {
        HC_LOG_ERROR("wrong address family");
    }
    return *this;
}

addr_storage& addr_storage::broadcast_addr(unsigned int suffix)
{
    if (get_addr_family() == AF_INET) {
        get_in_addr_mutable().s_addr |= htonl(static_cast<unsigned int>(-1) >> suffix);
    } else if (get_addr_family() == AF_INET6) {
        for (int i = 3; i >= 0; --i) {
            uint32_t& tmp_addr = get_in6_addr_mutable().s6_addr32[i];
            int tmp_suffix = suffix - (i * 32 /*Bit*/) ;
            if (tmp_suffix <= 0) {
                tmp_addr = static_cast<unsigned int>(-1);
            } else if (tmp_suffix < 32) {
                tmp_addr |= htonl(static_cast<unsigned int>(-1) >> suffix);
            } else if (tmp_suffix >= 32) {
                break;
            }
        }
    } else {
        HC_LOG_ERROR("wrong address family");
    }
    return *this;
}

bool addr_storage::is_multicast_addr() const
{

    if (get_addr_family() == AF_INET) {
        return IN_MULTICAST(ntohl(get_in_addr().s_addr));
    } else if (get_addr_family() == AF_INET6) {
        return IN6_IS_ADDR_MULTICAST(&get_in6_addr());
    } else {
        HC_LOG_ERROR("wrong address family");
        return false;
    }

}

bool addr_storage::is_valid() const
{
    return get_addr_family() != AF_UNSPEC;
}

void addr_storage::set_invalid()
{
    clean();
}

