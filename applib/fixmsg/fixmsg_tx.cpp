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
#include "applib/fixutils/fixmsg_utils.h"

/*
 * Will return k iff k is a power of two, or the next power of two
 * which is greater than k.
 */
static inline size_t
next_power_of_two(size_t k)
{
        size_t i;

        if (!k)
                return 1;

        --k;
        for (i = 1; i < sizeof(size_t)*CHAR_BIT; i <<= 1)
                k = k | k >> i;

        return ++k;
}

int
FIXMessageTX::append_field(const unsigned int tag,
                           const size_t length,
                           const uint8_t *value)
{
        const uint8_t *start = pos_;

	if (52 == tag)
		sending_time_appended_ = 1;

        if (35 != tag) {
                // we'll need to decide whether to extend buffer or
                // not and we need to do it with a minimum of
                // effort. So... I'm making a best effort guess here.
                //
                // I'll assume (rather safely I think) that "tag=" is
                // never more than 21 characters long. Then I'm adding
                // that to "length + 4" (4 bytes is needed to make
                // room for "<SOH>10=").
		//
		// If that would make the buffer overflow, I'll
		// reallocate.

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

			if (buf_size_) {
				tmp = (uint8_t*)realloc(buf_, 2 * buf_size_);
				if (tmp) {
					buf_ = tmp;
					buf_size_ *= 2;
				} else {
					return 0;
				}
			} else {
				if (!init())
					return 0;
			}
			goto again;
                }
        } else {
                if (MAX_MSGTYPE_LENGTH > length) {
                        memcpy(msg_type_, value, length);
                        msg_type_[length] = '\0';
                } else {
                        return 0;
                }
        }

        return 1;
}

int
FIXMessageTX::expose(const struct timeval **ttl,
		     size_t & len,
                     const uint8_t **data,
                     const char **msg_type)
{
        if (msg_type_[0] && sending_time_appended_) {

                // tack on "10="
                *(pos_) = '1';
                *(pos_ + 1) = '0';
                *(pos_ + 2) = '=';

		*ttl = &ttl_;
                len = length_ + 3;
                *data = buf_;
                *msg_type = msg_type_;

                // re-use already allocated memory but don't overwrite
                // leading SOH
                length_ = 1;
                pos_ = buf_ + 1;

		// reset indicator for sending time appended
		sending_time_appended_ = 0;
        } else {
                return 0;
        }

        return 1;
}

int 
FIXMessageTX::clone_from(const PartialMessage * const pmsg)
{
	if (!pmsg || !pmsg->length) {
		init();
	} else {
		if (pmsg->length > buf_size_) {
			uint8_t *tmp = buf_;

			buf_size_ = next_power_of_two(pmsg->length);
			tmp = (uint8_t*)realloc(buf_, buf_size_);
			if (tmp) {
				buf_ = tmp;
			} else {
				init();
				return 0;
			}
		}

		// we do not copy the terminating "10="
		memcpy(buf_, pmsg->part_msg, pmsg->length - 3);
		pos_ = buf_ + pmsg->length - 3;
		length_ = pmsg->length - 3;

		if (MAX_MSGTYPE_LENGTH > strlen(pmsg->msg_type)) {
			memcpy(msg_type_, pmsg->msg_type, strlen(pmsg->msg_type));
			msg_type_[strlen(pmsg->msg_type)] = '\0';
		} else {
			init();
			return 0;
		}

		char st[5]; // "<SOH>52="
		sprintf(st, "%c52=", soh_);
		sending_time_appended_ = strnstr((const char*)pmsg->part_msg, st, length_) ? 1 : 0;
	} 
	ttl_.tv_sec = pmsg ? pmsg->ttl.tv_sec : 0;
	ttl_.tv_usec = pmsg ? pmsg->ttl.tv_usec : 0;

	return 1;
}

