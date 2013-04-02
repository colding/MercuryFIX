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
#include <string.h>
#include <stdio.h>
#include "stdlib/log/log.h"
#include "stdlib/process/threads.h"
#include "config.h"

struct KeyValue {
        char *key;
        ConfigItem *value;
        ConfigFile *config_src;
};

// this implementation is for file based configuration only
const Config::DataSource Config::data_source = Config::File;

static void*
config_updater_thread(void *arg)
{
        struct KeyValue *kv = (struct KeyValue*)arg;
        char *item_value;
        int load_res;

        while (0 < kv->value->refcnt()) {
                sleep(300); // change to push from database when configuration changes

                load_res = kv->config_src->reload();
                if (load_res) {
                        M_ERROR("could not reload configuration: %d", load_res);
			continue;
		}

                item_value = kv->config_src->get_value(kv->key);
		if (!item_value) {
			M_ALERT("no memory");
			continue;
		}
                if (!kv->value->fill(Config::data_source, (const void * const)item_value))
                        M_ERROR("could not fill ConfigItem for key \"%s\" and value \"%s\"", kv->key, item_value);
                free(item_value);
        }

        delete kv->value;
        kv->config_src->release();
        free(kv->key);
        free(kv);

        return NULL;
}

bool
Config::init(const char * const source)
{
        config_file_ = new (std::nothrow) ConfigFile();
        if (!config_file_) {
                M_ALERT("no memory");
                return false;
        }
        if (!config_file_->init())
                return false;

        config_source = source ? strdup(source) : NULL;
        int retv = config_file_->load(source);
	if (retv) {
		M_ERROR("error prsing configuration file at line %d", retv);
		return false;
	}
	return true;
}


bool
Config::subscribe(const char * const identity,
                  const char * const domain,
                  const char * const item,
                  ConfigItem *value)
{
        if (!value)
                return false;
        if (!value->refcnt())
                return false;

        const char *id = identity ? identity : default_identity;
        size_t len = id ? strlen(id) : 0;
        len += domain ? strlen(domain) : 0;
        len += item ? strlen(item) : 0;
        len += 3; // ':' + ':' + '\0'

        char *key = (char*)malloc(len);
        if (!key) {
                M_ALERT("no memory");
                return false;
        }
        snprintf(key, len, "%s:%s:%s",
                 id ? id : "",
                 domain ? domain : "",
                 item ? item : "");

        struct KeyValue *kv = (struct KeyValue*)malloc(sizeof(struct KeyValue));
        if (!kv) {
                free(key);
                M_ALERT("no memory");
                return false;
        }
        kv->config_src = (ConfigFile*)config_file_->retain();
        kv->key = key;
        kv->value = value;

	char *item_value = config_file_->get_value(key);
	if (!item_value) {
		M_ERROR("could not get value for key \"%s\"", key);
		goto err;
	}
	if (!value->fill(Config::data_source, (const void * const)item_value)) {
                free(item_value);
		M_ERROR("could not fill ConfigItem for key \"%s\" and value \"%s\"", key, item_value);
		goto err;
	}

        pthread_t thread_id;
        if (!create_detached_thread(&thread_id, (void*)kv, config_updater_thread)) {
                M_ERROR("could not create ConfigItem updater thread");
		goto err;
        }

        return true;
err:
	free(key);
	config_file_->release();
	return false;
}
