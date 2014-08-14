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

#pragma once

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "applib/base/appbase.h"
#include "utillib/config/config_item_fix_session.h"

class FIX_Session : public AppBase
{
public:
        FIX_Session(const char * const identity,
                    const char * const config_source);

        ~FIX_Session(void);


	/*
	 * Instance creation strategy.
	 *
	 * Duplex:
	 * =======
	 *
	 * ConnectToThis - 1 instance created when connection to remote endpoint is actively
	 *                 established.
	 *                 dup(2) socket when connection is established to emulate simplex.
	 *
	 * ListenOnThis  - accept(2) on socket in a loop.
	 *                 Spawn instance when a connection is accepted.
	 *                 dup(2) socket when connection is established to emulate simplex.
	 *
	 * Simplex:
	 * ========
	 *
	 * Only one instance will ever be made. This is due to
	 * difficulties macthing up in- and out-going connections in
	 * the simplex scenario, when those connections are created by
	 * accepting uncontrolled incoming connection requests.
	 *
	 * InGoing
	 * -------
	 * ConnectToThis - Connect to remote. Henceforth,
	 *                 ingoing messages are only coming from this connection.
	 * ListenOnThis  - accept(2) on socket once. Henceforth,
	 *                 ingoing messages are only coming from this connection.
	 *
	 * OutGoing
	 * --------
	 * ConnectToThis - Connect to remote. Henceforth, outgoing messages are
	 *                 only being sent using this connection.
	 * ListenOnThis  - accept(2) on socket once. Henceforth, outgoing messages
	 *                 are only being sent using this connection.
	 */
        bool init(void *data);

        bool run(void);

private:

	void accept_duplex_FIX_connections(socket_t listen_socket);

        /*
         * Sleeps until lead time seconds before the session is
         * scheduled to become active if invoked when the session is
         * not within the scheduled active hours.
         *
         * It will return immediately if invoked when the session is
         * within the scheduled active hours.
         */
        void sleep_til_session_start(void) const;

        /*
         * Sleeps until the session is scheduled to become inactive if
         * invoked when the session is within the scheduled active
         * hours.
         *
         * It will return immediately if invoked when the session is
         * outside the scheduled active hours.
         */
        void sleep_til_session_end(void) const;

        /*
         * Returns the number of seconds remaining in the current
         * session or 0 (zero) if not in a session.
         *
         * If warm-up time is non-zero, then the session is perceived
         * as if starting warm-up time seconds early.
         */
        time_t seconds_remaining_in_session(void) const;





        /*
         * Sleeps until lead_time seconds before the next start of a
         * session.
         */
//      void sleep_til_next_session(const time_t lead_time) const;

        /*
         * Returns the number of seconds remaining until the current
         * or (if outside the active period) the next sessions ends.
         */
//      time_t seconds_til_next_session_end(void) const;

        /*
         * Returns true if we are within an active session or within
         * the warm up period preceeding the sessions start. Returns
         * false otherwise.
         */
	bool active_or_warming_up(void) const;

        ConfigItemFIXSession *ci_fix_session_;
        struct FIX_Session_Config session_config_;
        socket_t in_going_;
        socket_t out_going_;
};
