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
 *     (3) The name of the author may not be used to endorse or
 *     promote products derived from this software without specific
 *     prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <stdlib.h>
#include <pthread.h>
#include "stdlib/process/refcnt.h"

/*
 * The ConfigFile class implements support for file based
 * configuration.
 *
 * The syntax of the configuration file is:
 *
 * A file with the same name as the configuration file, but with the
 * extension ".ready", must exist in the same directory as the
 * configuration file or the load() method will wait indefinitely
 * before returning.
 *
 * <> ==> Required
 * [] ==> Optional
 *
 *
 *    Whitespace characters:
 *
 *          ' '
 *          '\t'
 *          '\r'
 *
 *    Line terminator:
 *
 *          '\n'
 *
 *    Token/Values:
 *
 *          "<TOKEN><WHITESPACE><VALUE(S)>[WHITESPACE]"
 *
 *    Comment:
 *
 *          "#[ANY COMBINATION OF COMMENT TEXT AND WHITESPACE]"
 *
 * The tokens and values in this configuration file are case
 * sensitive.
 *
 * It is not allowed to start a line with a whitespace character
 * unless that line is composed of whitespace characters only. A
 * whitespace character is defined as ' ', '\t' or '\r'. All lines are
 * terminated by the newline character '\n'.
 *
 * The order of configuration token/value lines does not matter. If
 * tokens are duplicated then the first one encountered is the one
 * which will be in effect.
 *
 * The content of the configuration file must be UTF8 encoded.
 */
class ConfigFile : public RefCount
{
public:
        ConfigFile(void);

        virtual ~ConfigFile(void);

        /*
         * Does initialization that may fail and by that reason can't
         * be put into the constructor.
         */
        bool init(void);

        /*
         * Will load the configuration file "source". Returns zero on success or
         * a positive number which indicates a faulty configuration line or a
         * negative number which indicates that the configuration file is to
         * big or for some reason can not be read.
         */
        int load(const char * const source);

        /*
         * Reloads the configuration file
         */
        int reload(void);

        /*
         * Will return the corresponding value string, the empty
         * string if non-existent or NULL if OOM. Callee must free the
         * return value.
         */
        char *get_value(const char * const token);

        /*
         * Returns the list of delimiters used to separate values.
         */
        static const char *delims;

private:
        enum READ_FILE_RESULT {
                READ_OK,
                READ_AGAIN,
                READ_ERROR,
        };

        struct item {
                char *token;
                char *value;
        };

        int lnum_;
        char *fbuf_;
        size_t fsize_;
        char *file_name_;
        pthread_rwlock_t rw_lock_;
        struct item *items_;

        // whitespace?
        bool wspace(char c) const
        {
                switch (c) {
                case ' ' :
                case '\t' :
                case '\r' :
                        return true;
                default:
                        return false;
                }
        };

        int itemize(void);

        /*
         * Upon return val points to the first character in
         * the null- or whitespace-terminated value string.
         *
         * *str is a pointer to the first character in a null-terminated
         * configuration line.
         *
         * *str is modified by this function to point to the first
         * character in the value string.
         *
         * Will return (-1) on error or zero on success.
         */
        int point_to_value(char **str,
                           char **val) const;

        /*
         * *str is a pointer to the first character of the next
         * configuration line skipping comments and empty lines.
         *
         * *str is NULL on end-of-buffer.
         *
         * Returns (-1) on error or zero on success.
         */
        int get_next_line(char **str);

        /*
         * Reads all of file_name into *buf, as a set of adjoined
         * strings, correcting for CRLF.
         *
         * *size is the size of the file in bytes, not the size of the
         * buffer *buf. Callee must free() *buf if zero is returned,
         * otherwise not.
         *
         * I am reading one byte of the file at a time. This is to
         * avoid a second copy/allocation. Not that this method is
         * particular fast, but it makes the purpose so much clearer.
         * This function is not a bottleneck, therefore clarity before
         * speed.
         *
         * Returns READ_ERROR on error.
         *
         * Returns READ_AGAIN if the file must be reread due to a missing
         * '#READY' heading.
         */
        ConfigFile::READ_FILE_RESULT read_conf(const char * const file_name,
					       size_t *size,
					       void **buf);
};
