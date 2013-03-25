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
#include <string.h>
#include "stdlib/log/log.h"
#include "config_item_simple.h"

bool
ConfigItemSimple::fill(const Config::DataSource data_source,
                       const void * const data)
{
	if (!refcnt_)
                return false;

        if (Config::File != data_source)
                return false;

 	pthread_rwlock_wrlock(&rw_lock_);
        free(simple_value_);
        if (data) {
                simple_value_ = strdup((const char*)data);
                if (!simple_value_) {
			pthread_rwlock_unlock(&rw_lock_);
                        M_ALERT("no memory");
                        return false;
                }
        } else {
                simple_value_ = NULL;
        }
 	pthread_rwlock_unlock(&rw_lock_);

        return true;
}

bool
ConfigItemSimple::get(char **value)
{
 	pthread_rwlock_rdlock(&rw_lock_);
	if (!simple_value_) {
		pthread_rwlock_unlock(&rw_lock_);
		return false;
	}
        
	*value = strdup(simple_value_);
	pthread_rwlock_unlock(&rw_lock_);
	if (!*value) {
		M_ALERT("no memory");
		return false;
	}

        return (*value ? true : false);
}