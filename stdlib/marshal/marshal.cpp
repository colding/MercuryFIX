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
#include "stdlib/log/log.h"
#include "marshal.h"
#include "primitives.h"
#include "ieee754.h"

uint32_t
marshal_size(const char *format,
             ...)
{
        uint32_t retv;
        va_list ap;

        va_start(ap, format);
        retv = vmarshal_size(format, ap);
        va_end(ap);

        return retv;
}

uint32_t
vmarshal_size(const char *format,
              va_list ap)
{
        uint32_t retv = 0;
        uint8_t ub __attribute__ ((unused));
        uint16_t uw __attribute__ ((unused));
        uint32_t ul __attribute__ ((unused));
        double f __attribute__ ((unused));
        char *s;

        if (!format || !strlen(format))
                return 0;

        for (; '\0' != *format; format++) {

                if ('%' == *format) {
                        format++;
                        if ('u' == *format)
                                format++;
                } else {
                        continue;
                }

                switch (*format) {
                case 's':
                        s = va_arg(ap, char*);
                        if (s)
                                retv += strlen(s) + sizeof(char);
                        break;
                case 'b':
                        ub = (uint8_t)va_arg(ap, int);
                        retv += sizeof(uint8_t);
                        break;
                case 'w':
                        uw = (uint16_t)va_arg(ap, int);
                        retv += sizeof(uint16_t);
                        break;
                case 'l':
                        ul = va_arg(ap, uint32_t);
                        retv += sizeof(uint32_t);
                        break;
                case 'f':
                        f = va_arg(ap, double);
                        retv += 32; // fixed size
                        break;
                default:
                        goto exit;
                }
                continue;
        exit:
                break;
        }

        return retv;
}

bool
marshal(uint8_t * const buf,
        const uint32_t len,
        uint32_t *count,
        const char *format,
        ...)
{
        bool retv;
        va_list ap;

        va_start(ap, format);
        retv = vmarshal(buf, len, count, format, ap);
        va_end(ap);

        return retv;
}

bool
vmarshal(uint8_t * const buf,
         const uint32_t len,
         uint32_t *count,
         const char *format,
         va_list ap)
{
        size_t inc = 0;
        uint8_t *pos = buf;
        bool not_signed = true;
        int8_t b;
        uint8_t ub;
        int16_t w;
        uint16_t uw;
        int32_t l;
        uint32_t ul;
        double f;
        char *s;

        *count = 0;
        if (!len) {
                M_DEBUG("Error marshalling");
                return false;
        }
        for (; '\0' != *format; format++) {

                if ('%' == *format) {
                        format++;
                        if ('u' == *format) {
                                not_signed = true;
                                format++;
                        } else {
                                not_signed = false;
                        }
                }

                switch (*format) {
                case 's':
                        s = va_arg(ap, char*);
                        inc = strlen(s) + 1;
                        if (len < *count + inc) {
                                M_DEBUG("Error marshalling");
                                return false;
                        }
                        memcpy((void*)pos, (const void*)s, inc);
                        break;
                case 'b':
                        inc = sizeof(uint8_t);
                        if (len < *count + inc) {
                                M_DEBUG("Error marshalling");
                                return false;
                        }
                        if (not_signed) {
                                ub = (uint8_t)va_arg(ap, int);
                                *pos = ub;
                        } else {
                                b = va_arg(ap, int);
                                *pos = (uint8_t)b;
                        }
                        break;
                case 'w':
                        inc = sizeof(uint16_t);
                        if (len < *count + inc) {
                                M_DEBUG("Error marshalling");
                                return false;
                        }
                        if (not_signed) {
                                uw = (uint16_t)va_arg(ap, int);
                                setus(pos, uw);
                        } else {
                                w = (int16_t)va_arg(ap, int);
                                setus(pos, (uint16_t)w);
                        }
                        break;
                case 'l':
                        inc = sizeof(uint32_t);
                        if (len < *count + inc) {
                                M_DEBUG("Error marshalling");
                                return false;
                        }
                        if (not_signed) {
                                ul = va_arg(ap, uint32_t);
                                setul(pos, ul);
                        } else {
                                l = va_arg(ap, int32_t);
                                setul(pos, (uint32_t)l);
                        }
                        break;
                case 'f':
                        inc = 32;
                        if (len < *count + inc) {
                                M_DEBUG("Error marshalling");
                                return false;
                        }
                        f = va_arg(ap, double);
                        ul = pack754_32(f);
                        setul(pos, ul);
                        break;
                default:
                        M_WARNING("invalid format string");
                        continue;
                }
                *count += inc;
                if (len == *count) {
                        format++; // to test for "end reached" of format string
                        break;
                }

                pos += inc;
        }

	return ('\0' == *format); // end reached of format string?
}


int
unmarshal(const uint8_t * const buf,
	  const uint32_t len,
	  const char *format,
	  ...)
{
	int retv;
	va_list ap;

	va_start(ap, format);
	retv = vunmarshal(buf, len, format, ap);
	va_end(ap);

	return retv;
}

int
vunmarshal(const uint8_t * const buf,
	   const uint32_t len,
	   const char *format,
	   va_list ap)
{
	int retv = 0;
	size_t cnt = 0;
	const uint8_t *pos = buf;
	bool not_signed = true;
	int8_t *b;
	uint8_t *ub;
	int16_t *w;
	uint16_t *uw;
	int32_t *l;
	uint32_t *ul;
	uint32_t n; // temporary value used when unpacking ieee 754 floats
	double *f;
	char **s = NULL;

	if (1 > len)
		return 0; // nipseriet

	for (; '\0' != *format; format++) {

		if ('%' == *format) {
			format++;
			if ('u' == *format) {
				not_signed = true;
				format++;
			} else
				not_signed = false;
		}

		switch (*format) {
		case 's':
			s = va_arg(ap, char**);
			*s = strdup((const char*)pos);
			if (*s) {
				cnt += strlen(*s) + sizeof(char);
			} else {
				return -1;
			}
			retv++;
			break;
		case 'b':
			cnt += sizeof(uint8_t);
			if (not_signed) {
				ub = (uint8_t*)va_arg(ap, uint8_t*);
				*ub = *pos;
			} else {
				b = (int8_t*)va_arg(ap, int8_t*);
				*b = (int8_t)*pos;
			}
			retv++;
			break;
		case 'w':
			cnt += sizeof(uint16_t);
			if (not_signed) {
				uw = (uint16_t*)va_arg(ap, uint16_t*);
				*uw = getus(pos);
			} else {
				w = (int16_t*)va_arg(ap, int16_t*);
				*w = (int16_t)getus(pos);
			}
			retv++;
			break;
		case 'l':
			cnt += sizeof(uint32_t);
			if (not_signed) {
				ul = va_arg(ap, uint32_t*);
				*ul = getul(pos);
			} else {
				l = va_arg(ap, int32_t*);
				*l = (int32_t)getul(pos);
			}
			retv++;
			break;
		case 'f':
			cnt += 32;
			f = va_arg(ap, double*);
			n = getul(pos);
			*f = unpack754_32(n);
			retv++;
			break;
		default:
//M_WARNING("invalid format string");
			return -1;
		}
		if (len == cnt)
			break;
		if (len < cnt) // overflow
			return -1;
		pos = buf + cnt;
	}

	return retv;
}
