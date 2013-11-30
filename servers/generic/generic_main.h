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
#include "stdlib/network/net_types.h"

/*
 * Thread argument given to all thread functions below. Allocated in
 * generic_main(), struct and members must be freed by receiving
 * thread function.
 */
typedef struct {
        char *identity;
        char *config_source;
        int socket;
} thread_arg_t;


/*
 * Implemented in specific child server process. Used by
 * generic_main() in pthread_create().
 *
 * Joinable, generic_main() waits for this thread to exit.
 *
 * Must handle main work task in slave.
 *
 * Arguments:
 *
 *    thread_arg_t.identity - identity of the slave process.
 *
 *    thread_arg_t.socket - 0, do not use.
 */
extern void*
slave_worker_thread(void *arg);

/*
 * Implemented in specific child server process. Used by
 * generic_main() in pthread_create().
 *
 * Detached, generic_main() does not wait for this thread to
 * exit.
 *
 * Must handle IPC communication with master.
 *
 * Arguments:
 *
 *    thread_arg_t.identity - identity of the master process in the
 *    other end.
 *
 *    thread_arg_t.socket - IPC socket connected to master process.
 */
extern void*
slave_ipc_thread(void *arg);

/*
 * Implemented in specific master server process. Used by
 * generic_main() in pthread_create().
 *
 * Joinable, generic_main() waits for this thread to exit.
 *
 * Must handle main work task in master.
 *
 * Arguments:
 *
 *    thread_arg_t.identity - identity of the master process.
 *
 *    thread_arg_t.socket - 0, do not use.
 */
extern void*
master_worker_thread(void *arg);

/*
 * Implemented in specific master server process. Used by
 * generic_main() in pthread_create().
 *
 * Detached, generic_main() does not wait for this thread to
 * exit.
 *
 * Must handle IPC communication with slave.
 *
 * Arguments:
 *
 *    thread_arg_t.identity - identity of the slave process in the
 *    other end.
 *
 *    thread_arg_t.socket - IPC socket connected to slave process.
 */
extern void*
master_ipc_thread(void *arg);

/*
 * Generic main() routine. Must be invoked directly by most
 * specialized servers. Does standard command line parsing and startup
 * processing.
 */
extern int
generic_main(int argc, char *argv[]);
