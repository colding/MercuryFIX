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
#include <errno.h>
#include "stdlib/log/log.h"
#include "utillib/config/config_item_string_vector.h"
#include "applib/fixlib/session/session.h"
#include "fix_gateway.h"

static void*
fix_session_thread(void *arg)
{
        FIX_Session *session = (FIX_Session*)arg;

	if (!session->init(NULL)) {
		M_ERROR("could not init FIX session");
		return NULL;
	}
	
	// returns when the session is over
	if (!session->run())
		M_ERROR("could not run FIX session");
        delete session;

        return NULL;
}

static bool
create_thread(pthread_t * const thread_id,
              FIX_Session *thread_arg,
              void *(*thread_func)(void *))
{
        bool retv = false;
        pthread_attr_t thread_attr;

        if (pthread_attr_init(&thread_attr))
                return false;

        if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE))
                goto err;

        if (pthread_create(thread_id, &thread_attr, thread_func, (void*)thread_arg))
                goto err;

        retv = true;
err:
        pthread_attr_destroy(&thread_attr);

        return retv;
}

FIX_Gateway::FIX_Gateway(const char * const identity,
                         const char * const config_source)
        : AppBase(identity, config_source)
{
	M_INFO("ID = %s, config = %s", identity, config_source);
}

FIX_Gateway::~FIX_Gateway(void)
{
}

bool 
FIX_Gateway::init(void *data)
{
        ConfigItemStringVector *vector_item = new (std::nothrow) ConfigItemStringVector();
	if (!vector_item) {
                M_ALERT("no memory");
                return false;
        }

        if (!AppBase::init(data)) {
                M_ERROR("could not initiate AppBase");
		delete vector_item;
                return false;
        }

        if (!config_->subscribe(NULL, "PROTOCOL", "FIX_SESSION_IDS", vector_item)) {
		delete vector_item;
                M_ERROR("could not read FIX session identities");
                return false;
        }
        if (!vector_item->get(session_IDs_)) {
		vector_item->release();
                M_ERROR("could not read FIX session identities");
                return false;
        }
	vector_item->release();

        if (!session_IDs_.size()) {
                M_ERROR("no FIX session identities");
                return false;
        }

        return true;
}

bool
FIX_Gateway::run(void)
{
        pthread_t thread_id;
        std::vector<pthread_t> fix_threads;
        FIX_Session *session = NULL;

        try {
                fix_threads.reserve(session_IDs_.size());
        }
        catch (...) {
                M_ALERT("could not create FIX thread array");
                return false;
        }

        for (size_t n = 0; n < session_IDs_.size(); ++n) {
		M_INFO("creating FIX session \"%s\"", session_IDs_[n].c_str());
                session = new (std::nothrow) FIX_Session(session_IDs_[n].c_str(), config_source_);
                if (!session) {
                        M_ERROR("could not create FIX session \"%s\"", session_IDs_[n].c_str());
                        continue;
                }

                if (!create_thread(&thread_id, session, fix_session_thread)) {
                        M_ERROR("could not create FIX session thread for \"%s\"", session_IDs_[n].c_str());
                        continue;
                }
                fix_threads.push_back(thread_id);
        }
	session_IDs_.clear();

        int jval;
        for (size_t n = 0; n < fix_threads.size(); ++n) {
		jval = pthread_join(fix_threads[n], NULL);
		if (jval) {
			const char *msg = "UNKNOWN ERROR";
			switch (jval) {
			case EDEADLK:
				msg = "EDEADLK";
				break;
			case EINVAL:
				msg = "EINVAL";
				break;
			case ESRCH:
				msg = "ESRCH";
				break;
			default:
				break;
			}
			M_ERROR("error joining FIX session thread: %s (%s)", msg, strerror(jval));
		}
        }

	return true;
}
