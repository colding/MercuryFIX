/*
 *    Copyright (C) 2013, Jules Colding <jcolding@gmail.com>.
 *
 *    All Rights Reserved.
 */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 *     (1) Redistributions of source code must retain the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer.
 * 
 *     (2) Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *     
 *     (3) Neither the name of the copyright holder nor the names of
 *     its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written
 *     permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#ifdef __linux__
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <string.h>
    #include <stdlib.h>
#endif
#include "stdlib/log/log.h"
#include "network.h"
#include "net_interfaces.h"

char*
get_ip_from_ifname(const int inet_family,
                   const char * const ifname)
{
        char *retv = NULL;
        struct ifaddrs *myaddrs;
        struct ifaddrs *ifa;
        struct sockaddr_in *s4;
        struct sockaddr_in6 *s6;
        char buf[64] = { '\0' };

        if (getifaddrs(&myaddrs))
                return NULL;

        switch (inet_family) {
        case AF_INET :
        case AF_INET6 :
                break;
        default :
                return NULL;
        }

        for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {
                if (NULL == ifa->ifa_addr)
                        continue;
                if (0 == (ifa->ifa_flags & IFF_UP))
                        continue;
                if (strcmp(ifname, ifa->ifa_name))
                        continue;

                if (AF_INET == inet_family) {
                        if (ifa->ifa_addr->sa_family == AF_INET) {
                                s4 = (struct sockaddr_in *)(ifa->ifa_addr);
                                if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(s4->sin_addr), buf, sizeof(buf)) == NULL) {
                                        break;
                                } else {
                                        retv = strdup(buf);
                                        break;
                                }
                        }
                }

                if (AF_INET6 == inet_family) {
                        if (ifa->ifa_addr->sa_family == AF_INET6) {
                                s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
                                if (inet_ntop(ifa->ifa_addr->sa_family, (void *)&(s6->sin6_addr), buf, sizeof(buf)) == NULL) {
                                        break;
                                } else {
                                        retv = strdup(buf);
                                        break;
                                }
                        }
                }
        }
        freeifaddrs(myaddrs);

        // paranoia check...
        if (retv && ('\0' == retv[0])) {
                free(retv);
                retv = NULL;
        }

        return retv;
}

/*
 * Sets server socket options. Returns 1 (one) on success, 0 (zero)
 * otherwise.
 */
static int
set_socket_options(const int sock)
{
        int flag = 1;
        struct linger sl;

        sl.l_onoff = 1;
        sl.l_linger = 2;

#ifdef __APPLE__
        if (setsockopt(sock, SOL_SOCKET, SO_LINGER_SEC, &sl, sizeof(sl))) {
                M_CRITICAL("could not setsockopt(SO_LINGER): %s", strerror(errno));
                return 0;
        }
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag))) {
                M_CRITICAL("could not setsockopt(SO_REUSEPORT): %s", strerror(errno));
                return 0;
        }
#elif defined __linux__
        if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl))) {
                M_CRITICAL("could not setsockopt(SO_LINGER): %s", strerror(errno));
                return 0;
        }
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag))) {
                M_CRITICAL("could not setsockopt(TCP_NODELAY): %s", strerror(errno));
                return 0;
        }
#endif
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {
                M_CRITICAL("could not setsockopt(SO_REUSEADDR): %s", strerror(errno));
                return 0;
        }

        return 1;
}

/*
 * Sets command socket options for low traffic IPC sockets. Returns 1
 * (one) on success, 0 (zero) otherwise.
 */
static int
set_socket_options_low_volume(const int sock)
{
        const int flag = 1;

        if (!set_socket_options(sock))
                return 0;

        if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag))) { // SIG_PIPE!!
                M_CRITICAL("could not setsockopt(SO_KEEPALIVE): %s", strerror(errno));
                return 0;
        }

        return 1;
}

int
create_listening_socket(const char * const interface,
			const uint16_t port,
			const int pf_family,
			const int socket_type,
                        const bool keep_alive)
{
        int tmp;
        int sock;
        char *ip = NULL;
        struct sockaddr_in s4;
        struct sockaddr_in6 s6;
        struct sockaddr *listen_addr;
        uint16_t nport;
        size_t addr_size;

        sock = -1;

        if (SOCK_STREAM != socket_type)
                return sock;

        nport = htons(port);
        memset(&s4, 0, sizeof(struct sockaddr_in));
        memset(&s6, 0, sizeof(struct sockaddr_in6));

        switch (pf_family) {
        case PF_INET:
                s4.sin_port = nport;
                s4.sin_family = AF_INET;
                listen_addr = (struct sockaddr*)&s4;
                addr_size = sizeof(struct sockaddr_in);
                break;
        case PF_INET6:
                s6.sin6_port = nport;
                s6.sin6_family = AF_INET6;
                listen_addr = (struct sockaddr*)&s6;
                addr_size = sizeof(struct sockaddr_in6);
                break;
        default:
                return sock;
        }

        sock = socket(pf_family, socket_type, 0);
        if (-1 == sock) {
                M_ERROR("could not create socket");
                return sock;
        }

        if (keep_alive) {
                if (!set_socket_options_low_volume(sock)) {
                        M_ERROR("could not set server socket options");
                        goto err;
                }
        } else {
                if (!set_socket_options(sock)) {
                        M_ERROR("could not set command socket options");
                        goto err;
                }
        }

        if (!strcmp("localhost", interface)) {
                if (PF_INET == pf_family) {
                        if (1 != inet_pton(AF_INET, "127.0.0.1", &s4.sin_addr)) {
                                M_ERROR("%s", strerror(errno));
                                goto err;
                        }
                } else {
                        if (1 != inet_pton(AF_INET6, "::1", &s6.sin6_addr)) {
                                M_ERROR("%s", strerror(errno));
                                goto err;
                        }
                }
        } else {
                if (PF_INET == pf_family) {
                        tmp = inet_pton(AF_INET, interface, &s4.sin_addr);
                        if (!tmp) {
                                ip = get_ip_from_ifname(AF_INET, interface);
                                if (!ip) {
                                        M_ERROR("could not deduce IP address for %s", interface);
                                        goto err;
                                }
                                tmp = inet_pton(AF_INET, ip, &s4.sin_addr);
                                free(ip);
                        }
                } else {
                        tmp = inet_pton(AF_INET6, interface, &s6.sin6_addr);
                        if (!tmp) {
                                ip = get_ip_from_ifname(AF_INET6, interface);
                                if (!ip) {
                                        M_ERROR("could not deduce IP address for %s", interface);
                                        goto err;
                                }
                                tmp = inet_pton(AF_INET6, ip, &s6.sin6_addr);
                                free(ip);
                        }
                }
                switch (tmp) {
                case 1:
                        break;
                case 0:
                        M_CRITICAL("invalid interface: %s", interface);
                        goto err;
                case -1:
                default:
                        M_CRITICAL("invalid interface: %s (%s)", interface, strerror(errno));
                        goto err;
                }
        }
        if (bind(sock, listen_addr, addr_size)) {
                M_ERROR("could not create endpoint %s:%d (%s)", interface, port, strerror(errno));
                goto err;
        }
        if (listen(sock, SOMAXCONN)) {
                M_ERROR("could not listen on socket (%s:%d): %s", interface, port, strerror(errno));
                goto err;
        }
        goto out;

err:
        if (-1 != sock) {
                close(sock);
                sock = -1;
        }

out:
        return sock;
}

int
connect_to_listening_socket(const char * const interface,
			    const uint16_t port,
			    const int pf_family,
			    const int socket_type,
                            const timeout_t timeout)
{
        int res;
        int sock;
        struct addrinfo hint;
        struct addrinfo *ai;
        struct addrinfo *ai_current;
        char pstr[32] = { '\0' };
        bool close_socket = true;

        sock = -1;

        if (SOCK_STREAM != socket_type)
                return sock;

        memset(&hint, 0, sizeof(struct addrinfo));
        hint.ai_flags = AI_NUMERICSERV;
        hint.ai_family = pf_family;
        hint.ai_socktype = socket_type;
        hint.ai_protocol = IPPROTO_TCP;

        M_DEBUG("connecting to %s:%u", interface, port);
        sprintf(pstr, "%u", port);
        res = getaddrinfo(interface, pstr, &hint, &ai);
        if (res) {
                M_ERROR("could not get address info: %s", gai_strerror(res));
                return sock;
        }

        for (ai_current = ai; ai_current; ai_current = ai_current->ai_next) {
		if (-1 == sock) {
			close(sock);
                        sock = -1;
		}
                sock = socket(ai_current->ai_family, ai_current->ai_socktype, ai_current->ai_protocol);
                if (-1 == sock)
                        continue;

                if (!set_send_timeout(sock, timeout)) {
                        M_ERROR("could not set timeout");
                        continue;
                }
                do {
                        if (!connect(sock, ai_current->ai_addr, ai_current->ai_addrlen)) {
				close_socket = false;
                                goto done;
			}

                        switch (errno) {
			case EAGAIN:
                        case ETIMEDOUT:
				M_WARNING("could not connect, timed out");
				goto done;
                        default:
                                M_ERROR("could not connect to socket: %s", strerror(errno));
                        }
                        break;
                } while (true);
        }
done:
        freeaddrinfo(ai);
	if (close_socket) {
		close(sock);
		sock = -1;
	}
        return sock;

}
