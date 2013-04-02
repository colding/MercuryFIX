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
#ifdef __APPLE__
    #include <libkern/OSAtomic.h>
#endif
#include <string.h>
#include <map>
#include <string>
#include <stdlib.h>
#include "stdlib/config/config_file.h"

/*
 * Abstract class for a configuration item
 */
class ConfigItem;

class Config
{
public:
        enum DataSource {
                UNKNOWN,
                File,
                DataBase
        };

        Config(void)
        {
                default_identity = strdup("");
                config_source = NULL;
                config_file_ = NULL;
        };

        Config(const char * const id)
        {
                default_identity = id ? strdup(id) : strdup("");
                config_source = NULL;
                config_file_ = NULL;
        };

        virtual ~Config(void)
        {
                free(default_identity);
                free(config_source);
                if (config_file_)
                        config_file_->release();
        };

        /*
         * Will load, read or connect to the configuration from
         * "source". Returns true on success or false otherwise. May
         * be invoked repeatedly.
         */
        virtual bool init(const char * const source);

        /*
         * Will subscribe the corresponding value and return true if
         * all is well or false if no corresponding configuration
         * could be found. All users of Config::subscribe() must invoke
         * ConfigItem::release() before the Config instance destructor is
         * called.
         */
        virtual bool subscribe(const char * const identity,
			       const char * const domain,
			       const char * const item,
			       ConfigItem *value);

        static const DataSource data_source;
        char *default_identity;
        char *config_source;

private:
        ConfigFile *config_file_;
};

/*
 * Abstract class for a configuration item. refcnt() will be non-zero
 * if initialization went well.
 *
 * The thread, that allocated it, must not invoke retain() but retain()
 * must be used whenever the instance is passed to a new thread.
 *
 * Invoke Config::release() when there is no further use for the
 * instance any more. The thread spawned by Config::subscribe() will
 * deallocate it when it's not in use by any other thread.
 *
 * The only execption is when Config::read() returns false. Then
 * callee must delete the instance manually.
 */
class ConfigItem
{
public:
	ConfigItem(void) 
		: refcnt_(0)
        {
                if (!pthread_rwlock_init(&rw_lock_, NULL)) {
                        refcnt_ = 1;
                } else {
                        M_ERROR("error initializing rw lock");
                }
        };

        virtual ~ConfigItem(void)
        {
                if (pthread_rwlock_destroy(&rw_lock_))
                        M_ERROR("error destrying rw lock");
        };

#ifdef __APPLE__
        void retain(void)
        {
                OSAtomicIncrement32Barrier(&refcnt_);
        };

        void release(void)
        {
		OSAtomicDecrement32Barrier(&refcnt_);
        };

#elif defined __linux__

        void retain(void)
        {
		__sync_add_and_fetch(&refcnt_, 1);
        };

        void release(void)
        {
		__sync_sub_and_fetch(&refcnt_, 1);
        };
#endif /* __linux__ */

        int refcnt(void) const
        {
                return refcnt_;
        };

        virtual bool fill(const Config::DataSource data_source,
                          const void * const data) = 0;

protected:
        pthread_rwlock_t rw_lock_;
#ifdef __APPLE__
        volatile int32_t refcnt_;
#elif defined __linux__
        int refcnt_;
#endif
};
