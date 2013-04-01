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

/*
 * 5 different disruptor types are declared for the Pusher and Popper
 * classes.
 *
 * First two bytes in entry always encodes the length of the data
 * except for slow_queue which has a structure consiting of a size_t
 * given the length of the data and a pointer to the data. The slow
 * queues entry processors takes ownership of the data pointed to.
 *
 * 1) fast_queue - Disruptor intended to hold one complete FIX message
 *
 * 2) blob_queue - Disruptor just holding arbitrary data.
 *
 * 3) slow_queue - Disruptor intended to hold pointers to FIX mesages
 *    to big to fit into the fast queue.
 *
 * 4) session_msg_recv_queue - The FIX sesison engine will pop
 *    messages from this
 *
 * 5) session_msg_send_queue - The FIX sesison engine will push
 *    messages on this
 *
 */

/*
 * FIX_Pusher:: class
 * ==================
 *
 * Neither Push methods takes ownership of provided data.
 *
 *  |
 *  | Flow direction of data
 *  |
 * \ /
 *  `
 *                       Push method (threadsafe)                         Session push method (single threaded)
 *                 (Will put entries on fast disruptor or                 (Always pushes to session disruptor)
 *                  slow disruptor if they are too big)
 *
 *
 *     (Alfa)  Fast disruptor            (Bravo) Slow disruptor          (Charlie) Session message send disruptor
 *  (large memory areas, big enough    (entry with pointer to message  (exclusively used by the FIX session engine)
 *   to hold an entire message, as         and the size of it)
 *   entries)
 *
 *                           Reads in turn from fast, slow and session disruptor and writes to sink
 *
 *                                   File descriptor data sink - Data flows down here
 *
 *
 * ################################################################################################################
 *
 * FIX_Popper:: class
 * ==================
 *
 *  .
 * / \
 *  |
 *  | Flow direction of data
 *  |
 *
 *             Pop method (threadsafe)                                  Session pop method (single threaded)
 *     (Will pop entries (i.e. messages) in order                       (Always pops from session disruptor)
 *       from fast disruptor or slow disruptor)
 *
 *
 *           (Delta)  Slow disruptor                                   (Echo) Session message recv disruptor
 *        (entry with pointer to message                             (exclusively used by the FIX session engine)
 *            and the size of it)
 *
 *
 *
 *             Reads data and extract complete messages one at a time. Writes one message to one entry in the
 *                 fast disruptor or one message to the fast disruptor if the message does n't fit in the
 *                    fast disruptor. Session messages always goes to the session disruptor.
 *
 *
 *                                              (Foxtrot) BLOB disruptor
 *                                with entries of a fair size holding data as it arrives
 *
 *
 *                                 File descriptor data source - Data flows up from here
 *
 */

/*
 * Alfa) Many publishers, one entry processor, 4K entry size, 1024 entries
 *
 * Bravo) Many publishers, one entry processor, sizeof(size_t)+sizeof(char*) entry size, 512 entries
 *
 * Charlie) One publisher, one entry processor, 512 byte entry size, 512 entries
 *
 * Delta) One publisher, many entry processors, sizeof(size_t)+sizeof(char*) entry size, 512 entries
 *
 * Echo) One publisher, one entry processor, 512 byte entry size, 512 entries
 *
 * Foxtrot) One publisher, one entry processor, 8K entry size, 512 entries
 *
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#define YIELD() sched_yield()
#include "stdlib/disruptor/disruptor.h"
#include "stdlib/process/threads.h"
#include "stdlib/marshal/primitives.h"
#include "stdlib/macros/macros.h"
#include "stdlib/log/log.h"
#include "applib/fixlib/defines.h"
#include "utillib/fixio/fixio.h"

// takes messages from foxtrot and puts them onto delta and echo as
// appropriate.
struct splitter_thread_args_t {
	delta_io_t *delta;
	echo_io_t *echo;
	foxtrot_io_t *foxtrot;
	char *begin_string;
	size_t begin_string_length;
	char soh;
};

// takes incomming data from source_fd_ and puts it onto foxtrot
struct sucker_thread_args_t {
	int *terminate_sucker;
	int *sucker_running;
	int *error;
	int source_fd;
	foxtrot_io_t *foxtrot;
};

static inline void
set_flag(int *flag, int val)
{
	__atomic_store_n(flag, val, __ATOMIC_RELEASE);
}

static inline int
get_flag(int *flag)
{
	return __atomic_load_n(flag, __ATOMIC_ACQUIRE);
}

/*
 * msg is a pointer to the first byte in a FIX messsage which is
 * included in the checksum calculation. len is the number of bytes
 * included in the checksum calculation.
 */
static inline int
get_FIX_checksum(const char *msg, size_t len)
{
	uint64_t sum = 0;
	size_t n;

	for (n = 0; n < len; ++n) {
		sum += (uint64_t)msg[n];
	}

	return (sum % 256);
}


/*
 * Content of delta, echo and foxtrotr disruptor entries:
 *
 * - foxtrot holds data as it arrives on the socket.
 *
 * - delta holds a complete non-session message per entry
 *
 * - echo holds a complete session message per entry
  */

/*
 * Delta) One publisher, many entry processors,
 * sizeof(size_t)+sizeof(char*) entry size, 128 entries
 *
 */
#define DELTA_QUEUE_LENGTH (128) // MUST be a power of two
#define DELTA_ENTRY_PROCESSORS (32)
struct delta_t {
	size_t size;
	void *data;
};

DEFINE_ENTRY_TYPE(struct delta_t, delta_entry_t);
DEFINE_RING_BUFFER_TYPE(DELTA_ENTRY_PROCESSORS, DELTA_QUEUE_LENGTH, delta_entry_t, delta_io_t);
DEFINE_RING_BUFFER_MALLOC(delta_io_t, delta_);
DEFINE_RING_BUFFER_INIT(DELTA_QUEUE_LENGTH, delta_io_t, delta_);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION(delta_entry_t, delta_io_t, delta_);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION(delta_entry_t, delta_io_t, delta_);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION(delta_io_t, delta_);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION(delta_io_t, delta_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION(delta_io_t, delta_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_NONBLOCKING_FUNCTION(delta_io_t, delta_);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION(delta_io_t, delta_);
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_BLOCKING_FUNCTION(delta_io_t, delta_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(delta_io_t, delta_);

/*
 * Echo) One publisher, one entry processor, 512 byte entry size, 512
 * entries
 *
 */
#define ECHO_QUEUE_LENGTH (512) // MUST be a power of two
#define ECHO_ENTRY_PROCESSORS (1)
#define ECHO_ENTRY_SIZE (512)
#define ECHO_MAX_DATA_SIZE (ECHO_ENTRY_SIZE - sizeof(uint32_t))
typedef uint8_t echo_t[ECHO_ENTRY_SIZE];

DEFINE_ENTRY_TYPE(echo_t, echo_entry_t);
DEFINE_RING_BUFFER_TYPE(ECHO_ENTRY_PROCESSORS, ECHO_QUEUE_LENGTH, echo_entry_t, echo_io_t);
DEFINE_RING_BUFFER_MALLOC(echo_io_t, echo_);
DEFINE_RING_BUFFER_INIT(ECHO_QUEUE_LENGTH, echo_io_t, echo_);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION(echo_entry_t, echo_io_t, echo_);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION(echo_entry_t, echo_io_t, echo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION(echo_io_t, echo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION(echo_io_t, echo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION(echo_io_t, echo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_NONBLOCKING_FUNCTION(echo_io_t, echo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION(echo_io_t, echo_);
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_BLOCKING_FUNCTION(echo_io_t, echo_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(echo_io_t, echo_);

/*
 * Foxtrot) One publisher, one entry processor, 4K entry size, 1024
 * entries
 *
 */
#define FOXTROT_QUEUE_LENGTH (1024) // MUST be a power of two
#define FOXTROT_ENTRY_PROCESSORS (1)
#define FOXTROT_ENTRY_SIZE (1024*4)
#define FOXTROT_MAX_DATA_SIZE (FOXTROT_ENTRY_SIZE - sizeof(uint32_t))
typedef uint8_t foxtrot_t[FOXTROT_ENTRY_SIZE];

DEFINE_ENTRY_TYPE(foxtrot_t, foxtrot_entry_t);
DEFINE_RING_BUFFER_TYPE(FOXTROT_ENTRY_PROCESSORS, FOXTROT_QUEUE_LENGTH, foxtrot_entry_t, foxtrot_io_t);
DEFINE_RING_BUFFER_MALLOC(foxtrot_io_t, foxtrot_);
DEFINE_RING_BUFFER_INIT(FOXTROT_QUEUE_LENGTH, foxtrot_io_t, foxtrot_);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION(foxtrot_entry_t, foxtrot_io_t, foxtrot_);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION(foxtrot_entry_t, foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_NONBLOCKING_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_BLOCKING_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(foxtrot_io_t, foxtrot_);

/*********************
 *      FIX_Pusher
 *********************/

/*
 * Returns the sequence number of last recieved message or 0 if the
 * session is new.
 */
static uint64_t
get_latest_msg_seq_number_recieved(void)
{
	return 0;
}

/*
 * Returns pointer to first character in message type. The field
 * terminating SOH will be changed to '\0'. Must be changed manually
 * back if FIX protocol compliance is to be maintained.
 */
static inline char*
get_message_type(const char soh, 
		 struct delta_entry_t * const entry)
{
	static const char msg_type_field[4] = { soh, '3', '5', '=' };
	char *msg_type = (char*)memmem(entry->content.data, entry->content.size, msg_type_field, sizeof(msg_type_field));
	char *tmp;

	tmp = msg_type;
	while (msg_type_field[0] != *tmp) {
		++tmp;
	}
	*tmp = '\0';

	return msg_type;
}

static inline bool
is_session_message(const char * const /*msg_type*/)
{
	return false;
}

enum FIX_Parse_State
{
	FindingBeginString,
	FindingBodyLength,
	CopyingBody,
};

void*
splitter_thread_func(void *arg)
{
	enum FIX_Parse_State state;
	int l;
	uint32_t k;
	uint32_t entry_length;
	uint32_t body_length;
	uint32_t bytes_left_to_copy;
	char length_str[32];
	size_t bs_pos;
	char *msg_type;
	uint64_t msg_seq_number;
	struct cursor_t n;

	// split onto these
	struct cursor_t delta_cursor;
	struct delta_entry_t *delta_entry;
	struct cursor_t echo_cursor;
	struct echo_entry_t *echo_entry;

	// from this source
	struct cursor_t foxtrot_cursor;
	struct count_t foxtrot_reg_number;
	const struct foxtrot_entry_t *foxtrot_entry;
	struct cursor_t cursor_upper_limit;

	struct splitter_thread_args_t *args = (struct splitter_thread_args_t*)arg;
	if (!args) {
		M_ERROR("splitter thread cannot run");
		abort();
	}

	// Get last message sequence number recieved - tag 34.  The
	// sequence number will be incremented whenever a message is
	// recieved and varified (by checksum and FIX version)
	msg_seq_number = get_latest_msg_seq_number_recieved();

	//
	// register entry processor for foxtrot
	//
	foxtrot_cursor.sequence = foxtrot_entry_processor_barrier_register(args->foxtrot, &foxtrot_reg_number);

	// acquire publisher entries
	delta_publisher_port_next_entry_blocking(args->delta, &delta_cursor);
	delta_entry = delta_ring_buffer_acquire_entry(args->delta, &delta_cursor);
	echo_publisher_port_next_entry_blocking(args->echo, &echo_cursor);
	echo_entry = echo_ring_buffer_acquire_entry(args->echo, &echo_cursor);

	// filter available data to echo and delta forever and ever
	l = 0;
	bs_pos = 0;
	bytes_left_to_copy = 0;
	state = FindingBeginString;
	do {
		cursor_upper_limit.sequence = foxtrot_cursor.sequence;
		if (!foxtrot_entry_processor_barrier_wait_for_nonblocking(args->foxtrot, &cursor_upper_limit))
			continue;

		for (n.sequence = foxtrot_cursor.sequence; n.sequence <= cursor_upper_limit.sequence; ++n.sequence) { // batching
			foxtrot_entry = foxtrot_ring_buffer_show_entry(args->foxtrot, &n);
			
			entry_length = getul(foxtrot_entry->content);
			for (k = 0; k < entry_length; ++k) {
				switch (state) {
				case FindingBeginString:
					if (args->begin_string[bs_pos] == *(foxtrot_entry->content + sizeof(uint32_t) + k)) {
						++bs_pos;
						continue;
					}
					if ('\0' != args->begin_string[bs_pos])
						state = FindingBodyLength;
					bs_pos = 0;
					break;
				case FindingBodyLength:
					length_str[l] = *(char*)(foxtrot_entry->content + sizeof(uint32_t) + k);
					++l; // for 64bit systems this is safe as the largest number of digits is 20
					if (args->soh == length_str[l]) {
						length_str[l] = '\0';						
						body_length = atoll(length_str);
						bytes_left_to_copy = body_length + 1 + 7; // we need to copy the soh_ following the BodyLength field (it isn't included in the field value) and the CheckSum plus ending soh_

						if (delta_entry->content.size < strlen(args->begin_string) + strlen(length_str) + body_length + 1 + 7) {
							free(delta_entry->content.data);
							delta_entry->content.size = strlen(args->begin_string) + strlen(length_str) + body_length + 1 + 7;
							delta_entry->content.data = malloc(delta_entry->content.size);
							if (!delta_entry->content.data) {
								M_ALERT("no memory");
								state = FindingBeginString; // skip this message and hope for better memory conditions later
								l = 0;
								break;
							}
						}

						memcpy(delta_entry->content.data, args->begin_string, strlen(args->begin_string)); // 8=FIX.X.Y<SOH>9=
						memcpy((char*)delta_entry->content.data + strlen(args->begin_string), length_str, strlen(length_str)); // <LENGTH>
						state = CopyingBody;
						l = 0;
					}
					break;
				case CopyingBody:
					if (entry_length - k >= bytes_left_to_copy) { // one memcpy enough
						memcpy((char*)delta_entry->content.data + strlen(args->begin_string) + strlen(length_str), 
						       foxtrot_entry->content + sizeof(uint32_t) + k, 
						       bytes_left_to_copy); // <SOH>ya-da ya-da<SOH>10=ABC<SOH>
						msg_type = get_message_type(args->soh, delta_entry);
						if (is_session_message(msg_type)) {
							if (ECHO_MAX_DATA_SIZE < delta_entry->content.size) {
								M_ALERT("o versized session message");
							} else {
								while (*msg_type) {
									++msg_type;
								}
								*msg_type = args->soh;
								
								setul(echo_entry->content, delta_entry->content.size);
								memcpy(echo_entry->content + sizeof(uint32_t), delta_entry->content.data, delta_entry->content.size);
								echo_publisher_port_commit_entry_blocking(args->echo, &echo_cursor);
								echo_publisher_port_next_entry_blocking(args->echo, &echo_cursor);
								echo_entry = echo_ring_buffer_acquire_entry(args->echo, &echo_cursor);
							}
						} else {
							while (*msg_type) {
								++msg_type;
							}
							*msg_type = args->soh;
							delta_publisher_port_commit_entry_blocking(args->delta, &delta_cursor);
							delta_publisher_port_next_entry_blocking(args->delta, &delta_cursor);
							delta_entry = delta_ring_buffer_acquire_entry(args->delta, &delta_cursor);
						}
						++msg_seq_number;
						state = FindingBeginString;
						k = entry_length;
					} else {
						memcpy((char*)delta_entry->content.data + strlen(args->begin_string) + strlen(length_str), 
						       foxtrot_entry->content + sizeof(uint32_t) + k,
						       entry_length - k); // <SOH>ya-da ya-da<SOH>10=ABC<SOH>
						bytes_left_to_copy -= entry_length - k;
						k = entry_length;
					}
					break;
				default:
					abort();
				}

 			}
			/* Should we release each entry as we are done
			 * with it or the entire batch in one go...?
			 * 
			 * OK, we need to release each entry as we are
			 * done with it for _very_ long messages which
			 * takes up more than the buffering capacity
			 * of the disruptor. We might detect this
			 * conditions as an optimization, but that is
			 * for later.
			 *
			 * Also, we need to release as we go if the
			 * input is happening faster than we are able
			 * to process it. Maybe the approach below
			 * really is the safest...
			 */
			foxtrot_entry_processor_barrier_release_entry(args->foxtrot, &foxtrot_reg_number, &n);
		}
	} while (1);

	foxtrot_entry_processor_barrier_unregister(args->foxtrot, &foxtrot_reg_number);

	return NULL;
}

void*
sucker_thread_func(void *arg)
{
	void *buf;
	ssize_t rval;
	struct cursor_t foxtrot_cursor;
	struct foxtrot_entry_t *foxtrot_entry = NULL;

	struct sucker_thread_args_t *args = (struct sucker_thread_args_t*)arg;
	if (!args) {
		M_ERROR("pusher thread cannot run");
		abort();
	}

	// pull data from source_fd onto foxtrot until told to stop
	while (!get_flag(args->terminate_sucker)) {
		foxtrot_publisher_port_next_entry_blocking(args->foxtrot, &foxtrot_cursor);
		foxtrot_entry = foxtrot_ring_buffer_acquire_entry(args->foxtrot, &foxtrot_cursor);
		buf = foxtrot_entry->content + sizeof(uint32_t);
	again:
		rval = recvfrom(args->source_fd, buf, FOXTROT_MAX_DATA_SIZE, 0, NULL, NULL);
		switch (rval) {
		case 0:
			M_ERROR("peer closed connection");
			goto out;
		case -1:
			switch (errno) {
			case ETIMEDOUT:
			case EAGAIN:
			case EINTR:
				if (get_flag(args->terminate_sucker))
					goto out;
				goto again;
			default:
				set_flag(args->error, errno);
				M_ERROR("error writing data: %s", strerror(errno));
				goto out;
			}
		default:
			setul(foxtrot_entry->content, rval);
			break;
		}
		foxtrot_publisher_port_commit_entry_blocking(args->foxtrot, &foxtrot_cursor);
	}
out:
	setul(foxtrot_entry->content, 0);
	foxtrot_publisher_port_commit_entry_blocking(args->foxtrot, &foxtrot_cursor);
	set_flag(args->sucker_running, 0);

	return NULL;
}


FIX_Popper::FIX_Popper(const char soh)
	: echo_max_data_length_(ECHO_MAX_DATA_SIZE),
	  foxtrot_max_data_length_(FOXTROT_MAX_DATA_SIZE),
	  soh_(soh)
{
	source_fd_ = -1;
	memset(begin_string_, '\0', sizeof(begin_string_));
	error_ = 0;
	delta_ = NULL;
	echo_ = NULL;
	foxtrot_ = NULL;
	splitter_args_ = NULL;
	sucker_args_ = NULL;
	set_flag(&running_, 0);
	set_flag(&terminate_, 1);
}

bool
FIX_Popper::init(const char * const FIX_ver, int source_fd)
{
	stop();

	if (sizeof(begin_string_) <= strlen(FIX_ver)) {
                M_ALERT("oversized FIX version: %s (%d)", FIX_ver, sizeof(begin_string_));
		goto err;
	}
	if (!begin_string_[0] && !FIX_ver) {
		M_ALERT("no FIX version specified");
		goto err;
	}
	snprintf(begin_string_, sizeof(begin_string_), "8=%s%c9=", FIX_ver, soh_);

	if (-1 != source_fd) {
		if (source_fd_)
			close(source_fd_);
		source_fd_ = source_fd;
	}
	if (-1 == source_fd_) {
		M_ALERT("no source file descriptor specified");
		goto err;
	}		

	if (!delta_) {
		delta_ = delta_ring_buffer_malloc();
		if (!delta_) {
			M_ALERT("no memory");
			goto err;
		}
		delta_ring_buffer_init(delta_);
	}

	if (!echo_) {
		echo_ = echo_ring_buffer_malloc();
		if (!echo_) {
			M_ALERT("no memory");
			goto err;
		}
		echo_ring_buffer_init(echo_);
	}

	if (!foxtrot_) {
		foxtrot_ = foxtrot_ring_buffer_malloc();
		if (!foxtrot_) {
			M_ALERT("no memory");
			goto err;
		}
		foxtrot_ring_buffer_init(foxtrot_);
	}

	// NOTE: We can't change the FIX version on the fly with this
	// design. Maybe we should abolish the filter thread and let
	// the sucker thread do the filtering? Anyways, that is for
	// later...
	if (!splitter_args_) {
		splitter_args_ = (splitter_thread_args_t*)malloc(sizeof(struct splitter_thread_args_t));
		if (!splitter_args_) {
			M_ALERT("no memory");
			goto err;
		}
		splitter_args_->begin_string = begin_string_;
		splitter_args_->begin_string_length = strlen(begin_string_);
		splitter_args_->delta = delta_;
		splitter_args_->echo = echo_;
		splitter_args_->foxtrot = foxtrot_;
		splitter_args_->soh = soh_;

		pthread_t splitter_thread_id;
		if (!create_detached_thread(&splitter_thread_id, splitter_args_, splitter_thread_func)) {
			M_ALERT("could not create filter thread");
			abort();
		}
	}

	free(sucker_args_);
	sucker_args_ = (sucker_thread_args_t*)malloc(sizeof(struct sucker_thread_args_t));
	if (!sucker_args_) {
		M_ALERT("no memory");
		goto err;
	}
	sucker_args_->terminate_sucker = &terminate_;
	sucker_args_->sucker_running = &running_;
	sucker_args_->source_fd = source_fd_;
	sucker_args_->error = &error_;
	sucker_args_->foxtrot = foxtrot_;

	return true;
err:
	return false;
}

int
FIX_Popper::pop(size_t * const /*len*/,
		void ** /*data*/)
{
	return 0;
}

int
FIX_Popper::session_pop(const size_t /*len*/,
			void * /*data*/)
{
	return 0;
}

void
FIX_Popper::stop(void)
{
	set_flag(&terminate_, 1);
	if (get_flag(&running_))
		pthread_join(sucker_thread_id_, NULL);
}

void
FIX_Popper::start(void)
{
	set_flag(&terminate_, 0);
	if (get_flag(&running_))
		return;

	if (!create_joinable_thread(&sucker_thread_id_, sucker_args_, sucker_thread_func)) {
		M_ALERT("could not create sucker thread");
		abort();
	}
	set_flag(&running_, 1);
}
