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
#include <stdarg.h>
#include <syslog.h>
#include <unistd.h>

extern bool
init_logging(const bool debug,
             const char * const identity);

extern void
vlog(int priority,
     const char * const format,
     va_list ap);

extern void
log(int priority,
    const char * const format,
    ...);

// Never use these macros other than below
#define QUOTEME__(x) #x
#define QUOTEME_(x) QUOTEME__(x)
#define CODE_POS__ __FILE__ "(" QUOTEME_(__LINE__)")"

// A panic condition. All hands on deck!
#undef M_EMERGENCY
#define M_EMERGENCY(format__, ...) do { log(LOG_EMERG, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// A software or hardware condition that should be corrected
// immediately by sys admins, such as a corrupted database or no
// memory (OOM)
#undef M_ALERT
#define M_ALERT(format__, ...) do { log(LOG_ALERT, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// A critical hardware condition that should be corrected immediately
// by sys admins or developers, such as configuration errors
#undef M_CRITICAL
#define M_CRITICAL(format__, ...) do { log(LOG_CRIT, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// Errors which must be handled at the soonest opportunity by the
// developers
#undef M_ERROR
#define M_ERROR(format__, ...) do { log(LOG_ERR, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// Errors which must be handled by the developers
#undef M_WARNING
#define M_WARNING(format__, ...) do { log(LOG_WARNING, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// Conditions that are not error conditions, but should possibly be
// handled specially
#undef M_NOTICE
#define M_NOTICE(format__, ...) do { log(LOG_NOTICE, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

/* 
 * Despite what you pass to setlogmask(), in its default
 * configuration, OS X, with the standard configuration, will only
 * write messages to the system log that have a priority of LOG_NOTICE
 * or higher. Same for fedora, but here the lower limit is LOG_INFO. 
 */
#ifdef __APPLE__

// Informational messages
#undef M_INFO
#define M_INFO(format__, ...) do { log(LOG_NOTICE, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// Messages that contain information normally of use only when
// debugging a program.
#undef M_DEBUG
#define M_DEBUG(format__, ...) do { log(LOG_NOTICE, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

#elif defined __linux__

// Informational messages
#undef M_INFO
#define M_INFO(format__, ...) do { log(LOG_INFO, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

// Messages that contain information normally of use only when
// debugging a program.
#undef M_DEBUG
#define M_DEBUG(format__, ...) do { log(LOG_INFO, "Process ID:%d, Function: %s(), File: %s, " format__, (int)getpid(), __FUNCTION__, CODE_POS__, ## __VA_ARGS__); } while (0)

#endif // __linux__
