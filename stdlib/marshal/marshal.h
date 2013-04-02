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
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

/*
 * Marshalling format specifiers
 * =============================
 *
 * Convention:
 * -----------
 *
 * The marshalling of data to and from a format suitable for
 * transmission over a network is controlled by a printf(3) inspired
 * format string. The type of any one argument in the variable
 * argument list is specified using a special character in the format
 * string. Any whitespace characters in a format string will be
 * ignored.
 *
 * All marshalled values are in network byte order. All unmarshalled
 * values are in host byte order.
 *
 * %[modifier]<specifier>
 *
 * Modifiers:
 * ----------
 *
 * 'u' - unsigned integer value following. Ignored for strings and
 * floats
 *
 *
 * Specifiers for marshalling:
 * ---------------------------
 *
 * 's' - Null terminated string
 * 'b' - 8 bit integer
 * 'w' - 16 bit integer
 * 'l' - 32 bit integer
 * 'f' - 32 bit IEEE 754 floating point a.k.a. binary32. Transmitted as an unsigned 32 bit integer.
 *
 * Specifiers for unmarshalling:
 * -----------------------------
 *
 * 's' - Pointer to memory with ample space for a null terminated string
 * 'b' - Pointer to 8 bit integer
 * 'w' - Pointer to 16 bit integer
 * 'l' - Pointer to 32 bit integer
 * 'f' - Pointer to 32 bit IEEE 754 floating point
 *
 */

/*
 * Returns the number of bytes needed to hold the variable argument
 * list.
 */
extern uint32_t
vmarshal_size(const char *format,
	      va_list ap);

extern uint32_t
marshal_size(const char *format,
	     ...);


/*
 * Marshals the variable argument list into the buffer.  Returns true
 * and *count receives the number of bytes written into "buf" unless
 * an error occurred, in which case *count is undefined, and false is
 * returned.
 */
extern bool
vmarshal(uint8_t * const buf, // buffer
	 const uint32_t len,  // size of buffer in bytes
	 uint32_t *count,     // number of bytes marshalled into the buffer
	 const char *format,  // format string specifying the argument list
	 va_list ap);         // argument list

extern bool
marshal(uint8_t * const buf,
	const uint32_t len,
	uint32_t *count,
	const char *format,
	...);

/*
 * Marshals content from buf into the variable argument list according
 * to the format string. Returns the number of arguments successfully
 * assigned with a value from buf or -1 in case of error.
 */
extern int
vunmarshal(const uint8_t * const buf, // buffer
	   const uint32_t len,        // size of buffer in bytes
	   const char *format,        // format string specifying the argument list
	   va_list ap);               // argument list

extern int
unmarshal(const uint8_t * const buf,
	  const uint32_t len,
	  const char *format,
	  ...);
