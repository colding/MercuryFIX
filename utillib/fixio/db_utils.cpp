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

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/log/log.h"
#include "db_utils.h"

#define CREATE_RECV_MSG_TABLE "CREATE TABLE IF NOT EXISTS RECV_MESSAGES (seqnum INTEGER PRIMARY KEY, timestamp INTEGER DEFAULT CURRENT_TIMESTAMP, msg BLOB)"
#define CREATE_SENT_MSG_TABLE "CREATE TABLE IF NOT EXISTS SENT_MESSAGES (seqnum INTEGER PRIMARY KEY, timestamp INTEGER DEFAULT CURRENT_TIMESTAMP, msg_type TEXT, partial_msg BLOB)"
#define INSERT_RECV_MESSAGE "INSERT INTO RECV_MESSAGES(seqnum, msg) VALUES(?1, ?2)"
#define INSERT_SENT_MESSAGE "INSERT INTO SENT_MESSAGES(seqnum, msg_type, partial_msg) VALUES(?1, ?2, ?3)"
#define SELECT_MAX_RECV_SEQNUM "SELECT MAX(seqnum) FROM RECV_MESSAGES"
#define SELECT_MAX_SENT_SEQNUM "SELECT MAX(seqnum) FROM SENT_MESSAGES"


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

        ret = sqlite3_exec(db_, CREATE_RECV_MSG_TABLE, NULL, NULL, &err_msg);
        if (SQLITE_OK != ret) {
                M_ALERT("could create recv_msg table");
                goto err;
        }

        ret = sqlite3_exec(db_, CREATE_SENT_MSG_TABLE, NULL, NULL, &err_msg);
        if (SQLITE_OK != ret) {
                M_ALERT("could create sent_msg table");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, INSERT_RECV_MESSAGE, -1,  &insert_recv_msg_statement, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could prepare insert recv msg statement");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, INSERT_SENT_MESSAGE, -1,  &insert_sent_msg_statement, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could prepare insert recv msg statement");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, SELECT_MAX_RECV_SEQNUM, -1,  &max_recv_seqnum_statement, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could prepare max recv seqnum statement");
                goto err;
        }

        ret = sqlite3_prepare_v2(db_, SELECT_MAX_SENT_SEQNUM, -1,  &max_sent_seqnum_statement, NULL);
        if (SQLITE_OK != ret) {
                M_ALERT("could prepare max sent seqnum statement");
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
close_db:
        sqlite3_finalize(insert_recv_msg_statement);
        sqlite3_finalize(insert_sent_msg_statement);
        sqlite3_finalize(max_recv_seqnum_statement);
        sqlite3_finalize(max_sent_seqnum_statement);
        insert_recv_msg_statement = NULL;
        insert_sent_msg_statement = NULL;
        max_recv_seqnum_statement = NULL;
        max_sent_seqnum_statement = NULL;

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

/*
 * Stores outgoing partial messages.
 *
 * seqnum: Sequence number
 * len: Number of bytes in msg
 * msg: The partial message
 * msg_type: The message type
 */
int
MsgDB::store_sent_msg(const uint64_t seqnum,
                      const uint64_t len,
                      const uint8_t * const msg,
                      const char * const msg_type)
{
        int ret = SQLITE_OK;

        if (!db_)
                return 0;

        ret = sqlite3_bind_int64(insert_sent_msg_statement, 1, seqnum);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_text(insert_sent_msg_statement, 2, msg_type, -1, SQLITE_TRANSIENT);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_blob(insert_sent_msg_statement, 3, msg, len, SQLITE_TRANSIENT);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into sent msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_step(insert_sent_msg_statement);
        if (SQLITE_DONE != ret) {
                M_ALERT("could not step sent msg statement: (%d) %s - %s", ret, sqlite3_errstr(ret), sqlite3_errmsg(db_));
                goto out;
        }

out:
        sqlite3_clear_bindings(insert_sent_msg_statement);
        sqlite3_reset(insert_sent_msg_statement);

        return ((SQLITE_DONE == ret) ? 1 : 0);
}

/*
 * Stores incoming complete messages.
 *
 * seqnum: Sequence number
 * msg: The complete message
 * len: Number of bytes in msg
 */
int
MsgDB::store_recv_msg(const uint64_t seqnum,
                      const uint64_t len,
                      const uint8_t * const msg)
{
        int ret = SQLITE_OK;

        if (!db_)
                return 0;

        ret = sqlite3_bind_int64(insert_recv_msg_statement, 1, seqnum);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_bind_blob(insert_recv_msg_statement, 2, msg, len, SQLITE_TRANSIENT);
        if (SQLITE_OK != ret) {
                M_ALERT("could not bind into recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

        ret = sqlite3_step(insert_recv_msg_statement);
        if (SQLITE_DONE != ret) {
                M_ALERT("could not step recv msg statement: %s", sqlite3_errstr(ret));
                goto out;
        }

out:
        sqlite3_clear_bindings(insert_recv_msg_statement);
        sqlite3_reset(insert_recv_msg_statement);

        return ((SQLITE_DONE == ret) ? 1 : 0);
}

/*
 * Gets the latest sequence number recieved.
 */
int
MsgDB::get_latest_recv_seqnum(uint64_t & seqnum) const
{
        int ret;

        if (!db_)
                return 0;

        ret = sqlite3_step(max_recv_seqnum_statement);
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
        seqnum = sqlite3_column_int64(max_recv_seqnum_statement, 0);
out:
        sqlite3_reset(max_recv_seqnum_statement);
        return 1;

err:
        sqlite3_reset(max_recv_seqnum_statement);
        M_ALERT("could not get latest recv seqnum: %s", sqlite3_errstr(ret));
        return 0;
}

/*
 * Gets the latest sequence number sent.
 */
int
MsgDB::get_latest_sent_seqnum(uint64_t & seqnum) const
{
        int ret;

        ret = sqlite3_step(max_sent_seqnum_statement);
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
        seqnum = sqlite3_column_int64(max_sent_seqnum_statement, 0);
out:
        sqlite3_reset(max_sent_seqnum_statement);
        return 1;

err:
        sqlite3_reset(max_sent_seqnum_statement);
        M_ALERT("could not get latest sent seqnum: %s", sqlite3_errstr(ret));
        return 0;
}


/*
 * Returns a vector of previously sent partial messages ready
 * to be re-sent starting with sequence number "start" and
 * ending with sequence number "end".
 *
 * If "end" is larger than the largest sequence number then
 * all messages, starting with "start", is returned.
 *
 * This method is not performance critical (re-sending is a
 * fairly rare occurrence) and the implementation reflects
 * that.
 */
std::vector<std::vector<uint8_t> > *
MsgDB::get_recv_msgs(uint64_t /*start*/,
                     uint64_t /*end*/) const
{
        std::vector<std::vector<uint8_t> > *retv = NULL;

        if (!db_)
                return NULL;

        return retv;
}
