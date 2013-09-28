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

#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/log/log.h"
#include "db_utils.h"

#define CREATE_RECV_MSG_TABLE "CREATE TABLE IF NOT EXISTS RECV_MESSAGES (seqnum INTEGER PRIMARY KEY, timestamp_seconds INTEGER, timestamp_microseconds INTEGER, msg BLOB)"
#define CREATE_SENT_MSG_TABLE "CREATE TABLE IF NOT EXISTS SENT_MESSAGES (seqnum INTEGER PRIMARY KEY, timestamp_seconds INTEGER, timestamp_microseconds INTEGER, ttl_seconds INTEGER, ttl_useconds INTEGER, msg_type TEXT, partial_msg_length INTEGER, partial_msg BLOB)"

#define INSERT_RECV_MESSAGE "INSERT OR REPLACE INTO RECV_MESSAGES(seqnum, timestamp_seconds, timestamp_microseconds, msg) VALUES(?1, ?2, ?3, ?4)"
#define INSERT_SENT_MESSAGE "INSERT OR REPLACE INTO SENT_MESSAGES(seqnum, timestamp_seconds, timestamp_microseconds, ttl_seconds, ttl_useconds, msg_type, partial_msg_length, partial_msg) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)"

#define SELECT_MAX_RECV_SEQNUM "SELECT MAX(seqnum) FROM RECV_MESSAGES"
#define SELECT_MAX_SENT_SEQNUM "SELECT MAX(seqnum) FROM SENT_MESSAGES"

/*
 * result = x - y. Subtract the `struct timeval' values X and Y,
 * storing the result in result.
 */
static int
timeval_subtract(struct timeval * const result, 
		 struct timeval * const x, 
		 struct timeval * const y)
{
	int nsec;

	// Perform the carry for the later subtraction by updating y
	if (x->tv_usec < y->tv_usec) {
		nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
     
	// Compute the time remaining to wait. tv_usec is certainly
	// positive
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	// Return 1 if result is negative
	return (x->tv_sec < y->tv_sec);
}

/*
 * Returns 1 (one) if now os greater than ttl, 0 (zero) otherwise.
 */
static inline int
is_ttl_expired(const time_t ttl_tv_sec,
               const suseconds_t ttl_tv_usec,
	       struct timeval & remaining_ttl)
{
        struct timeval ttl = { ttl_tv_sec, ttl_tv_usec };
        struct timeval now;

        gettimeofday(&now, NULL);
	if (timeval_subtract(&remaining_ttl, &ttl, &now)) {
		remaining_ttl.tv_sec = 0;
		remaining_ttl.tv_usec = 0;
	}

	return (!remaining_ttl.tv_sec && !remaining_ttl.tv_usec);
}


int
MsgDB::open(void)
{
        int ret;
        char *err_msg = NULL;

        if (db_)
                return 1;

        if (!db_path_)
                return 0;

        ret = sqlite3_open(db_path_, &db_);
        if (SQLITE_OK != ret) {
                M_ALERT("could not open local database");
                goto err;
        }
        sqlite3_exec(db_, "PRAGMA journal_mode=WAL", NULL, NULL, &err_msg);
        if (SQLITE_OK != ret) {
                M_ALERT("could not enable WAL");
                /* continuing in default mode */
        }

        ret = sqlite3_exec(db_, CREATE_RECV_MSG_TABLE, NULL, NULL, &err_msg);
        if (SQLITE_OK != ret) {
                M_ALERT("could not create recv_msg table");
                goto err;
        }

        ret = sqlite3_exec(db_, CREATE_SENT_MSG_TABLE, NULL, NULL, &err_msg);
        if (SQLITE_OK != ret) {
                M_ALERT("could not create sent_msg table");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, INSERT_RECV_MESSAGE, -1,  &insert_recv_msg_statement_, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could not prepare insert recv msg statement");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, INSERT_SENT_MESSAGE, -1,  &insert_sent_msg_statement_, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could not prepare insert recv msg statement");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, SELECT_MAX_RECV_SEQNUM, -1,  &max_recv_seqnum_statement_, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could not prepare max recv seqnum statement");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, SELECT_MAX_SENT_SEQNUM, -1,  &max_sent_seqnum_statement_, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could not prepare max sent seqnum statement");
                goto err;
        }

        return 1;

err:
        if (SQLITE_OK != ret) {
                if (err_msg) {
                        M_ALERT("local database operation failed: %s", err_msg);
                } else {
                        M_ALERT("local database operation failed: %s", sqlite3_errstr(ret));
                }
        }
        sqlite3_free(err_msg);
        if (!this->close()) {
                M_ALERT("could not close local database - aborting");
                abort();
        }

        return 0;
}

int
MsgDB::close(void)
{
        int cnt = 0;
        int ret;

        if (!db_)
                return 1;

        sqlite3_finalize(insert_recv_msg_statement_);
        sqlite3_finalize(insert_sent_msg_statement_);
        sqlite3_finalize(max_recv_seqnum_statement_);
        sqlite3_finalize(max_sent_seqnum_statement_);
        insert_recv_msg_statement_ = NULL;
        insert_sent_msg_statement_ = NULL;
        max_recv_seqnum_statement_ = NULL;
        max_sent_seqnum_statement_ = NULL;

close_db:
        ret = sqlite3_close(db_);
        switch (ret) {
        case SQLITE_OK:
                break;
        case SQLITE_BUSY:
                ++cnt;
                if (5 == cnt) {
                        M_ALERT("SQLITE_BUSY for too long - aborting");
                        return 0;
                }
                sleep(1);
                goto close_db;
        case SQLITE_MISUSE:
                M_ALERT("could not close local cache: SQLITE_MISUSE");
                return 0;
        default:
                M_ALERT("trouble closing local cache: %s", sqlite3_errstr(ret));
                return 0;
        }
        db_ = NULL;

        return 1;
}

int
MsgDB::store_sent_msg(const uint64_t seqnum,
                      const uint64_t len,
                      const uint64_t ttl_tv_sec,
                      const uint64_t ttl_tv_usec,
                      const uint8_t * const msg,
                      const char * const msg_type)
{
        int ret;
        struct timeval tval;

        if (!db_)
                return 0;

        // ignore errors
        gettimeofday(&tval, NULL);

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 1, seqnum);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 2, (uint64_t)tval.tv_sec);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 3, (uint64_t)tval.tv_usec);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 4, ttl_tv_sec);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 5, ttl_tv_usec);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_text(insert_sent_msg_statement_, 6, msg_type, -1, SQLITE_TRANSIENT);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 7, len);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_blob(insert_sent_msg_statement_, 8, msg, len, SQLITE_TRANSIENT);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_step(insert_sent_msg_statement_);
        if (SQLITE_DONE != ret) {
                M_ALERT("could not step sent msg statement: (%d) %s - %s", ret, sqlite3_errstr(ret), sqlite3_errmsg(db_));
                goto out;
        }

out:
        sqlite3_clear_bindings(insert_sent_msg_statement_);
        sqlite3_reset(insert_sent_msg_statement_);

        return ((SQLITE_DONE == ret) ? 1 : 0);
}

int
MsgDB::store_recv_msg(const uint64_t seqnum,
                      const uint64_t len,
                      const uint8_t * const msg)
{
        int ret;
        struct timeval tval;

        if (!db_)
                return 0;

        // ignore errors
        gettimeofday(&tval, NULL);

        ret = sqlite3_bind_int64(insert_recv_msg_statement_, 1, seqnum);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 2, tval.tv_sec);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_int64(insert_sent_msg_statement_, 3, tval.tv_usec);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_blob(insert_recv_msg_statement_, 4, msg, len, SQLITE_TRANSIENT);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_step(insert_recv_msg_statement_);
        if (SQLITE_DONE != ret) {
                M_ALERT("could not step recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

out:
        sqlite3_clear_bindings(insert_recv_msg_statement_);
        sqlite3_reset(insert_recv_msg_statement_);

        return ((SQLITE_DONE == ret) ? 1 : 0);
}

int
MsgDB::get_latest_recv_seqnum(uint64_t & seqnum) const
{
        int ret;

        if (!db_)
                return 0;

        ret = sqlite3_step(max_recv_seqnum_statement_);
        switch (ret) {
        case SQLITE_ROW:
                break;
        case SQLITE_DONE:
                seqnum = 0;
                goto out;
        default:
                M_ALERT("error getting latest recv seqnum");
                goto err;
        }
        seqnum = sqlite3_column_int64(max_recv_seqnum_statement_, 0);
out:
        sqlite3_reset(max_recv_seqnum_statement_);
        return 1;

err:
        sqlite3_reset(max_recv_seqnum_statement_);
        M_ALERT("could not get latest recv seqnum: %s", sqlite3_errstr(ret));
        return 0;
}

int
MsgDB::get_latest_sent_seqnum(uint64_t & seqnum) const
{
        int ret;

        ret = sqlite3_step(max_sent_seqnum_statement_);
        switch (ret) {
        case SQLITE_ROW:
                break;
        case SQLITE_DONE:
                seqnum = 0;
                goto out;
        default:
                M_ALERT("error getting latest sent seqnum");
                goto err;
        }
        seqnum = sqlite3_column_int64(max_sent_seqnum_statement_, 0);
out:
        sqlite3_reset(max_sent_seqnum_statement_);
        return 1;

err:
        sqlite3_reset(max_sent_seqnum_statement_);
        M_ALERT("could not get latest sent seqnum: %s", sqlite3_errstr(ret));
        return 0;
}

PartialMessageList*
MsgDB::get_sent_msgs(uint64_t start,
                     uint64_t end) const
{
        static const char *select_fmt_no_end = "SELECT ttl_seconds, ttl_useconds, msg_type, partial_msg_length, partial_msg FROM SENT_MESSAGES WHERE seqnum >= %llu";
        static const char *select_fmt = "SELECT ttl_seconds, ttl_useconds, msg_type, partial_msg_length, partial_msg FROM SENT_MESSAGES WHERE seqnum >= %llu AND seqnum <= %llu";
        PartialMessageList *retv = NULL;
        PartialMessage *pmsg = NULL;
        sqlite3_stmt *select_statement = NULL;
        char select_str[256];
        time_t ttl_sec;
        suseconds_t ttl_usec;
        int ret;
        const void *part_msg = NULL;

        if (!db_)
                return NULL;

	if (!end) {
		sprintf(select_str, select_fmt_no_end, start);
	} else {
		sprintf(select_str, select_fmt, start, end);
	}
        ret = sqlite3_prepare_v2(db_, select_str, -1,  &select_statement, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could not prepare select sent messages statement: ret = %d", ret);
                goto out;
        }
        retv = new (std::nothrow) PartialMessageList;
        if (!retv)
                goto out;

        do {
                ret = sqlite3_step(select_statement);
                switch (ret) {
                case SQLITE_ROW:
                        break;
                case SQLITE_BUSY:
                        continue;
                case SQLITE_DONE:
                        goto out;
                default:
                        M_ALERT("error selecting sent messages: ret = %d", ret);
                        goto out;
                }

                pmsg = new (std::nothrow) PartialMessage;
                if (!pmsg) {
                        delete retv;
			M_ALERT("no memory");
                        retv = NULL;
                        goto out;
                }

                ttl_sec = (time_t)sqlite3_column_int64(select_statement, 0);
                ttl_usec = (suseconds_t)sqlite3_column_int64(select_statement, 1);
                if (is_ttl_expired(ttl_sec, ttl_usec, pmsg->ttl)) {
			retv->push_back(pmsg); 
                        continue;
		}

                pmsg->msg_type = strdup((const char*)sqlite3_column_text(select_statement, 2));
                pmsg->length = sqlite3_column_int64(select_statement, 3);
                pmsg->part_msg = (uint8_t*)malloc(pmsg->length + 5); // "+ 5" is to avoid realloc when inserting "43=Y<SOH>"
		if (!pmsg->part_msg) {
			delete pmsg;
                        delete retv;
			M_ALERT("no memory");
                        retv = NULL;
                        goto out;
		}
                part_msg = sqlite3_column_blob(select_statement, 4);
                memcpy((void*)pmsg->part_msg, (const uint8_t*)part_msg, pmsg->length);

                retv->push_back(pmsg);
                pmsg = NULL;
        } while (1);

out:
        sqlite3_finalize(select_statement);

        return retv;
}

std::vector<std::vector<uint8_t> >*
MsgDB::get_recv_msgs(uint64_t /*start*/,
                     uint64_t /*end*/) const
{
        std::vector<std::vector<uint8_t> > *retv = NULL;

        if (!db_)
                return NULL;

        return retv;
}
