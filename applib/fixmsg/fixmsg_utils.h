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

#pragma once

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>

/*
 * NOTE: Test if moving these into the RX message class gives better
 * performance
 */

/*
 * Hint to the compiler that an expression is likely to be false.
 */
#ifdef UNLIKELY__
#undef UNLIKELY__
#endif
#define UNLIKELY__(expr__) (__builtin_expect(((expr__) ? 1 : 0), 0))

// used to detect emminent integer overflow
#define MAX_TAG__ ((int)((INT_MAX - 9) / 10))
#define MAX_LENGTH__ ((long long)((LLONG_MAX - 9) / 10))

/*
 * Return the FIX tag as a non-zero integer. *str is a pointer to the
 * first character in the FIX tag. In case of error -1 is returned. 0
 * (zero) is never returned.
 *
 * Upon return, *str will point to the first character in the tag's
 * value.
 *
 * IMPORTANT: The standard is not quite crystal clear on this, but my
 * interpretation is that the first character of a tag is immidiately
 * preceeded by SOH and this first character is never '0'.
 *
 * Performance notes:
 *
 * This function is more han 3 times faster than the equivalent one
 * based on atoi().
 *
 * Keeping the function here in the header file is consistently 5
 * percent faster that having a regular extern declaration.
 *
 * This may seem like an immature quest for non-essential performance
 * over good clean code, but it's not. This function is exclusively
 * used in the hot path of the FIX RX message parser, so every little bit
 * of verifiable performance counts.
 */
static inline int
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

/*
 * Return the FIX length value as an integer. str is a pointer to the
 * first character in the length value. In case of error -1 is returned. 0
 * (zero) may be returned.
 *
 * IMPORTANT: The standard is not quite crystal clear on this, but my
 * interpretation is that the first character of an int or length
 * typed value (the length datatype was introduced in FIX 4.3) is
 * immidiately preceeded by '=', must be a digit in the range [0-9]. A
 * sequence of leading '0's are allowed. The terminating character is
 * assumed, as always, to be SOH.
 *
 * Performance notes: Please see get_fix_tag() above.
 */
static inline long long
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
