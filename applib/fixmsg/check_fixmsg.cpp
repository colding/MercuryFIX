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

#include <check.h>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <stdio.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "stdlib/log/log.h"
#include "fix_msg.h"
#include "applib/fixio/fixio.h"

#define DELIM '|'

/*
 * Valid partial messages for push class
 */
static const char *partial_messages[2] =
{
        "|49=BANZAI|95=00004|91=1|9||52=20121105-23:25:16|56=EXEC|11=1352157916437|38=10000|41=1352157912357|54=1|55=SPY|10=",
        "|49=BANZAI|95=4|91=1|9||52=20121105-23:25:25|56=EXEC|95=7|91=1| d=3||11=1352157925309|38=10000|41=1352157912357|54=1|55=SPY|10=",
};

/*
 * Extracted field values of partial_messages[1] above
 */
static const char *field_values[14] =
{
        "FOOBAR",
        "1",
        "BANZAI",
        "4",
        "1|9|",
        "20121105-23:25:25",
        "EXEC",
        "7",
        "1| d=3|",
        "1352157925309",
        "10000",
        "1352157912357",
        "1",
        "SPY",
};

/*
 * Some message types to feed the pusher
 */
static const char *message_types[2] =
{
        "8",
        "FOOBAR",
};

/*
 * Test imprint() and done().
 */
START_TEST(test_FIXMessageRX_resource_management)
{
        int n;
        int tag;
        size_t value_length;
        uint8_t *value;
        uint32_t len;
        uint32_t msgtype_offset;
        uint8_t *msg;
        FIXMessageRX rx_msg = FIXMessageRX::make_fix_message_mem_owner_on_stack(FIX_4_1, DELIM);

        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(1 == pusher->init(), NULL);
        fail_unless(1 == popper->init(), NULL);
        fail_unless(1 == rx_msg.init(), NULL);

        pusher->start(":memory:", "FIX.4.1", sockets[0]);
        popper->start(":memory:", "FIX.4.1", NULL, sockets[1]);

        for (n = 0; n < 2; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), (const uint8_t *)partial_messages[n], message_types[n]), NULL);
                popper->pop(&len, &msgtype_offset, &msg);
                rx_msg.imprint(msgtype_offset, msg);

                do {
                        tag = rx_msg.next_field(value_length, &value);
                } while (0 < tag);
                rx_msg.done();

                fail_unless(0 == tag, NULL);
        }

        pusher->stop();
        popper->stop();
}
END_TEST

/*
 * Test next_field()
 */
START_TEST(test_FIXMessageRX_next_field)
{
        int n;
        int tag;
        size_t value_length;
        uint8_t *value;
        uint32_t len;
        uint32_t msgtype_offset;
        uint8_t *msg;
        FIXMessageRX rx_msg = FIXMessageRX::make_fix_message_mem_owner_on_stack(FIX_4_1, DELIM);

        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(1 == pusher->init(), NULL);
        fail_unless(1 == popper->init(), NULL);
        fail_unless(1 == rx_msg.init(), NULL);

        pusher->start(":memory:", "FIX.4.1", sockets[0]);
        popper->start(":memory:", "FIX.4.1", NULL, sockets[1]);

        fail_unless(0 == pusher->push(strlen(partial_messages[1]), (const uint8_t *)partial_messages[1], message_types[1]), NULL);
        popper->pop(&len, &msgtype_offset, &msg);
        rx_msg.imprint(msgtype_offset, msg);

        n = 0;
        do {
                tag = rx_msg.next_field(value_length, &value);
                if (!tag)
                        break;

                fail_unless(n < 14, NULL);
                fail_unless(value_length == strlen(field_values[n]), NULL);
                fail_unless(0 == memcmp(value, field_values[n], value_length), NULL);
                ++n;
        } while (0 < tag);
        rx_msg.done();

        pusher->stop();
        popper->stop();
}
END_TEST

Suite*
fixmsg_suite(void)
{
        Suite *s = suite_create ("FIX MSG");

        /* Core test case */
        TCase *tc_core = tcase_create("Core");

        tcase_add_test(tc_core, test_FIXMessageRX_resource_management);
        tcase_add_test(tc_core, test_FIXMessageRX_next_field);
        suite_add_tcase(s, tc_core);

        return s;
}

int
main(int /*argc*/, char **/*argv*/)
{

        int number_failed = 0;
        Suite *s = fixmsg_suite();
        SRunner *sr = srunner_create(s);

        // initiate logging
        if (!init_logging(false, "check_fixmsg")) {
                fprintf(stderr, "could not initiate logging\n");
                return EXIT_FAILURE;
        }

        // run the tests
        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
