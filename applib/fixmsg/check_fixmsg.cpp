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
#include "stdlib/log/log.h"

#if 0

/*
 * Test start and stop. The pusher and they popper must be able to
 * stop and start again without loosing any messages.
 */
START_TEST(test_FIX_start_stop)
{
        const char * const sent_db = "23E19F70-616C-4551-BB0E-2EF61EFB9474.sent";
        const char * const recv_db = "538B402A-18CD-4D23-A685-94D31773D50F.recv";

        int n;
        uint32_t len;
        uint32_t msgtype_offset;
        uint8_t *msg;
        FIX_Popper *popper = new (std::nothrow) FIX_Popper(DELIM);
        FIX_Pusher *pusher = new (std::nothrow) FIX_Pusher(DELIM);
        int sockets[2] = { -1, -1 };

        remove(sent_db);
        remove(recv_db);

        fail_unless(0 == socketpair(PF_LOCAL, SOCK_STREAM, 0, sockets), NULL);
        fail_unless(1 == pusher->init(), NULL);
        fail_unless(1 == popper->init(), NULL);
        pusher->start(sent_db, "FIX.4.1", sockets[0]);
        popper->start(recv_db, "FIX.4.1", NULL, sockets[1]);

        fail_unless(0 == pusher->push(strlen(partial_messages[0]), (const uint8_t *)partial_messages[0], message_types[0]), NULL);
        fail_unless(0 == popper->pop(&len, &msgtype_offset, &msg), NULL);
        fail_unless(len == strlen(complete_messages[0]), NULL);
        fail_unless(0 == memcmp(complete_messages[0], msg, len), NULL);
        free(msg);

        fail_unless(0 == pusher->push(strlen(partial_messages[1]), (const uint8_t *)partial_messages[1], message_types[1]), NULL);
        fail_unless(0 == popper->pop(&len, &msgtype_offset, &msg), NULL);
        fail_unless(len == strlen(complete_messages[1]), NULL);
        fail_unless(0 == memcmp(complete_messages[1], msg, len), NULL);

        pusher->stop();
        pusher->start(NULL, NULL, -1);
        popper->stop();
        popper->start(NULL, NULL, NULL, -1);

        for (n = 2; n < 16; ++n) {
                fail_unless(0 == pusher->push(strlen(partial_messages[n]), (const uint8_t *)partial_messages[n], message_types[n]), NULL);
                fail_unless(0 == popper->pop(&len, &msgtype_offset, &msg), NULL);
                fail_unless(len == strlen(complete_messages[n]), NULL);
                fail_unless(0 == memcmp(complete_messages[n], msg, len), NULL);
                free(msg);
        }

        pusher->stop();
        popper->stop();

        remove(sent_db);
        remove(recv_db);
}
END_TEST

Suite*
fixmsg_suite(void)
{
        Suite *s = suite_create ("FIX MSG");

        /* Core test case */
        TCase *tc_core = tcase_create("Core");

//        tcase_add_test(tc_core, test_FIX_Pusher_create);
        suite_add_tcase(s, tc_core);

        return s;
}
#endif

int
main(int /*argc*/, char **/*argv*/)
{

        int number_failed = 0;
        // Suite *s = fixmsg_suite();
        // SRunner *sr = srunner_create(s);

        // // initiate logging
        // if (!init_logging(false, "check_fixmsg")) {
        //         fprintf(stderr, "could not initiate logging\n");
        //         return EXIT_FAILURE;
        // }

        // // run the tests
        // srunner_run_all(sr, CK_VERBOSE);
        // number_failed = srunner_ntests_failed(sr);
        // srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
