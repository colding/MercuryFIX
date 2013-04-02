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
#include <string.h>
#include <stdlib.h>
#include "stdlib/config/config.h"
#include "stdlib/log/log.h"

enum AppPurpose {
	FixGateway,
};

class AppBase
{
public:
	/*
	 * Must be invoked by all descending classes
	 */
	AppBase(const char * const identity,
		const char * const config_source)
		: identity_(identity ? strdup(identity) : NULL),
		config_source_(config_source ? strdup(config_source) : NULL)
	{
		config_ = NULL;
	};

	virtual ~AppBase(void)
	{
		free((char*)identity_);
		free((char*)config_source_);
		if (config_)
			delete config_;

	};

	/*
	 * Must be invoked by all descending classes
	 */
	bool init(void *data)
	{
		config_ = new(std::nothrow) Config(identity_);
		if (!config_) {
			M_ERROR("could not allocate Config(%s)", identity_);
			return false;
		}

		if (!config_->init(config_source_)) {
			M_ERROR("Config::init(%s) failed", config_source_);
			return false;
		}
		return (data ? true : true);
	};

	virtual bool run(void) = 0;

protected:
        Config *config_;	
	const char *identity_;
	const char *config_source_;
};

