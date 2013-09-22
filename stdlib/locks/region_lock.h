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
#include "stdlib/disruptor/memsizes.h"

union __attribute__ ((__packed__)) reflock_t
{
   uint64_t comb;
   struct __attribute__ ((__packed__))
      {
	      uint32_t low_part;  // contains reference count of region
	      uint32_t high_part; // contains region lock
      } parts;
};

/*
 * Cacheline padded timeout value.  Not intended for use elsewhere.
 */
struct nano_yield_t__ {
        struct timespec timeout;
        uint8_t padding[(CACHE_LINE_SIZE > sizeof(struct timespec)) ? (CACHE_LINE_SIZE - sizeof(struct timespec)) : (sizeof(struct timespec) % CACHE_LINE_SIZE)];
} __attribute__((aligned(CACHE_LINE_SIZE)));


/*
 * The following functions implements a method to ensure exclusive
 * access to any number of specific regions. It goes like this:
 *
 * init_region() must be invoked before any of the following methods
 * are used. It leaves the region accessible to other threads.
 *
 * All threads entering a protected region must call
 * enter_region(). This function will block until access is
 * granted. Upon leaving the region leave_region() must be
 * called. This access is shared with other threads.
 *
 * Now, a thread which seeks exclusive access to a region must call
 * block_region() and invoke waitfor_region() before entering
 * it. waitfor_region() blocks until the calling thread has exclusive
 * access to the region.
 *
 * unblock_region() must be called to allow other threads access to
 * the region.
 *
 * NOTE: The maximum accumulated number of concurrent accessors to
 * regions protected by a common reflock_t is UINT_MAX. The behaviour
 * is undefined should that number be exceeded.
 */

static inline void
init_region(reflock_t * const lock)
{
	lock->comb = 0;
}

static inline void
block_region(reflock_t * const lock)
{
	uint8_t expected = 0;

	while (!__atomic_compare_exchange_n(&lock->parts.high_part, &expected, 1, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
		expected = 0;
	}
}

static inline void
waitfor_region(reflock_t * const lock)
{
	const struct nano_yield_t__ timeout__ = { {0, 1}, { 0 } };

	while (__atomic_load_n(&lock->parts.low_part, __ATOMIC_ACQUIRE)) {
		nanosleep(&timeout__.timeout, NULL); 
	}
}

static inline void
unblock_region(reflock_t * const lock)
{
	__atomic_store_n(&lock->parts.high_part, 0, __ATOMIC_RELEASE);
}

static inline void
enter_region(reflock_t * const lock)
{
	const struct nano_yield_t__ timeout__ = { {0, 1}, { 0 } };
	uint64_t lck = __atomic_load_n(&lock->comb, __ATOMIC_ACQUIRE);

	while (lck >> 32) {
		lck = __atomic_load_n(&lock->comb, __ATOMIC_ACQUIRE);
                nanosleep(&timeout__.timeout, NULL);
	} 

	while (!__atomic_compare_exchange_n(&lock->comb, &lck, lck + 1, true,  __ATOMIC_RELEASE, __ATOMIC_RELAXED)) {
	again:
		lck = __atomic_load_n(&lock->comb, __ATOMIC_ACQUIRE);
		if (lck >> 32) {
			nanosleep(&timeout__.timeout, NULL); 
			goto again;
		}
	}
}

static inline void
leave_region(reflock_t * const lock)
{
	__atomic_sub_fetch(&lock->comb, 1, __ATOMIC_RELEASE);
}

