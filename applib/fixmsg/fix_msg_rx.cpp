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

#include "fix_msg.h"

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "fixmsg_utils.h"
#include <stdio.h>

int
FIXMessageRX::field_contains_data(unsigned int tag)
{
        std::set<unsigned int>::const_iterator it;

        it = data_tags_.find(tag);
        if (data_tags_.end() == it)
                return 0;

        return 1;
}

int
FIXMessageRX::init(void)
{
        const struct FIX_Tag *fix_tags;
        const struct FIX_Tag *fix_data_tags;

        switch (version_) {
        case FIX_4_0:
                fix_tags = fix40_std_tags;
                fix_data_tags = fix40_std_data_tags;
                break;
        case FIX_4_1:
                fix_tags = fix41_std_tags;
                fix_data_tags = fix41_std_data_tags;
                break;
        case FIX_4_2:
                fix_tags = fix42_std_tags;
                fix_data_tags = fix42_std_data_tags;
                break;
        case FIX_4_3:
                fix_tags = fix43_std_tags;
                fix_data_tags = fix43_std_data_tags;
                break;
        case FIX_4_4:
                fix_tags = fix44_std_tags;
                fix_data_tags = fix44_std_data_tags;
                break;
        case FIX_5_0:
                fix_tags = fix50_std_tags;
                fix_data_tags = fix50_std_data_tags;
                break;
        case FIX_5_0_SP1:
                fix_tags = fix50sp1_std_tags;
                fix_data_tags = fix50sp1_std_data_tags;
                break;
        case FIX_5_0_SP2:
                fix_tags = fix50sp2_std_tags;
                fix_data_tags = fix50sp2_std_data_tags;
                break;
        case FIXT_1_1:
                fix_tags = fixt11_std_tags;
                fix_data_tags = fixt11_std_data_tags;
                break;
        default:
                return 0;
        }

        int n = 0;
        try {
                while (fix_tags[n].tag) {
                        tags_[fix_tags[n].tag] = fix_tags[n].type;
                        ++n;
                }

                n = 0;
                while (fix_data_tags[n].tag) {
                        data_tags_.insert(fix_data_tags[n].tag);
                        ++n;
                }
        }
        catch (...) {
                return 0;
        }

        return 1;
}

int
FIXMessageRX::add_custom_tag(const struct FIX_Tag & custom_tag)
{
        try {
                tags_[custom_tag.tag] = custom_tag.type;

                if (ft_data == custom_tag.type) {
                        data_tags_.insert(custom_tag.tag);
                }
        }
        catch (...) {
                return 0;
        }

        return 1;
}

int
FIXMessageRX::next_field(size_t & length, uint8_t **value)
{
        uint8_t *msg_pos = pos_;
        int prev_val = 0;
        int retv;

        if (!msg_pos)
                return 0;

        retv = get_fix_tag((const char**)&msg_pos);
        if (UNLIKELY__(0 >= retv))
                return -1;
        if (UNLIKELY__(10 == retv)) // This is the end - My only friend, the end
                return 0;
        *value = msg_pos;

        if (UNLIKELY__(field_contains_data(retv))) {
                prev_val = get_fix_length_value(soh_, (const char*)prev_value_);
                prev_value_ = msg_pos;
                if (0 > prev_val)
                        return -1;

                length = prev_val;
                pos_ = msg_pos + length + 1; // now points at first character after SOH
                return retv;
        }
        prev_value_ = msg_pos;

        while (soh_ != *msg_pos)
                ++msg_pos;
        length = msg_pos - prev_value_;
        pos_ = msg_pos + 1; // now points at first character after SOH

        return retv;
}
