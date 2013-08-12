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

/*
 * Defines helper functions to do very fast "string => index of that
 * string in an array" conversions. With this data set they are
 * actually about 10 times faster than the corresponding strcmp()
 * based algorithms.
 *
 * It is also faster than the equivalent std::map<string, int> based
 * algorithm by a factor of about 5.
 *
 * But, there's an upper limit on the length of the strings involved
 * of 4 characters. An eventual terminating zero character is not
 * included in this count.
 */

#include "fix_types.h"

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <string.h>
#include <map>

static std::map<uint_fast32_t, int> *fix_session_msgtypes_map;
static std::map<uint_fast32_t, FIX_MsgType> *fix_msgtypes_map;

__attribute__((constructor)) static void
get_fix_msgtype_ctor(void)
{
        uint_fast32_t is;
        unsigned int len;
        unsigned int n;

        // no memory leak here - it is going to live forever...
        fix_msgtypes_map = new std::map<uint_fast32_t, FIX_MsgType>;

        for (n = 0; n < FIX_MSGTYPES_COUNT; ++n) {
                len = strlen(fix_msgtype_string[n]);
                switch (len) {
                case 0:
                        is = 0;
                        break;
                case 1:
                        is = (uint_fast32_t)fix_msgtype_string[n][0];
                        break;
                case 2:
                        is = (((uint_fast32_t)fix_msgtype_string[n][1] << 8) |
                              (uint_fast32_t)fix_msgtype_string[n][0]);
                        break;
                case 3:
                        is = (((uint_fast32_t)fix_msgtype_string[n][2] << 16) |
                              ((uint_fast32_t)fix_msgtype_string[n][1] << 8) |
                              (uint_fast32_t)fix_msgtype_string[n][0]);
                        break;
                case 4:
                        is = (((uint_fast32_t)fix_msgtype_string[n][3] << 24) |
                              ((uint_fast32_t)fix_msgtype_string[n][2] << 16) |
                              ((uint_fast32_t)fix_msgtype_string[n][1] << 8) |
                              (uint_fast32_t)fix_msgtype_string[n][0]);
                        break;
                default:
                        abort();
                }
                (*fix_msgtypes_map)[is] = (FIX_MsgType)n;
        }
}

__attribute__((constructor)) static void
is_session_message_ctor(void)
{
        uint_fast32_t is;
        unsigned int len;
        unsigned int n;

        // no memory leak here - it is going to live forever...
        fix_session_msgtypes_map = new std::map<uint_fast32_t, int>;

        for (n = 0; n < FIX_SESSION_MSGTYPES_COUNT; ++n) {
                len = strlen(fix_session_msgtype_string[n]);
                switch (len) {
                case 0:
                        is = 0;
                        break;
                case 1:
                        is = (uint_fast32_t)fix_session_msgtype_string[n][0];
                        break;
                case 2:
                        is = (((uint_fast32_t)fix_session_msgtype_string[n][1] << 8) |
                              (uint_fast32_t)fix_session_msgtype_string[n][0]);
                        break;
                case 3:
                        is = (((uint_fast32_t)fix_session_msgtype_string[n][2] << 16) |
                              ((uint_fast32_t)fix_session_msgtype_string[n][1] << 8) |
                              (uint_fast32_t)fix_session_msgtype_string[n][0]);
                        break;
                case 4:
                        is = (((uint_fast32_t)fix_session_msgtype_string[n][3] << 24) |
                              ((uint_fast32_t)fix_session_msgtype_string[n][2] << 16) |
                              ((uint_fast32_t)fix_session_msgtype_string[n][1] << 8) |
                              (uint_fast32_t)fix_session_msgtype_string[n][0]);
                        break;
                default:
                        abort();
                }
                (*fix_session_msgtypes_map)[is] = n;
        }
}

FIX_MsgType
get_fix_msgtype(const char soh,
                const char * const str)
{
        uint_fast32_t is;
        unsigned int len = 0;
        std::map<uint_fast32_t, FIX_MsgType>::const_iterator it;

        while (soh != str[len])
                ++len;

        switch (len) {
        case 0:
                is = 0;
                break;
        case 1:
                is = ((uint_fast32_t)(str[0]));
                break;
        case 2:
                is = (((uint_fast32_t)str[1] << 8) |
                      (uint_fast32_t)str[0]);
                break;
        case 3:
                is = (((uint_fast32_t)str[2] << 16) |
                      ((uint_fast32_t)str[1] << 8) |
                      (uint_fast32_t)str[0]);
                break;
        case 4:
                is = (((uint_fast32_t)str[3] << 24) |
                      ((uint_fast32_t)str[2] << 16) |
                      ((uint_fast32_t)str[1] << 8) |
                      (uint_fast32_t)str[0]);
                break;
        default:
                return fmt_CustomMsg;
        }

        it = fix_msgtypes_map->find(is);
        if (fix_msgtypes_map->end() == it)
                return fmt_CustomMsg;

        return it->second;
}

int
is_session_message(const char soh,
                   const char * const str)
{
        uint_fast32_t is;
        unsigned int len = 0;
        std::map<uint_fast32_t, int>::const_iterator it;

        while (soh != str[len])
                ++len;

        switch (len) {
        case 0:
                is = 0;
                break;
        case 1:
                is = ((uint_fast32_t)(str[0]));
                break;
        case 2:
                is = (((uint_fast32_t)str[1] << 8) |
                      (uint_fast32_t)str[0]);
                break;
        case 3:
                is = (((uint_fast32_t)str[2] << 16) |
                      ((uint_fast32_t)str[1] << 8) |
                      (uint_fast32_t)str[0]);
                break;
        case 4:
                is = (((uint_fast32_t)str[3] << 24) |
                      ((uint_fast32_t)str[2] << 16) |
                      ((uint_fast32_t)str[1] << 8) |
                      (uint_fast32_t)str[0]);
                break;
        default:
                return 0;
        }

        it = fix_session_msgtypes_map->find(is);
        if (fix_session_msgtypes_map->end() == it)
                return 0;

        return 1;
}
