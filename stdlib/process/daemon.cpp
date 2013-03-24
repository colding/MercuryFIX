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

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "stdlib/log/log.h"
#include "daemon.h"

/*
 * Dummy function for zombie disabling
 */
static void
sigchld_handler(int /* sig */)
{
        return;
}

daemon_exit_t
become_daemon(void)
{
        pid_t pid = -1;
        struct sigaction sig_act;

        // do not create zombies
        sig_act.sa_handler = sigchld_handler; // TODO: investigate whether SIG_IGN can be used
        sigfillset(&sig_act.sa_mask); // ignore as many signals as possible when in the handler!
        sig_act.sa_flags = SA_NOCLDWAIT | SA_RESTART;
        sigaction(SIGCHLD, &sig_act, NULL);

        pid = fork();
        switch (pid) {
        case -1 :
                return PROCESS_ERROR;
        case 0 :
                /*
                 * (0 == pid) we are now a worker child process. We
                 * know that we do not create zombies so we do not
                 * need any trickery here to avoid that
                 */
                return PROCESS_IS_CHILD;
        default :
                /*
                 * We are the controlling daemon
                 */
                return PROCESS_IS_PARENT;
        }
}

bool
lock_down_process(void)
{
        struct rlimit rl;
        unsigned int n;
        int fd0;
        int fd1 __attribute__ ((unused));
        int fd2 __attribute__ ((unused));

        // Reset file mode mask
        umask(0);

        // change the working directory
        if (chdir("/")) {
                return false;
        }

        // close any and all open file descriptors
        if (getrlimit(RLIMIT_NOFILE, &rl)) {
                return false;
        }
        if (RLIM_INFINITY == rl.rlim_max) {
                rl.rlim_max = 1024;
        }

        // an explicit call to closelog() is needed on linux and
        // doesn't harm on other platforms
        closelog();
        for (n = 0; n < rl.rlim_max; n++) {
                if (close(n) && (EBADF != errno)) {
                        return false;
                }
        }

        // attach file descriptors 0, 1 and 2 to /dev/null
        fd0 = open("/dev/null", O_RDWR);
        fd1 = dup2(fd0, 1);
        fd2 = dup2(fd0, 2);
        if (0 != fd0) {
                return false;
        }

        return false;
}

bool
switch_user(const char * const user,
            const char * const group)
{
        struct passwd *pwd = NULL;
        struct group *grp = NULL;

        if (user && strlen(user)) {
                pwd = getpwnam(user);
                if (!pwd) {
                        M_ALERT("invalid user name\n");
                        return false;
                }
                if (setuid(pwd->pw_uid)) {
                        M_ALERT("could not switch to user\n");
                        return false;
                }
        }

        if (group && strlen(group)) {
                grp = getgrnam(group);
                if (!grp) {
                        M_ALERT("invalid group name\n");
                        return false;
                }
                if (setgid(grp->gr_gid)) {
                        M_ALERT("could not change group\n");
                        return false;
                }
        }

        return true;
}
