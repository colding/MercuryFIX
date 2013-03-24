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

struct pusher_thread_args {
        int *terminate;
        int *running;
        int *error;
        int sink_fd;
        alfa_io_t *alfa;
        bravo_io_t *bravo;
        charlie_io_t *charlie;
        const char *FIX_start;
        int FIX_start_length;
};

static inline void
set_flag(int *flag, int val)
{
        __atomic_store_n(flag, val, __ATOMIC_SEQ_CST);
}

static inline int
get_flag(int *flag)
{
        return __atomic_load_n(flag, __ATOMIC_SEQ_CST);
}

/*
 * msg is a pointer to the first character in a FIX messsage which is
 * included in the checksum calculation. len is the number of bytes
 * included in the checksum calculation.
 */
static inline int
get_FIX_checksum(char *msg, size_t len)
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
 * FIX sample messages:
 *
 * 8=FIX.4.19=6135=A34=149=EXEC52=20121105-23:24:0656=BANZAI98=0108=3010=003
 * 8=FIX.4.19=6135=A34=149=BANZAI52=20121105-23:24:0656=EXEC98=0108=3010=003
 * 8=FIX.4.19=4935=034=249=BANZAI52=20121105-23:24:3756=EXEC10=228
 * 8=FIX.4.19=4935=034=249=EXEC52=20121105-23:24:3756=BANZAI10=228
 * 8=FIX.4.19=10335=D34=349=BANZAI52=20121105-23:24:4256=EXEC11=135215788257721=138=1000040=154=155=MSFT59=010=062
 * 8=FIX.4.19=13935=834=349=EXEC52=20121105-23:24:4256=BANZAI6=011=135215788257714=017=120=031=032=037=138=1000039=054=155=MSFT150=2151=010=059
 * 8=FIX.4.19=15335=834=449=EXEC52=20121105-23:24:4256=BANZAI6=12.311=135215788257714=1000017=220=031=12.332=1000037=238=1000039=254=155=MSFT150=2151=010=230
 * 8=FIX.4.19=10335=D34=449=BANZAI52=20121105-23:24:5556=EXEC11=135215789503221=138=1000040=154=155=ORCL59=010=047
 * 8=FIX.4.19=13935=834=549=EXEC52=20121105-23:24:5556=BANZAI6=011=135215789503214=017=320=031=032=037=338=1000039=054=155=ORCL150=2151=010=049
 * 8=FIX.4.19=15335=834=649=EXEC52=20121105-23:24:5556=BANZAI6=12.311=135215789503214=1000017=420=031=12.332=1000037=438=1000039=254=155=ORCL150=2151=010=220
 * 8=FIX.4.19=10835=D34=549=BANZAI52=20121105-23:25:1256=EXEC11=135215791235721=138=1000040=244=1054=155=SPY59=010=003
 * 8=FIX.4.19=13835=834=749=EXEC52=20121105-23:25:1256=BANZAI6=011=135215791235714=017=520=031=032=037=538=1000039=054=155=SPY150=2151=010=252
 * 8=FIX.4.19=10435=F34=649=BANZAI52=20121105-23:25:1656=EXEC11=135215791643738=1000041=135215791235754=155=SPY10=198
 * 8=FIX.4.19=8235=334=849=EXEC52=20121105-23:25:1656=BANZAI45=658=Unsupported message type10=000
 * 8=FIX.4.19=10435=F34=749=BANZAI52=20121105-23:25:2556=EXEC11=135215792530938=1000041=135215791235754=155=SPY10=197
 * 8=FIX.4.19=8235=334=949=EXEC52=20121105-23:25:2556=BANZAI45=758=Unsupported message type10=002
 */

/*
 * Returns the sequence number of last recieved message or 0 if the
 * session is new.
 */
static uint64_t
get_msg_seq_number(void)
{
        return 0;
}

void*
popper_thread_func(void *arg)
{
        int rval;
        uint64_t msg_seq_number;

        struct cursor_t alfa_cursor;
        struct count_t alfa_reg_number;

        struct cursor_t bravo_cursor;
        struct count_t bravo_reg_number;

        struct cursor_t charlie_cursor;
        struct count_t charlie_reg_number;

        struct pusher_thread_args *args = (struct pusher_thread_args*)arg;
        if (!args) {
                M_ERROR("pusher thread cannot run");
		set_flag(args->error, ENOMEM);
                return NULL;
        }

        struct iovec *vdata = (struct iovec*)malloc(sizeof(struct iovec)*IOV_MAX);
        if (!vdata) {
                free(args);
                M_ALERT("no memory");
		set_flag(args->error, ENOMEM);
                return NULL;
        }

        // Get last message sequence number sent - tag 34.  The
	// sequence numvber will be incremented whenever a message is
	// sent.
        msg_seq_number = get_msg_seq_number();

        //
        // register entry processors
        //
        alfa_cursor.sequence = alfa_entry_processor_barrier_register(args->alfa, &alfa_reg_number);
        bravo_cursor.sequence = bravo_entry_processor_barrier_register(args->bravo, &bravo_reg_number);
        charlie_cursor.sequence = charlie_entry_processor_barrier_register(args->charlie, &charlie_reg_number);

        // push data into sink until told to stop
        while (!get_flag(args->terminate)) {
                // alfa
                rval = push_alfa(&alfa_cursor, &alfa_reg_number, &msg_seq_number, args, vdata);
                if (rval) {
                        set_flag(args->error, rval);
                        goto out;
                }

                // bravo
                rval = push_bravo(&bravo_cursor, &bravo_reg_number, &msg_seq_number, args, vdata);
                if (rval) {
                        set_flag(args->error, rval);
                        goto out;
                }

                // charlie
                rval = push_charlie(&charlie_cursor, &charlie_reg_number, &msg_seq_number, args, vdata);
                if (rval) {
                        set_flag(args->error, rval);
                        goto out;
                }
        }
out:
        free(vdata);
        free(args);
        alfa_entry_processor_barrier_unregister(args->alfa, &alfa_reg_number);
        bravo_entry_processor_barrier_unregister(args->bravo, &bravo_reg_number);
        charlie_entry_processor_barrier_unregister(args->charlie, &charlie_reg_number);

        set_flag(args->running, 0);

        return NULL;
}

FIX_Popper::FIX_Popper(const int source_fd)
        : echo_max_data_length_(ECHO_MAX_DATA_SIZE),
          foxtrot_max_data_length_(FOXTROT_MAX_DATA_SIZE)
{
        source_fd_ = source_fd;
}

FIX_Popper::~FIX_Popper()
{
	int n;

	free(echo_);
	free(foxtrot_);
	for (n = 0; n < DELTA_QUEUE_LENGTH; ++n) 
		free(delta_->buffer[n].content.data);
	free(delta_);

	if (-1 != source_fd_)
		close(source_fd_);
}

bool
FIX_Popper::init(void)
{
        delta_ = delta_ring_buffer_malloc();
        if (!delta_)
                return false;

        echo_ = echo_ring_buffer_malloc();
        if (!echo_)
                return false;

        foxtrot_ = foxtrot_ring_buffer_malloc();
        if (!foxtrot_)
                return false;

        delta_ring_buffer_init(delta_);
        echo_ring_buffer_init(echo_);
        foxtrot_ring_buffer_init(foxtrot_);

        return true;
}

int
FIX_Popper::pop(const size_t /*len*/,
                void * const /*data*/)
{
	return 0;
}

int
FIX_Popper::session_pop(const size_t /*len*/,
                        void * const /*data*/)
{
	return 0;
}
