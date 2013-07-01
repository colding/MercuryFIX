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
#ifdef  __linux__
    #include <stdarg.h>
    #include <string.h>
#endif
#include <errno.h>
#include "network.h"
#include "stdlib/log/log.h"

int
send_all(int sock,
         const uint8_t * const buf,
         const int len)
{
        int total = 0;
        int bytesleft = len;
        int n;

        while (total < len) {
                n = send(sock, buf + total, bytesleft, 0);
                if (-1 == n) {
                        M_ERROR("error sending all bytes: %s", strerror(errno));
                        return 0;
                }
                total += n;
                bytesleft -= n;
        }

        return 1;
}

int
set_recv_timeout(int sock,
                 const timeout_t time_out)
{
        struct timeval t;

        t.tv_sec = time_out.seconds;
        t.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(struct timeval))) {
                M_ERROR("could not setsockopt(SO_RECVTIMEO): %s", strerror(errno));
                return 0;
        }

        return 1;
}

int
set_send_timeout(int sock,
                 const timeout_t time_out)
{
        struct timeval t;

        t.tv_sec = time_out.seconds;
        t.tv_usec = 0;

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(struct timeval))) {
                M_ERROR("could not setsockopt(SO_SNDTIMEO): %s", strerror(errno));
                return 0;
        }

        return 1;
}

int
set_min_recv_sice(int sock,
                  const int ipc_header_size)
{
        if (setsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, &ipc_header_size, sizeof(ipc_header_size))) {
                M_ERROR("could not setsockopt(SO_RCVLOWAT): %s", strerror(errno));
                return 0;
        }

        return 1;
}

int
send_fd(int fd,
        void *data,
        const uint32_t len,
        int fd_to_send,
        uint32_t * const bytes_sent)
{
        ssize_t n;
        struct msghdr msg;
        struct iovec iov;

        memset(&msg, 0, sizeof(struct msghdr));
        memset(&iov, 0, sizeof(struct iovec));

#ifdef HAVE_MSGHDR_MSG_CONTROL
        union {
                struct cmsghdr cm;
                char control[CMSG_SPACE_SIZEOF_INT];
        } control_un;
        struct cmsghdr *cmptr;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);
        memset(msg.msg_control, 0, sizeof(control_un.control));

        cmptr = CMSG_FIRSTHDR(&msg);
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;
        *((int *) CMSG_DATA(cmptr)) = fd_to_send;
#else
        msg.msg_accrights = (caddr_t) &fd_to_send;
        msg.msg_accrightslen = sizeof(int);
#endif
        msg.msg_name = NULL;
        msg.msg_namelen = 0;

        iov.iov_base = data;
        iov.iov_len = len;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

#ifdef __linux__
        msg.msg_flags = MSG_EOR;
        n = sendmsg(fd, &msg, MSG_EOR);
#elif defined __APPLE__
        n = sendmsg(fd, &msg, 0); /* MSG_EOR is not supported on Mac
                                   * OS X due to lack of
                                   * SOCK_SEQPACKET support on
                                   * socketpair() */
#endif
        switch (n) {
        case EMSGSIZE:
                M_WARNING("message too big");
                return EMSGSIZE;
        case -1:
                M_WARNING("sendmsg error: %s", strerror(errno));
                return 1;
        default:
                *bytes_sent = n;
        }

        return 0;
}

int
recv_fd(int fd,
        void *buf,
        const uint32_t len,
        int *recvfd,
        uint32_t * const bytes_recv)
{
        struct msghdr msg;
        struct iovec iov;
        ssize_t n = 0;
#ifndef HAVE_MSGHDR_MSG_CONTROL
        int newfd;
#endif
        memset(&msg, 0, sizeof(struct msghdr));
        memset(&iov, 0, sizeof(struct iovec));

#ifdef HAVE_MSGHDR_MSG_CONTROL
        union {
                struct cmsghdr  cm;
                char control[CMSG_SPACE_SIZEOF_INT];
        } control_un;
        struct cmsghdr *cmptr;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);
        memset(msg.msg_control, 0, sizeof(control_un.control));
#else
        msg.msg_accrights = (caddr_t) &newfd;
        msg.msg_accrightslen = sizeof(int);
#endif
        msg.msg_name = NULL;
        msg.msg_namelen = 0;

        iov.iov_base = buf;
        iov.iov_len = len;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        if (recvfd)
                *recvfd = -1;

        n = recvmsg(fd, &msg, 0);
        if (msg.msg_flags) {
                M_WARNING("recvmsg() had something to say, msg_flags = 0x%08x", msg.msg_flags);
                return 1;
        }
        if (bytes_recv)
                *bytes_recv = n;
        switch (n) {
        case 0:
                *bytes_recv = 0;
                return 0;
        case -1:
                return 1;
        default:
                break;
        }

#ifdef HAVE_MSGHDR_MSG_CONTROL
        if ((NULL != (cmptr = CMSG_FIRSTHDR(&msg))) &&
            cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
                if (SOL_SOCKET != cmptr->cmsg_level) {
                        return 0;
                }
                if (SCM_RIGHTS != cmptr->cmsg_type) {
                        return 0;
                }
                if (recvfd)
                        *recvfd = *((int *) CMSG_DATA(cmptr));
        }
#else
        if (recvfd && (sizeof(int) == msg.msg_accrightslen))
                *recvfd = newfd;
#endif
        return 0;
}

int
set_non_blocking(int sock)
{
	int flags;

	flags = fcntl(sock, F_GETFL, 0);
	if (-1 == flags) {
		M_ERROR("could not get flags", strerror(errno));
		return 0;
	}

	flags = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	if (-1 == flags) {
		M_ERROR("could not set flags", strerror(errno));
		return 0;
	}

	return 1;
}

int
set_blocking(int sock)
{
	int flags;

	flags = fcntl(sock, F_GETFL, 0);
	if (-1 == flags) {
		M_ERROR("could not get flags", strerror(errno));
		return 0;
	}
	flags &= ~O_NONBLOCK;

	flags = fcntl(sock, F_SETFL, flags);
	if (-1 == flags) {
		M_ERROR("could not set flags", strerror(errno));
		return 0;
	}

	return 1;
}
