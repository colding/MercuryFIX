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
 *     (3) The name of the author may not be used to endorse or
 *     promote products derived from this software without specific
 *     prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <errno.h>
#include <time.h>
#include <arpa/inet.h>
#include "stdlib/log/log.h"
#include "stdlib/network/network.h"
#include "stdlib/network/net_interfaces.h"
#include "session_instance.h"
#include "session.h"

#ifdef __linux__
static inline bool
accept_connection(timeout_t duration,
                  socket_t listening_socket,
                  socket_t & accepted_socket)
{
        static socklen_t addr_size = sizeof(struct sockaddr);
        static struct sockaddr remote_addr;
	time_t elapsed = 0;

        if (!duration.seconds)
                return false;

        if (-1 == listening_socket.socket)
                return false;

        set_blocking(listening_socket);
        do {
		if (!set_recv_timeout(listening_socket, duration)) {
			M_ERROR("could not set timout");
			continue;
		}
		elapsed = time(NULL);

                accepted_socket.socket = accept(listening_socket.socket, &remote_addr, &addr_size);
                if (-1 == accepted_socket.socket) {
                        switch (errno) {
                        case EAGAIN:
				elapsed = time(NULL) - elapsed;
				if (elapsed >= duration.seconds) {
					break;
				} else {
					duration.seconds -= elapsed;
					continue;
				}
                        default:
                                M_ERROR("could not accept connection attempt: %s", strerror(errno));
                                break;
                        }
                }
        } while (false);

        return (-1 != accepted_socket.socket);
}
#endif
#ifdef __APPLE__
static inline bool
accept_connection(timeout_t duration,
                  socket_t listening_socket,
                  socket_t & accepted_socket)
{
        static socklen_t addr_size = sizeof(struct sockaddr);
        static struct sockaddr remote_addr;

        if (!duration.seconds)
                return false;

        if (-1 == listening_socket.socket)
                return false;

        set_non_blocking(listening_socket);
        do {
                accepted_socket.socket = accept(listening_socket.socket, &remote_addr, &addr_size);
                if (-1 == accepted_socket.socket) {
                        switch (errno) {
                        case EAGAIN:
                                sleep(1);
                                if (--duration.seconds)
                                        continue;
                                break;
                        default:
                                M_ERROR("could not accept connection attempt: %s", strerror(errno));
                                break;
                        }
                }
        } while (false);

        return (-1 != accepted_socket.socket);
}
#endif

static bool
create_socket(const endpoint_t * const endpoint,
              socket_t & socket)
{
        static const int delay = 60;
        static const timeout_t timeout = { 10 };

        switch (endpoint->kind) {
        case ConnectToThis:
                M_DEBUG("ConnectToThis");
                do {
                        socket = connect_to_listening_socket(endpoint->interface.c_str(),
                                                             endpoint->port,
                                                             endpoint->pf_family,
                                                             SOCK_STREAM,
                                                             timeout);
                        if (-1 == socket.socket) {
                                M_CRITICAL("could not connect to %s:%d. Retrying in %d seconds.",
                                           endpoint->interface.c_str(),
                                           endpoint->port,
                                           delay);
                                sleep(delay);
                                continue;
                        }
                        break;
                } while (true);
                break;
        case ListenOnThis:
                M_DEBUG("ListenOnThis");
                socket = create_listening_socket(endpoint->interface.c_str(),
                                                 endpoint->port,
                                                 endpoint->pf_family,
                                                 SOCK_STREAM,
                                                 false);
                if (-1 == socket.socket) {
                        M_CRITICAL("could not create listening socket on %s:%d.",
                                   endpoint->interface.c_str(),
                                   endpoint->port);
                }
                break;
        default:
                M_ERROR("unknown socket kind");
                return false;
        }

        return  (-1 != socket.socket);
}

FIX_Session::FIX_Session(const char * const identity,
                         const char * const config_source)
        : AppBase(identity, config_source)
{
        in_going_.socket = -1;
        out_going_.socket = -1;
        ci_fix_session_ = NULL;
}

FIX_Session::~FIX_Session(void)
{
        if (ci_fix_session_)
                ci_fix_session_->release();
        if (-1 != in_going_.socket)
                close(in_going_.socket);
        if (-1 != out_going_.socket)
                close(out_going_.socket);
}

bool
FIX_Session::init(void *data)
{
        if (!ci_fix_session_) {
                if (!AppBase::init(data)) {
                        M_ERROR("could not initiate AppBase");
                        return false;
                }
                M_DEBUG("initiating FIX session \"%s\"", identity_);

                ci_fix_session_ = new (std::nothrow) ConfigItemFIXSession();
                if (!ci_fix_session_) {
                        M_ALERT("no memory");
                        return false;
                }

                if (!config_->subscribe(NULL, "PROTOCOL", "FIX_SESSION_CONFIG", ci_fix_session_)) {
                        delete ci_fix_session_;
                        ci_fix_session_ = NULL;
                        M_ERROR("could not read FIX session configuration");
                        return false;
                }
        }
        if (-1 != in_going_.socket)
                close(in_going_.socket);
        if (-1 != out_going_.socket)
                close(out_going_.socket);

        if (!ci_fix_session_->get(session_config_)) {
                ci_fix_session_->release();
                M_ERROR("could not get FIX session configuration");
                return false;
        }

        if (session_config_.is_duplex) {
                if (!create_socket(&session_config_.in_going, in_going_)) {
                        M_ERROR("could not connect endpoint");
                        return false;
                }
                M_INFO("Network duplex socket created");
                if (ConnectToThis == session_config_.in_going.kind) {
                        out_going_.socket = dup(in_going_.socket);
                        if (-1 == out_going_.socket) {
                                M_ALERT("could not dup socket: %s", strerror(errno));
                                close(in_going_.socket);
                                in_going_.socket = -1;
                                return false;
                        }
                } else {
                        out_going_.socket = -1; // do not dup before we have a connection
                }
        } else {
                if (!create_socket(&session_config_.in_going, in_going_)) {
                        M_ERROR("could not connect endpoint");
                        return false;
                }
                M_INFO("Network ingoing simplex socket created");

                if (!create_socket(&session_config_.out_going, out_going_)) {
                        M_ERROR("could not connect endpoint");
                        return false;
                }
                M_INFO("Network outgoing simplex socket created");
        }
        M_INFO("FIX session initiated");

        return true;
}

void
FIX_Session::accept_duplex_FIX_connections(socket_t listen_socket)
{
        FIX_Session_Instance *instance;
        socket_t in_socket = { -1 };
        socket_t out_socket =  { -1 };
        timeout_t session_duration;

        do {
                if (-1 != in_socket.socket)
                        close(in_socket.socket);
                in_socket.socket = -1;

                if (-1 != out_socket.socket)
                        close(out_socket.socket);
                out_socket.socket = -1;

                if (!active_or_warming_up())
                        break;

                session_duration.seconds = seconds_remaining_in_session();
                if (!accept_connection(session_duration, listen_socket, out_socket)) {
                        continue;
                }
                in_socket.socket = dup(out_socket.socket);
                if (unlikely(-1 == in_socket.socket)) {
                        M_ERROR("could not dup socket: %s", strerror(errno));
                        continue;
                }

                instance = new (std::nothrow) FIX_Session_Instance(identity_,
								   config_source_,
								   ci_fix_session_,
								   in_socket,
								   out_socket);
                if (!instance) {
                        sleep(10); // relax, maybe it's better in 10 seconds...
                        M_ALERT("no memory");
                        continue;
                }
                if (!instance->init(NULL)) {
                        M_ERROR("could not initialize FIX session instance");
                        delete instance;
                        instance = NULL;
                        continue;
                }
                if (!instance->run())
                        M_ERROR("could not run FIX session instance");
		delete instance;
        } while (true);
}

bool
FIX_Session::run(void)
{
        FIX_Session_Instance *instance;
        socket_t in_socket = { -1 };
        socket_t out_socket = { -1 };
        timeout_t session_duration;

        sleep_til_session_start();

        if (session_config_.is_duplex) {
                if (ListenOnThis == session_config_.in_going.kind) {
                        accept_duplex_FIX_connections(in_going_);
                } else {
                        out_going_.socket = dup(in_going_.socket);
                        if (-1 == out_going_.socket) {
                                M_ERROR("could not dup socket: %s", strerror(errno));
                                return false;
                        }

                        instance = new (std::nothrow) FIX_Session_Instance(identity_,
									   config_source_,
									   ci_fix_session_,
									   in_going_,
									   out_going_);
                        if (!instance) {
                                M_ALERT("no memory");
                                return false;
                        }

                        if (!instance->init(NULL)) {
                                M_ERROR("could not initialize FIX session instance");
                                delete instance;
                                return false;
                        }
                        instance->run();
			delete instance;
                }
        } else {
                if (ListenOnThis == session_config_.in_going.kind) {
                        do {
                                session_duration.seconds = seconds_remaining_in_session();
                                if (!accept_connection(session_duration, in_going_, in_socket)) {
                                        M_ERROR("could not accept connection");
                                        continue;
                                }
                        } while (false);
                } else {
                        in_socket.socket = dup(in_going_.socket);
                }
                close(in_going_.socket);
                in_going_.socket = -1;

                if (ListenOnThis == session_config_.out_going.kind) {
                        do {
                                session_duration.seconds = seconds_remaining_in_session();
                                if (!accept_connection(session_duration, out_going_, out_socket)) {
                                        M_ERROR("could not accept connection");
                                        continue;
                                }
                        } while (false);
                } else {
                        out_socket.socket = dup(out_going_.socket);
                }
                close(out_going_.socket);
                out_going_.socket = -1;

                instance = new (std::nothrow) FIX_Session_Instance(identity_,
								   config_source_,
								   ci_fix_session_,
								   in_going_,
								   out_going_);
                if (!instance) {
                        M_ALERT("no memory");
                        return false;
                }

                if (!instance->init(NULL)) {
                        M_ERROR("could not initialize FIX session instance");
                        delete instance;
                        return false;
                }
                if (!instance->run())
                        M_ERROR("could not run FIX session instance");
		delete instance;
        }
        sleep_til_session_end();
	
	return true;
}

void
FIX_Session::sleep_til_session_start(void) const
{
        unsigned int unslept = (unsigned int) (session_config_.session_warm_up_time - 0);

        do {
                unslept = sleep(unslept);
        } while (unslept);

        return;
}

void
FIX_Session::sleep_til_session_end(void) const
{
        unsigned int unslept = (unsigned int) (session_config_.session_warm_up_time - 0);

        do {
                unslept = sleep(unslept);
        } while (unslept);

        return;
}

time_t
FIX_Session::seconds_remaining_in_session(void) const
{
        unsigned int unslept = (unsigned int) (session_config_.session_warm_up_time - 0);

        do {
                unslept = sleep(unslept);
        } while (unslept);

        return 0;
}


bool
FIX_Session::active_or_warming_up(void) const
{
        return !!session_config_.session_warm_up_time;
}
