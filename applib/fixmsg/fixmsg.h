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
#include <set>
#include <map>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "stdlib/disruptor/memsizes.h"
#include "applib/fixmsg/fix_types.h"
#include "applib/fixutils/db_utils.h"

#define INITIAL_TX_BUFFER_SIZE (2048)
#define MAX_MSGTYPE_LENGTH (CACHE_LINE_SIZE)

/*
 * FIXMessageTX will prepare a message for sending by the FIXIO
 * framework, specifically a FIX_PushBase instance. Instances of this
 * class must be pooled.
 */
class FIXMessageTX
{
public:
        FIXMessageTX(const char soh)
                : soh_(soh),
		  ttl_{0,0},
		  msg_type_{'\0'},
                  buf_size_(0),
                  length_(0),
		  sending_time_appended_(0),
                  buf_(NULL),
                  pos_(NULL)
                {
                };

        virtual ~FIXMessageTX()
                {
                        free(buf_);
                };

        /*
         * Must be invoked before an instance of this class is set to
         * work. This is a heavy operation so objects of this class
         * must be pooled. They are not cheap to create.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
         */
        int init(void)
                {
			free(buf_);
                        buf_ = (uint8_t*)malloc(INITIAL_TX_BUFFER_SIZE);
                        pos_ = buf_;
                        if (pos_) {
				buf_size_ = INITIAL_TX_BUFFER_SIZE;
                                *pos_ = soh_;
                                length_ = 1;
                                ++pos_;
                        }
                        msg_type_[0] = '\0';
			sending_time_appended_ = 0;

			ttl_.tv_sec = 0;
			ttl_.tv_usec = 0;

                        return (buf_ ? 1 : 0);
                };

        /*
         * Appends a FIX field, in order, into the
         * message. insert_value() does not take ownership of the
         * memory pointed to by value.
         *
         * len must not be bigger than (CACHE_LINE_SIZE - 1) if, and
         * only if, tag is 35 (MsgType).
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
	 *
	 * NOTE: It is required that sending time (tag 52) is appended
	 * before any data typed field.
         */
        virtual int append_field(const unsigned int tag,
				 const size_t length,
				 const uint8_t *value);

	/*
	 * Determines the time to live (ttl) for this particular
	 * message. The ttl will remain valid for this instance until
	 * overwritten by another call to set_time_to_live() or
	 * clone_from().
	 *
	 * Messages which are expired will never be re-sent.
	 *
	 */
	void set_time_to_live(const time_t seconds,
			      const time_t micro_seconds)
		{
			ttl_.tv_sec = seconds;
			ttl_.tv_usec = micro_seconds;
		};

        /*
         * Exposes information required by FIX_PushBase::push(). The
         * first invocation of insert_field() after this method has
         * been invoked, will be inserting data into a blank message.
         *
         * expose() will fail if message type (tag 35) or sending time
         * (tag 52) has not been inserted.
         *
         * *msg_type will remain valid until a new message type is
         * inserted using insert_field(). You may refrain from
         * inserting a new message type if the message type hasn't
         * changed since your previous expose().
         *
         * The message type is cached within the FIXMessageTX and only
         * changed when overwritten by insert_field() of a new tag 35.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
         */
        int expose(const struct timeval **ttl,
		   size_t & len,
                   const uint8_t **data,
                   const char **msg_type);

	/*
	 * Imports the partial message's state into this TX
	 * object. Any state present will be overwritten. You can
	 * continue calling append_field() or expose() after the
	 * partial message has been imported.
	 *
	 * The FIXMessageTX will be left in an initial state if pmsg
	 * is NULL.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
	 */
	int clone_from(const PartialMessage * const pmsg);

private:
        const char soh_;
	struct timeval ttl_;
        char msg_type_[MAX_MSGTYPE_LENGTH];
        size_t buf_size_;
        size_t length_;
	int sending_time_appended_;
        uint8_t *buf_;
        uint8_t *pos_;
};

/*
 * FIXMessageRX will handle a recieved message from the FIXIO
 * framework, specifically from a FIX_Popper instance. Instances of
 * this class must be pooled.
 */
class FIXMessageRX
{
public:
        /*
         * Named constructors. Makes it explicit whether or not the
         * instance owns the memory of the message or not. done() or
         * the destructor will free() it if it does.
         */
        static FIXMessageRX make_fix_message_mem_owner_on_stack(const FIX_Version version,
                                                                const char soh);

        static FIXMessageRX *make_fix_message_mem_owner_on_heap(const FIX_Version version,
                                                                const char soh);

        static FIXMessageRX make_fix_message_with_provided_mem_on_stack(const FIX_Version version,
                                                                        const char soh);

        static FIXMessageRX *make_fix_message_with_provided_mem_on_heap(const FIX_Version version,
                                                                        const char soh);
        virtual ~FIXMessageRX()
                {
                        if (owns_memory_)
                                free(msg_);
                };


        /*
         * Must be invoked before an instance of this class is set to
         * work. This is a heavy operation so objects of this class
         * must be pooled. They are not cheap to create.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
         */
        int init(void);

        /*
         * Any custom tags must be explicitly added here. They must be
         * added before traversing the message using next_field().
         *
         * These custom tags can never be removed and they will
         * overwrite the standard tags in case of a tag collision.
         *
         * Again, this is a heavy operation and should only be invoked
         * once in the objects lifetime.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
         */
        int add_custom_tag(const struct FIX_Tag & custom_tag);

        /*
         * Regardless of whether the instance owns the memory of the
         * message or not, this method makes the instance able to read
         * and write to the message memory area.
         */
        void imprint(const uint32_t msgtype_offset,
                     uint8_t * const msg)
                {
                        if (owns_memory_)
                                free(msg_);
                        msg_ = msg;
                        prev_value_ = NULL;
                        pos_ = msg_ + msgtype_offset - 3; // reverse over "35=" to point at '3'
                };

        /*
         * Resets the RX message and readies it for another imprint().
         */
        void done(void)
                {
                        if (owns_memory_)
                                free(msg_);
                        msg_ = NULL;
                        pos_ = NULL;
                        prev_value_ = NULL;
                };


        /*
         * Used to traverse the message fields.
         *
         * Parameters:
         *
         * length - The size in bytes of *value
         * value  - A pointer to the first byte in the field value
         *
         * Returns:
         *
         *  - the tag value (a number always greater than zero) if all
         *    is well.
         *
         *  - 0 (zero) if there are no more fields in the message or if
         *    none has been imprinted upon it.
         *
         * - -1 (minus one) if there is a serious parsing error. The
         *   message in question must be rejected. Further
         *   invocations of this method will have undefined effects.
         *
         * Parsing errors must always result in a session level reject
         * (FIX MsgType 35=3).
         *
         * The first field to be returned is always tag 35 (MsgType).
         *
         * The last field to be returned in full is whichever
         * immediately preceeds the checksum field. The next
         * invocation of next_field() will return 0 (zero) to signal
         * that the end of the FIX message has been reached. lenght
         * and value are undefined when 0 (zero) is returned. All
         * further invocations of next_field() have undefined, but
         * likely bad, behaviour.
         *
         * The caller has exclusive read/write access to the message
         * until done() is called. But, and this is a big but, the
         * behavior is undefined if the caller actually do write to
         * it. Please grok the next_field() code to learn when writes
         * are allowed and when not.
         *
         * IMPORTANT: The standard is not quite crystal clear on this,
         * but my interpretation is that the '=' delimiting the tag
         * and the value does not have bytes on either side which are
         * not part of the tag or the value.
         */
        int next_field(size_t & length, uint8_t **value);

protected:
        FIXMessageRX(const FIX_Version version,
                     const int owns_memory,
                     const char soh)
                : version_(version),
                  owns_memory_(owns_memory),
                  soh_(soh),
                  msg_(NULL),
                  pos_(NULL),
                  prev_value_(NULL)
                {
                };

private:
        /*
         * Returns 1 (one) if the tag is of type ft_data, 0 (zero)
         * otherwise.
         */
        int field_contains_data(unsigned int tag);

        const FIX_Version version_;
        const int owns_memory_;
        const char soh_;
        std::set<unsigned int> data_tags_;
        std::map<unsigned int, FIX_Type> tags_;
        uint8_t *msg_;
        uint8_t *pos_;
        uint8_t *prev_value_;
};

inline FIXMessageRX
FIXMessageRX::make_fix_message_mem_owner_on_stack(const FIX_Version version,
                                                  const char soh)
{
        return FIXMessageRX(version, 1, soh);
}

inline FIXMessageRX*
FIXMessageRX::make_fix_message_mem_owner_on_heap(const FIX_Version version,
                                                 const char soh)
{
        return new FIXMessageRX(version, 1, soh);
}

inline FIXMessageRX
FIXMessageRX::make_fix_message_with_provided_mem_on_stack(const FIX_Version version,
                                                          const char soh)
{
        return FIXMessageRX(version, 0, soh);
}

inline FIXMessageRX*
FIXMessageRX::make_fix_message_with_provided_mem_on_heap(const FIX_Version version,
                                                         const char soh)
{
        return new FIXMessageRX(version, 0, soh);
}
