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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "stdlib/process/cpu.h"

/*
 * NOTE: Test if moving these into the RX message class gives better
 * performance
 */

/*
 * Return the FIX tag as a non-zero integer. *str is a pointer to the
 * first character in the FIX tag which must be of the format:
 *
 *    <TAG>=<VALUE>, <TAG> is a positive non-zero integer
 *
 * In case of error -1 is returned. 0 (zero) is never returned.
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
 * Keeping the function as a static inline in this header file is
 * consistently 5 percent faster that having a regular extern
 * declaration. I must consider whether to do that or not...
 *
 * This may seem like an immature quest for non-essential performance
 * over good clean code, but it's not. This function is exclusively
 * used in the hot path of the FIX RX message parser, so every little
 * bit of verifiable performance counts. Decisions, decisions...
 */
extern int
get_fix_tag(const char **str);


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
extern long long
get_fix_length_value(const char soh,
                     const char *str);


/*
 * Optimized for integers less than 10.000.000.000.
 *
 * Upon entry: *str points to the start of the memory which will be
 * written to.
 *
 * Upon return: *str will point to the terminator character.
 *
 * Performance notes:
 *
 * This function is more han 6 times faster than the equivalent one
 * based on sprintf().
 */
extern void
uint_to_str(const char terminator,
            uint64_t value,
            char **str);

/*
 * As uint_to_str() but adds leading padding of zeros.
 *
 * available: Amount of space available. The last byte in the
 * available memory will receive the "terminator" character. The
 * remaining bytes will be used to write:
 *
 * value: The unsigned interger to be written. Leading zero ('0') will
 * be padded if the number of digits in value is less than (available
 * - 1).
 *
 * Upon entry: *str points to the start of the memory where
 * "available" bytes is available for writting.
 *
 * Upon return: *str will point to the terminator character.
 *
 * Performance notes:
 *
 * This function is roughly 5 times faster than the equivalent one
 * based on snprintf().
 */
extern int
uint_to_str_zero_padded(const size_t available,
                        const char terminator,
                        uint64_t value,
                        char **str);
