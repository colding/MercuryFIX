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
#include <inttypes.h>

/*
 * Functions to manipulate the generic Mercury data structure which is
 * used for IPC over network sockets. It consist of an IPC header plus
 * an optional data segment. The layout is:
 *
 * 4 bytes | 4 bytes | (0 <= n) bytes
 * COMMAND   LENGTH    <DATA>
 *
 * COMMAND: A uint32_t value. In big endian byte order.  Part of the
 *          IPC header.
 *
 * LENGTH: A uint32_t specifying the length in bytes of the following
 *         data array. In big endian byte order. May be 0 (zero). Part
 *         of the IPC header.
 *
 * DATA: An array of uint8_t. The layout of this array is determined
 *       by the value of COMMAND. This array is non-present if LENGTH
 *       is 0 (zero). All encoded numbers are in big endian byte
 *       order.
 */


#ifdef HAVE_LITTLE_ENDIAN
static inline void
setu64(uint8_t * const buf,
       const uint64_t val)
{
        buf[0] = (val & 0xFF00000000000000) >> 56;
        buf[1] = (val & 0x00FF000000000000) >> 48;
        buf[2] = (val & 0x0000FF0000000000) >> 40;
        buf[3] = (val & 0x000000FF00000000) >> 32;
        buf[4] = (val & 0x00000000FF000000) >> 24;
        buf[5] = (val & 0x0000000000FF0000) >> 16;
        buf[6] = (val & 0x000000000000FF00) >> 8;
        buf[7] =  val & 0x00000000000000FF;
}

static inline uint64_t
getu64(const uint8_t * const buf)
{
        uint64_t retv = 0;
        uint64_t tmp = buf[0];

        retv |= tmp << 56;

        tmp = buf[1];
        retv |= tmp << 48;

        tmp = buf[2];
        retv |= tmp << 40;

        tmp = buf[3];
        retv |= tmp << 32;

        tmp = buf[4];
        retv |= tmp << 24;

        tmp = buf[5];
        retv |= tmp << 16;

        tmp = buf[6];
        retv |= tmp << 8;

        retv |= buf[7];

        return retv;
}

static inline void
setu32(uint8_t * const buf,
       const uint32_t val)
{
        buf[0] = (val & 0xFF000000) >> 24;
        buf[1] = (val & 0x00FF0000) >> 16;
        buf[2] = (val & 0x0000FF00) >> 8;
        buf[3] =  val & 0x000000FF;
}

static inline uint32_t
getu32(const uint8_t * const buf)
{
        return ((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]);
}

static inline void
setu16(uint8_t * const buf,
       const uint16_t val)
{
        buf[0] = (val & 0xFF00) >> 8;
        buf[1] =  val & 0x00FF;
}

static inline uint16_t
getu16(const uint8_t * const buf)
{
        return ((buf[0] << 8) | buf[1]);
}
#endif // HAVE_LITTLE_ENDIAN

#ifdef HAVE_BIG_ENDIAN
static inline void
setu64(uint8_t * const buf,
       const uint64_t val)
{
        buf[7] = (val & 0xFF00000000000000) >> 56;
        buf[6] = (val & 0x00FF000000000000) >> 48;
        buf[5] = (val & 0x0000FF0000000000) >> 40;
        buf[4] = (val & 0x000000FF00000000) >> 32;
        buf[3] = (val & 0x00000000FF000000) >> 24;
        buf[2] = (val & 0x0000000000FF0000) >> 16;
        buf[1] = (val & 0x000000000000FF00) >> 8;
        buf[0] =  val & 0x00000000000000FF;
}

static inline uint64_t
getu64(const uint8_t * const buf)
{
        uint64_t retv = 0;
        uint64_t tmp = buf[7];

        retv |= tmp << 56;

        tmp = buf[6];
        retv |= tmp << 48;

        tmp = buf[5];
        retv |= tmp << 40;

        tmp = buf[4];
        retv |= tmp << 32;

        tmp = buf[3];
        retv |= tmp << 24;

        tmp = buf[2];
        retv |= tmp << 16;

        tmp = buf[1];
        retv |= tmp << 8;

        retv |= buf[0];

        return retv;
}

static inline void
setu32(uint8_t * const buf,
       const uint32_t val)
{
        buf[3] = (val & 0xFF000000) >> 24;
        buf[2] = (val & 0x00FF0000) >> 16;
        buf[1] = (val & 0x0000FF00) >> 8;
        buf[0] =  val & 0x000000FF;
}

static inline uint32_t
getu32(const uint8_t * const buf)
{
        return ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0]);
}

static inline void
setu16(uint8_t * const buf,
       const uint16_t val)
{
        buf[1] = (val & 0xFF00) >> 8;
        buf[0] = (val & 0x00FF);
}

static inline uint16_t
getu16(const uint8_t * const buf)
{
        return ((buf[1] << 8) | buf[0]);
}
#endif // HAVE_BIG_ENDIAN
