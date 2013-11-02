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
#include "stdlib/log/log.h"
#include "applib/fixutils/fixmsg_utils.h"


/*
 * Tests get_fix_tag().
 */
START_TEST(test_get_fix_tag)
{
	int tag;
        const char *str;

        str = "1=value";
	tag = get_fix_tag(&str);
	fail_unless(1 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "12=value";
	tag = get_fix_tag(&str);
	fail_unless(12 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "123=value";
	tag = get_fix_tag(&str);
	fail_unless(123 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "1234=value";
	tag = get_fix_tag(&str);
	fail_unless(1234 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "12345=value";
	tag = get_fix_tag(&str);
	fail_unless(12345 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "123456=value";
	tag = get_fix_tag(&str);
	fail_unless(123456 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "1234567=value";
	tag = get_fix_tag(&str);
	fail_unless(1234567 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "12345678=value";
	tag = get_fix_tag(&str);
	fail_unless(12345678 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "123456789=value";
	tag = get_fix_tag(&str);
	fail_unless(123456789 == tag, NULL);
	fail_unless(!strcmp(str, "value"), NULL);

        str = "3000000000=value";
	tag = get_fix_tag(&str);
	fail_unless(-1 == tag, NULL);

        str = "=value";
	tag = get_fix_tag(&str);
	fail_unless(-1 == tag, NULL);

        str = "k=value";
	tag = get_fix_tag(&str);
	fail_unless(-1 == tag, NULL);
}
END_TEST

/*
 * Tests get_fix_length_value().
 */
START_TEST(test_get_fix_length_value)
{
	long long val;
        const char *str;

        str = "0";
	val = get_fix_length_value('\0', str);
	fail_unless(0 == val, NULL);

        str = "01";
	val = get_fix_length_value('\0', str);
	fail_unless(1 == val, NULL);

        str = "12";
	val = get_fix_length_value('\0', str);
	fail_unless(12 == val, NULL);

        str = "000123";
	val = get_fix_length_value('\0', str);
	fail_unless(123 == val, NULL);

        str = "1234";
	val = get_fix_length_value('\0', str);
	fail_unless(1234 == val, NULL);

        str = "3000000000";
	val = get_fix_length_value('\0', str);
	fail_unless(3000000000 == val, NULL);

        str = "1K";
	val = get_fix_length_value('\0', str);
	fail_unless(-1 == val, NULL);

        str = "01K";
	val = get_fix_length_value('\0', str);
	fail_unless(-1 == val, NULL);

        str = "A";
	val = get_fix_length_value('\0', str);
	fail_unless(-1 == val, NULL);

        str = "0A";
	val = get_fix_length_value('\0', str);
	fail_unless(-1 == val, NULL);
}
END_TEST

/*
 * Tests uint_to_str().
 */
START_TEST(test_uint_to_str)
{
        char buf[10];
        char std[10];
        char *str = buf;
        unsigned int n;


        for (n = 0; n < 1000000; ++n) {
                str = buf;
		snprintf(std, 10, "%u", n);

		uint_to_str('\0', n, &str);

		fail_unless(strlen(std) == strlen(buf), NULL);
		fail_unless(!strncmp(std, buf, 10), NULL);
        }
}
END_TEST

/*
 * Test uint_to_str_zero_padded().
 */
START_TEST(test_uint_to_str_zero_padded)
{
        char buf[10];
        char std[10];
        char *str = buf;
        unsigned int n;

	fail_unless(uint_to_str_zero_padded(1, '\0', 87, &str), NULL);
	fail_unless(uint_to_str_zero_padded(2, '\0', 87, &str), NULL);
	fail_unless(!uint_to_str_zero_padded(3, '\0', 87, &str), NULL);
	fail_unless(!uint_to_str_zero_padded(4, '\0', 87, &str), NULL);

        for (n = 0; n < 1000000; ++n) {
                str = buf;
		snprintf(std, 10, "%09u", n);

                fail_unless(!uint_to_str_zero_padded(10, '\0', n, &str), NULL);
		fail_unless(str == &buf[9], NULL);		
		fail_unless(strlen(std) == strlen(buf), NULL);
		fail_unless(!strncmp(std, buf, 10), NULL);
        }
}
END_TEST

Suite*
fixio_suite(void)
{
        Suite *s = suite_create ("FIX IO");

        /* Core test case */
        TCase *tc_core = tcase_create("Core");

        tcase_set_timeout(tc_core, 20);

	tcase_add_test(tc_core, test_get_fix_tag);
	tcase_add_test(tc_core, test_get_fix_length_value);
        tcase_add_test(tc_core, test_uint_to_str);
        tcase_add_test(tc_core, test_uint_to_str_zero_padded);
        suite_add_tcase(s, tc_core);

        return s;
}

int
main(int /* argc */, char ** /* argv */)
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
        srunner_run_all(sr, CK_VERBOSE);
        number_failed = srunner_ntests_failed(sr);
        srunner_free(sr);

        return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
