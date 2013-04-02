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

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "stdlib/log/log.h"
#include "config_file.h"

#ifdef COMMENT_CHAR
    #undef COMMENT_CHAR
#endif
#define COMMENT_CHAR '#'

#ifdef MAX_CONFIG_FILE_SIZE
    #undef MAX_CONFIG_FILE_SIZE
#endif
#define MAX_CONFIG_FILE_SIZE (20480)

#ifdef READY_HEADER
    #undef READY_HEADER
#endif
#define READY_EXT ".ready"

const char *ConfigFile::delims = " \r\t";

ConfigFile::ConfigFile(void)
{
        lnum_ = 1;
        fsize_ = 0;
        fbuf_ = NULL;
        items_ = NULL;
        file_name_ = NULL;
}

ConfigFile::~ConfigFile(void)
{
        if (pthread_rwlock_destroy(&rw_lock_))
                M_ERROR("error destroying rw lock");

        if (items_) { // token/values must be freed individually
                size_t n = 0;
                while (items_[n].value) {
                        free(items_[n].token);
                        free(items_[n].value);
                        ++n;
                }
                free(items_);
                items_ = NULL;
        }
        free(fbuf_);
        free(file_name_);
}

bool
ConfigFile::init(void)
{
        int retv = pthread_rwlock_init(&rw_lock_, NULL);
        if (retv)
                M_ERROR("could not initialize rwlock: %d", retv);

        return (retv ? false : true);
}

int
ConfigFile::load(const char * const source)
{
        if (!source)
                return -1;

        int lockval = pthread_rwlock_wrlock(&rw_lock_);
        if (lockval) {
                M_ERROR("could not lock: %d", lockval);
                return -1;
        }

        // initialize
        if (items_) {
                size_t n = 0;
                while (items_[n].value) {
                        free(items_[n].token);
                        free(items_[n].value);
                        ++n;
                }
                free(items_);
                items_ = NULL;
        }
        free(fbuf_);
        fbuf_ = NULL;
        lnum_ = 1;

        // read file into buffer
        READ_FILE_RESULT readval;
        do {
                readval = read_conf(source, &fsize_, (void**)&fbuf_);
                switch (readval) {
                case READ_AGAIN:
                        sleep(60);
                case READ_OK:
                        break;
                case READ_ERROR:
                default:
                        lockval = pthread_rwlock_unlock(&rw_lock_);
                        if (lockval)
                                M_ERROR("could not unlock: %d", lockval);
                        return -1;
                }
        } while (READ_OK != readval);

        int retv = itemize();
        lockval = pthread_rwlock_unlock(&rw_lock_);
        if (lockval)
                M_ERROR("could not unlock: %d", lockval);

        return retv;
}

int
ConfigFile::reload(void)
{
        return load(file_name_);
}

char*
ConfigFile::get_value(const char * const token)
{
        size_t n = 0;
        int lockval = pthread_rwlock_rdlock(&rw_lock_);
        if (lockval) {
                M_ERROR("could not lock: %d", lockval);
                return NULL;
        }

        if (!items_) {
                lockval = pthread_rwlock_unlock(&rw_lock_);
                if (lockval)
                        M_ERROR("could not unlock: %d", lockval);

                return NULL;
        }

        while (items_[n].value) {
                if ((items_[n].token) && !strcmp(token, items_[n].token)) {
			char *retv = strdup(items_[n].value);
			if (!retv) 
				M_ALERT("no memory");

                        lockval = pthread_rwlock_unlock(&rw_lock_);
                        if (lockval)
                                M_ERROR("could not unlock: %d", lockval);

                        return retv;
                }
                n++;
        }
        lockval = pthread_rwlock_unlock(&rw_lock_);
        if (lockval)
                M_ERROR("could not unlock: %d", lockval);

        return NULL;
}

int
ConfigFile::itemize(void)
{
        int retv = 0;
        size_t count = 0;
        char *p = fbuf_;

        // count maximum number of effective configuration lines
        for (size_t n = 0; n < fsize_; n++, p++) {
                // newline?
                if ('\0' == *p)
                        count++;
        }
        items_ = (struct item*)calloc(count + 1, sizeof(struct item));
        if (!items_) {
                M_ALERT("no memory");
                return -1;
        }

        // skip empty lines
        p = fbuf_;
        if (!*p || wspace(*p) || (COMMENT_CHAR == *p)) {
                if (get_next_line(&p)) {
                        retv = lnum_;
                        goto out;
                }
        }

        count = 0;
        while (true) {
                if (!p || (EOF == *p))
                        break;
                items_[count].token = p;

                // p will be somewhere within the value string when the function returns
                if (point_to_value(&p, &items_[count].value)) {
                        retv = lnum_;
                        goto out;
                }

                // now copy token and value
                items_[count].token = strdup(items_[count].token);
                if (!items_[count].token) {
                        M_ALERT("no memory");
                        retv = -1;
                        goto out;
                }
                items_[count].value = strdup(items_[count].value);
                if (!items_[count].value) {
                        free(items_[count].token);
                        items_[count].token = NULL;
                        M_ALERT("no memory");
                        retv = -1;
                        goto out;
                }
                ++count;

                if (get_next_line(&p)) {
                        retv = lnum_;
                        goto out;
                }
        }
out:
        if (retv) {
                size_t n = 0;
                while (items_[n].value) {
                        free(items_[n].token);
                        free(items_[n].value);
                        ++n;
                }
                free(items_);
                items_ = NULL;
        }
        free(fbuf_);
        fbuf_ = NULL;

        return retv;
}

int
ConfigFile::point_to_value(char **str,
                           char **val) const
{
        // not possible
        if (wspace(**str))
                return -1;

        // skip past token
        while ((!wspace(**str)) && ('\0' != **str))
                (*str)++;
        if ('\0' == **str) { // empty value OK
		*val = *str;
		return 0;
	}

        // skip past whitespace to value
        while (wspace(**str)) {
                **str = '\0';
                (*str)++;
        }
        *val = *str;

        // remove trailing whitespace
        char *tmp = *val;
        while ('\0' != *tmp)
                tmp++;
        tmp--;
        while (wspace(*tmp)) {
                *tmp = '\0';
                tmp--;
        }

        return 0;
}

int
ConfigFile::get_next_line(char **str)
{
        int retv = 0;
        size_t offset = *str - fbuf_;

        do {
                // skip printable characters
                while ('\0' != **str) {
                        (*str)++;

                        offset++;
                        if (offset == fsize_) {
                                *str = NULL;
                                goto out;
                        }
                }

                // skip '\0's
                while ('\0' == **str) {
                        lnum_++;
                        (*str)++;

                        offset++;
                        if (offset == fsize_) {
                                *str = NULL;
                                goto out;
                        }
                }

                // quick check for floating text, i.e.: "<WHITESPACE>TEXT"
                if (wspace(**str)) {
                        while (wspace(**str)) {
                                (*str)++;
                                offset++;
                                if (offset == fsize_) {
                                        *str = NULL;
                                        goto out;
                                }
                        }

                        // floating text
                        if ('\0' != **str) {
                                retv = -1;
                                goto out;
                        }
                        continue;
                }

                // configuration line?
                if (COMMENT_CHAR != **str)
                        break;
        } while (true);

out:
        return retv;
}

ConfigFile::READ_FILE_RESULT
ConfigFile::read_conf(const char * const file_name,
                      size_t *size,
                      void **buf)
{
        FILE *file;
        READ_FILE_RESULT retv = READ_OK;
        size_t len;
        char *tmp;
        char *first;
        struct stat fstatus;

        *buf = NULL;
        len = strlen(file_name) + strlen(READY_EXT) + 1;
        char *ready_file = (char*)malloc(len);
        if (!ready_file) {
                M_ALERT("no memory");
                return READ_ERROR;
        }
        sprintf(ready_file, "%s%s", file_name, READY_EXT);
        int stat_retv = stat(ready_file, &fstatus);
        free(ready_file);
        if (stat_retv) {
                if (ENOENT == errno)
                        return READ_AGAIN;
                else
                        return READ_ERROR;
        }

        stat_retv = stat(file_name, &fstatus);
        if (stat_retv)
                return READ_ERROR;

        if (fstatus.st_size > MAX_CONFIG_FILE_SIZE)
                return READ_ERROR;

        tmp = (char*)malloc(fstatus.st_size);
        if (!tmp)
                return READ_ERROR;
        memset((void*)tmp, '\0', fstatus.st_size);
        first = tmp;

        file = fopen(file_name, "r");
        if (NULL == file) {
                free(first);
                return READ_ERROR;
        }

        char c;
        off_t pos = 0;
        while (true) {
                // for clarity, not speed
                len = fread((void*)&c, 1, 1, file);
                if (!len) {
                        if (ferror(file))
                                retv = READ_ERROR;
                        break;
                }

                // create null-terminated strings
                switch (c) {
                case '\n' :
                case '\r' :
                        c = '\0';
                default:
                        break;
                }

                // now store the character
                *tmp = c;
                pos++;
                if (pos < fstatus.st_size)
                        ++tmp;
        }
        if (file)
                fclose(file);

        switch (retv) {
        case READ_OK:
                *buf = first;
                *size = pos;
                free(file_name_);
                file_name_ = strdup(file_name);
                break;
        default:
                free(first);
                break;
        }

        return retv;
}
