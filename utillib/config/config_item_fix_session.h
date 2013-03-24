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

#pragma once

#ifdef HAVE_CONFIG_H
    #include "ac_config.h"
#endif
#include "stdlib/config/config.h"
#include "stdlib/network/net_types.h"
#include "utillib/config/config_item_string_vector.h"

class ConfigItemFIXSession;

enum FIX_Application_Version {
        FIX_4_0,
        FIX_4_1,
        FIX_4_2,
        FIX_4_3,
        FIX_4_4,
        FIX_5_0,
        FIX_5_0_SP1,
        FIX_5_0_SP2,
};

enum FIX_Session_Version {
        NO_FIXT, // no support for FIX Session Protocol
        FIXT_1_1,
};

// all properties must be explicitly set by the ConfigItem
struct FIX_Session_Config 
{
        bool is_duplex;                   // identifies whether to dup or not
        bool must_initiate_logon;
        bool reset_seq_numbers_at_logon;
        bool session_days_[7];             // boolmask of session days - session_day[0] is Sunday
        FIX_Application_Version fixA_ver;
        FIX_Session_Version fixT_ver;
        unsigned long heartbeat_interval;   // in seconds
        unsigned long test_request_delay;   // in seconds
	unsigned long session_warm_up_time; // in seconds
        unsigned long session_start;        // number of seconds since 00:00:00
        unsigned long session_end;          // number of seconds since 00:00:00
        char *timezone;                     // the timezone in which the times above are stated (ISO name)
        endpoint_t in_going;
        endpoint_t out_going;
};

/*
 * Explicit class for a configuration item
 */
class ConfigItemFIXSession : public ConfigItem
{
public:
        ConfigItemFIXSession(void)
        {
                data_source_ = Config::UNKNOWN;
        };

        virtual ~ConfigItemFIXSession(void)
        {
        };

        bool fill(const Config::DataSource data_source,
                  const void * const data);

        bool get(struct FIX_Session_Config & session_config);

private:
        Config::DataSource data_source_;
        ConfigItemStringVector config_vector_item_;
};
