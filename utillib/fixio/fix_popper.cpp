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
 *                           Reads in turn from fast, slow and session disruptor and writes to sink <== start()/stop() invokes here
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
 *             Reads data and extract complete messages one at a time. Writes one message to one entry in the
 *                 fast disruptor or one message to the fast disruptor if the message does n't fit in the
 *                    fast disruptor. Session messages always goes to the session disruptor.
 *
 *
 *                                              (Foxtrot) BLOB disruptor
 *                                with entries of a fair size holding data as it arrives <== start()/stop() invokes here
 *
 *
 *                                 File descriptor data source - Data flows up from here
 *
 */

/*
 * Delta) One publisher, many entry processors (but only one registered), sizeof(size_t)+sizeof(char*) entry size, 512 entries
 *
 * Echo) One publisher, one entry processor, 512 byte entry size, 512 entries
 *
 * Foxtrot) One publisher, one entry processor, 8K entry size, 512 entries
 */

#include <ctype.h>
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
#include "stdlib/network/network.h"
#include "stdlib/log/log.h"
#include "applib/fixlib/defines.h"
#include "utillib/fixio/stack_utils.h"
#include "utillib/fixio/fixio.h"

// takes messages from foxtrot and puts them onto delta and echo as
// appropriate.
struct splitter_thread_args_t {
        int *pause_thread;
        int *db_is_open;
        MsgDB * db;
        delta_io_t *delta;
        echo_io_t *echo;
        foxtrot_io_t *foxtrot;
        char *begin_string;
        int *begin_string_length;
        char soh;
};

// takes incomming data from source_fd_ and puts it onto foxtrot
struct sucker_thread_args_t {
        int *pause_thread;
        int *sucker_is_running;
        int *error;
        int *source_fd;
        foxtrot_io_t *foxtrot;
};

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
 * Delta) One publisher, many entry processors but we need to
 * serialize access to delta, so effectively we only have one. At
 * least for now. Alternatively we need poppers to maintain a
 * registration number and a counter themselves...
 *
 * TODO: Test what the fastest approah is. I'll implement the
 * appropiate pop() method so that I'll have it ready when
 * performance testing commences.
 *
 * sizeof(size_t)+sizeof(char*) entry size, 128 entries
 *
 */
#define DELTA_QUEUE_LENGTH (128) // MUST be a power of two
#define DELTA_ENTRY_PROCESSORS (16) // maybe more later
struct delta_t {
        size_t size;
        uint8_t *data;
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
#define FOXTROT_ENTRY_SIZE (1024*4) // if changing, please check the test_FIX_challenge_buffer_boundaries_*
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
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_NONBLOCKING_FUNCTION(foxtrot_io_t, foxtrot_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(foxtrot_io_t, foxtrot_);

/*********************
 *      FIX_Pusher
 *********************/

/*
 * msg_type is terminated with soh, not zero as normal c-strings
 */
static inline int
is_session_message(const char /*soh*/,
                   const uint8_t * const msg_type)
{
        if ((uint8_t)'0' == *msg_type) {
                return 1;
        }

        return 0;
}

/*
 * This will actually accept sequence numbers in the form of:
 *
 * "<SOH>34=134 hg utf<SOH>".
 *
 * I'm going to let this one pass for the sake of simpler code.
 */
static inline uint64_t
get_sequence_number(const char soh,
                    const uint64_t len,
                    uint8_t * const msg)
{
        static char segnum_str[5] = " 34=";
        char *end;
        char *start;
        uint64_t retv;

        segnum_str[0] = soh;
        start = (char*)memmem(msg, len, segnum_str, sizeof(segnum_str) - 1);
        if (!start)
                return 0;
        start += 4; // we know this is safe as this function is called after we have verified the checksum
        end = start;

        do {
                if (soh == *end)
                        break;

                if (!isdigit(*end))
                        break;

                ++end;
        } while (1);
        *end = '\0';

        retv = strtoull(start, NULL, 10);
        *end = soh;

        return (ULLONG_MAX != retv ? retv : 0);
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
        char chksum[4];
        int l;
        size_t offset;
        uint32_t k;
        uint32_t entry_length;
        uint32_t body_length;
        uint32_t bytes_left_to_copy = 0;
        char length_str[32] = { '\0' };
        const uint8_t *msg_type;
        uint64_t msg_seq_number_expected;
        uint64_t msg_seq_number_recieved;
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

        set_flag(args->db_is_open, 0);
        while (get_flag(args->pause_thread))
                sched_yield();
        if (!args->db->open()) {
                M_ERROR("could not open local database");
                abort();
        }
        set_flag(args->db_is_open, 1);

        // Get last message sequence number recieved - tag 34.  The
        // sequence number will be incremented whenever a message is
        // recieved and varified (by checksum and FIX version)
        if (!args->db->get_latest_recv_seqnum(msg_seq_number_expected)) {
                M_ALERT("error getting latest recieved sequence number");
                abort();
        }

        //
        // register entry processor for foxtrot
        //
        foxtrot_cursor.sequence = foxtrot_entry_processor_barrier_register(args->foxtrot, &foxtrot_reg_number);
        cursor_upper_limit.sequence = foxtrot_cursor.sequence;

        // acquire publisher entries
        delta_publisher_port_next_entry_blocking(args->delta, &delta_cursor);
        delta_entry = delta_ring_buffer_acquire_entry(args->delta, &delta_cursor);
        echo_publisher_port_next_entry_blocking(args->echo, &echo_cursor);
        echo_entry = echo_ring_buffer_acquire_entry(args->echo, &echo_cursor);

        // filter available data to echo and delta forever and ever
        l = 0;
        offset = 0;
        state = FindingBeginString;
        do {
                if (UNLIKELY(get_flag(args->pause_thread))) {
                        if (!args->db->close()) {
                                M_ERROR("could not close local database");
                                continue;
                        }
                        set_flag(args->db_is_open, 0);

                        do {
                                sched_yield();
                        } while (get_flag(args->pause_thread));

                        if (!args->db->open()) {
                                M_ERROR("could not open local database");
                                abort();
                        }
                        set_flag(args->db_is_open, 1);
                }

                if (!foxtrot_entry_processor_barrier_wait_for_nonblocking(args->foxtrot, &cursor_upper_limit))
                        continue;

                for (n.sequence = foxtrot_cursor.sequence; n.sequence <= cursor_upper_limit.sequence; ++n.sequence) { // batching
                        foxtrot_entry = foxtrot_ring_buffer_show_entry(args->foxtrot, &n);

                        entry_length = getul(foxtrot_entry->content);
                        for (k = 0; k < entry_length; ++k) {
                                switch (state) {
                                case FindingBeginString:
                                        if (args->begin_string[l] == *(foxtrot_entry->content + sizeof(uint32_t) + k)) {
                                                ++l;
                                                continue;
                                        } else {
                                                if (!args->begin_string[l] && isdigit(*(foxtrot_entry->content + sizeof(uint32_t) + k))) {
                                                        l = 0;
                                                        state = FindingBodyLength;
                                                } else {
                                                        if (args->begin_string[0] == *(foxtrot_entry->content + sizeof(uint32_t) + k)) {
                                                                l = 1;
                                                                continue;
                                                        }
                                                        l = 0;
                                                        continue;
                                                }
                                        }
                                case FindingBodyLength:
                                        if (21 == l) {
                                                l = 0;
                                                state = FindingBeginString; // not a valid number, skip this message
                                                continue;
                                        }
                                        length_str[l] = *(foxtrot_entry->content + sizeof(uint32_t) + k);
                                        if (!isdigit(length_str[l]) && (args->soh != length_str[l])) {
                                                state = FindingBeginString; // not a valid number, skip this message
                                                l = 0;
                                                continue;
                                        }
                                        if (args->soh != length_str[l++]) // for 64bit systems this is safe as the largest number of digits is 20
                                                continue;
                                        length_str[--l] = '\0';
                                        l = 0;
                                        body_length = atoll(length_str);
                                        if (!body_length || (ERANGE == errno)) {
                                                state = FindingBeginString; // not a valid number, skip this message
                                                continue;
                                        }
                                        bytes_left_to_copy = body_length + 1 + 7; // we need to copy the soh_ following the BodyLength field (it isn't included in the field value) and the CheckSum plus ending soh_
                                        if (delta_entry->content.size < *args->begin_string_length + strlen(length_str) + bytes_left_to_copy) {
                                                free(delta_entry->content.data);
                                                delta_entry->content.size = *args->begin_string_length + strlen(length_str) + bytes_left_to_copy;
                                                delta_entry->content.data = (uint8_t*)malloc(delta_entry->content.size);
                                                if (!delta_entry->content.data) {
                                                        M_ALERT("no memory");
                                                        state = FindingBeginString; // skip this message and hope for better memory conditions later
                                                        continue;
                                                }
                                        } else {
                                                delta_entry->content.size = *args->begin_string_length + strlen(length_str) + bytes_left_to_copy;
                                        }
                                        memcpy(delta_entry->content.data, args->begin_string, *args->begin_string_length); // 8=FIX.X.Y<SOH>9=
                                        memcpy(delta_entry->content.data + *args->begin_string_length, length_str, strlen(length_str)); // <LENGTH>
                                        offset = *args->begin_string_length + strlen(length_str);
                                        state = CopyingBody;
                                case CopyingBody:
                                        if (entry_length - k >= bytes_left_to_copy) { // one memcpy enough
                                                memcpy(delta_entry->content.data + offset,
                                                       foxtrot_entry->content + sizeof(uint32_t) + k,
                                                       bytes_left_to_copy); // <SOH>ya-da ya-da<SOH>10=ABC<SOH>
                                                sprintf(chksum, "%03u", get_FIX_checksum(delta_entry->content.data, delta_entry->content.size - 7));
                                                if (!memcmp(delta_entry->content.data + delta_entry->content.size - 4, chksum, 3)) { // validate checksum
                                                        msg_type = delta_entry->content.data + *args->begin_string_length + strlen(length_str) + 4;
                                                        msg_seq_number_recieved = get_sequence_number(args->soh, delta_entry->content.size, delta_entry->content.data);
                                                        if (msg_seq_number_recieved != ++msg_seq_number_expected) { // must be equal to the recieved number
                                                                M_ALERT("wrong sequence number: %d != %d", msg_seq_number_recieved, msg_seq_number_expected);
                                                                --msg_seq_number_expected;
                                                        } else {
                                                                if (is_session_message(args->soh, msg_type)) {
                                                                        if (ECHO_MAX_DATA_SIZE < delta_entry->content.size) {
                                                                                M_ALERT("oversized session message");
                                                                                --msg_seq_number_expected;
                                                                        } else {
                                                                                args->db->store_recv_msg(msg_seq_number_recieved, delta_entry->content.size, delta_entry->content.data);
                                                                                setul(echo_entry->content, delta_entry->content.size);
                                                                                memcpy(echo_entry->content + sizeof(uint32_t), delta_entry->content.data, delta_entry->content.size);
                                                                                echo_publisher_port_commit_entry_blocking(args->echo, &echo_cursor);
                                                                                echo_publisher_port_next_entry_blocking(args->echo, &echo_cursor);
                                                                                echo_entry = echo_ring_buffer_acquire_entry(args->echo, &echo_cursor);
                                                                        }
                                                                } else {
                                                                        args->db->store_recv_msg(msg_seq_number_recieved, delta_entry->content.size, delta_entry->content.data);
                                                                        delta_publisher_port_commit_entry_blocking(args->delta, &delta_cursor);
                                                                        delta_publisher_port_next_entry_blocking(args->delta, &delta_cursor);
                                                                        delta_entry = delta_ring_buffer_acquire_entry(args->delta, &delta_cursor);
                                                                }
                                                        }
                                                }
                                                state = FindingBeginString;
                                                k += bytes_left_to_copy - 1;
                                        } else {
                                                memcpy(delta_entry->content.data + offset,
                                                       foxtrot_entry->content + sizeof(uint32_t) + k,
                                                       entry_length - k); // <SOH>ya-da ya-da<SOH>10=ABC<SOH>
                                                bytes_left_to_copy -= entry_length - k;
                                                offset += entry_length - k;
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
                foxtrot_cursor.sequence = ++cursor_upper_limit.sequence;
        } while (1);
        foxtrot_entry_processor_barrier_unregister(args->foxtrot, &foxtrot_reg_number);

        return NULL;
}

void*
sucker_thread_func(void *arg)
{
        void *buf;
        ssize_t rval;
        int entry_open = 0;
        const timeout_t timeout = { 1 };
        struct cursor_t foxtrot_cursor;
        struct foxtrot_entry_t *foxtrot_entry = NULL;

        struct sucker_thread_args_t *args = (struct sucker_thread_args_t*)arg;
        if (!args) {
                M_ERROR("sucker thread cannot run (no parameters)");
                abort();
        }

        // wait for start
        while (get_flag(args->pause_thread))
                sched_yield();

        if (!set_recv_timeout(*args->source_fd, timeout)) {
                M_ERROR("sucker thread cannot run (cannot set timeout)");
                abort();
        }

        // pull data from source_fd onto foxtrot until told to stop
        set_flag(args->sucker_is_running, 1);
        do {
                if (UNLIKELY(get_flag(args->pause_thread))) {
                        set_flag(args->sucker_is_running, 0);
                        do {
                                sched_yield();
                        } while (get_flag(args->pause_thread));
                        set_flag(args->sucker_is_running, 1);
                }

                if (!foxtrot_publisher_port_next_entry_nonblocking(args->foxtrot, &foxtrot_cursor))
                        continue;

                entry_open = 1;
                foxtrot_entry = foxtrot_ring_buffer_acquire_entry(args->foxtrot, &foxtrot_cursor);
                buf = foxtrot_entry->content + sizeof(uint32_t);
        again:
                rval = recvfrom(*args->source_fd, buf, FOXTROT_MAX_DATA_SIZE, 0, NULL, NULL);
                switch (rval) {
                case 0:
                        M_ERROR("peer closed connection");
                        goto out;
                case -1:
                        switch (errno) {
                        case ETIMEDOUT:
                        case EAGAIN:
                        case EINTR:
                                if (UNLIKELY(get_flag(args->pause_thread))) {
                                        set_flag(args->sucker_is_running, 0);
                                        do {
                                                sched_yield();
                                        } while (get_flag(args->pause_thread));
                                        set_flag(args->sucker_is_running, 1);
                                }
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
                entry_open = 0;
        } while (1);
out:
        if (entry_open) {
                setul(foxtrot_entry->content, 0);
                foxtrot_publisher_port_commit_entry_blocking(args->foxtrot, &foxtrot_cursor);
        }

        return NULL;
}


FIX_Popper::FIX_Popper(const char soh)
        : echo_max_data_length_(ECHO_MAX_DATA_SIZE),
          foxtrot_max_data_length_(FOXTROT_MAX_DATA_SIZE),
          soh_(soh)
{
        source_fd_ = -1;
        memset(begin_string_, '\0', sizeof(begin_string_));
        begin_string_length_ = 0;
        error_ = 0;
        delta_ = NULL;
        echo_ = NULL;
        foxtrot_ = NULL;
        splitter_args_ = NULL;
        sucker_args_ = NULL;
        pause_threads_ = 1;
        db_is_open_ = 0;
        sucker_is_running_ = 0;
        started_ = 0;
}

int
FIX_Popper::init(void)
{
        stop();

        if (!delta_) {
                delta_ = delta_ring_buffer_malloc();
                if (!delta_) {
                        M_ALERT("no memory");
                        goto err;
                }
                delta_ring_buffer_init(delta_);

                // register and setup single entry processor for pop()
                delta_n_.sequence = delta_entry_processor_barrier_register(delta_, &delta_reg_number_);
                delta_cursor_upper_limit_.sequence = delta_n_.sequence;
        }

        if (!echo_) {
                echo_ = echo_ring_buffer_malloc();
                if (!echo_) {
                        M_ALERT("no memory");
                        goto err;
                }
                echo_ring_buffer_init(echo_);

                // register and setup single entry processor for session_pop()
                echo_n_.sequence = echo_entry_processor_barrier_register(echo_, &echo_reg_number_);
                echo_cursor_upper_limit_.sequence = echo_n_.sequence;
        }

        if (!foxtrot_) {
                foxtrot_ = foxtrot_ring_buffer_malloc();
                if (!foxtrot_) {
                        M_ALERT("no memory");
                        goto err;
                }
                foxtrot_ring_buffer_init(foxtrot_);
        }

        if (!splitter_args_) {
                splitter_args_ = (splitter_thread_args_t*)malloc(sizeof(struct splitter_thread_args_t));
                if (!splitter_args_) {
                        M_ALERT("no memory");
                        goto err;
                }
                splitter_args_->pause_thread = &pause_threads_;
                splitter_args_->db_is_open = &db_is_open_;
                splitter_args_->db = &db_;
                splitter_args_->begin_string = begin_string_;
                splitter_args_->begin_string_length = &begin_string_length_;
                splitter_args_->delta = delta_;
                splitter_args_->echo = echo_;
                splitter_args_->foxtrot = foxtrot_;
                splitter_args_->soh = soh_;

                pthread_t splitter_thread_id;
                if (!create_detached_thread(&splitter_thread_id, splitter_args_, splitter_thread_func)) {
                        M_ALERT("could not create filter thread");
                        goto err;
                }
        }

        if (!sucker_args_) {
                sucker_args_ = (sucker_thread_args_t*)malloc(sizeof(struct sucker_thread_args_t));
                if (!sucker_args_) {
                        M_ALERT("no memory");
                        goto err;
                }
                sucker_args_->pause_thread = &pause_threads_;
                sucker_args_->sucker_is_running = &sucker_is_running_;
                sucker_args_->source_fd = &source_fd_;
                sucker_args_->error = &error_;
                sucker_args_->foxtrot = foxtrot_;

                pthread_t sucker_thread_id;
                if (!create_detached_thread(&sucker_thread_id, sucker_args_, sucker_thread_func)) {
                        M_ALERT("could not create sucker thread");
                        goto err;
                }
        }

        return 1;
err:
        return 0;
}

int
FIX_Popper::pop(size_t * const len,
                uint8_t **data)
{
        const struct delta_entry_t *delta_entry;
        struct cursor_t n;

        if (guard_.enter()) {
                M_ALERT("could not lock");
                return 1;
        }

        n.sequence = __atomic_fetch_add(&delta_n_.sequence, 1, __ATOMIC_RELEASE);
        if (n.sequence == delta_cursor_upper_limit_.sequence) {
                delta_entry_processor_barrier_wait_for_blocking(delta_, &delta_cursor_upper_limit_);
                __atomic_fetch_add(&delta_cursor_upper_limit_.sequence, 1, __ATOMIC_RELEASE);
        }
        delta_entry = delta_ring_buffer_show_entry(delta_, &n);

        *len = delta_entry->content.size;
        *data = delta_entry->content.data;
        delta_entry_processor_barrier_release_entry(delta_, &delta_reg_number_, &n);

        guard_.leave();

        return 0;
}

int
FIX_Popper::pop(struct cursor_t * const /*cursor*/,
                struct count_t * const /*reg_number*/,
                size_t * const /*len*/,
                void **/*data*/)
{
        return 0;
}

void
FIX_Popper::register_popper(struct cursor_t * const /*cursor*/,
                            struct count_t * const /*reg_number*/)
{
}

/*
 * Only called from a single thread, so no need to use atomic builtins
 */
void
FIX_Popper::session_pop(size_t * const len,
                        uint8_t **data)
{
        struct echo_entry_t *echo_entry;
        struct cursor_t n;

        n.sequence = echo_n_.sequence - 1;
        echo_entry_processor_barrier_release_entry(echo_, &echo_reg_number_, &n);

        n.sequence = echo_n_.sequence++;
        if (n.sequence == echo_cursor_upper_limit_.sequence) {
                echo_entry_processor_barrier_wait_for_blocking(echo_, &echo_cursor_upper_limit_);
                ++echo_cursor_upper_limit_.sequence;
        }
        echo_entry = echo_ring_buffer_acquire_entry(echo_, &n);

        *len = getul(echo_entry->content);
        *data = echo_entry->content + sizeof(uint32_t);
}

int
FIX_Popper::start(const char * const local_cache,
                  const char * const FIX_ver,
                  int source_fd)
{
        if (get_flag(&started_)) {
		if (local_cache || FIX_ver || (0 <= source_fd)) {
			M_ALERT("attempt to change settings while pusher is started");
			return 0;
		}
                return 1;
	}

        if (FIX_ver) {
                if (sizeof(begin_string_) <= strlen(FIX_ver)) {
                        M_ALERT("oversized FIX version: %s (%d)", FIX_ver, sizeof(begin_string_));
                        goto out;
                }
                snprintf(begin_string_, sizeof(begin_string_), "8=%s%c9=", FIX_ver, soh_);
                begin_string_length_ = strlen(begin_string_);
        }
        if (!begin_string_[0]) {
                M_ALERT("no FIX version specified");
                goto out;
        }

        if (0 <= source_fd) {
                if (0 <= source_fd_)
                        close(source_fd_);
                source_fd_ = source_fd;
        }
        if (0 > source_fd_) {
                M_ALERT("no source file descriptor specified");
                goto out;
        }

        if (local_cache) {
                if (!db_.set_db_path(local_cache)) {
                        M_ALERT("could not set local database path");
                        goto out;
                }
        }

        set_flag(&pause_threads_, 0);
        while (!get_flag(&db_is_open_)) {
                sched_yield();
        }
        while (!get_flag(&sucker_is_running_)) {
                sched_yield();
        }

        set_flag(&started_, 1);
out:
        return get_flag(&started_);
}

int
FIX_Popper::stop(void)
{
        if (!get_flag(&started_))
                return 1;

        set_flag(&pause_threads_, 1);
        while (get_flag(&db_is_open_)) {
                sched_yield();
        }
        while (get_flag(&sucker_is_running_)) {
                sched_yield();
        }

        set_flag(&started_, 0);

        return !get_flag(&started_);
}
