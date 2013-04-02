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
#include <sys/socket.h>
#include "stdlib/log/log.h"
#include "stdlib/config/config_file.h"
#include "config_item_network.h"

bool
ConfigItemNetwork::fill(const Config::DataSource data_source,
                        const void * const data)
{
	if (!refcnt_)
                return false;

        if (Config::File != data_source)
                return false;

 	pthread_rwlock_wrlock(&rw_lock_);
        free(config_line_);
        if (data) {
                config_line_ = strdup((const char*)data);
                if (!config_line_) {
			pthread_rwlock_unlock(&rw_lock_);
                        M_ALERT("no memory");
                        return false;
                }
        } else {
                config_line_ = NULL;
        }
	pthread_rwlock_unlock(&rw_lock_);

        return true;
}

bool
ConfigItemNetwork::get(std::vector<endpoint_t> & endpoints)
{
        bool retv = false;
        endpoint_t endp;
	char *tmp = NULL;
        char *data_dup = NULL;
        char *to_free = NULL;
        char *pstr = NULL;
        long port = -1;

	endpoints.clear();

	pthread_rwlock_rdlock(&rw_lock_);
        if (!config_line_) {
		pthread_rwlock_unlock(&rw_lock_);
                return false;
	}

	data_dup = strdup(config_line_);
	pthread_rwlock_unlock(&rw_lock_);
	if (!data_dup) {
		M_ALERT("no memory");
		return false;
	}
	to_free = data_dup;

        do {
                tmp = strsep(&data_dup, ConfigFile::delims);
                if (!tmp) {
                        retv = true;
                        break;
                }

                switch (*tmp) {
		case '?':
			endp.pf_family = PF_UNSPEC;
			break;
                case '4':
                        endp.pf_family = PF_INET;
                        break;
                case '6':
                        endp.pf_family = PF_INET6;
                        break;
                default:
                        M_ERROR("invalid pf family: %c", *tmp);
                        goto out;
                }
                tmp++;

                switch (*tmp) {
                case 'C':
                        endp.kind = ConnectToThis;
                        break;
                case 'L':
                        endp.kind = ListenOnThis;
                        break;
                default:
                        M_ERROR("invalid endpoint kind: %c", *tmp);
                        goto out;
                }
                tmp++;

                pstr = strchr(tmp, '|');
                *pstr = '\0';
                endp.interface = tmp;
                if (++pstr) {
                        port = strtol(pstr, &pstr, 10);
                        if ('\0' != *pstr) {
                                M_ERROR("invalid port value in endpoint: %s", tmp);
                                goto out;
                        }
                        if ((1 > port) || (0xFFFF < port)) {
                                M_ERROR("invalid port value in endpoint: %l", port);
                                goto out;
                        }
                        endp.port = port;
                } else {
                        M_ERROR("missing port value in endpoint: %s", tmp);
                        goto out;
                }

                if (1 >= strlen(tmp)) {
                        M_ERROR("invalid endpoint value: %s", tmp);
                        goto out;
                }

                endpoints.push_back((const endpoint_t)endp);
        } while (true);

out:
        free(to_free);

        return retv;
}
