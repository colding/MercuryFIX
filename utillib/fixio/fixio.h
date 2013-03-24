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

#pragma once

#include <unistd.h>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/disruptor/disruptor_types.h"
#include <stdlib.h>
#include <pthread.h>

struct alfa_io_t;
struct bravo_io_t;
struct charlie_io_t;
struct delta_io_t;
struct echo_io_t;
struct foxtrot_io_t;


/*
 * No copies or assignments of instances of this class. It doesn't
 * make sense...
 */
class FIX_Pusher
{
public:
	/* 
	 * Call this with a valid file descriptor or -1
	 */
        FIX_Pusher(int sink_fd);

        virtual ~FIX_Pusher();

        /*
         * Allocates and initializes private members.
         *
         * FIX_ver must be in the format of "FIX.X.Y" or in the case
         * of FIX 5.x "FIXT.1.1". It must in other words be a valid
         * value for tag 8, BeginString.
         */
        bool init(const char * const FIX_ver,
		  int sink_fd);

        /*
         * Pushes a FIX messages onto the outgoing stack.
         *
         * Must not contain tags:
         *   8 (BeginString), 9 (BodyLength), 35 (MsgType), 34 (MsgSeqNum)
         *
         * But must end with:
         *   <SOH>10=
         *
         * And must begin with:
         *   <SOH>
         *
         * The mentioned tags will be added be the push function.
         */
        int push(const size_t len, 
		 const void * const data, 
		 char * const msg_type);

        /*
         * Please see FIX_Pusher::push() for the data format.
         */
        int session_push(const size_t len, 
			 const void * const data,
			 char * const msg_type);
private:
        /*
         * Default constructor disallowed
         */
        FIX_Pusher(void)
                : alfa_max_data_length_(0),
                  charlie_max_data_length_(0)
                {
                };

        /*
         * Copy constructor disallowed
         */
        FIX_Pusher(const FIX_Pusher&)
                : alfa_max_data_length_(0),
                  charlie_max_data_length_(0)
                {
                };

        /*
         * Assignemnt operator disallowed
         */
        FIX_Pusher& operator=(const FIX_Pusher&)
                {
                        return *this;
                };

        char FIX_start_bytes_[16];   // standard prefilled FIX start characters - "8=FIX.X.Y<SOH>9="
        int FIX_start_bytes_length_; // strlen of FIX version field
        int error_;                  // errno from the pusher thread running
        int running_;                // is the pusher thread running?
        int terminate_;              // terminate the pusher thread running
        pthread_t pusher_thread_id_; // the pusher thread ID

        int sink_fd_; // the file descriptor of the socket sink

        alfa_io_t *alfa_;
        struct cursor_t alfa_cursor_;
        struct alfa_entry_t *alfa_entry_;
        const size_t alfa_max_data_length_;

        bravo_io_t *bravo_;
        struct cursor_t bravo_cursor_;
        struct bravo_entry_t *bravo_entry_;

        charlie_io_t *charlie_;
        struct cursor_t charlie_cursor_;
        struct charlie_entry_t *charlie_entry_;
        const size_t charlie_max_data_length_;
};


class FIX_Popper
{
public:
	/* 
	 * Call this with a valid file descriptor or -1
	 */
        FIX_Popper(const int source_fd);

        virtual ~FIX_Popper();

        /*
         * Allocates and initializes private members
         */
        bool init(void);

        /*
         *  threadsafe - each pop will read one complete message from
         *  the source.
         */
        int pop(const size_t len, 
		void * const data);

        /*
         *  threadsafe - each pop will read one complete message from
         *  the source.
         */
        int session_pop(const size_t len,
                        void * const data);
private:
        /*
         * Default constructor disallowed
         */
        FIX_Popper(void)
                : echo_max_data_length_(0),
                  foxtrot_max_data_length_(0)
                {
                };

        /*
         * Copy constructor disallowed
         */
        FIX_Popper(const FIX_Popper&)
                : echo_max_data_length_(0),
                  foxtrot_max_data_length_(0)
                {
                };

        /*
         * Assignemnt operator disallowed
         */
        FIX_Popper& operator=(const FIX_Popper&)
                {
                        return *this;
                };

        const size_t echo_max_data_length_;
        const size_t foxtrot_max_data_length_;
        int source_fd_;
        delta_io_t *delta_;
        echo_io_t *echo_;
        foxtrot_io_t *foxtrot_;
};
