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
#include <pthread.h>
#include <stdlib.h>

#ifdef __APPLE__
    #include <libkern/OSAtomic.h>
#endif

class MutexGuard {
public:
        MutexGuard()
        {
		if (pthread_mutex_init(&mutex_, NULL))
			abort();
        };

        ~MutexGuard()
        {
                pthread_mutex_destroy(&mutex_);
        };

        int enter(void)
        {
                return pthread_mutex_lock(&mutex_);
        };

        int try_enter(void)
        {
                return pthread_mutex_trylock(&mutex_);
        };

        void leave(void)
        {
                pthread_mutex_unlock(&mutex_);
        };

private:
        pthread_mutex_t mutex_;
};

#ifdef __linux__

class SpinlockGuard {
public:
        SpinlockGuard()
        {
		if (pthread_spin_init(&spinlock_, PTHREAD_PROCESS_PRIVATE))
			abort();
        };

        ~SpinlockGuard()
        {
		pthread_spin_destroy(&spinlock_);
        };

        int enter(void)
        {
		return pthread_spin_lock(&spinlock_);
        };

        int try_enter(void)
        {
		return pthread_spin_trylock(&spinlock_);
        };

        void leave(void)
        {
		pthread_spin_unlock(&spinlock_);
        };

private:
	pthread_spinlock_t spin_lock_;
};

#elif defined __APPLE__

class SpinlockGuard {
public:
        SpinlockGuard()
        {
		spinlock_ = OS_SPINLOCK_INIT;
        };

        ~SpinlockGuard()
        {
        };

        int enter(void)
        {
		OSSpinLockLock(&spinlock_);
		return 0;
        };

        int try_enter(void)
        {
		return (OSSpinLockTry(&spinlock_) ? 0 : 1);
        };

        void leave(void)
        {
		OSSpinLockUnlock(&spinlock_);
        };

private:
	OSSpinLock spinlock_;
};

#endif
