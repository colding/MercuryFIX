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

#include <inttypes.h>
#include <string.h>
//#include <pthread.h>
#include <vector>
#include <string>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/local_db/sqlite3.h"

class PartialMessage {
public:
	PartialMessage() 
		: length(0), 
		  msg_type(NULL),
		  part_msg(NULL)
		{
		};

	~PartialMessage()
		{
			free(msg_type);
			free(part_msg);
		};

	uint32_t length;
	char *msg_type;
	uint8_t *part_msg;
};

class PartialMessageList {
public:
	~PartialMessageList()
		{
			unsigned int n;

			for (n = 0; n < list_.size(); ++n)
				delete list_[n];
		};

	size_t size(void) const
		{
			return list_.size();
		};

	void push_back(PartialMessage *pmsg)
		{
			list_.push_back(pmsg);
		};

	/*
	 * Returns message number n. If the message in question has
	 * exceeded its time to live, then a NULL will be
	 * returned. NULL will also be returned if n >= size().
	 */ 
	const PartialMessage *get_at(const unsigned int n) const
		{
			if (n > (list_.size() - 1))
				return NULL;

			return list_[n];
		};

private:
	std::vector<PartialMessage*> list_;
};

class MsgDB {
public:

        MsgDB(void)
		: db_(NULL),
		  db_path_(NULL),
		  insert_recv_msg_statement_(NULL),
		  insert_sent_msg_statement_(NULL),
		  max_recv_seqnum_statement_(NULL),
		  max_sent_seqnum_statement_(NULL)
                {
                };

        ~MsgDB()
                {
                        this->close();
                        free(db_path_);
                };

        /*
         * Does whatever needs doing to get the database initialized
         * and ready for action.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
         */
        int open(void);

        /*
         * On-disk database file path
         */
        int set_db_path(const char * const db_path)
                {
                        free(db_path_);
                        db_path_ = strdup(db_path);

                        return (NULL != db_path_);
                }

        /*
         * Returns when the underlying store is closed. Blocks
         * potentially for a long(-ish) time.
         *
         * Returns 1 (one) if all is well, 0 (zero) if not.
         */
        int close(void);

        /*
         * Stores outgoing partial messages.
         *
         * seqnum: Sequence number
         * msg: The partial message
         * len: Number of bytes in msg
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int store_sent_msg(const uint64_t seqnum,
                           const uint64_t len,
			   const uint64_t ttl_tv_sec,
			   const uint64_t ttl_tv_usec,
                           const uint8_t * const msg,
                           const char * const msg_type);

        /*
         * Stores incoming complete messages.
         *
         * seqnum: Sequence number
         * msg: The complete message
         * len: Number of bytes in msg
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int store_recv_msg(const uint64_t seqnum,
                           const uint64_t len,
                           const uint8_t * const msg);

        /*
         * Returns the sequence number of last recieved message or 0
         * if the session is new.
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int get_latest_recv_seqnum(uint64_t & seqnum) const;

        /*
         * Returns the sequence number of last transmitted message or
         * 0 if the session is new.
         *
         * Returns 1 (one) if all is well, 0 (zero) otherwise.
         */
        int get_latest_sent_seqnum(uint64_t & seqnum) const;

        /*
         * Returns a vector of previously sent partial messages ready
         * to be re-sent starting with sequence number "start" and
         * ending with sequence number "end", both included.
         *
         * If "end" is larger than the largest sequence number then
         * all messages, starting with "start", is returned.
         *
         * This method is not performance critical (re-sending is a
         * fairly rare occurrence) and the implementation reflects
         * that.
	 *
	 * Only messages within their respective TTL will be
	 * returned. Messages which has exceeded their time to live
	 * will be returned as NULL.
	 *
	 * Memory allocation errors will result in NULL being returned.
         */
        PartialMessageList *get_sent_msgs(uint64_t start, uint64_t end) const;

        /*
         * Returns a vector of previously recieved complete messages
         * starting with sequence number "start" and ending with
         * sequence number "end", both included.
         *
         * If "end" is larger than the largest sequence number then
         * all messages, starting with "start", is returned.
         *
         * This method is not performance critical and the
         * implementation reflects that.
         */
        std::vector<std::vector<uint8_t> > *get_recv_msgs(uint64_t start, uint64_t end) const;

private:
        sqlite3 *db_;
        char *db_path_;
        sqlite3_stmt *insert_recv_msg_statement_;
        sqlite3_stmt *insert_sent_msg_statement_;
        sqlite3_stmt *max_recv_seqnum_statement_;
        sqlite3_stmt *max_sent_seqnum_statement_;
};
