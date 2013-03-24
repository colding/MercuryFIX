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

#ifdef __linux__
    #include <sys/param.h>
#endif
#include "stdlib/log/log.h"
#include "cpu.h"
#include "id.h"

bool
generate_default_ids(std::vector<std::string> & IDs)
{
        // this leave one CPU to attend to everything else but our work
        int cpu_cnt = get_available_cpu_count();
        if (1 > cpu_cnt) {
                M_NOTICE("CPU count does not meet minimum requirements");
                return false;
        }
        M_DEBUG("cpu_cnt = %d\n", cpu_cnt);

        char host_name[MAXHOSTNAMELEN] = { '\0' };
        if (gethostname(host_name, sizeof(host_name))) {
                M_ERROR("could not get host name %s", strerror(errno));
                return false;
        }

        char domain_name[MAXHOSTNAMELEN] = { '\0' };
        if (getdomainname(domain_name, sizeof(domain_name))) {
                M_ERROR("could not get domain name %s", strerror(errno));
                return false;
        }

        IDs.empty();
        std::string id;
        for (int n = 0; n < cpu_cnt; ++n) {
                id = host_name;
		id += ".";
                id += domain_name;
		id += n;
                IDs.push_back(id);
        }

	return true;
}
