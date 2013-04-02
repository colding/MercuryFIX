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
#include <arpa/inet.h>
#include "ipc_server.h"

// struct sock_pair {
//      int sockd;
//      int ipc_sockd;
//      CGuard *guard;
// };


/*
 * Create thread to handle IO over a socket. Return 1 if successful, 0
 * (zero) otherwise.
 */
// static int
// create_io_thread(CGuard * const guard,
//               int sockd,
//               int ipc_sockd,
//               void *(*thread_func)(void *))
// {
//      pthread_t thread_id;
//      pthread_attr_t thread_attr;
//      struct sock_pair *sp = (struct sock_pair*)malloc(sizeof(struct sock_pair));

//      if (!sp) {
//              M_CRITICAL("no memory\n");
//              return 0;
//      }

//      if (0 > sockd) {
//              M_CRITICAL("invalid socket\n");
//              return 0;
//      }
//      sp->sockd = sockd;
//      sp->ipc_sockd = ipc_sockd;
//      sp->guard = guard;

//      if (pthread_attr_init(&thread_attr))
//              return 0;

//      if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED))
//              return 0;

//      if (pthread_create(&thread_id, &thread_attr, thread_func, (void*)sp))
//              return 0;

//      pthread_attr_destroy(&thread_attr);

//      return 1;
// }

/*
 * This thread is created in a worker which has accept()'ed a command
 * connection from a Mercury client. It handles the connection to one,
 * and only one, Mercury client. Other clients are handled in other
 * threads.
 */
// static void*
// worker_cmd_client_thread(void *sockets)
// {
//      struct sock_pair *sp = (struct sock_pair*)sockets;
//      int fd = -1;          // socket accept()'ed from client
//      int ctrl_fd = -1;     // socket to the controller
//      CGuard *guard = NULL; // mutex protecting ctrl_fd
//      uint32_t cnt = 0;     // bytes received into buf
//      uint8_t buf[MERCURY_COMMAND_BUFFER_RECV_SIZE]; // receive and transmit buffer

//      if (!sp)
//              return NULL;
//      fd = sp->sockd;
//      ctrl_fd = sp->ipc_sockd;
//      guard = sp->guard;
//      free(sockets);
//      if ((-1 == fd) || (-1 == ctrl_fd))
//              return NULL;

//      M_DEBUG("entering worker command loop\n");
//      do {
//              // get command from client
//              if (!recv_cmd(fd, buf, MERCURY_COMMAND_BUFFER_RECV_SIZE, &cnt)) {
//                      M_WARNING("error receiving from client\n");
//                      break;
//              }

//              // hand over to controller
//              if (send_fd(guard, ctrl_fd, buf, cnt, fd, &cnt)) {
//                      M_WARNING("could not forward command to controller: %s\n", strerror(errno));
//                      break;
//              }
//      } while (1);

//      close(fd);

//      return NULL;
// }

// void
// worker_handle_command_socket(socket_t cmd_socket,
//                           socket_t ipc_sockd_to_controller,
//                           const int domain,
//                           const int cpu_id)
// {
//      CGuard guard; // will protect the IPC socket to the controller
//      socklen_t addr_size;
//      int comm_sockd;
//      struct sockaddr_in s4;
//      struct sockaddr_in6 s6;
//      struct sockaddr *peer_addr;

//      if (0 > cmd_socket)
//              return;
//      if (-1 == cpu_id)
//              M_WARNING("invalid CPU ID\n");

//      switch (domain) {
//      case PF_INET:
//              peer_addr = (struct sockaddr *)&s4;
//              addr_size = sizeof(struct sockaddr_in);
//              break;
//      case PF_INET6:
//              peer_addr = (struct sockaddr *)&s6;
//              addr_size = sizeof(struct sockaddr_in6);
//              break;
//      default:
//              M_CRITICAL("invalid address family\n");
//              return;
//      }

//      do {
//              comm_sockd = accept(cmd_socket, peer_addr, &addr_size);
//              if (-1 == comm_sockd) {
//                      M_CRITICAL("error on accept: %s\n", strerror(errno));
//                      continue;
//              }
//              if (!create_io_thread(&guard, comm_sockd, ipc_sockd_to_controller, worker_cmd_client_thread)) {
//                      close(comm_sockd);
//                      M_CRITICAL("could not create client command thread\n");
//              }
//      } while (1);
// }

/*
 * Handle FIX sessions from Mercury clients
 */
// static void*
// worker_fix_client_thread(void *sockets)
// {
//      struct sock_pair *sp = (struct sock_pair*)sockets;
//      int fd = -1;
//      int ctrl_fd = -1;
//      CGuard *guard;
//      ssize_t retv = -1;
//      uint8_t buf[MERCURY_FIX_BUFFER_RECV_SIZE];
//      struct sockaddr address;
//      if (!sp)
//              return NULL;
//      fd = sp->sockd;
//      ctrl_fd = sp->ipc_sockd;
//      guard = sp->guard;
//      free(sockets);

//      do {
//              retv = recvfrom(fd, (void*)buf, MERCURY_FIX_BUFFER_RECV_SIZE, 0, &address, NULL);
//      } while (retv);

//      close(fd);

//      return NULL;
// }

// void
// worker_handle_FIX_socket(socket_t fix_socket,
//                       socket_t ipc_sockd_to_controller,
//                       const int domain,
//                       const int cpu_id)
// {
//      CGuard guard; // will protect the IPC socket to the controller
//      socklen_t addr_size;
//      int comm_sockd;
//      struct sockaddr_in s4;
//      struct sockaddr_in6 s6;
//      struct sockaddr *peer_addr;

//      if (0 > fix_socket)
//              return;
//      if (-1 == cpu_id)
//              M_WARNING("invalid CPU ID\n");

//      switch (domain) {
//      case PF_INET:
//              peer_addr = (struct sockaddr *)&s4;
//              addr_size = sizeof(struct sockaddr_in);
//              break;
//      case PF_INET6:
//              peer_addr = (struct sockaddr *)&s6;
//              addr_size = sizeof(struct sockaddr_in6);
//              break;
//      default:
//              M_CRITICAL("invalid protocol family\n");
//              return;
//      }

//      do {
//              comm_sockd = accept(fix_socket, peer_addr, &addr_size);
//              if (-1 == comm_sockd) {
//                      M_CRITICAL("error on accept: %d\n", strerror(errno));
//                      continue;
//              }
//              if (!create_io_thread(&guard, comm_sockd, ipc_sockd_to_controller, worker_fix_client_thread)) {
//                      close(comm_sockd);
//                      M_CRITICAL("could not create client FIX thread\n");
//              }
//      } while (1);
// }


// static inline void
// handle_register_client(ClientBank *client_bank,
//                     const socket_t client_fd,
//                     const ipcdata_t const data)
// {
//      uint32_t id;
//      char *target_comp_id = NULL;
//      char *target_sub_id = NULL;

//      if (M_CMD_REGISTER_SERVER_FORMAT_ARG_COUNT != unmarshal(ipcdata_get_data(data),
//                                                              ipcdata_get_datalen(data),
//                                                              M_CMD_REGISTER_SERVER_FORMAT,
//                                                              &target_comp_id,
//                                                              &target_sub_id))
//              goto error;
//      id = client_bank->register_server(client_fd, target_comp_id, target_sub_id);
//      if (!id) {
//              free(target_comp_id);
//              free(target_sub_id);
//              goto error;
//      }

//      if (!send_result(client_fd, Mercury::M_OK)) {
//              client_bank->unregister_server(id);
//              M_WARNING("error sending result to client\n");
//              goto error;
//      }

//      // Now we have the server socket and IDs. Ask
//      // it to create a connection pool with a
//      // supplied GUID.
//      if (!send_cmd((*client_bank)[id]->cmd_socket,
//                    M_CMD_CONNECT_WITH_GIVEN_UID,
//                    M_CMD_CONNECT_WITH_GIVEN_UID_FORMAT,
//                    id)) {
//              client_bank->unregister_server(id);
//              M_WARNING("error commanding client\n");
//              goto error; // the error cause does not need to be
//                          // network related so we try to return an
//                          // error code
//      }

//      M_DEBUG("server registered\n");
//      return;

// error:
//      if (!send_result(client_fd, Mercury::M_FAILURE)) {
//              client_bank->unregister_server(id);
//              M_WARNING("error sending result to client\n");
//      }
// }

// static inline void
// handle_reconnect_with_given_id(ClientBank *client_bank,
//                             const socket_t client_fix_fd,
//                             const ipcdata_t const data)
// {
//      uint32_t id;

//      if (M_CMD_RECONNECTION_WITH_GIVEN_UID_FORMAT_ARG_COUNT != unmarshal(ipcdata_get_data(data),
//                                                                          ipcdata_get_datalen(data),
//                                                                          M_CMD_RECONNECTION_WITH_GIVEN_UID_FORMAT,
//                                                                          &id)) {
//              M_WARNING("error unmarshalling\n");
//              goto error;
//      }

//      if (!(*client_bank)[id]->add_FIX_socket(client_fix_fd)) {
//              client_bank->unregister_server(id);
//              M_WARNING("error adding worker socket for client\n");
//              goto error;
//      }

//      if (!send_result(client_fix_fd, Mercury::M_OK)) {
//              client_bank->unregister_server(id);
//              M_WARNING("error sending result to client\n");
//              return; // this error is surely network related so we
//                      // do not even try to return a network error
//      }

//      M_DEBUG("server reconnected\n");
//      return;

// error:
//      if (!send_result(client_fix_fd, Mercury::M_FAILURE)) {
//              client_bank->unregister_server(id);
//              M_WARNING("error sending result to client\n");
//      }
// }

// void
// controller_handle_ipc_socket(ClientBank *client_bank,
//                           socket_t ipc_socket,
//                           const int cpu_id)
// {
//      socket_t client_fd = SOCKET_INIT; // received socket from client
//      int retv;
//      uint8_t buf[MERCURY_COMMAND_BUFFER_RECV_SIZE]; // receive and transmit buffer
//      uint32_t cnt = 0;   // bytes received into buf
//      uint32_t cmd;

//      if (-1 == ipc_socket)
//              return;

//      if (pin_thread(cpu_id))
//              M_WARNING("could not set CPU affinity - %s\n", strerror(errno));

//      do {
//              // get command from worker process
//              retv = recv_fd(NULL,
//                             ipc_socket,
//                             buf,
//                             MERCURY_COMMAND_BUFFER_RECV_SIZE,
//                             &client_fd, // client socket
//                             &cnt);
//              if (retv || !cnt)  { // error or worker closing connection
//                      M_DEBUG("retv = %d, cnt = %d\n", retv, cnt);
//                      break; // exit thread
//              }
//              cmd = ipcdata_get_cmd((ipcdata_t)buf);

//              /*
//               * Do something with the received command!
//               */

//              switch (cmd) {
//              case M_CMD_REGISTER_SERVER:
//                      handle_register_client(client_bank, client_fd, (ipcdata_t)buf);
//                      break;
//              case M_CMD_RECONNECTION_WITH_GIVEN_UID:
//                      handle_reconnect_with_given_id(client_bank, client_fd, (ipcdata_t)buf);
//                      break;
//              default:
//                      M_WARNING("unknown command received\n");
//                      break;
//              }
//      } while (cnt);

//      M_DEBUG("exiting controler command loop\n");
//      return;
// }
