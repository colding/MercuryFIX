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

/*
 * Defines helper functions to do very fast "string => index of that
 * string in an array" conversions. With this data set it's actually
 * about 10 times faster than the corresponding strcmp() based
 * algorithm.
 *
 * It is also faster than the equivalent std::map<string, int> based
 * algorithm by a factor of about 5.
 *
 * BTW, there's an upper limit on the length of the strings involved
 * of 4 characters. An eventual terminating zero character is not
 * included in this count.
 */
#define INDEXOF_FUNCTION(INIT_NAME__, INDEXOF_NAME__, ARRAY_LENGTH__, STRING_ARRAY__, LOOKUP_MAP__, LOOKUP_MAP_ITERATOR__) \
__attribute__((constructor)) static void                                                 \
INIT_NAME(void)                                                                          \
{                                                                                        \
        unsigned int is;                                                                 \
        int len = 0;                                                                     \
        int n;                                                                           \
                                                                                         \
        for (n = 0; n < ARRAY_LENGTH__; ++n) {                                           \
                len = strlen(STRING_ARRAY__[n]);                                         \
                switch (len) {                                                           \
                case 0:                                                                  \
                        LOOKUP_MAP__[n] = 0;                                             \
                        break;                                                           \
                case 1:                                                                  \
                        LOOKUP_MAP__[n] = (uint_fast32_t)STRING_ARRAY__[n][0];           \
                        break;                                                           \
                case 2:                                                                  \
                        LOOKUP_MAP__[n] = (((uint_fast32_t)STRING_ARRAY__[n][1] << 8) |  \
					    (uint_fast32_t)STRING_ARRAY__[n][0]) ;       \
                        break;                                                           \
                case 3:                                                                  \
                        LOOKUP_MAP__[n] = (((uint_fast32_t)STRING_ARRAY__[n][2] << 16) | \
					   ((uint_fast32_t)STRING_ARRAY__[n][1] << 8) |	 \
					    (uint_fast32_t)STRING_ARRAY__[n][0]);        \
                        break;                                                           \
                case 4:                                                                  \
                        LOOKUP_MAP__[n] = (((uint_fast32_t)STRING_ARRAY__[n][3] << 24) | \
					   ((uint_fast32_t)STRING_ARRAY__[n][2] << 16) | \
					   ((uint_fast32_t)STRING_ARRAY__[n][1] << 8) |	 \
					    (uint_fast32_t)STRING_ARRAY__[n][0]);        \
                        break;                                                           \
                default:                                                                 \
                        abort();                                                         \
                }                                                                        \
        }                                                                                \
                                                                                         \
        return 0;                                                                        \
}                                                                                        \
                                                                                         \
static int                                                                               \
INDEXOF_NAME__(const char soh, const char * const str)			                 \
{                                                                                        \
        unsigned int n;                                                                  \
        uint_fast32_t is;                                                                \
        unsigned int len;                                                                \
                                                                                         \
        len = 0;                                                                         \
        while (soh != str[len])                                                          \
                ++len;                                                                   \
                                                                                         \
        switch (len) {                                                                   \
        case 0:                                                                          \
                is = 0;                                                                  \
                break;                                                                   \
        case 1:                                                                          \
                is = ((uint_fast32_t)(str[0]));                                          \
                break;                                                                   \
        case 2:                                                                          \
                is = (((uint_fast32_t)str[1] << 8) |                                     \
                       (uint_fast32_t)str[0]);                                           \
                break;                                                                   \
        case 3:                                                                          \
                is = (((uint_fast32_t)str[2] << 16) |                                    \
                      ((uint_fast32_t)str[1] << 8) |                                     \
                       (uint_fast32_t)str[0]);                                           \
                break;                                                                   \
        case 4:                                                                          \
                is = (((uint_fast32_t)str[3] << 24) |                                    \
                      ((uint_fast32_t)str[2] << 16) |                                    \
                      ((uint_fast32_t)str[1] << 8) |                                     \
                       (uint_fast32_t)str[0]);                                           \
                break;                                                                   \
        default:                                                                         \
                return -1;                                                               \
        }                                                                                \
                                                                                         \
	LOOKUP_MAP_ITERATOR__ = LOOKUP_MAP__.find(is);                                   \
        if (LOOKUP_MAP__.end() == LOOKUP_MAP_ITERATOR__)                                 \
				return -1;                                               \
                                                                                         \
        return LOOKUP_MAP_ITERATOR__->second;                                            \
}
