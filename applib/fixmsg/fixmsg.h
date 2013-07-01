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
#include "applib/fixmsg/fix_types.h"

#define INITAL_TX_BUFFER_SIZE (2048)

/*
 * FIXMessageTX will prepare a message for sending by the FIXIO
 * framework, specifically a FIX_Pusher instance.
 */
class FIXMessageTX
{
public:
	/*
	 * strict: Guarantee that the messge is composed correctly
	 * according to the FIX specification. Only use this while
	 * testing as it is quite expensive.
	 */
	FIXMessage(const FIX_Version fix_version,
		   const FIX_MsgType msg_type,
		   const bool strict)
		: fix_version_(fix_version), 
		  msg_type_(msg_type), 
		  strict_(strict)
		{
			buf_size_ = INITAL_TX_BUFFER_SIZE;
			extbuf_ = NULL;
			buf_ = stdbuf_;
		};

	void reset(void);

	int insert_tag(const type_t type,
		       const uint32_t tag,
		       const void *value);

	void expose(size_t *len,
		    uint8_t **data,
		    char **msg_type) const;

private:
	FIXMessage(void)
		{
		};

	const FIX_Version fix_version_;
	const FIX_MsgType msg_type_;
	const bool strict_;
	size_t buf_size_;
	uint8_t *buf_;
	uint8_t *extbuf_;
	uint8_t stdbuf_[INITAL_TX_BUFFER_SIZE];
	std::set<int> required_fields_;
};

/*
 * FIXMessageRX will handle a recieved message from the FIXIO
 * framework, specifically from a FIX_Popper instance.
 */
class FIXMessageRX
{
public:
	FIXMessage(void)
		{
			take_ownership_ = 0;
			len_ = 0;
			msgtype_offset_ = 0;
			msg_ = NULL;
		};

	FIXMessage(const int take_ownership,
		   const uint32_t len,
		   const uint32_t msgtype_offset,
		   uint8_t *msg)
		{
			take_ownership_ = take_ownership;
			len_ = len;
			msgtype_offset_ = msgtype_offset;
			msg_ = msg;
		};

	import(const int take_ownership,
	       const uint32_t len,
	       const uint32_t msgtype_offset,
	       uint8_t *msg)
		{
			if (take_ownership_)
				free(msg);

			take_ownership_ = take_ownership;
			len_ = len;
			msgtype_offset_ = msgtype_offset;
			msg_ = msg;
		};

private:
	int take_ownership_;
	uint32_t len_;
	uint32_t msgtype_offset_;
	uint8_t *msg_;
};


