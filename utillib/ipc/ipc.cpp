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
#include <errno.h>
#include "stdlib/log/log.h"
#include "stdlib/macros/macros.h"
#include "stdlib/marshal/marshal.h"
#include "ipc.h"

bool
recv_result(int socket,
            const IPC_Command issuing_cmd,
            IPC_ReturnCode & return_code,
            void * const buf,
            const uint32_t buf_len,
            uint32_t *count)
{
        IPC_Command cmd;

        if (!buf || !buf_len) {
                M_WARNING("NULL buffers");
                return false;
        }

        if (command_is_oneway(issuing_cmd)) {
                M_WARNING("no result from this command: %X", issuing_cmd);
                return false;
        }

        if (!recv_cmd(socket, buf, buf_len, count))
                return false;

	/*
	 * now parse the result
	 */
        cmd = ipcdata_get_cmd((ipcdata_t)buf);
        if (CMD_RESULT == cmd) {
                return_code = ipcdata_get_return_code((ipcdata_t)buf);
                if (sizeof(uint32_t) != ipcdata_get_datalen((ipcdata_t)buf))
                        M_DEBUG("received complex result data");
        } else {
                M_DEBUG("expected result but got command %X", cmd);
                return false;
        }

        return true;
}


bool
recv_cmd(int socket,
         void * const buf,
         const uint32_t buf_len,
         uint32_t *count)
{
        timeout_t t;
        ipcdata_t pos = buf;
        uint32_t rem;
        ssize_t packet_size;
        ssize_t recv_cnt_acc = 0; // accumulated receive count
        ssize_t recv_cnt = 0;

        if (!buf || !buf_len) {
                M_WARNING("NULL buffer");
                return false;
        }

	/*
	 * Initially we set an infinite timeout.
	 */
        t.seconds = 0;
        if (!set_recv_timeout(socket, t)) {
                M_WARNING("could not set timeout");
                return false;
        }

        do {
                recv_cnt = recvfrom(socket, (void*)((uint8_t*)pos + recv_cnt_acc), buf_len - recv_cnt_acc, MSG_WAITALL, NULL, NULL);
                switch (recv_cnt) {
                case -1:
                        M_WARNING("error: %s", strerror(errno));
                        return false;
                case 0:
                        M_WARNING("peer disconnected");
                        return false;
                default:
			/*
			 * Now we set a timeout of 5 seconds. Continue
			 * transmitting or fail within that limit.
			 *
			 * Using UNLIKELY to tell the compiler that we
			 * only think this will happen once in a while.
			 */
                        if UNLIKELY(!recv_cnt_acc) {
                                t.seconds = 60;
                                if (!set_recv_timeout(socket, t)) {
                                        M_WARNING("could not set timeout");
                                        return false;
                                }
                        }
                        break;
                }
                recv_cnt_acc += recv_cnt;
                if ((ssize_t)IPC_HEADER_SIZE <= recv_cnt_acc)
                        break;
        } while (true);

        // infer command length and get the rest
        rem = ipcdata_get_datalen(pos);
        packet_size = rem + IPC_HEADER_SIZE;
        if (buf_len < packet_size) {
                M_WARNING("buffer too small. Required %d, available %d", packet_size, buf_len);
                return false;
        }
        while UNLIKELY(recv_cnt_acc < packet_size) {
                recv_cnt = recvfrom(socket, (void*)((uint8_t*)pos + recv_cnt_acc), buf_len - recv_cnt_acc, MSG_WAITALL, NULL, NULL);
                switch (recv_cnt) {
                case -1:
                        M_WARNING("error: %s", strerror(errno));
                        return false;
                case 0:
                        M_WARNING("peer disconnected");
                        return false;
                default:
                        break;
                }
                recv_cnt_acc += recv_cnt;
        }
        *count = recv_cnt_acc;

        return true;
}

uint32_t
send_cmd(int socket,
         IPC_Command cmd,
         const char * const format,
         ...)
{
        uint32_t retv;
        va_list ap;

        va_start(ap, format);
        retv = vsend_cmd(socket, cmd, format, ap);
        va_end(ap);

        return retv;
}

uint32_t
vsend_cmd(int socket,
          IPC_Command cmd,
          const char * const format,
          va_list ap)
{
        static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        static uint8_t buf[IPC_BUFFER_SIZE];
        uint32_t len;
        uint8_t *pos;
        va_list ap2;

        if (!cmd) {
                M_ERROR("Illegal comand: %x", cmd);
                return 0;
        }

        va_copy(ap2, ap);
        len = vmarshal_size(format, ap2);
        va_end(ap2);
        if ((IPC_BUFFER_SIZE - IPC_HEADER_SIZE) < len) {
                errno = EINVAL;
                return 0;
        }

        while (pthread_mutex_trylock(&mutex))
                usleep(1);

        ipcdata_set_header(cmd, len, (ipcdata_t)buf);
        if (len) {
                pos = buf + IPC_HEADER_SIZE;
                if (!vmarshal(pos, len, &len, format, ap)) {
                        M_ERROR("could not vmarshal");
                        goto error;
                }
        }
        len += IPC_HEADER_SIZE;
        if (!send_all(socket, buf, len)) {
                M_ERROR("could not send_all");
                goto error;
        }
        goto exit;

error:
        len = 0;

exit:
        pthread_mutex_unlock(&mutex);

        return len;
}
