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

#include <unistd.h>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/disruptor/disruptor_types.h"
#include "stdlib/locks/guard.h"
#include <stdlib.h>
#include <pthread.h>

struct alfa_io_t;
struct bravo_io_t;
struct charlie_io_t;
struct delta_io_t;
struct echo_io_t;
struct foxtrot_io_t;
struct pusher_thread_args_t;
struct sucker_thread_args_t;
struct splitter_thread_args_t;

/*
 * Outstanding issue: Do Popper and Pusher instances live forever? If
 * yes, how do I handle socket errors with blocking disruptor
 * functions? Hmm, amswer: The disruptors do live forever, but the
 * source/sink socket must be re-initialisable independently from the
 * disruptors themselves.
 *
 * Neither class can be copied or assigned.
 */

/*
 * Puts partial messages on the sending stack.
 */
class FIX_Pusher
{
public:
	/* 
	 * Call this with SOH or whatever you want as delimiter for testing
	 */
        FIX_Pusher(const char soh);

        /*
         * Allocates and initializes private members. It may be called
         * repeatedly, but only from one thread.
         *
         * FIX_ver must be in the format of "FIX.X.Y" or in the case
         * of FIX 5.x "FIXT.1.1" or similar. It must, in other words,
         * be a valid value for tag 8, BeginString.
	 *
	 * If sink_fd if different from -1 it will be used as the new
	 * sink. 
	 *
	 * The FIX_Pusher instance takes ownership of the sink_fd file
	 * descriptor.
	 *
	 * This method calls stop(), but not start(). You must call
	 * start().
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
		 const char * const msg_type);

        /*
         * Please see FIX_Pusher::push() for the data format.
	 *
	 * Only one thread must call this method.
         */
        int session_push(const size_t len, 
			 const void * const data,
			 const char * const msg_type);

	/*
	 * Flushes stale content.
	 *
	 * All, if any, data still present within the FIX_Pusher
	 * instance will be flushed to /dev/null and logged as not
	 * sent when this method is called.
	 *
	 * Only one thread must call this method.
	 */ 
	void flush(void);

	/*
	 * Stops the pushing of message into the sink.
	 *
	 * Only one thread must call this method.
	 */ 
	void stop(void);

	/*
	 * Starts the pushing of message into the sink.
	 *
	 * Only one thread must call this method.
	 */ 
	void start(void);

private:
        /*
         * Default constructor disallowed
         */
        FIX_Pusher(void)
                : alfa_max_data_length_(0),
                  charlie_max_data_length_(0),
		  soh_('B')
                {
                };

        /*
         * Copy constructor disallowed
         */
        FIX_Pusher(const FIX_Pusher&)
                : alfa_max_data_length_(0),
                  charlie_max_data_length_(0),
		  soh_('S')
                {
                };

	/*
	 * This object must live forever
	 */
	~FIX_Pusher()
		{
		};

        /*
         * Assignemnt operator disallowed
         */
        FIX_Pusher& operator=(const FIX_Pusher&)
                {
                        return *this;
                };

        char FIX_start_bytes_[16];          // standard prefilled FIX start characters - "8=FIX.X.Y<SOH>9="
        int FIX_start_bytes_length_;        // strlen of FIX version field
        int error_;                         // errno from the pusher thread
        int terminate_;                     // terminate the pusher thread running
        pthread_t pusher_thread_id_;        // the pusher thread ID
	struct pusher_thread_args_t *args_; // parameters for the pusher thread

        int sink_fd_; // the file descriptor of the socket sink
	int dev_null_; // needed as sink to flush old messages 

        alfa_io_t *alfa_;
        const size_t alfa_max_data_length_;

        bravo_io_t *bravo_;

        charlie_io_t *charlie_;
        const size_t charlie_max_data_length_;

	const char soh_; // used to overwrite SOH ('\1') for testing
};


/*
 * Pops complate messages from the recieve stack. Takes, by necessity,
 * care of detecting message gabs and sending ResendRequests.
 */
class FIX_Popper
{
public:
	/* 
	 * Call this with SOH or whatever you want as delimiter for testing
	 */
        FIX_Popper(const char soh);

        /*
         * Allocates and initializes private members. It may be called
         * repeatedly, but only from one thread.
         *
         * FIX_ver must be in the format of "FIX.X.Y" or in the case
         * of FIX 5.x "FIXT.1.1" or similar. It must, in other words,
         * be a valid value for tag 8, BeginString.
	 *
	 * If source_fd if different from -1 it will be used as the
	 * new source.
	 *
	 * The FIX_Popper instance takes ownership of the source_fd
	 * file descriptor.
	 *
	 * This method calls stop(), but not start(). You must call
	 * start().
         */
        bool init(const char * const FIX_ver, int source_fd);

        /*
         * threadsafe - each pop will read one complete message from
         * the source. Callee takes ownership of data and must free
         * it when done processing. A pop will never return the same
         * entry twize regardless of whether it is called from
         * separate threads or not.
	 *
	 * len is the total length of the message, not the value of
	 * tag 9, BodyLength.
	 * 
	 * Returns zero if all is well, non-zero if not.
         */
        int pop(size_t * const len, 
		void **data);

        /*
         * Same as above, but this method depends on each caller to
         * maintain a cursor and a registration number.
	 * 
	 * Returns zero if all is well, non-zero if not.
         */
        int pop(struct cursor_t * const cursor,
		struct count_t * const reg_number,
		size_t * const len, 
		void **data);

        /*
         * Register state variables for above method. This method will
         * block until caller may start popping.
         */
        void register_popper(struct cursor_t * const cursor,
			     struct count_t * const reg_number);

        /*
         * threadsafe - each pop will return a non-const pointer to a
         * complete message which must be processed in situ or
         * copied.
	 *
	 * len is the total length of the message, not the value of
	 * tag 9, BodyLength.
	 *
	 * Only one thread must call this method.
	 *
	 * Return value is zero if all is well, or an errno value if
	 * not. 
         */
        int session_pop(const size_t len,
                        void *data);

	/*
	 * Stops the popping of message of the source.
	 *
	 * Only one thread must call this method.
	 */ 
	void stop(void);

	/*
	 * Starts the popping of message of the source.
	 *
	 * Only one thread must call this method.
	 */ 
	void start(void);

private:
        /*
         * Default constructor disallowed
         */
        FIX_Popper(void)
                : echo_max_data_length_(0),
                  foxtrot_max_data_length_(0),
		  soh_('B')
                {
                };

        /*
         * Copy constructor disallowed
         */
        FIX_Popper(const FIX_Popper&)
                : echo_max_data_length_(0),
                  foxtrot_max_data_length_(0),
		  soh_('S')
                {
                };

	/*
	 * This object must live forever
	 */
	~FIX_Popper()
                {
                };

        /*
         * Assignemnt operator disallowed
         */
        FIX_Popper& operator=(const FIX_Popper&)
                {
                        return *this;
                };

        int source_fd_;
	char begin_string_[32];                        // 8="FIX ver"<SOH>9="
        int error_;                                    // errno from the sucker thread
        int terminate_;                                // terminate the sucker thread running
        pthread_t sucker_thread_id_;                   // the sucker thread ID
	struct sucker_thread_args_t *sucker_args_;     // parameters for the sucker thread
	struct splitter_thread_args_t *splitter_args_; // parameters for the splitter thread

	MutexGuard guard_;
        delta_io_t *delta_;
        struct cursor_t delta_n_;
        struct cursor_t delta_cursor_upper_limit_;
	struct count_t delta_reg_number_;

        echo_io_t *echo_;
        const size_t echo_max_data_length_;

        foxtrot_io_t *foxtrot_;
        const size_t foxtrot_max_data_length_;

	const char soh_; // used to overwrite SOH ('\1') for testing
};
