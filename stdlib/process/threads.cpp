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
#ifdef __APPLE__
    #include <mach/mach.h>
    #include <mach/thread_policy.h>
#elif defined __linux__
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <sys/types.h>
    #include <sched.h>
#endif
#include "threads.h"


#ifdef __APPLE__
int
pin_thread(const int cpu_tag)
{
        thread_affinity_policy ap;

        ap.affinity_tag = cpu_tag;

        return (KERN_SUCCESS == thread_policy_set(mach_thread_self(),
                                                  THREAD_AFFINITY_POLICY,
                                                  (integer_t*) &ap,
                                                  THREAD_AFFINITY_POLICY_COUNT)) ? 0 : 1;
}
#elif defined __linux__
int
pin_thread(const int cpu_tag)
{
        cpu_set_t mask;

        CPU_ZERO(&mask);
        CPU_SET(cpu_tag - 1, &mask);

        return sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}
#endif

static inline bool
create_thread(const bool detached,
              pthread_t * const thread_id,
              void *thread_arg,
              void *(*thread_func)(void *))
{
        bool retv = false;
        pthread_attr_t thread_attr;

        if (pthread_attr_init(&thread_attr))
                return false;

        if (pthread_attr_setdetachstate(&thread_attr, (detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE)))
                goto err;

        if (pthread_create(thread_id, &thread_attr, thread_func, thread_arg))
                goto err;

        retv = true;
err:
        pthread_attr_destroy(&thread_attr);

        return retv;
}

bool
create_joinable_thread(pthread_t * const thread_id,
		       void *thread_arg,
		       void *(*thread_func)(void *))
{
	return create_thread(false, thread_id, thread_arg, thread_func);
}

bool
create_detached_thread(pthread_t * const thread_id,
		       void *thread_arg,
		       void *(*thread_func)(void *))
{
	return create_thread(true, thread_id, thread_arg, thread_func);
}
