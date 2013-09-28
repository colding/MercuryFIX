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

#include "fixmsg_utils.h"

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif

int
get_fix_tag(const char **str)
{
        int num = 0;
        char c = **str;

        // the tag value must be greater than zero
        if (('=' == c) || ('0' == c))
                return -1;

        do {
                if (('0' > c) || ('9' < c))
                        return -1;

                // prevent integer overflow
                if (MAX_TAG__ < num)
                        return -1;

                num = (10 * num) + (c - '0');
                ++(*str);
                c = **str;

                // FIX tags are '=' terminated
                if (UNLIKELY__('=' == c)) {
                        ++(*str); // now points to first byte in value
                        break;
                }
        } while (1);

        return num;
}

long long
get_fix_length_value(const char soh,
                     const char *str)
{
        long long num = 0;
        char c = *str;

        do {
                // the value must not be blank nor contain non-digits
                if (('0' > c) || ('9' < c))
                        return -1;

                // prevent integer overflow
                if (MAX_LENGTH__ < num)
                        return -1;

                num = (10 * num) + (c - '0');
                ++str;
                c = *str;

                // terminated with non-digit
                if (UNLIKELY__(soh == c))
                        break;
        } while (1);

        return num;
}

void
uint_to_str(const char terminator,
            uint64_t value,
            char **str)
{
        char *pos = *str;
        int size = 0;

        if (value >= 10000) {
                if (value >= 10000000) {
                        if (value >= 1000000000) {
                                size = 10;
                                if (value >= 10000000000) {
                                        sprintf(*str, "%llu", value);
					while ('\0' != **str)
						++(*str);
					**str = terminator;
                                        return;
                                }
                        } else if (value >= 100000000) {
                                size = 9;
                        } else {
                                size = 8;
                        }
                } else {
                        if (value >= 1000000) {
                                size = 7;
                        } else if (value >= 100000) {
                                size = 6;
                        } else {
                                size = 5;
                        }
                }
        } else {
                if (value >= 100) {
                        if (value >= 1000) {
                                size = 4;
                        } else {
                                size = 3;
                        }
                } else {
                        if (value >= 10) {
                                size = 2;
                        } else {
                                size = 1;
                        }
                }
        }

	*str += size;
	pos = *str;
        *pos = terminator;

        do {
                *(--pos) = (char)('0' + (value % 10));
        } while (value /= 10);

        return;
}
