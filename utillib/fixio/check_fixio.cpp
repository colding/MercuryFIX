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
 * Valid complete messages
 */
const char *complete_messages[16] = {
        "8=FIX.4.1|9=61|35=A|34=1|49=EXEC|52=20121105-23:24:06|56=BANZAI|98=0|108=30|10=086|",
        "8=FIX.4.1|9=49|35=0|34=2|49=EXEC|52=20121105-23:24:37|56=BANZAI|10=065|",
        "8=FIX.4.1|9=139|35=8|34=3|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=0|11=1352157882577|14=0|17=1|20=0|31=0|32=0|37=1|38=10000|39=0|54=1|55=MSFT|150=2|151=0|10=082|",
        "8=FIX.4.1|9=153|35=8|34=4|49=EXEC|52=20121105-23:24:42|56=BANZAI|6=12.3|11=1352157882577|14=10000|17=2|20=0|31=12.3|32=10000|37=2|38=10000|39=2|54=1|55=MSFT|150=2|151=0|10=253|",
        "8=FIX.4.1|9=139|35=8|34=5|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=0|11=1352157895032|14=0|17=3|20=0|31=0|32=0|37=3|38=10000|39=0|54=1|55=ORCL|150=2|151=0|10=072|",
        "8=FIX.4.1|9=153|35=8|34=6|49=EXEC|52=20121105-23:24:55|56=BANZAI|6=12.3|11=1352157895032|14=10000|17=4|20=0|31=12.3|32=10000|37=4|38=10000|39=2|54=1|55=ORCL|150=2|151=0|10=243|",
        "8=FIX.4.1|9=138|35=8|34=7|49=EXEC|52=20121105-23:25:12|56=BANZAI|6=0|11=1352157912357|14=0|17=5|20=0|31=0|32=0|37=5|38=10000|39=0|54=1|55=SPY|150=2|151=0|10=019|",
        "8=FIX.4.1|9=82|35=3|34=8|49=EXEC|52=20121105-23:25:16|56=BANZAI|45=6|58=Unsupported message type|10=083|",
        "8=FIX.4.1|9=82|35=3|34=9|49=EXEC|52=20121105-23:25:25|56=BANZAI|45=7|58=Unsupported message type|10=085|",
	"8=FIX.4.1|9=62|35=A|34=10|49=BANZAI|52=20121105-23:24:06|56=EXEC|98=0|108=30|10=135|",
	"8=FIX.4.1|9=50|35=0|34=11|49=BANZAI|52=20121105-23:24:37|56=EXEC|10=105|",
	"8=FIX.4.1|9=104|35=D|34=12|49=BANZAI|52=20121105-23:24:42|56=EXEC|11=1352157882577|21=1|38=10000|40=1|54=1|55=MSFT|59=0|10=041|",
	"8=FIX.4.1|9=104|35=D|34=13|49=BANZAI|52=20121105-23:24:55|56=EXEC|11=1352157895032|21=1|38=10000|40=1|54=1|55=ORCL|59=0|10=026|",
	"8=FIX.4.1|9=109|35=D|34=14|49=BANZAI|52=20121105-23:25:12|56=EXEC|11=1352157912357|21=1|38=10000|40=2|44=10|54=1|55=SPY|59=0|10=105|",
	"8=FIX.4.1|9=105|35=F|34=15|49=BANZAI|52=20121105-23:25:16|56=EXEC|11=1352157916437|38=10000|41=1352157912357|54=1|55=SPY|10=187|",
	"8=FIX.4.1|9=105|35=F|34=16|49=BANZAI|52=20121105-23:25:25|56=EXEC|11=1352157925309|38=10000|41=1352157912357|54=1|55=SPY|10=186|",
};

/*
 * Valid message types
 */
const char *message_types[16] = {
        "A",
        "0",
        "8",
        "8",
        "8",
        "8",
        "8",
        "3",
        "3",
	"A",
	"0",
	"D",
	"D",
	"D",
	"F",
	"F",
};

/*
 * msg is a pointer to the first character in a zero terminated FIX
 * messsage
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
	popper->start();
	pusher->start();

        for (n = 0; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), partial_messages[n], message_types[n]), NULL);
        }

	int k;
        for (n = 0; n < 16; ++n) {
                fail_unless(0 == popper->pop(&len, &msg), NULL);
		// M_ALERT("n = %d, len = %d", n, len);
		printf("\n Msg #%d\n", n);
		for (k = 0; k < len; ++k)
			printf("%c", ((char*)msg)[k]);
		printf("\n");
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
        }
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
        tcase_add_test(tc_core, test_FIX_send_and_recv_in_bursts);
        suite_add_tcase(s, tc_core);

        return s;
}

int
main(int argc, char **argv)
{
        int number_failed;
        Suite *s = fixio_suite();
        SRunner *sr = srunner_create(s);
	
        // initiate logging
        if (!init_logging(false, "check_fixio")) {
                fprintf(stderr, "could not initiate logging\n");
                return EXIT_FAILURE;
        }

	// run the tests
        srunner_run_all(sr, CK_NORMAL);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
