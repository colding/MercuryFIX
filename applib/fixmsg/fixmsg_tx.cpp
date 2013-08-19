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

#include "fixmsg.h"

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/log/log.h"
#include "stdlib/process/cpu.h"
#include "applib/fixmsg/fixmsg_utils.h"

int
FIXMessageTX::append_field(const unsigned int tag,
                           const size_t length,
                           const uint8_t *value)
{
        const uint8_t *start = pos_;

        if (35 != tag) {
                // we'll need to decide whether to extend buffer or
                // not and we need to do it with a minimum of
                // effort. So... I'm making a best effort guess here.
                //
                // I'll assume (rather safely I think) that "tag=" is
                // never more than 21 characters long. Than I'm adding
                // that to "length + 4" ("4" needed to make room for
                // "<SOH>10="). If that makes the buffer overflow,
                // I'll reallocate.

        again:
                if (25 + length <= buf_size_) {
                        uint_to_str('=', tag, (char**)&pos_);
                        ++pos_;

                        memcpy(pos_, value, length);
                        pos_ += length;
                        *pos_ = soh_;
                        ++pos_;
                        length_ += pos_ - start;
                } else {
                        uint8_t *tmp = buf_;

                        tmp = (uint8_t*)realloc(buf_, 2 * buf_size_);
                        if (tmp) {
                                buf_ = tmp;
                                buf_size_ *= 2;
                                goto again;
                        } else {
                                return 0;
                        }
                }
        } else {
                if (CACHE_LINE_SIZE > length) {
                        memcpy(msg_type_, value, length);
                        msg_type_[length] = '\0';
                } else {
                        return 0;
                }
        }

        return 1;
}

int
FIXMessageTX::expose(size_t & len,
                     const uint8_t **data,
                     const char **msg_type)
{
        if (msg_type[0]) {
                // tack on "10="
                *(pos_) = '1';
                *(pos_ + 1) = '0';
                *(pos_ + 2) = '=';

                len = length_ + 3;
                *data = buf_;
                *msg_type = msg_type_;

                // don't overwrite leading SOH
                length_ = 1;
                pos_ = buf_ + 1;
        } else {
                return 0;
        }

        return 1;
}
