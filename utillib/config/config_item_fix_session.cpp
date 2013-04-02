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
#include <map>
#include <list>
#include <string.h>
#include <errno.h>
#include "stdlib/log/log.h"
#include "utillib/config/config_item_network.h"
#include "config_item_fix_session.h"

#define DUPLEX_KEY                     "IS_DUPLEX" // "YES" or anything else for false
#define MUST_INITIATE_LOGON_KEY        "INITIATE_LOGON" // "YES" or anything else for false
#define RESET_SEQ_NUMBERS_AT_LOGON_KEY "RESET_SEQ_NUMBERS_AT_LOGON" // "YES" or anything else for false
#define SESSION_DAYS_KEY               "SESSION_DAYS" // ',' delimited array of: "MO" "TU" "WE" "TH" "FR" "SA" "SU"
#define FIX_APPLICATION_VER_KEY        "FIX_APPLICATION_VERSION" // e.g "FIX_4_2"
#define FIX_SESSION_VER_KEY            "FIX_SESSION_VERSION"     // e.g "NO_FIXT"
#define HEARTBEAT_INTERVAL_KEY         "HEARTBEAT_INTERVAL" // seconds as a positive integer number
#define TEST_REQUEST_DELAY_KEY         "TEST_REQUEST_DELAY" // seconds as a positive integer number
#define SESSION_WARM_UP_TIME_KEY       "SESSION_WARM_UP_TIME" // preload seconds as a positive integer number
#define SESSION_START_KEY              "SESSION_START" // HH:MM, if start == end then the session will not end, ever
#define SESSION_END_KEY                "SESSION_END"   // HH:MM, if start == end then the session will not end, ever
#define TIMEZONE_KEY                   "TIMEZONE"      // e.g "Europe/Copenhagen"
#define ENDPOINT_IN_GOING_KEY          "ENDPOINT_IN_GOING"
#define ENDPOINT_OUT_GOING_KEY         "ENDPOINT_OUT_GOING"
#define ENDPOINT_IN_OUT_KEY            "ENDPOINT_IN_OUT"

#define SUNDAY    "SU"
#define MONDAY    "MO"
#define TUESDAY   "TU"
#define WEDNESDAY "WE"
#define THURSDAY  "TH"
#define FRIDAY    "FR"
#define SATURDAY  "SA"

#define FIX_4_0_VAL     "FIX_4_0"
#define FIX_4_1_VAL     "FIX_4_1"
#define FIX_4_2_VAL     "FIX_4_2"
#define FIX_4_3_VAL     "FIX_4_3"
#define FIX_4_4_VAL     "FIX_4_4"
#define FIX_5_0_VAL     "FIX_5_0"
#define FIX_5_0_SP1_VAL "FIX_5_0_SP1"
#define FIX_5_0_SP2_VAL "FIX_5_0_SP2"

#define NO_FIXT_VAL  "NO_FIXT"
#define FIXT_1_1_VAL "FIXT_1_1"

#define SECONDS_IN_HOUR (60*60)

static bool
str_to_ulong(const char * const str,
             unsigned long & val)
{
        val = strtoul(str, NULL, 10);
        return ((ERANGE != errno) && (EINVAL != errno));
}

static bool
split(const std::string & original,
      std::string & first,
      std::string & second,
      const char delimiter)
{
        size_t del_pos = original.find(delimiter);

        // delimiter not present - error!
        if (std::string::npos == del_pos) {
                first = "";
                second = "";
                return false;
        }

        try {
                first = original.substr(0, del_pos);
                second = original.substr(del_pos + 1, original.size() - del_pos - 1);
        }
        catch (...) {
                M_ALERT("no memory");
                return false;
        }
        return true;
}

static bool
hhmm_to_seconds(const std::string & HHMM,
                unsigned long & seconds)
{
        unsigned long tmp;
        std::string hours;
        std::string minutes;

        if (!split(HHMM, hours, minutes, ':'))
                return false;
        if (!str_to_ulong(hours.c_str(), seconds))
                return false;
        if (23 < seconds)
                return false;
        seconds = seconds * SECONDS_IN_HOUR;

        if (!str_to_ulong(minutes.c_str(), tmp))
                return false;
        if (59 < tmp)
                return false;
        tmp = tmp * 60;

        seconds += tmp;

        return true;
}

static bool
make_vector(std::vector<std::string> & strings,
            const char * const array,
            const char *delims)
{
        bool retv = true;
        char *tmp;
        char *line;

        if (!array || !strlen(array))
                return false;

        line = strdup(array);
        if (!line) {
                M_ALERT("no memory");
                return false;
        }

        strings.clear();
        do {
                tmp = strsep(&line, delims);
                if (!tmp)
                        break;
                try {
                        strings.push_back((const char*)tmp);
                }
                catch (...) {
                        M_ALERT("no memory");
                        retv = false;
                }
        } while (retv);

        free(line);

        return retv;
}

static bool
check_session_props(std::map<std::string, std::string> session_props)
{
        std::map<std::string, std::string>::iterator it;

        it = session_props.find(DUPLEX_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(MUST_INITIATE_LOGON_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(RESET_SEQ_NUMBERS_AT_LOGON_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(SESSION_DAYS_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(FIX_APPLICATION_VER_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(FIX_SESSION_VER_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(HEARTBEAT_INTERVAL_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(TEST_REQUEST_DELAY_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(SESSION_WARM_UP_TIME_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(SESSION_START_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(SESSION_END_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        it = session_props.find(TIMEZONE_KEY);
        if (session_props.end() == it) {
                M_CRITICAL("session config check failed");
                return false;
        }

        return true;
}

bool
ConfigItemFIXSession::fill(const Config::DataSource data_source,
                           const void * const data)
{
	if (!refcnt_)
                return false;

        if (!config_vector_item_.fill(data_source, data)) {
                M_ERROR("could not fill vector item");
                return false;
        }
        data_source_ = data_source;

        return true;
}

bool
ConfigItemFIXSession::get(struct FIX_Session_Config & session_config)
{
        std::vector<std::string> items;
        std::map<std::string, std::string> session_props;
        ConfigItemNetwork ci_network;
        std::vector<endpoint_t> enpoints;

        if (!config_vector_item_.get(items)) {
                M_ERROR("could not get vector item");
                return false;
        }

        std::string token;
        std::string value;
        for (size_t n = 0; n < items.size(); ++n) {
                if (!split(items[n], token, value, ':'))
                        return false;
                if (token.size() && value.size())
                        session_props[token] = value;;
        }
        if (!check_session_props(session_props))
                return false;

        if ("YES" == session_props[DUPLEX_KEY]) {
                session_config.is_duplex = true;
        } else {
                session_config.is_duplex = false;
        }
        session_config.must_initiate_logon = ("YES" == session_props[MUST_INITIATE_LOGON_KEY]);
        session_config.reset_seq_numbers_at_logon = ("YES" == session_props[RESET_SEQ_NUMBERS_AT_LOGON_KEY]);

        std::vector<std::string> days;
        if (!make_vector(days, session_props[SESSION_DAYS_KEY].c_str(), ","))
                goto err;
        for (size_t n = 0; n < 7; ++n) {
                session_config.session_days_[n] = false;
        }
        for (size_t n = 0; n < days.size(); ++n) {
                if (SUNDAY == days[n]) {
                        session_config.session_days_[0] = true;
                        continue;
                }
                if (MONDAY == days[n]) {
                        session_config.session_days_[1] = true;
                        continue;
                }
                if (TUESDAY == days[n]) {
                        session_config.session_days_[2] = true;
                        continue;
                }
                if (WEDNESDAY == days[n]) {
                        session_config.session_days_[3] = true;
                        continue;
                }
                if (THURSDAY == days[n]) {
                        session_config.session_days_[4] = true;
                        continue;
                }
                if (FRIDAY == days[n]) {
                        session_config.session_days_[5] = true;
                        continue;
                }
                if (SATURDAY == days[n]) {
                        session_config.session_days_[6] = true;
                        continue;
                }

                goto err;
        }

        do {
                if (FIX_4_0_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_4_0;
                        break;
                }
                if (FIX_4_1_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_4_1;
                        break;
                }
                if (FIX_4_2_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_4_2;
                        break;
                }
                if (FIX_4_3_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_4_3;
                        break;
                }
                if (FIX_4_4_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_4_4;
                        break;
                }
                if (FIX_5_0_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_5_0;
                        break;
                }
                if (FIX_5_0_SP1_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_5_0_SP1;
                        break;
                }
                if (FIX_5_0_SP2_VAL == session_props[FIX_APPLICATION_VER_KEY]) {
                        session_config.fixA_ver = FIX_5_0_SP2;
                        break;
                }

                goto err;
        } while (false);

        do {
                if (NO_FIXT_VAL == session_props[FIX_SESSION_VER_KEY]) {
                        session_config.fixT_ver = NO_FIXT;
                        break;
                }
                if (FIXT_1_1_VAL == session_props[FIX_SESSION_VER_KEY]) {
                        session_config.fixT_ver = FIXT_1_1;
                        break;
                }

                goto err;
        } while (false);

        if (!str_to_ulong(session_props[HEARTBEAT_INTERVAL_KEY].c_str(), session_config.heartbeat_interval))
                goto err;

        if (!str_to_ulong(session_props[TEST_REQUEST_DELAY_KEY].c_str(), session_config.test_request_delay))
                goto err;

        if (!str_to_ulong(session_props[SESSION_WARM_UP_TIME_KEY].c_str(), session_config.session_warm_up_time))
                goto err;

        if (!hhmm_to_seconds(session_props[SESSION_START_KEY], session_config.session_start))
                goto err;

        if (!hhmm_to_seconds(session_props[SESSION_END_KEY], session_config.session_end))
                goto err;

        if (!session_props[TIMEZONE_KEY].size())
                goto err;
        session_config.timezone = strdup(session_props[TIMEZONE_KEY].c_str());
        if (!session_config.timezone)
                goto err;

        switch (session_config.is_duplex) {
        case true:
                ci_network.fill(data_source_, session_props[ENDPOINT_IN_OUT_KEY].c_str());
                ci_network.get(enpoints);
                if (1 != enpoints.size())
                        goto err;
                session_config.in_going.kind = enpoints[0].kind;
                session_config.in_going.interface = enpoints[0].interface;
                session_config.in_going.port = enpoints[0].port;
                session_config.in_going.pf_family = enpoints[0].pf_family;

                session_config.out_going.kind = enpoints[0].kind;
                session_config.out_going.interface = enpoints[0].interface;
                session_config.out_going.port = enpoints[0].port;
                session_config.out_going.pf_family = enpoints[0].pf_family;
		break;
        case false:
                ci_network.fill(data_source_, session_props[ENDPOINT_IN_GOING_KEY].c_str());
                ci_network.get(enpoints);
                if (1 != enpoints.size())
                        goto err;
                session_config.in_going.kind = enpoints[0].kind;
                session_config.in_going.interface = enpoints[0].interface;
                session_config.in_going.port = enpoints[0].port;
                session_config.in_going.pf_family = enpoints[0].pf_family;

                ci_network.fill(data_source_, session_props[ENDPOINT_OUT_GOING_KEY].c_str());
                ci_network.get(enpoints);
                if (1 != enpoints.size())
                        goto err;
                session_config.out_going.kind = enpoints[0].kind;
                session_config.out_going.interface = enpoints[0].interface;
                session_config.out_going.port = enpoints[0].port;
                session_config.out_going.pf_family = enpoints[0].pf_family;
		break;
        default:
                goto err;
        }

        return true;
err:
        return false;
}
