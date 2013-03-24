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
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef enum {
        PROCESS_IS_CHILD = 0,  /* we are the daemon                */
        PROCESS_IS_PARENT = 1, /* we are the parent process        */
        PROCESS_ERROR = 2,     /* we should exit with EXIT_FAILURE */
} daemon_exit_t;

/*
 * Calls fork(2) and returns a value telling callee whether it is the
 * child (daemon) or the parent.
 */
extern daemon_exit_t
become_daemon(void);

/*
 * Closes all superfluous file descriptors.
 */
extern bool
lock_down_process(void);

/*
 * Switch to another user and group.
 */
extern bool
switch_user(const char * const user,
	    const char * const group);

static inline int
sync_fd(int fd)
{
#ifdef __APPLE__
        return fcntl(fd, F_FULLFSYNC);
#elif defined __linux__
        return fdatasync(fd);
#endif
}

/*
 * Will attempt to lock the PID file and write the
 * pid into it.
 */
class PidFile
{
public:
        PidFile(const char * const path)
        {
                error = true;
                fd = -1;

                if (!path)
                        return;

                fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
                if (-1 == fd) {
			M_CRITICAL("could not open pid file");
                        return;
		}

                // lock it
                if (flock(fd, LOCK_EX | LOCK_NB)) {
			M_CRITICAL("could not lock pid file");
                        return;
		}

                // we have the lock
                if (ftruncate(fd, 0)) {
			M_CRITICAL("could not truncate pid file");
                        return;
		}

                /* write pid */
                char buf[32] = { '\0' };
                int p = snprintf(buf, sizeof(buf), "%d", getpid());
                ssize_t w = write(fd, buf, strlen(buf) + sizeof(char));
                p += sizeof(char);
                if (p != (int)w) {
			M_CRITICAL("could not write pid to file");
                        return;
		}
                if (-1 == sync_fd(fd)) {
			M_CRITICAL("could not sync pid file");
                        return;
		}

                error = false;
        };

        ~PidFile()
        {
                if (-1 == fd)
                        return;

                flock(fd, LOCK_UN);
                close(fd);
        };

        bool error;

private:
        int fd;

        PidFile()
	{ };

};
