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

/*******************************************************
 *           Apple and Linux specific code             *
 *******************************************************/

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include <errno.h>
#include <string.h>
#include <sys/sysctl.h>

/*
 * Hints to the compiler whether an expression is likely to be true or
 * not
 */
#define LIKELY__(expr__) (__builtin_expect(((expr__) ? 1 : 0), 1))
#define UNLIKELY__(expr__) (__builtin_expect(((expr__) ? 1 : 0), 0))

#ifdef __APPLE__
static inline int
get_cpu_count(void)
{
        int cnt = -1;
        int mib_name[2];
        size_t len;

        mib_name[0] = CTL_HW;
        mib_name[1] = HW_AVAILCPU;
        len = sizeof(cnt);
        if (sysctl(mib_name, 2, &cnt, &len, NULL, 0)) {
                cnt = -1;
                //M_CRITICAL("error calling sysctl: %s\n", strerror(errno));
        }

        return cnt;
}
#elif defined __linux__
static inline int
get_cpu_count(void)
{
        int cnt;

        cnt = sysconf(_SC_NPROCESSORS_ONLN);
        if (-1 == cnt) {
                //M_CRITICAL("error calling sysconf: %s\n", strerror(errno));
        }

        return cnt;
}
#endif

static inline int
get_available_cpu_count(void)
{
        return get_cpu_count() - 1;
}

