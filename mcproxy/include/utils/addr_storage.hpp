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

#ifndef ADDR_STORAGE_HPP
#define ADDR_STORAGE_HPP

#include <sys/socket.h>
#include <netinet/in.h>

#include <string>

/**
 * @brief Wrapper for IPv4 and IPv6 addresses.
 */
class addr_storage
{
private:
    sockaddr_storage m_addr;

    inline void clean();
    inline socklen_t get_addr_len(int addr_family) const;
    inline void set_addr_family(int addr_family);
    inline in_addr& get_in_addr_mutable();
    inline in6_addr& get_in6_addr_mutable();
public:
    addr_storage(addr_storage&&) = default;
    addr_storage& operator=(addr_storage &&) = default;

    addr_storage(const addr_storage&) = default;
    addr_storage& operator=(const addr_storage&) = default;

    /**
     * @brief Create an empty and invalid addr_storage.
     */
    addr_storage();

    /**
     * @brief Create a zero address specific storage.
     */
    explicit addr_storage(int addr_family);

    /**
     * @brief Create an addr_storage based on a clear text ip.
     */
    explicit addr_storage(const std::string& addr);

    /**
     * @brief Create an addr_storage based on the sockaddr_storage.
     */
    explicit addr_storage(const sockaddr_storage& addr);

    /**
     * @brief Create an addr_storage based on in_add.
     */
    explicit addr_storage(const in_addr& addr);

    /**
     * @brief Create an addr_storage based on in6_addr.
     */
    explicit addr_storage(const in6_addr& addr);

    /**
     * @brief Create an addr_storage based on sockaddr.
     */
    explicit addr_storage(const sockaddr& addr);

    /**
     * @brief Create an addr_storage based on sockaddr_in.
     */
    explicit addr_storage(const sockaddr_in& addr);

    /**
     * @brief Create an addr_storage based on sockaddr_in6.
     */
    explicit addr_storage(const sockaddr_in6& addr);

//-----------------------------------------------------------

    /**
     * @brief copy operator sockaddr_storage to class addr_storage
     */
    addr_storage& operator=(const sockaddr_storage& s);

    /**
     * @brief copy operator string to class addr_storage
     */
    addr_storage& operator=(const std::string& s);

    /**
     * @brief copy operator in_addr to class addr_storage
     */
    addr_storage& operator=(const in_addr& s);

    /**
     * @brief copy operator in6_addr to class addr_storage
     */
    addr_storage& operator=(const in6_addr& s);

    /**
     * @brief copy operator sockaddr to class addr_storage
     */
    addr_storage& operator=(const sockaddr& s);

    /**
     * @brief copy operator sockaddr_in to class addr_storage
     */
    addr_storage& operator=(const sockaddr_in& s);

    /**
     * @brief copy operator sockaddr_in6 to class addr_storage
     */
    addr_storage& operator=(const sockaddr_in6& s);

    //-----------------------------------------------------------

    /**
     * @brief compare two addresses if one of this addresses unknown the function returns false
     * basic implementation
     */
    bool operator==(const addr_storage& addr) const;

    /**
     * @brief lower then operator
     * basic implementation
     */
    friend bool operator<(const addr_storage& addr1, const addr_storage& addr2);

    /**
     * @brief disjunction to operator==
     */
    bool operator!=(const addr_storage& addr) const;

    friend bool operator>(const addr_storage& addr1, const addr_storage& addr2);

    friend bool operator<=(const addr_storage& addr1, const addr_storage& addr2);

    friend bool operator>=(const addr_storage& addr1, const addr_storage& addr2);

    addr_storage& operator++(); //prefix ++
    addr_storage operator++(int); //postfix ++ (has to do a copy of addr_storage)

    addr_storage& operator--(); //prefix --
    addr_storage operator--(int); //postfix -- (has to do a copy of addr_storage)

    /**
     * @brief mask an addr with a netmask
     */
    addr_storage& mask_ipv4(const addr_storage& subnet_mask);
    addr_storage& mask(unsigned int suffix);
    addr_storage& broadcast_addr(unsigned int suffix);

    //-----------------------------------------------------------

    /**
     * @brief return current address family AF_INET or AF_INET6 or AF_UNSPEC
     */
    int get_addr_family() const;

    /**
     * @brief return current port in host bye order or return 0 if no port exist
     */
    in_port_t get_port() const;

    /**
     * @brief set a port in host byte order
     */
    addr_storage& set_port(uint16_t port);

    /**
     * @brief return current port or return 0 if no port exist
     */
    addr_storage& set_port(const std::string& port);

    /**
     * @brief return length of the address in byte or return 0 if no address family is available
     */
    socklen_t get_addr_len() const;

    /**
     * @brief return a sockaddr_storage struct
     */
    const sockaddr_storage& get_sockaddr_storage() const;

    /**
     * @brief return a in_addr struct
     */
    const in_addr& get_in_addr() const;

    /**
     * @brief return a in6_addr struct
     */
    const in6_addr& get_in6_addr() const;

    /**
     * @brief return a sockaddr struct
     */
    const sockaddr& get_sockaddr() const;

    /**
     * @brief return a sockaddr_in struct
     */
    const sockaddr_in& get_sockaddr_in() const;

    /**
     * @brief return a sockaddr_in6 struct
     */
    const sockaddr_in6& get_sockaddr_in6() const;

    //-----------------------------------------------------------

    /**
     * @brief return current address as string
     */
    std::string to_string() const;

    /**
     * @brief cout output operator
     */
    friend std::ostream& operator <<(std::ostream& s, const addr_storage a);

    //-----------------------------------------------------------

    bool is_multicast_addr() const;

    bool is_valid() const;

    void set_invalid();

    //-----------------------------------------------------------

    /**
     * @brief simple test output
     */
    static void test_addr_storage_a();
    static void test_addr_storage_b();
};

#endif // ADDR_STORAGE_HPP
