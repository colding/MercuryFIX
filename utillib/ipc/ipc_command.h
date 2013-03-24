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
#include <inttypes.h>
#include "stdlib/log/log.h"

/*
 * These commands are documented below.
 *
 * Remember that the size of the recieving buffer is limited to
 * (8*1024) bytes so you must not use IPC to return big data.
 *
 * 0 (zero) is not a valid command. That is to limit the possibility
 * of someone sending uninitialized data over IPC and getting it
 * interpreted without any alarm bells going of.
 */
#ifdef HAVE_STRONGLY_TYPED_ENUMS
enum IPC_Command : uint32_t {
#else
enum IPC_Command {
#endif
	CMD_ILLEGAL = 0x00000000,
        CMD_UNDEF   = 0xDEADBEEF,
	CMD_RESULT  = 0x00000001,
	CMD_MESSAGE = 0x00000002,
	CMD_PING    = 0x00000003,
};

/*
 * Simple codes saying how a specific command went about.
 *
 * 0 (zero) is not a valid return code. That is to limit the
 * possibility of someone sending uninitialized data over IPC and
 * getting it interpreted without any alarm bells going of.
 */
#ifdef HAVE_STRONGLY_TYPED_ENUMS
enum IPC_ReturnCode : uint32_t {
#else
enum IPC_ReturnCode {
#endif
	RES_ILLEGAL = 0x00000000,
	RES_UNDEF   = 0xDEADBEEF,
        RES_OK      = 0x00000001,
        RES_FAILURE = 0x00000002,
};


/*
 * Any command which does not return a value must be listed here
 */
static inline bool
command_is_oneway(const IPC_Command cmd)
{
        switch (cmd) {
	case CMD_ILLEGAL:
		M_ERROR("Illegal comand: %x", cmd);
		break;
        case CMD_UNDEF :
        case CMD_RESULT :
        case CMD_MESSAGE :
                return true;
        default :
                break;
        }

        return false;
}

/*
 * CMD_UNDEF - oneway
 *
 * The empty command. It shall have no effect other than being a
 * convenient initialization value.
 */
#define CMD_UNDEF_FORMAT ""
#define CMD_UNDEF_FORMAT_ARG_COUNT (0)

/*
 * CMD_RESULT - oneway
 *
 * Send result to command issuer.
 *
 * Send a return value back to whoever has just issued a command. The
 * first sizeof(uint32_t) bytes of the data is the resulting
 * IPC_Result in network byte order.
 *
 * The remaining data contains any complex return data for commands
 * querying that. Those commands have *_RETURN_DATA_FORMAT definitions
 * that specifies how the complex return data is formatted. The format
 * is:
 *
 * "%ul....."
 *
 * The leading %ul is the IPC_Result. The remaining data is the actual
 * return data if any.
 */
#define CMD_RESULT_FORMAT "%ul"
#define CMD_RESULT_FORMAT_ARG_COUNT (1)

/*
 * CMD_MESSAGE - oneway
 *
 * Sends a message.
 */
#define CMD_MESSAGE_FORMAT "%s"
#define CMD_MESSAGE_FORMAT_ARG_COUNT (1)

/*
 * CMD_PING
 *
 * Pings the recipient and expects RES_OK back.
 */
#define CMD_PING_FORMAT ""
#define CMD_PING_FORMAT_ARG_COUNT (0)
#define CMD_RESULT_RETURN_FORMAT "%ul"
#define CMD_RESULT_RETURN_FORMAT_VALUE_COUNT (1)
