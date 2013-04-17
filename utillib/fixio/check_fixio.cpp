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

#include <sys/socket.h>
#include <stdio.h>
#include <check.h>
#include <fcntl.h>
#include <new>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "utillib/fixio/fixio.h"
#include "stdlib/log/log.h"
#include "stdlib/network/network.h"

/*
 * Valid FIX sample messages with the real SOH:
 *
 * 8=FIX.4.19=6135=A34=149=EXEC52=20121105-23:24:0656=BANZAI98=0108=3010=003
 * 8=FIX.4.19=6135=A34=149=BANZAI52=20121105-23:24:0656=EXEC98=0108=3010=003
 * 8=FIX.4.19=4935=034=249=BANZAI52=20121105-23:24:3756=EXEC10=228
 * 8=FIX.4.19=4935=034=249=EXEC52=20121105-23:24:3756=BANZAI10=228
 * 8=FIX.4.19=10335=D34=349=BANZAI52=20121105-23:24:4256=EXEC11=135215788257721=138=1000040=154=155=MSFT59=010=062
 * 8=FIX.4.19=13935=834=349=EXEC52=20121105-23:24:4256=BANZAI6=011=135215788257714=017=120=031=032=037=138=1000039=054=155=MSFT150=2151=010=059
 * 8=FIX.4.19=15335=834=449=EXEC52=20121105-23:24:4256=BANZAI6=12.311=135215788257714=1000017=220=031=12.332=1000037=238=1000039=254=155=MSFT150=2151=010=230
 * 8=FIX.4.19=10335=D34=449=BANZAI52=20121105-23:24:5556=EXEC11=135215789503221=138=1000040=154=155=ORCL59=010=047
 * 8=FIX.4.19=13935=834=549=EXEC52=20121105-23:24:5556=BANZAI6=011=135215789503214=017=320=031=032=037=338=1000039=054=155=ORCL150=2151=010=049
 * 8=FIX.4.19=15335=834=649=EXEC52=20121105-23:24:5556=BANZAI6=12.311=135215789503214=1000017=420=031=12.332=1000037=438=1000039=254=155=ORCL150=2151=010=220
 * 8=FIX.4.19=10835=D34=549=BANZAI52=20121105-23:25:1256=EXEC11=135215791235721=138=1000040=244=1054=155=SPY59=010=003
 * 8=FIX.4.19=13835=834=749=EXEC52=20121105-23:25:1256=BANZAI6=011=135215791235714=017=520=031=032=037=538=1000039=054=155=SPY150=2151=010=252
 * 8=FIX.4.19=10435=F34=649=BANZAI52=20121105-23:25:1656=EXEC11=135215791643738=1000041=135215791235754=155=SPY10=198
 * 8=FIX.4.19=8235=334=849=EXEC52=20121105-23:25:1656=BANZAI45=658=Unsupported message type10=000
 * 8=FIX.4.19=10435=F34=749=BANZAI52=20121105-23:25:2556=EXEC11=135215792530938=1000041=135215791235754=155=SPY10=197
 * 8=FIX.4.19=8235=334=949=EXEC52=20121105-23:25:2556=BANZAI45=758=Unsupported message type10=002
 */

#define DELIM '|'

/*
 * Valid complete messages
 */
const char *complete_messages[16] = {
        "8=FIX.4.1|9=61|35=8|34=1|49=EXEC|52=20121105-23:24:06|56=BANZAI|98=0|108=30|10=077|",
        "8=FIX.4.1|9=54|35=FOOBAR|34=2|49=EXEC|52=20121105-23:24:37|56=BANZAI|10=198|",
        "8=FIX.4.1|9=139|35=8|34=3|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=0|11=1352157882577|14=0|17=1|20=0|31=0|32=0|37=1|38=10000|39=0|54=1|55=MSFT|150=2|151=0|10=082|",
        "8=FIX.4.1|9=153|35=8|34=4|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=12.3|11=1352157882577|14=10000|17=2|20=0|31=12.3|32=10000|37=2|38=10000|39=2|54=1|55=MSFT|150=2|151=0|10=253|",
        "8=FIX.4.1|9=139|35=8|34=5|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=0|11=1352157895032|14=0|17=3|20=0|31=0|32=0|37=3|38=10000|39=0|54=1|55=ORCL|150=2|151=0|10=072|",
        "8=FIX.4.1|9=153|35=8|34=6|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=12.3|11=1352157895032|14=10000|17=4|20=0|31=12.3|32=10000|37=4|38=10000|39=2|54=1|55=ORCL|150=2|151=0|10=243|",
        "8=FIX.4.1|9=138|35=8|34=7|49=EXEC|52=20121105-23:25:12|56=BANZAI|6=0|11=1352157912357|14=0|17=5|20=0|31=0|32=0|37=5|38=10000|39=0|54=1|55=SPY|150=2|151=0|10=019|",
        "8=FIX.4.1|9=82|35=D|34=8|49=EXEC|52=20121105-23:25:16|56=BANZAI|45=6|58=Unsupported message type|10=100|",
        "8=FIX.4.1|9=82|35=D|34=9|49=EXEC|52=20121105-23:25:25|56=BANZAI|45=7|58=Unsupported message type|10=102|",
        "8=FIX.4.1|9=62|35=D|34=10|49=BANZAI|52=20121105-23:24:06|56=EXEC|98=0|108=30|10=138|",
        "8=FIX.4.1|9=50|35=D|34=11|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=125|",
        "8=FIX.4.1|9=104|35=D|34=12|49=BANZAI|52=20121105-23:24:42|56=EXEC|11=1352157882577|21=1|38=10000|40=1|54=1|55=MSFT|59=0|10=041|",
        "8=FIX.4.1|9=104|35=D|34=13|49=BANZAI|52=20121105-23:24:55|56=EXEC|11=1352157895032|21=1|38=10000|40=1|54=1|55=ORCL|59=0|10=026|",
        "8=FIX.4.1|9=109|35=D|34=14|49=BANZAI|52=20121105-23:25:12|56=EXEC|11=1352157912357|21=1|38=10000|40=2|44=10|54=1|55=SPY|59=0|10=105|",
        "8=FIX.4.1|9=106|35=BA|34=15|49=BANZAI|52=20121105-23:25:16|56=EXEC|11=1352157916437|38=10000|41=1352157912357|54=1|55=SPY|10=249|",
        "8=FIX.4.1|9=106|35=BA|34=16|49=BANZAI|52=20121105-23:25:25|56=EXEC|11=1352157925309|38=10000|41=1352157912357|54=1|55=SPY|10=248|",
};

/*
 * Valid partial messages for push class
 */
const char *partial_messages[16] = {
        "|49=EXEC|52=20121105-23:24:06|56=BANZAI|98=0|108=30|10=",
        "|49=EXEC|52=20121105-23:24:37|56=BANZAI|10=",
        "|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=0|11=1352157882577|14=0|17=1|20=0|31=0|32=0|37=1|38=10000|39=0|54=1|55=MSFT|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=12.3|11=1352157882577|14=10000|17=2|20=0|31=12.3|32=10000|37=2|38=10000|39=2|54=1|55=MSFT|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=0|11=1352157895032|14=0|17=3|20=0|31=0|32=0|37=3|38=10000|39=0|54=1|55=ORCL|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=12.3|11=1352157895032|14=10000|17=4|20=0|31=12.3|32=10000|37=4|38=10000|39=2|54=1|55=ORCL|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:25:12|56=BANZAI|6=0|11=1352157912357|14=0|17=5|20=0|31=0|32=0|37=5|38=10000|39=0|54=1|55=SPY|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:25:16|56=BANZAI|45=6|58=Unsupported message type|10=",
        "|49=EXEC|52=20121105-23:25:25|56=BANZAI|45=7|58=Unsupported message type|10=",
        "|49=BANZAI|52=20121105-23:24:06|56=EXEC|98=0|108=30|10=",
        "|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=",
        "|49=BANZAI|52=20121105-23:24:42|56=EXEC|11=1352157882577|21=1|38=10000|40=1|54=1|55=MSFT|59=0|10=",
        "|49=BANZAI|52=20121105-23:24:55|56=EXEC|11=1352157895032|21=1|38=10000|40=1|54=1|55=ORCL|59=0|10=",
        "|49=BANZAI|52=20121105-23:25:12|56=EXEC|11=1352157912357|21=1|38=10000|40=2|44=10|54=1|55=SPY|59=0|10=",
        "|49=BANZAI|52=20121105-23:25:16|56=EXEC|11=1352157916437|38=10000|41=1352157912357|54=1|55=SPY|10=",
        "|49=BANZAI|52=20121105-23:25:25|56=EXEC|11=1352157925309|38=10000|41=1352157912357|54=1|55=SPY|10=",
};

/*
 * Valid message types
 */
const char *message_types[16] = {
        "8",
        "FOOBAR",
        "8",
        "8",
        "8",
        "8",
        "8",
        "D",
        "D",
        "D",
        "D",
        "D",
        "D",
        "D",
        "BA",
        "BA",
};

/*
 * Valid complete session messages
 */
const char *complete_session_messages[12] = {
        "8=FIX.4.1|9=61|35=0|34=1|49=EXEC|52=20121105-23:24:06|56=BANZAI|98=0|108=30|10=069|",
        "8=FIX.4.1|9=49|35=0|34=2|49=EXEC|52=20121105-23:24:37|56=BANZAI|10=065|",
        "8=FIX.4.1|9=139|35=0|34=3|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=0|11=1352157882577|14=0|17=1|20=0|31=0|32=0|37=1|38=10000|39=0|54=1|55=MSFT|150=2|151=0|10=074|",
        "8=FIX.4.1|9=153|35=0|34=4|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=12.3|11=1352157882577|14=10000|17=2|20=0|31=12.3|32=10000|37=2|38=10000|39=2|54=1|55=MSFT|150=2|151=0|10=245|",
        "8=FIX.4.1|9=139|35=0|34=5|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=0|11=1352157895032|14=0|17=3|20=0|31=0|32=0|37=3|38=10000|39=0|54=1|55=ORCL|150=2|151=0|10=064|",
        "8=FIX.4.1|9=153|35=0|34=6|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=12.3|11=1352157895032|14=10000|17=4|20=0|31=12.3|32=10000|37=4|38=10000|39=2|54=1|55=ORCL|150=2|151=0|10=235|",
        "8=FIX.4.1|9=138|35=0|34=7|49=EXEC|52=20121105-23:25:12|56=BANZAI|6=0|11=1352157912357|14=0|17=5|20=0|31=0|32=0|37=5|38=10000|39=0|54=1|55=SPY|150=2|151=0|10=011|",
        "8=FIX.4.1|9=82|35=0|34=8|49=EXEC|52=20121105-23:25:16|56=BANZAI|45=6|58=Unsupported message type|10=080|",
        "8=FIX.4.1|9=82|35=0|34=9|49=EXEC|52=20121105-23:25:25|56=BANZAI|45=7|58=Unsupported message type|10=082|",
        "8=FIX.4.1|9=62|35=0|34=10|49=BANZAI|52=20121105-23:24:06|56=EXEC|98=0|108=30|10=118|",
        "8=FIX.4.1|9=50|35=0|34=11|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=105|",
        "8=FIX.4.1|9=104|35=0|34=12|49=BANZAI|52=20121105-23:24:42|56=EXEC|11=1352157882577|21=1|38=10000|40=1|54=1|55=MSFT|59=0|10=021|",
};

/*
 * Valid complete session messages
 */
const char *partial_session_messages[12] = {
        "|49=EXEC|52=20121105-23:24:06|56=BANZAI|98=0|108=30|10=",
        "|49=EXEC|52=20121105-23:24:37|56=BANZAI|10=",
        "|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=0|11=1352157882577|14=0|17=1|20=0|31=0|32=0|37=1|38=10000|39=0|54=1|55=MSFT|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=12.3|11=1352157882577|14=10000|17=2|20=0|31=12.3|32=10000|37=2|38=10000|39=2|54=1|55=MSFT|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=0|11=1352157895032|14=0|17=3|20=0|31=0|32=0|37=3|38=10000|39=0|54=1|55=ORCL|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=12.3|11=1352157895032|14=10000|17=4|20=0|31=12.3|32=10000|37=4|38=10000|39=2|54=1|55=ORCL|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:25:12|56=BANZAI|6=0|11=1352157912357|14=0|17=5|20=0|31=0|32=0|37=5|38=10000|39=0|54=1|55=SPY|150=2|151=0|10=",
        "|49=EXEC|52=20121105-23:25:16|56=BANZAI|45=6|58=Unsupported message type|10=",
        "|49=EXEC|52=20121105-23:25:25|56=BANZAI|45=7|58=Unsupported message type|10=",
        "|49=BANZAI|52=20121105-23:24:06|56=EXEC|98=0|108=30|10=",
        "|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=",
        "|49=BANZAI|52=20121105-23:24:42|56=EXEC|11=1352157882577|21=1|38=10000|40=1|54=1|55=MSFT|59=0|10=",
};

/*
 * Valid session message types
 */
const char *session_message_types[12] = {
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
        "0",
};

/*
 * "Random" crap
 */
const char *noise[16] = {
        "akjdlksjladkd",
        "dsjkhd47",
        "9",
        "dwmnfjfci2ojef8974yunjcd€#%&%&#FFC",
        "dkjdk498ic4mfr88h5ub4tj",
        "dc7764yfjnch73",
        "cu27uj42nrfu5#€TR#%€GVCC",
        "dnewo43%&€CRG€&vfsfdvfg",
        "d9=767",
        "",
        "coi3",
        "8=FIXdwifimciri",
        "ioi478=FIX.4.1|9=100djijimm",
        "cwe83",
        "cwu",
        "9=9=ecewe#€€FC%",
};

/*
 * msg is a pointer to the first character in a zero terminated FIX
 * messsage
 */
static inline unsigned int
get_FIX_checksum(const uint8_t *msg, size_t len)
{
        uint64_t sum = 0;
        size_t n;

        for (n = 0; n < len; ++n) {
                sum += (uint64_t)msg[n];
        }

        return (sum % 256);
}

/*
 * Returns a pointer to a very long FIX message with garbage content
 * to test buffer boundaries.
 *
 * length - body length upon input, total length upon return.
 */
static inline char*
make_fix_message(const char * const msg_type,
                 const char * const fix_version,
                 const size_t seqnum,
                 size_t * const length)
{
        const char filler = 'A'; //0xAA;
        unsigned int checksum;
        char *pos;
        int n;
        char *msg = (char*)malloc(*length + 128);
        if (!msg) {
                M_ALERT("no memory");
                return NULL;
        }
        memset(msg, filler, *length + 128); // overshooting a bit, but what the hell...

        sprintf(msg, "8=%s%c9=%lu%c35=%s%c34=%lu%c", fix_version, DELIM, *length, DELIM, msg_type, DELIM, seqnum, DELIM);
        *(msg + strlen(msg)) = filler;

        n = 0;
        pos = msg;
        do {
                if (DELIM == *pos)
                        ++n;
                ++pos;
        } while (n < 2 );
        *(pos + *length - 1) = DELIM;
        *(pos + *length + 7) = '\0';
        checksum = get_FIX_checksum((uint8_t*)msg, strlen(msg) - 7);
        pos += *length;
        sprintf(pos, "10=%03u%c", checksum, DELIM);
        *length = (size_t)(pos + 3 + 4 - msg);

        return msg;
}


/*
 * Test initalization of FIX_Pusher class
 */
START_TEST(test_FIX_Pusher_create)
{
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sink_fd = open("/dev/null", O_WRONLY);

        fail_unless(NULL != pusher, NULL);
        fail_unless(true == pusher->init("FIX.4.2", sink_fd), NULL);
}
END_TEST

/*
 * Test initalization of FIX_Popper class
 */
START_TEST(test_FIX_Popper_create)
{
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        int source_fd = open("/dev/null", O_WRONLY);

        fail_unless(NULL != popper, NULL);
        fail_unless(true == popper->init("FIX.4.2", source_fd), NULL);
}
END_TEST

/*
 * Test send and recieve of test messages sequentially
 */
START_TEST(test_FIX_send_and_recv_sequentially)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

        for (n = 0; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
                fail_unless(0 == popper->pop(&len, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }
        pusher->stop();
}
END_TEST

/*
 * Test send and recieve of test messages sequentially
 */
START_TEST(test_FIX_send_and_recv_session_messages_sequentially)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

        for (n = 0; n < 12; ++n) {
                fail_unless(0 == pusher->session_push(strlen(partial_session_messages[n]), partial_session_messages[n], session_message_types[n]), NULL);
                popper->session_pop(&len, &msg);
                fail_unless(len == strlen(complete_session_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_session_messages[n], msg, len), NULL);
        }
        pusher->start();
}
END_TEST

/*
 * Test send and recieve of a mix of session and non-session messages
 */
START_TEST(test_FIX_send_and_recv_session_and_non_session_messages)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

	fail_unless(0 == pusher->push(strlen(partial_messages[0]), partial_messages[0], message_types[0]), NULL);
	fail_unless(0 == popper->pop(&len, &msg), NULL);
	fail_unless(len == strlen(complete_messages[0]), NULL);
	fail_unless(0 == memcmp(complete_messages[0], msg, len), NULL);
	free(msg);

	fail_unless(0 == pusher->session_push(strlen(partial_session_messages[1]), partial_session_messages[1], session_message_types[1]), NULL);
	popper->session_pop(&len, &msg);
	fail_unless(len == strlen(complete_session_messages[1]), NULL);
	fail_unless(0 == memcmp(complete_session_messages[1], msg, len), NULL);

	fail_unless(0 == pusher->push(strlen(partial_messages[2]), partial_messages[2], message_types[2]), NULL);
	fail_unless(0 == popper->pop(&len, &msg), NULL);
	fail_unless(len == strlen(complete_messages[2]), NULL);
	fail_unless(0 == memcmp(complete_messages[2], msg, len), NULL);
	free(msg);

	fail_unless(0 == pusher->session_push(strlen(partial_session_messages[3]), partial_session_messages[3], session_message_types[3]), NULL);
	popper->session_pop(&len, &msg);
	fail_unless(len == strlen(complete_session_messages[3]), NULL);
	fail_unless(0 == memcmp(complete_session_messages[3], msg, len), NULL);

	fail_unless(0 == pusher->push(strlen(partial_messages[4]), partial_messages[4], message_types[4]), NULL);
	fail_unless(0 == popper->pop(&len, &msg), NULL);
	fail_unless(len == strlen(complete_messages[4]), NULL);
	fail_unless(0 == memcmp(complete_messages[4], msg, len), NULL);
	free(msg);

	fail_unless(0 == pusher->session_push(strlen(partial_session_messages[5]), partial_session_messages[5], session_message_types[5]), NULL);
	popper->session_pop(&len, &msg);
	fail_unless(len == strlen(complete_session_messages[5]), NULL);
	fail_unless(0 == memcmp(complete_session_messages[5], msg, len), NULL);

        for (n = 6; n < 12; ++n) {
                fail_unless(0 == pusher->session_push(strlen(partial_session_messages[n]), partial_session_messages[n], session_message_types[n]), NULL);
                popper->session_pop(&len, &msg);
                fail_unless(len == strlen(complete_session_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_session_messages[n], msg, len), NULL);
        }

        for (n = 12; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
                fail_unless(0 == popper->pop(&len, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }

        pusher->stop();
}
END_TEST

/*
 * Test send and recieve of a mix of session and non-session messages
 * intermingled with noise.
 *
 * NOTE: On rare occasions this function hangs in one of the
 * send_all() or push() methods. Must investigate. It is interesting
 * to note that the other test functions in the suite never hangs...
 */
START_TEST(test_FIX_send_and_recv_session_and_non_session_messages_with_noise)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

	fail_unless(0 == pusher->push(strlen(partial_messages[0]), partial_messages[0], message_types[0]), NULL);
	fail_unless(0 == popper->pop(&len, &msg), NULL);
	fail_unless(len == strlen(complete_messages[0]), NULL);
	fail_unless(0 == memcmp(complete_messages[0], msg, len), NULL);
	free(msg);

	fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[0], strlen(noise[0])), NULL);

	fail_unless(0 == pusher->session_push(strlen(partial_session_messages[1]), partial_session_messages[1], session_message_types[1]), NULL);
	popper->session_pop(&len, &msg);
	fail_unless(len == strlen(complete_session_messages[1]), NULL);
	fail_unless(0 == memcmp(complete_session_messages[1], msg, len), NULL);

	fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[1], strlen(noise[1])), NULL);
	fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[2], strlen(noise[2])), NULL);

	fail_unless(0 == pusher->push(strlen(partial_messages[2]), partial_messages[2], message_types[2]), NULL);
	fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[3], strlen(noise[3])), NULL);
	fail_unless(0 == popper->pop(&len, &msg), NULL);
	fail_unless(len == strlen(complete_messages[2]), NULL);
	fail_unless(0 == memcmp(complete_messages[2], msg, len), NULL);
	free(msg);

	fail_unless(0 == pusher->session_push(strlen(partial_session_messages[3]), partial_session_messages[3], session_message_types[3]), NULL);
	popper->session_pop(&len, &msg);
	fail_unless(len == strlen(complete_session_messages[3]), NULL);
	fail_unless(0 == memcmp(complete_session_messages[3], msg, len), NULL);

	fail_unless(0 == pusher->push(strlen(partial_messages[4]), partial_messages[4], message_types[4]), NULL);
	fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[12], strlen(noise[12])), NULL);
	fail_unless(0 == popper->pop(&len, &msg), NULL);
	fail_unless(len == strlen(complete_messages[4]), NULL);
	fail_unless(0 == memcmp(complete_messages[4], msg, len), NULL);
	free(msg);

	fail_unless(0 == pusher->session_push(strlen(partial_session_messages[5]), partial_session_messages[5], session_message_types[5]), NULL);
	popper->session_pop(&len, &msg);
	fail_unless(len == strlen(complete_session_messages[5]), NULL);
	fail_unless(0 == memcmp(complete_session_messages[5], msg, len), NULL);

        for (n = 6; n < 12; ++n) {
		fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[12], strlen(noise[12])), NULL);
                fail_unless(0 == pusher->session_push(strlen(partial_session_messages[n]), partial_session_messages[n], session_message_types[n]), NULL);
		fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[12], strlen(noise[12])), NULL);
                popper->session_pop(&len, &msg);
                fail_unless(len == strlen(complete_session_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_session_messages[n], msg, len), NULL);
        }

        for (n = 12; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
                fail_unless(0 == popper->pop(&len, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }

        pusher->stop();
}
END_TEST



/*
 * Test send and recieve of test messages sequentially with noise in
 * between
 */
START_TEST(test_FIX_send_and_recv_sequentially_with_noise)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };


        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

        for (n = 0; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
                fail_unless(1 == send_all(sockets[0], (const uint8_t*)noise[n], strlen(noise[n])), NULL);
                fail_unless(0 == popper->pop(&len, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }
}
END_TEST

/*
 * Test send and recieve of test messages in bursts
 */
START_TEST(test_FIX_send_and_recv_in_bursts)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };


        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

        for (n = 0; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
        }

        for (n = 0; n < 16; ++n) {
                fail_unless(0 == popper->pop(&len, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }
}
END_TEST

/*
 * Test send and recieve of test messages eratically
 */
START_TEST(test_FIX_send_and_recv_eratically)
{
        int n;
        size_t len;
        void *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };


        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == pusher->init("FIX.4.1", sockets[0]), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        pusher->start();
        popper->start();

        // push 0..2
        for (n = 0; n < 3; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
        }

        // pop 0
        fail_unless(0 == popper->pop(&len, &msg), NULL);
        fail_unless(len == strlen(complete_messages[0]), NULL);
        fail_unless(0 == memcmp(complete_messages[0], msg, len), NULL);
        free(msg);

        // push 3
        fail_unless(0 == pusher->push(strlen(partial_messages[3]), partial_messages[3], message_types[3]), NULL);

        // pop 1
        fail_unless(0 == popper->pop(&len, &msg), NULL);
        fail_unless(len == strlen(complete_messages[1]), NULL);
        fail_unless(0 == memcmp(complete_messages[1], msg, len), NULL);
        free(msg);

        // pop 2
        fail_unless(0 == popper->pop(&len, &msg), NULL);
        fail_unless(len == strlen(complete_messages[2]), NULL);
        fail_unless(0 == memcmp(complete_messages[2], msg, len), NULL);
        free(msg);

        // push 4..15
        for (n = 4; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
        }

        // pop 3..15
        for (n = 3; n < 16; ++n) {
                fail_unless(0 == popper->pop(&len, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }
}
END_TEST

/*
 * This is a test of the popper.
 *
 * Test send and recieve of test messages designed to hit buffer
 * boundaries.
 *
 * We cheat a little here and use the non-published knowledge of the
 * buffer implementation sizes:
 *
 * #define FOXTROT_ENTRY_SIZE (1024*4)
 * #define FOXTROT_MAX_DATA_SIZE (FOXTROT_ENTRY_SIZE - sizeof(uint32_t))
 *
 */
START_TEST(test_FIX_challenge_buffer_boundaries_overflow)
{
        int n;
        size_t send_len;
        size_t recv_len;
        void *send_msg;
        void *recv_msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        popper->start();

        send_len = 1024*10;
        send_msg = make_fix_message("B", "FIX.4.1", 1, &send_len);

        fail_unless(1 == send_all(sockets[0], (const uint8_t*)send_msg, send_len), NULL);
        fail_unless(0 == popper->pop(&recv_len, &recv_msg), NULL);
        fail_unless(send_len == recv_len, NULL);
        fail_unless(0 == memcmp(send_msg, recv_msg, send_len), NULL);

        free(send_msg);
        free(recv_msg);
}
END_TEST

/*
 * This is a test of the popper.
 *
 * Test send and recieve of test messages designed to hit buffer
 * boundaries and have leading crap.
 *
 * We cheat a little here and use the non-published knowledge of the
 * buffer implementation sizes:
 *
 * #define FOXTROT_ENTRY_SIZE (1024*4)
 * #define FOXTROT_MAX_DATA_SIZE (FOXTROT_ENTRY_SIZE - sizeof(uint32_t))
 *
 */
START_TEST(test_FIX_challenge_buffer_boundaries_with_crap)
{
        int n;
        size_t send_len;
        size_t recv_len;
        void *send_msg;
        void *recv_msg;
        char crap[16] = { 'H' };
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(true == popper->init("FIX.4.1", sockets[1]), NULL);
        popper->start();

        send_len = 1024*10;
        send_msg = make_fix_message("B", "FIX.4.1", 1, &send_len);

        fail_unless(1 == send_all(sockets[0], (const uint8_t*)crap, sizeof(crap)), NULL);
        fail_unless(1 == send_all(sockets[0], (const uint8_t*)send_msg, send_len), NULL);
        fail_unless(0 == popper->pop(&recv_len, &recv_msg), NULL);
        fail_unless(send_len == recv_len, NULL);
        fail_unless(0 == memcmp(send_msg, recv_msg, send_len), NULL);

        free(send_msg);
        free(recv_msg);
}
END_TEST

Suite*
fixio_suite(void)
{
        Suite *s = suite_create ("FIX IO");

        /* Core test case */
        TCase *tc_core = tcase_create("Core");
        tcase_add_test(tc_core, test_FIX_Pusher_create);
        tcase_add_test(tc_core, test_FIX_Popper_create);
        tcase_add_test(tc_core, test_FIX_send_and_recv_sequentially);
        tcase_add_test(tc_core, test_FIX_send_and_recv_session_messages_sequentially);
	tcase_add_test(tc_core, test_FIX_send_and_recv_session_and_non_session_messages);
	tcase_add_test(tc_core, test_FIX_send_and_recv_session_and_non_session_messages_with_noise);
        tcase_add_test(tc_core, test_FIX_send_and_recv_sequentially_with_noise);
        tcase_add_test(tc_core, test_FIX_send_and_recv_in_bursts);
        tcase_add_test(tc_core, test_FIX_send_and_recv_eratically);
        tcase_add_test(tc_core, test_FIX_challenge_buffer_boundaries_overflow);
        tcase_add_test(tc_core, test_FIX_challenge_buffer_boundaries_with_crap);
        suite_add_tcase(s, tc_core);

        return s;
}

int
main(int argc, char **argv)
{
        int number_failed;
        Suite *s = fixio_suite();
        SRunner *sr = srunner_create(s);

	int n;
	for (n = 0; n < 12; ++n) {
		printf("checksum of %s is %03u\n", complete_session_messages[n], get_FIX_checksum((const uint8_t*)complete_session_messages[n], strlen(complete_session_messages[n]) - 7));
	}

        // initiate logging
        if (!init_logging(false, "check_fixio")) {
                fprintf(stderr, "could not initiate logging\n");
                return EXIT_FAILURE;
        }

        // run the tests
        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
