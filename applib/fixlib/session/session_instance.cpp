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
#include <arpa/inet.h>
#include "stdlib/log/log.h"
#include "stdlib/process/threads.h"
#include "stdlib/network/network.h"
#include "session_instance.h"

static void*
incoming_fix_thread(void *arg)
{
	InstanceArgs *instance_args = (InstanceArgs*)arg;

	set_non_blocking(instance_args->in);
	set_non_blocking(instance_args->out);
	// handle live sockets


	ring_buffer_init(&in_going_queue);

	// session dead...
	close(instance_args->in.socket);
	close(instance_args->out.socket);
	instance_args->config->release();

	return NULL;
}

FIX_Session_Instance::FIX_Session_Instance(const char * const identity,
					   const char * const config_source,
					   ConfigItemFIXSession * const session_config,
					   socket_t in_going,
					   socket_t out_going)
	: AppBase(identity, config_source)
{
	in_going_.socket = dup(in_going.socket);
	out_going_.socket = dup(out_going.socket);
	session_config_ = session_config;
	session_config_->retain();
}

FIX_Session_Instance::~FIX_Session_Instance(void)
{
	if (-1 != in_going_.socket)
		close(in_going_.socket);
	if (-1 != out_going_.socket)
		close(out_going_.socket);
	session_config_->release();
}

bool
FIX_Session_Instance::init(void *data)
{
	if (!AppBase::init(data)) {
		M_ERROR("could not initiate AppBase");
		return false;
	}
	M_DEBUG("initiating FIX session instance \"%s\"", identity_);

	return true;
}

bool
FIX_Session_Instance::run(void)
{
	int retv = 0;
	pthread_t thread_id;
	InstanceArgs *args = NULL;

	args = (InstanceArgs*)malloc(sizeof(InstanceArgs));
	if (!args) {
		M_ALERT("no memory");
		goto out;
	}
	args->sock = dup(in_going_.socket);
	args->config = session_config_;
	args->config->retain();
	retv = create_detached_thread(&thread_id, args, incoming_fix_thread);
	if (!retv) {
		M_ERROR("could not create session instance thread");
	} else {
		args = NULL;
	}

out:
	if (args) {
		if (-1 != args->in.socket)
			close(args->in.socket);
		if (-1 != args->out.socket)
			close(args->out.socket);
		if (args->config)
			args->config->release();
		free(args);
	}

	return retv ? false : true;
}
