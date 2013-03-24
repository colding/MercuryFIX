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
#include "ipc.h"

/*
 * Will, internally in the controller, handle all IPC commands from
 * worker processes on the specified socket. Blocks forever or until
 * an error occurs.
 */
/* extern void */
/* controller_handle_ipc_socket(ClientBank *client_bank, */
/*                           socket_t ipc_socket, */
/*                           const int cpu_id); */

/*
 * Will, internally in a worker, create a thread to handle IPC
 * commands from Mercury clients on the specified socket. Blocks
 * forever or until an error occurs.
 */
/* extern void */
/* worker_handle_command_socket(socket_t cmd_socket, */
/*                           socket_t ipc_sockd_to_controller, */
/*                           const int domain, */
/*                           const int cpu_id); */

/*
 * Will, internally in a worker, create a thread to handle FIX
 * sessions from Mercury clients on the specified socket. Blocks
 * forever or until an error occurs.
 */
/* extern void */
/* worker_handle_FIX_socket(socket_t fix_socket, */
/*                       socket_t ipc_sockd_to_controller, */
/*                       const int domain, */
/*                       const int cpu_id); */
