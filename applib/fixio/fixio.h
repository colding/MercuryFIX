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

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <queue>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/disruptor/disruptor_types.h"
#include "stdlib/local_db/sqlite3.h"
#include "stdlib/locks/region_lock.h"
#include "stdlib/locks/guard.h"
#include "applib/fixutils/db_utils.h"
#include "applib/fixutils/stack_utils.h"
#include "applib/fixmsg/fix_types.h"

struct alfa_io_t;
struct bravo_io_t;
struct charlie_io_t;
struct delta_io_t;
struct echo_io_t;
struct foxtrot_io_t;
struct romeo_io_t;
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
 * Neither class may be copied nor assigned.
 */

/*
 * Service contract for the pusher class.
 */
class FIX_PushBase
{
public:
        /*
         * Pushes a FIX messages onto the outgoing stack.
         *
         * Must not contain these tags:
         *   8 (BeginString), 9 (BodyLength), 35 (MsgType), 34 (MsgSeqNum)
         *
         * But must begin with:
         *   <SOH>
	 *
         * and must end with:
         *   <SOH>10=
         *
         *
         * The mentioned tags will be added by the push function and
         * so will the checksum value.
         *
	 * ttl: Time To Live, message will not be resend if ttl has
	 *      passed.
	 *
         * len: The size of data in bytes.
	 *
         * data: A pointer to a partial FIX message.
	 *
         * msg_type: The tag 35 message type value.
	 *
         * Returns 0 (zero) if all is well or an errno value if not.
         */
        virtual int push(const struct timeval * const ttl,
			 const size_t len,
                         const uint8_t * const data,
                         const char * const msg_type) = 0;

        /*
         * Please see FIX_PushBase::push() for the data format.
         *
         * Only one thread must call this method.
	 *
	 * Parameters as in push().
	 *
         * Returns 0 (zero) if all is well or an errno value if not.
         */
        virtual int session_push(const struct timeval * const ttl,
				 const size_t len,
                                 const uint8_t * const data,
                                 const char * const msg_type) = 0;

	/*
	 * Instrucs the pusher to resend a sequence of messages.
	 *
	 * start: The sequence number of the first message to be
	 *        resend.
	 *
	 * end: The sequence number of the last message to be
	 *      resend. If this number is greater than the greatest
	 *      sequence number in the current session, or 0 (zero),
	 *      then all sent message, starting with sequence number
	 *      "start", will be resend.
	 *
         * Returns 0 (zero) if all is well, 1 (one) if not.
	 */
	virtual int resend(const uint64_t start,
			   const uint64_t end) = 0;
};

/*
 * Puts partial messages on the sending stack.
 */
class FIX_Pusher : public FIX_PushBase
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
         * This method calls stop(), but not start(). You must call
         * start().
         *
         * local_cache: Path of local database caching sent
         * messages. Must not be NULL.
	 *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int init(const char * const local_cache);

        /*
         * Please see base class documentation.
         */
        int push(const struct timeval * const ttl,
		 const size_t len,
                 const uint8_t * const data,
                 const char * const msg_type);

        /*
         * Please see base class documentation.
         */
        int session_push(const struct timeval * const ttl,
			 const size_t len,
                         const uint8_t * const data,
                         const char * const msg_type);

        /*
         * Please see base class documentation.
         */
	int resend(const uint64_t start,
		   const uint64_t end);

        /*
         * Starts the pushing of messages into the sink.
         *
         * Only one thread must call this method.
         *
         * local_cache: Path of local database caching sent
         * messages. local_cache is ignored if it is NULL.
         *
         * From the SQLite documentation:
         *
         *   If the filename is ":memory:", then a private, temporary
         *   in-memory database is created for the connection. This
         *   in-memory database will vanish when the database
         *   connection is closed. Future versions of SQLite might
         *   make use of additional special filenames that begin with
         *   the ":" character. It is recommended that when a database
         *   filename actually does begin with a ":" character you
         *   should prefix the filename with a pathname such as "./"
         *   to avoid ambiguity.
         *
         *   If the filename is an empty string, then a private,
         *   temporary on-disk database will be created. This private
         *   database will be automatically deleted as soon as the
         *   database connection is closed.
         *
         * FIX_ver must be in the format of "FIX.X.Y" or in the case
         * of FIX 5.x "FIXT.1.1" or similar. It must, in other words,
         * be a valid value for tag 8, BeginString. FIX_ver will be
         * ignored if it is NULL. You could use
         * fix_version_string[enum FIX_Version] or a literal string...
         *
         * If sink_fd is non-negative it will be used as the new
         * sink. It is iognored otherwise.
         *
         * The FIX_Pusher instance takes ownership of the sink_fd file
         * descriptor.
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int start(const char * const local_cache,
                  const char * const FIX_ver,
                  int sink_fd);

        /*
         * Stops the pushing of message into the sink.
         *
         * Only one thread must call this method.
         */
        void stop(void);

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

        /*
         * Exclusively used for resending
         */
	int push_to_romeo(const struct timeval * const ttl,
			  const size_t len,
			  const uint8_t * const data,
			  const char * const msg_type);

	/*
	 * Returns a suitable formatted value for tag 52
	 * (SendingTime).
	 */
	void get_sendingtime(char sendingtime[24]) const;

	/*
	 * Will update tag 52 (SendingTime) to prepare the message for
	 * resending. It will save a zero terminated copy of the old
	 * sending time value in orig_sendingtime to be used as value
	 * for tag 122 (OrigSendingTime).
	 */
	void update_sendingtime(PartialMessage * const pmsg,
				char orig_sendingtime[24]) const;

        char FIX_start_bytes_[32];          // standard prefilled FIX start characters - "8=FIX.X.Y<SOH>9="
	FIX_Version fix_ver_;               // FIX protocol version
	uint64_t msg_seq_number_;           // message sequence number
	uint64_t push_loop_count_;
        int FIX_start_bytes_length_;        // strlen of FIX version field
        int error_;                         // errno from the pusher thread
        int pause_thread_;                  // pause pusher thread
        int db_is_open_;                    // 1 (one) if the database is open, 0 (zero) if not
        int started_;                       // 1 (one) if started, 0 (zero) if not
        struct pusher_thread_args_t *args_; // parameters for the pusher thread
        MsgDB db_;                          // holding sent partial messages

        int sink_fd_; // the file descriptor of the socket sink

        alfa_io_t *alfa_;
        const size_t alfa_max_data_length_;

        bravo_io_t *bravo_;

        charlie_io_t *charlie_;
        const size_t charlie_max_data_length_;

	// specific queue for resending
        romeo_io_t *romeo_;
        struct cursor_t romeo_cursor_;
        struct count_t romeo_reg_number_;

        const char soh_; // used to overwrite SOH ('\1') for testing
	char sending_time_tag_[5]; // "<SOH>52="
};


/*
g * Pops complate messages from the recieve stack. Takes, by necessity,
 * care of detecting message gabs and ResendRequest/SequenceReset.
 */
class FIX_Popper
{
public:
        struct RawMessage {
                uint32_t len;            // length of message in bytes
                uint32_t msgtype_offset; // offset in bytes of first character in message type field
                uint8_t *data;           // the FIX message itself
        };

        /*
         * Call this with SOH or whatever you want as delimiter for
         * testing
         */
        FIX_Popper(const char soh);

        /*
         * Allocates and initializes private members. It may be called
         * repeatedly, but only from one thread.
         *
         * This method calls stop(), but not start(). You must call
         * start().
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int init(void);

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
         * *msgtype_offset is the offset in bytes from the first byte
         * in the message till the first character of the message
         * type field value.
         *
         * **data is the message data. This data is owened by
         * the caller which must free it.
         *
         * Returns zero if all is well, 1 (one) if not.
         */
        int pop(uint32_t * const len,
                uint32_t * const msgtype_offset,
                uint8_t **data);

        /*
         * Same as above, but lockfree and depends on each caller to
         * maintain a cursor and a registration number.
         *
         * "*messages" is a bunch of recieved raw messages collected
         * by disruptor batching. They are ordered by recieval
         * time. Caller must free() RawMessage.data.
         */
        void pop(const struct count_t * const reg_number,
                 struct cursor_t * const cursor,
                 std::queue<struct FIX_Popper::RawMessage> * const messages);

        /*
         * Register state variables for above method. This method will
         * block until caller may start popping.
         */
        void register_popper(struct cursor_t * const cursor,
                             struct count_t * const reg_number);


        /*
         * Unregister a registered popper.
         */
        void unregister_popper(const struct count_t * const reg_number);

        /*
         * Not threadsafe - each pop will return a non-const pointer
         * to a complete message which can be processed in situ or may
         * be copied. Caller does not own the memory pointed to and
         * may not free() it.
         *
         * *len is the total length of the message, not the value of
         * tag 9, BodyLength.
         *
         * *msgtype_offset is the offset in bytes from the first byte
         * in the message till the first character of the message
         * type field.
         *
         * **data is the session message data. This data is owened by
         * the popper class which will free it, but the caller may
         * modify the pointed to bytes as much as it will.
         *
         * Only one thread must call this method or the behavior is
         * undefined.
         */
        void session_pop(uint32_t * const len,
                         uint32_t * const msgtype_offset,
                         uint8_t **data);

        /*
         * Starts the popping of messages off the source.
         *
         * local_cache: Path of local database caching sent
         * messages. local_cache is ignored if it is NULL.
         *
         * From the SQLite documentation:
         *
         *   If the filename is ":memory:", then a private, temporary
         *   in-memory database is created for the connection. This
         *   in-memory database will vanish when the database
         *   connection is closed. Future versions of SQLite might
         *   make use of additional special filenames that begin with
         *   the ":" character. It is recommended that when a database
         *   filename actually does begin with a ":" character you
         *   should prefix the filename with a pathname such as "./"
         *   to avoid ambiguity.
         *
         *   If the filename is an empty string, then a private,
         *   temporary on-disk database will be created. This private
         *   database will be automatically deleted as soon as the
         *   database connection is closed.
         *
         * FIX_ver: Must be in the format of "FIX.X.Y" or in the case
         * of FIX 5.x "FIXT.1.1" or similar. It must, in other words,
         * be a valid value for tag 8, BeginString. FIX_ver will be
         * ignored if it is NULL. It must be noted that for FIX
         * versions 4.0 and 4.1, tag 52 (SendingTime) will be defined
         * as being in the format "YYYYMMDD-HH:MM:SS". All other FIX
         * versions will see tag52 defined in format
         * "YYYYMMDD-HH:MM:SS.sss". This only has effect when
         * resending messages as an automatic reaction to a
         * ResendRequest message.
         *
         * *session_message_types: messages with these message types
         * are split into the session queue. The popper takes
         * ownership of the pointer. ResendRequest/SequenceReset (type
         * '2' and '4') is handles directly by the popper. It will be
         * ignored if it is NULL and overwrite any previous pointer if
         * not.
         *
         * *pusher is used by the popper to respond to
         * ResendRequest/SequenceReset. The popper does not take
         * ownership of the pusher. It will be ignored if it is NULL
         * and overwrite any previous pointer if not.
         *
         * If sink_fd is non-negative it will be used as the new
         * sink. It is iognored otherwise.
         *
         * The FIX_Pusher instance takes ownership of the sink_fd file
         * descriptor.
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int start(const char * const local_cache,
                  const char * const FIX_ver,
                  FIX_PushBase * const pusher,
                  int source_fd);

        /*
         * Stops the popping of message of the source.
         *
         * Only one thread must call this method.
         */
        int stop(void);

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
	FIX_Version fix_ver_;
        char begin_string_[32];                        // 8="FIX ver"<SOH>9="
        int begin_string_length_;                      // strlen of begin_string_
        int error_;                                    // errno from the sucker thread
        int pause_threads_;                            // pause sucker and splitter threads
        int db_is_open_;                               // 1 (one) if the database is open, 0 (zero) if not
        int sucker_is_running_;                        // 1 (one) if the sucker thread is running, 0 (zero) if not
        int started_;                                  // 1 (one) if started, 0 (zero) if not
        struct sucker_thread_args_t *sucker_args_;     // parameters for the sucker thread
        struct splitter_thread_args_t *splitter_args_; // parameters for the splitter thread
        MsgDB db_;                                     // holding recieved messages

        MutexGuard guard_;
        delta_io_t *delta_;
        struct cursor_t delta_n_;
        struct cursor_t delta_cursor_upper_limit_;
        struct count_t delta_reg_number_;

        echo_io_t *echo_;
        const size_t echo_max_data_length_;
        struct cursor_t echo_n_;
        struct cursor_t echo_cursor_upper_limit_;
        struct count_t echo_reg_number_;

        foxtrot_io_t *foxtrot_;
        const size_t foxtrot_max_data_length_;

        FIX_PushBase *pusher_;
        const char soh_; // used to overwrite SOH ('\1') for testing
};
