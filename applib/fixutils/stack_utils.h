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

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif

/*
 * Perpetuallly increasing counter
 */
static inline void
inc_counter(uint64_t *cntr)
{
        __atomic_add_fetch(cntr, 1, __ATOMIC_RELAXED);
}

static inline uint64_t
read_counter(uint64_t *cntr)
{
        return __atomic_load_n(cntr, __ATOMIC_RELAXED);
}

/*
 * Helper functions for coupled pairs of synchronization
 * flags. Someone, please, please review the memory model used?
 */
static inline void
set_flag(int *flag, int val)
{
        __atomic_store_n(flag, val, __ATOMIC_RELEASE);
}

static inline int
get_flag(int *flag)
{
        return __atomic_load_n(flag, __ATOMIC_ACQUIRE);
}

static inline void
set_flag_weak(int *flag, int val)
{
        __atomic_store_n(flag, val, __ATOMIC_RELAXED);
}

static inline int
get_flag_weak(int *flag)
{
        return __atomic_load_n(flag, __ATOMIC_RELAXED);
}
