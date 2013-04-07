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
 */

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>

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

/*
 * Reserved initial space for the tags 8, 9, 35 and 34 in the FIX standard
 * header in push buffers.
 */
#define FIX_BUFFER_RESERVED_HEAD (128)

/*
 * Reserved terminal space for the checksum and terminating SOH
 * character.
 */
#define FIX_BUFFER_RESERVED_TAIL (4)

/*
 * The maximum length in bytes for the message type string, tag
 * 35. The terminating null character is not included in the count.
 */
#define MSG_TYPE_MAX_LENGTH (16)

struct pusher_thread_args_t {
	bool flushing;
        int *terminate;
        int *running;
        int *error;
        int sink_fd;
        alfa_io_t *alfa;
        bravo_io_t *bravo;
        charlie_io_t *charlie;
        const char *FIX_start;
        int FIX_start_length;
	char soh;
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
 * msg is a pointer to the first character in a FIX messsage which is
 * included in the checksum calculation. len is the number of bytes
 * included in the checksum calculation.
 */
static inline unsigned int
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
 * OK, this is butt ugly, but I need a fast solution. The maximum
 * value of a uin64_t (18446744073709551615) has 20 digits. 0 (zero)
 * is returned for larger numbers.
 */
static inline int
get_digit_count(const uint64_t num)
{
        if (10 > num)
                return 1;
        if (100 > num)
                return 2;
        if (1000 > num)
                return 3;
        if (10000 > num)
                return 4;
        if (100000 > num)
                return 5;
        if (1000000 > num)
                return 6;
        if (10000000 > num)
                return 7;
        if (100000000 > num)
                return 8;
        if (1000000000 > num)
                return 9;
        if (10000000000 > num)
                return 10;
        if (100000000000 > num)
                return 11;
        if (1000000000000 > num)
                return 12;
        if (10000000000000 > num)
                return 13;
        if (100000000000000 > num)
                return 14;
        if (1000000000000000 > num)
                return 15;
        if (10000000000000000 > num)
                return 16;
        if (100000000000000000 > num)
                return 17;
        if (1000000000000000000 > num)
                return 18;
        if (10000000000000000000ULL > num)
                return 19;
        if (UINT64_MAX >= num)
                return 20;

        return 0;
}

/*
 * Content of alfa, bravo and charlie disruptor entries:
 *
 * - sizeof(uint32_t) bytes holding the length of partial FIX message
 * - Then FIX_BUFFER_RESERVED_HEAD bytes which holds the message type
 *   in the begining and which will later serve as working area when
 *   the complete FIX message is put together.
 *
 * - The partial FIX message begins at: entry + sizeof(uint32_t)
 *   + FIX_BUFFER_RESERVED_HEAD
 *
 * - There is always at least FIX_BUFFER_RESERVED_TAIL bytes left in the end to hold the
 *   checksum and terminating <SOH>.
 */

/*
 * Alfa) Many publishers, one entry processor, 4K entry size, 1024 entries
 *
 */
#define ALFA_QUEUE_LENGTH (1024) // MUST be a power of two
#define ALFA_ENTRY_PROCESSORS (1)
#define ALFA_ENTRY_SIZE (1024*4)
#define ALFA_MAX_DATA_SIZE (ALFA_ENTRY_SIZE - sizeof(uint32_t))
typedef uint8_t alfa_t[ALFA_ENTRY_SIZE];

DEFINE_ENTRY_TYPE(alfa_t, alfa_entry_t);
DEFINE_RING_BUFFER_TYPE(ALFA_ENTRY_PROCESSORS, ALFA_QUEUE_LENGTH, alfa_entry_t, alfa_io_t);
DEFINE_RING_BUFFER_MALLOC(alfa_io_t, alfa_);
DEFINE_RING_BUFFER_INIT(ALFA_QUEUE_LENGTH, alfa_io_t, alfa_);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION(alfa_entry_t, alfa_io_t, alfa_);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION(alfa_entry_t, alfa_io_t, alfa_);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION(alfa_io_t, alfa_);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION(alfa_io_t, alfa_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION(alfa_io_t, alfa_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_NONBLOCKING_FUNCTION(alfa_io_t, alfa_);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION(alfa_io_t, alfa_);
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_BLOCKING_FUNCTION(alfa_io_t, alfa_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(alfa_io_t, alfa_);

/*
 * Bravo) Many publishers, one entry processor,
 * sizeof(size_t)+sizeof(char*) entry size, 128 entries
 */
#define BRAVO_QUEUE_LENGTH (128) // MUST be a power of two
#define BRAVO_ENTRY_PROCESSORS (1)
struct bravo_t {
        size_t size;
        void *data;
};

DEFINE_ENTRY_TYPE(struct bravo_t, bravo_entry_t);
DEFINE_RING_BUFFER_TYPE(BRAVO_ENTRY_PROCESSORS, BRAVO_QUEUE_LENGTH, bravo_entry_t, bravo_io_t);
DEFINE_RING_BUFFER_MALLOC(bravo_io_t, bravo_);
DEFINE_RING_BUFFER_INIT(BRAVO_QUEUE_LENGTH, bravo_io_t, bravo_);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION(bravo_entry_t, bravo_io_t, bravo_);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION(bravo_entry_t, bravo_io_t, bravo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION(bravo_io_t, bravo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION(bravo_io_t, bravo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION(bravo_io_t, bravo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_NONBLOCKING_FUNCTION(bravo_io_t, bravo_);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION(bravo_io_t, bravo_);
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_BLOCKING_FUNCTION(bravo_io_t, bravo_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(bravo_io_t, bravo_);

/*
 * Charlie) One publisher, one entry processor, 512 byte entry size,
 * 512 entries
 *
 */
#define CHARLIE_QUEUE_LENGTH (512) // MUST be a power of two
#define CHARLIE_ENTRY_PROCESSORS (1)
#define CHARLIE_ENTRY_SIZE (512)
#define CHARLIE_MAX_DATA_SIZE (CHARLIE_ENTRY_SIZE - sizeof(uint32_t))
typedef uint8_t charlie_t[CHARLIE_ENTRY_SIZE];

DEFINE_ENTRY_TYPE(charlie_t, charlie_entry_t);
DEFINE_RING_BUFFER_TYPE(CHARLIE_ENTRY_PROCESSORS, CHARLIE_QUEUE_LENGTH, charlie_entry_t, charlie_io_t);
DEFINE_RING_BUFFER_MALLOC(charlie_io_t, charlie_);
DEFINE_RING_BUFFER_INIT(CHARLIE_QUEUE_LENGTH, charlie_io_t, charlie_);
DEFINE_RING_BUFFER_SHOW_ENTRY_FUNCTION(charlie_entry_t, charlie_io_t, charlie_);
DEFINE_RING_BUFFER_ACQUIRE_ENTRY_FUNCTION(charlie_entry_t, charlie_io_t, charlie_);
DEFINE_ENTRY_PROCESSOR_BARRIER_REGISTER_FUNCTION(charlie_io_t, charlie_);
DEFINE_ENTRY_PROCESSOR_BARRIER_UNREGISTER_FUNCTION(charlie_io_t, charlie_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_BLOCKING_FUNCTION(charlie_io_t, charlie_);
DEFINE_ENTRY_PROCESSOR_BARRIER_WAITFOR_NONBLOCKING_FUNCTION(charlie_io_t, charlie_);
DEFINE_ENTRY_PROCESSOR_BARRIER_RELEASEENTRY_FUNCTION(charlie_io_t, charlie_);
DEFINE_ENTRY_PUBLISHERPORT_NEXTENTRY_BLOCKING_FUNCTION(charlie_io_t, charlie_);
DEFINE_ENTRY_PUBLISHERPORT_COMMITENTRY_BLOCKING_FUNCTION(charlie_io_t, charlie_);

/*
 * Returns the sequence number of last transmitted message or 0 if the
 * session is new.
 */
static uint64_t
get_latest_msg_seq_number_sent(void)
{
        return 0;
}

static int
do_writev(int fd,
          size_t total_bytes_to_write,
          size_t len,
          struct iovec *iov)
{
        int err;
        size_t n;
        size_t sum;
        ssize_t written;
        size_t len_offset = 0;
        struct iovec *iov_tmp = iov;

        do {
                written = writev(fd, iov_tmp, len - len_offset);
                if (-1 == written) {
                        switch (errno) {
                        case EAGAIN:
                        case EINTR:
                                continue;
                        default:
                                err = errno;
                                M_ERROR("error writing data: %s", strerror(errno));
                                return err;
                        }
                }
                total_bytes_to_write -= written;
                if (!total_bytes_to_write)
                        break;

                sum = 0;
                for (n = len_offset; n < len; ++n) {
                        sum += iov[n].iov_len;
                        if (sum > (size_t)written)
                                break;
                }
                len_offset = n;
                iov_tmp = &iov[len_offset];
                iov[len_offset].iov_base = (uint8_t*)iov[len_offset].iov_base + (sum - written);
                iov[len_offset].iov_len -= sum - written;
        } while (1);

        return 0;
}

/*
 * This function is ugly, butt ugly, but it has to be as we are trying
 * to build the complete FIX message in-situ whitout any temporary
 * strings and with as little copy as possible,
 *
 * Partial FIX message begins at buffer[FIX_BUFFER_RESERVED_HEAD]. It
 * must not contain the tags:
 *
 * 8 (BeginString), 9 (BodyLength), 35 (MsgType), 34 (MsgSeqNum)
 *
 * It must begin with:
 *   <SOH>
 *
 * And must end with:
 *   <SOH>10=
 *
 * Sample complete FIX message ('|' represents <SOH>):
 *
 * 8=FIX.4.1|9=49|35=0|34=2|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=228
 *
 * data_length is number of bytes in the partial FIX message contained
 * in buffer. On return it will be the total length of the completed
 * FIX message.
 *
 * The partial FIX message in a disruptor buffer begins from and
 * includes the character '|' (which would be <SOH>) immidiately
 * following "8=FIX.4.1|9=49|35=0", up to and including the character
 * '=' immidiately following "|10" at the very end of the FIX sample
 * message.
 *
 * The message length is calculated by counting the number of
 * characters in the message following the BodyLength field ("9=49|"
 * in the FIX sample message) up to, and including, the delimiter
 * immediately preceeding the CheckSum tag ("10=")
 *
 * Parameters:
 *
 * msg_seq_number - previous sequence number to be incremented
 *
 * buffer         - Contains uint32_t data length counted from (buffer +
 *                  FIX_BUFFER_RESERVED_HEAD), then msg type, then
 *                  partial FIX message with enough free space in the
 *                  end to hold the checksum.
 *
 * msg_length     - Output parameter to contain the total length of the
 *                  complete FIX message
 *
 * args           - Thread parameters
 */
static const char*
complete_FIX_message(uint64_t * const msg_seq_number,
                     void * const buffer,
                     size_t *msg_length,
                     const struct pusher_thread_args_t * const args)
{
        size_t body_length;
        int total_prefix_length;
        int body_length_digits;
        int msg_seq_number_digits;
        unsigned int checksum;
	char * const buf = (char*)buffer;

        ++(*msg_seq_number);
        msg_seq_number_digits = get_digit_count(*msg_seq_number);

	// We must construct the prefix string,
	// e.g. "8=FIX.4.1|9=49|35=0". So one thing we must know is
	// the body length. The body length fills a variable amont of
	// characters therefore we must know it beforehand.
        body_length =
                + 3                                /* 35= */
                + strlen((buf + sizeof(uint32_t))) /* msg type */
                + 1                                /* <SOH> */
                + 3                                /* 34= */
                + msg_seq_number_digits            /* digits in sequence number */
		+ *msg_length                      /* length of partial FIX message */
                - 3;                               /* the 3 bytes comprising "10=" in the end of the partial FIX message must not be included in the body length */
        body_length_digits = get_digit_count(body_length);
        total_prefix_length = args->FIX_start_length + body_length_digits + 1  + strlen(buf + sizeof(uint32_t)) + 1 + get_digit_count(*msg_seq_number) + strlen("35=34=");

        // build message
        sprintf((char*)(buf + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD - total_prefix_length), "%s%lu%c35=%s%c34=%llu", args->FIX_start, body_length, args->soh, buf + sizeof(uint32_t), args->soh, *msg_seq_number);
	*(buf + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD) = args->soh;

        // add final checksum
        checksum = get_FIX_checksum(buf + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD - total_prefix_length, total_prefix_length + *msg_length - 3);
        sprintf((buf + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD + *msg_length), "%03u%c", checksum, args->soh);

        // adjust data length to new value
        *msg_length += total_prefix_length + 4; // 4 is CHK<SOH>

        return (buf + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD - total_prefix_length);
}

static int
push_alfa(const bool /* flushing */,
	  struct cursor_t * const alfa_cursor,
          const struct count_t * const alfa_reg_number,
          uint64_t * const msg_seq_number,
          struct pusher_thread_args_t * const args,
          struct iovec * const vdata)
{
        int retv;
        size_t idx;
        size_t total;
        struct cursor_t n;
        struct cursor_t cursor_upper_limit;
        struct alfa_entry_t *alfa_entry;

        cursor_upper_limit.sequence = alfa_cursor->sequence;
        if (alfa_entry_processor_barrier_wait_for_nonblocking(args->alfa, &cursor_upper_limit)) {
                idx = 0;
                total = 0;
                for (n.sequence = alfa_cursor->sequence; n.sequence <= cursor_upper_limit.sequence; ++n.sequence) { // batching
                        alfa_entry = alfa_ring_buffer_acquire_entry(args->alfa, &n);
                        vdata[idx].iov_len = getul(alfa_entry->content); // length of partial message
                        vdata[idx].iov_base = (void*)complete_FIX_message(msg_seq_number, alfa_entry->content, &vdata[idx].iov_len, args);
                        total += vdata[idx].iov_len;

                        ++idx;
                        if (UNLIKELY(IOV_MAX == idx)) {
                                retv = do_writev(args->sink_fd, total, idx, vdata);
                                if (retv) {
                                        M_WARNING("%s", strerror(retv));
                                        return retv; // we don't bother to release the entries as we are shutting down when in error anyways
                                }
                                idx = 0;
                        }
                }
                retv = do_writev(args->sink_fd, total, idx, vdata);
                if (retv) {
                        M_WARNING("%s", strerror(retv));
                        return retv;
                }
                alfa_entry_processor_barrier_release_entry(args->alfa, alfa_reg_number, &cursor_upper_limit);
                alfa_cursor->sequence = ++cursor_upper_limit.sequence;
        }

        return 0;
}

static int
push_bravo(const bool /* flushing */,
	   struct cursor_t * const bravo_cursor,
           const struct count_t * const bravo_reg_number,
           uint64_t * const msg_seq_number,
           struct pusher_thread_args_t * const args,
           struct iovec * const vdata)
{
        int retv;
        size_t idx;
        size_t total;
        struct cursor_t n;
        struct cursor_t cursor_upper_limit;
        struct bravo_entry_t *bravo_entry;

        cursor_upper_limit.sequence = bravo_cursor->sequence;
        if (bravo_entry_processor_barrier_wait_for_nonblocking(args->bravo, &cursor_upper_limit)) {
                idx = 0;
                total = 0;
                for (n.sequence = bravo_cursor->sequence; n.sequence <= cursor_upper_limit.sequence; ++n.sequence) { // batching
                        bravo_entry = bravo_ring_buffer_acquire_entry(args->bravo, &n);

                        vdata[idx].iov_len = bravo_entry->content.size;
                        vdata[idx].iov_base = (void*)complete_FIX_message(msg_seq_number, bravo_entry->content.data, &vdata[idx].iov_len, args);
                        total += vdata[idx].iov_len;

                        ++idx;
                        if (UNLIKELY(IOV_MAX == idx)) {
                                retv = do_writev(args->sink_fd, total, idx, vdata);
                                if (retv) {
                                        M_WARNING("%s", strerror(retv));
                                        return retv; // we don't bother to release the entries as we are shutting down when in error anyways
                                }
                                idx = 0;
                        }
                }
                retv = do_writev(args->sink_fd, total, idx, vdata);
                if (retv) {
                        M_WARNING("%s", strerror(retv));
                        return retv;
                }
                bravo_entry_processor_barrier_release_entry(args->bravo, bravo_reg_number, &cursor_upper_limit);
                bravo_cursor->sequence = ++cursor_upper_limit.sequence;
        }

        return 0;
}

static int
push_charlie(const bool /* flushing */,
	     struct cursor_t * const charlie_cursor,
             const struct count_t * const charlie_reg_number,
             uint64_t * const msg_seq_number,
             struct pusher_thread_args_t * const args,
             struct iovec * const vdata)
{
        int retv;
        size_t idx;
        size_t total;
        struct cursor_t n;
        struct cursor_t cursor_upper_limit;
        struct charlie_entry_t *charlie_entry;

        cursor_upper_limit.sequence = charlie_cursor->sequence;
        if (charlie_entry_processor_barrier_wait_for_nonblocking(args->charlie, &cursor_upper_limit)) {
                idx = 0;
                total = 0;
                for (n.sequence = charlie_cursor->sequence; n.sequence <= cursor_upper_limit.sequence; ++n.sequence) { // batching
                        charlie_entry = charlie_ring_buffer_acquire_entry(args->charlie, &n);

                        vdata[idx].iov_len = getul(charlie_entry->content);
                        vdata[idx].iov_base = (void*)complete_FIX_message(msg_seq_number, charlie_entry->content, &vdata[idx].iov_len, args);
                        total += vdata[idx].iov_len;

                        ++idx;
                        if (UNLIKELY(IOV_MAX == idx)) {
                                retv = do_writev(args->sink_fd, total, idx, vdata);
                                if (retv) {
                                        M_WARNING("%s", strerror(retv));
                                        return retv; // we don't bother to release the entries as we are shutting down when in error anyways
                                }
                                idx = 0;
                        }
                }
                retv = do_writev(args->sink_fd, total, idx, vdata);
                if (retv) {
                        M_WARNING("%s", strerror(retv));
                        return retv;
                }
                charlie_entry_processor_barrier_release_entry(args->charlie, charlie_reg_number, &cursor_upper_limit);
                charlie_cursor->sequence = ++cursor_upper_limit.sequence;
        }

        return 0;
}

/*
 * This function will NOT free arg
 */
void*
pusher_thread_func(void *arg)
{
        int rval;
        uint64_t msg_seq_number;

        struct cursor_t alfa_cursor;
        struct count_t alfa_reg_number;

        struct cursor_t bravo_cursor;
        struct count_t bravo_reg_number;

        struct cursor_t charlie_cursor;
        struct count_t charlie_reg_number;

        struct pusher_thread_args_t *args = (struct pusher_thread_args_t*)arg;
        if (!args) {
                M_ERROR("pusher thread cannot run");
		abort();
        }

        struct iovec *vdata = (struct iovec*)malloc(sizeof(struct iovec)*IOV_MAX);
        if (!vdata) {
                M_ALERT("no memory");
		set_flag(args->error, ENOMEM);
		set_flag(args->running, 0);
                return NULL;
        }

        // Get last message sequence number sent - tag 34.  The
	// sequence number will be incremented whenever a message is
	// sent.
        msg_seq_number = get_latest_msg_seq_number_sent();

        //
        // register entry processors
        //
        alfa_cursor.sequence = alfa_entry_processor_barrier_register(args->alfa, &alfa_reg_number);
        bravo_cursor.sequence = bravo_entry_processor_barrier_register(args->bravo, &bravo_reg_number);
        charlie_cursor.sequence = charlie_entry_processor_barrier_register(args->charlie, &charlie_reg_number);

        // Push data into sink until told to stop. I'm using a
        // do-while because then I can use this function to flush the
        // disruptors as well.
        do {
                // alfa
                rval = push_alfa(args->flushing, &alfa_cursor, &alfa_reg_number, &msg_seq_number, args, vdata);
		if (rval) {
                        set_flag(args->error, rval);
                        goto out;
                }

                // bravo
                rval = push_bravo(args->flushing, &bravo_cursor, &bravo_reg_number, &msg_seq_number, args, vdata);
                if (rval) {
                        set_flag(args->error, rval);
                        goto out;
                }

                // charlie
                rval = push_charlie(args->flushing, &charlie_cursor, &charlie_reg_number, &msg_seq_number, args, vdata);
                if (rval) {
                        set_flag(args->error, rval);
                        goto out;
                }
        } while (!get_flag(args->terminate));
out:
        free(vdata);
        alfa_entry_processor_barrier_unregister(args->alfa, &alfa_reg_number);
        bravo_entry_processor_barrier_unregister(args->bravo, &bravo_reg_number);
        charlie_entry_processor_barrier_unregister(args->charlie, &charlie_reg_number);

        set_flag(args->running, 0);

        return NULL;
}

FIX_Pusher::FIX_Pusher(const char soh)
        : alfa_max_data_length_(ALFA_MAX_DATA_SIZE),
          charlie_max_data_length_(CHARLIE_MAX_DATA_SIZE),
	  soh_(soh)
{
        memset(FIX_start_bytes_, '\0', sizeof(FIX_start_bytes_));
        FIX_start_bytes_length_ = 0;
        sink_fd_ = -1;
	dev_null_ = -1;
        error_ = 0;
        alfa_ = NULL;
        bravo_ = NULL;
        charlie_ = NULL;
	args_ = NULL;
        set_flag(&running_, 0);
        set_flag(&terminate_, 1);
}

bool
FIX_Pusher::init(const char * const FIX_ver, int sink_fd)
{
	stop();

	if (sizeof(FIX_start_bytes_) <= strlen(FIX_ver)) {
                M_ALERT("oversized FIX version: %s (%d)", FIX_ver, sizeof(FIX_start_bytes_));
		goto err;
	}
        if (!FIX_start_bytes_[0] && !FIX_ver) {
                M_ALERT("no FIX version specified");
                goto err;
        }
        snprintf(FIX_start_bytes_, sizeof(FIX_start_bytes_), "8=%s%c9=", FIX_ver, soh_);
        FIX_start_bytes_length_ = strlen(FIX_start_bytes_);

	if (-1 != sink_fd) {
		if (sink_fd_)
			close(sink_fd_);
		sink_fd_ = sink_fd;
	}
	if (-1 == sink_fd_) {
		M_ALERT("no sink file descriptor specified");
		goto err;
	}

	if (-1 == dev_null_) {
		dev_null_ = open("/dev/null", O_WRONLY);
		if (-1 == dev_null_) {
			M_ALERT("could not open /dev/null: %s", strerror(errno));
			goto err;
		}
	}
	
	if (!alfa_) {
		alfa_ = alfa_ring_buffer_malloc();
		if (!alfa_) {
			M_ALERT("no memory");
			goto err;
		}
		alfa_ring_buffer_init(alfa_);
	}

	if (!bravo_) {
		bravo_ = bravo_ring_buffer_malloc();
		if (!bravo_) {
			M_ALERT("no memory");
			goto err;
		}
		bravo_ring_buffer_init(bravo_);
	}

	if (!charlie_) {
		charlie_ = charlie_ring_buffer_malloc();
		if (!charlie_) {
			M_ALERT("no memory");
			goto err;
		}
		charlie_ring_buffer_init(charlie_);
	}

	free(args_);
        args_ = (pusher_thread_args_t*)malloc(sizeof(pusher_thread_args_t));
        if (!args_) {
                M_ALERT("no memory");
                goto err;
        }
	args_->flushing = false;
        args_->terminate = &terminate_;
        args_->running = &running_;
        args_->sink_fd = sink_fd_;
        args_->error = &error_;
        args_->alfa = alfa_;
        args_->bravo = bravo_;
        args_->charlie = charlie_;
        args_->FIX_start = FIX_start_bytes_;
        args_->FIX_start_length = FIX_start_bytes_length_;
	args_->soh = soh_;

        return true;
err:
        return false;
}

int
FIX_Pusher::push(const size_t len,
                 const void * const data,
                 const char * const msg_type)
{
        struct cursor_t alfa_cursor;
        struct alfa_entry_t *alfa_entry;
        struct cursor_t bravo_cursor;
        struct bravo_entry_t *bravo_entry;
        const size_t strl = strnlen(msg_type, FIX_BUFFER_RESERVED_HEAD);

        if (UNLIKELY(MSG_TYPE_MAX_LENGTH < strl))
                return EINVAL;
		
        /* the "- FIX_BUFFER_RESERVED_TAIL" is because we need room for the checksum and the final delimiter */
        if (LIKELY(len <= (alfa_max_data_length_ - sizeof(uint32_t) - FIX_BUFFER_RESERVED_HEAD - FIX_BUFFER_RESERVED_TAIL))) {
                alfa_publisher_port_next_entry_blocking(alfa_, &alfa_cursor);
                alfa_entry = alfa_ring_buffer_acquire_entry(alfa_, &alfa_cursor);

                setul(alfa_entry->content, len);
                strcpy((char*)alfa_entry->content + sizeof(uint32_t), msg_type);
                memcpy(alfa_entry->content + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD, data, len);

                alfa_publisher_port_commit_entry_blocking(alfa_, &alfa_cursor);
        } else {
                bravo_publisher_port_next_entry_blocking(bravo_, &bravo_cursor);
                bravo_entry = bravo_ring_buffer_acquire_entry(bravo_, &bravo_cursor);

                // either the first time and therefore NULL or
                // something allocated from a previous parse. So we
                // reuse or allocate new memory. This migth lead to
                // some interresting memory pressure once it has run
                // for a while...
                if (bravo_entry->content.size >= len + FIX_BUFFER_RESERVED_HEAD + FIX_BUFFER_RESERVED_TAIL) {
                        bravo_entry->content.size = len;
                } else {
                        free(bravo_entry->content.data);
                        bravo_entry->content.size = len;
                        bravo_entry->content.data = malloc(len + FIX_BUFFER_RESERVED_HEAD + FIX_BUFFER_RESERVED_TAIL); // for "+ FIX_BUFFER_RESERVED_TAIL" see above
                }
                strcpy((char*)bravo_entry->content.data, msg_type);
                memcpy((char*)bravo_entry->content.data + FIX_BUFFER_RESERVED_HEAD, data, len);

                bravo_publisher_port_commit_entry_blocking(bravo_, &bravo_cursor);
        }

	return get_flag(&error_);
}

int
FIX_Pusher::session_push(const size_t len,
                         const void * const data,
                         const char * const msg_type)
{
        struct cursor_t charlie_cursor;
        struct charlie_entry_t *charlie_entry;
        const size_t strl = strnlen(msg_type, FIX_BUFFER_RESERVED_HEAD);

        if (UNLIKELY(MSG_TYPE_MAX_LENGTH < strl))
                return EINVAL;

        /* the "- FIX_BUFFER_RESERVED_TAIL" is because we need room for the checksum and the final delimiter */
        if (UNLIKELY(len > (charlie_max_data_length_ - FIX_BUFFER_RESERVED_HEAD - FIX_BUFFER_RESERVED_TAIL))) {
                M_CRITICAL("session message oversized");
        } else {
                charlie_publisher_port_next_entry_blocking(charlie_, &charlie_cursor);
                charlie_entry = charlie_ring_buffer_acquire_entry(charlie_, &charlie_cursor);

                setul(charlie_entry->content, len);
                strcpy((char*)charlie_entry->content + sizeof(uint32_t), msg_type);
                memcpy(charlie_entry->content + sizeof(uint32_t) + FIX_BUFFER_RESERVED_HEAD, data, len);

                charlie_publisher_port_commit_entry_blocking(charlie_, &charlie_cursor);
        }

	return get_flag(&error_);
}

void
FIX_Pusher::flush(void)
{
        int n;
	int fd;

	stop();

	fd = sink_fd_;
	sink_fd_ = dev_null_;
	args_->flushing = true;
	pusher_thread_func(args_);
	args_->flushing = false;

	for (n = 0; n < BRAVO_QUEUE_LENGTH; ++n) {
		free(bravo_->buffer[n].content.data);
		bravo_->buffer[n].content.size = 0;
		bravo_->buffer[n].content.data = NULL;
	}
	sink_fd_ = fd;

	start();
}

void
FIX_Pusher::stop(void)
{
        set_flag(&terminate_, 1);
        if (get_flag(&running_))
                pthread_join(pusher_thread_id_, NULL);
}

void
FIX_Pusher::start(void)
{
        set_flag(&terminate_, 0);
        if (get_flag(&running_))
		return;

        if (!create_joinable_thread(&pusher_thread_id_, args_, pusher_thread_func)) {
                M_ALERT("could not create pusher thread");
                abort();
        }
        set_flag(&running_, 1);
}

