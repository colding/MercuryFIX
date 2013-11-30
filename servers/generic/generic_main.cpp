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
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include "stdlib/scm_state/scm_state.h"
#include "stdlib/cmdline/argopt.h"
#include "stdlib/log/log.h"
#include "stdlib/config/config.h"
#include "stdlib/process/id.h"
#include "stdlib/process/daemon.h"
#include "stdlib/process/threads.h"
#include "stdlib/network/network.h"
#include "utillib/config/config_item_simple.h"
#include "utillib/config/config_item_string_vector.h"
#include "utillib/ipc/ipc.h"
#include "generic_main.h"

#define MASTER_SOCKET (0)
#define SLAVE_SOCKET  (1)

static void
print_version(void)
{
	fprintf(stdout, "This is %s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	fprintf(stdout, "Copyright (C) 2013, Jules Colding\n");
	fprintf(stdout, "All Rights Reserved\n");
}

/*
 * Kills all slave processes
 */
static inline void
kill_slave_processes(int /* sig */)
{
        kill(0, SIGTERM);
}

/*
 * Exists gracefully when getting SIGTERM
 */
static void
kill_self(int /* sig */)
{
        _exit(EXIT_SUCCESS);
}

static inline bool
create_ipc_thread(thread_arg_t *thread_arg,
                  void *(*thread_func)(void *))
{
        pthread_t thread_id;

        return create_detached_thread(&thread_id, (void*)thread_arg, thread_func);
}

static inline bool
create_worker_thread(thread_arg_t *thread_arg,
                     pthread_t * const thread_id,
                     void *(*thread_func)(void *))
{
        return create_joinable_thread(thread_id, (void*)thread_arg, thread_func);
}

/*
 * Signal handling for slave process
 */
static void
set_signal_handlers()
{
        struct sigaction sig_act;

        sig_act.sa_handler = kill_self;
        sigfillset(&sig_act.sa_mask); // ignore as many signals as possible when in the handler!
        sig_act.sa_flags = SA_RESETHAND;
        sigaction(SIGTERM, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESETHAND;
        sigaction(SIGUSR1, &sig_act, NULL);

        /* CONSIDER! restart the daemon if SIGUSR2 is caught */
        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESETHAND;
        sigaction(SIGUSR2, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESETHAND;
        sigaction(SIGINT, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESTART;
        sigaction(SIGPIPE, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESTART;
        sigaction(SIGALRM, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESTART;
        sigaction(SIGTTIN, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESTART;
        sigaction(SIGTTOU, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESTART;
        sigaction(SIGIO, &sig_act, NULL);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESTART;
        sigaction(SIGWINCH, &sig_act, NULL);
}

static inline thread_arg_t*
create_thread_arg(const char * const identity,
		  const char * const source,
                  const int socket)
{
        thread_arg_t *thread_arg = (thread_arg_t*)malloc(sizeof(thread_arg_t));
        if (!thread_arg) {
                return NULL;
        }
        thread_arg->identity = identity ? strdup(identity) : strdup("");
        if (!thread_arg->identity) {
                free(thread_arg);
                return NULL;
        }
        thread_arg->config_source = source ? strdup(source) : strdup("");
        if (!thread_arg->config_source) {
		free(thread_arg->config_source);
                free(thread_arg);
                return NULL;
        }
        thread_arg->socket = socket;

        return thread_arg;
}

static int
start_server(const bool debug,
             Config * const config)
{
        int retv = EXIT_FAILURE;
        int jval;
        int count;
        int ipc_socket_type;
        int socket;
        sigset_t mask;
        struct sigaction sig_act;
        pthread_t thread_id;
        thread_arg_t *thread_arg = NULL;
        daemon_exit_t dstat = PROCESS_IS_PARENT;
        int ipc_sockets[2] = { -1, -1 };
        ConfigItemSimple *simple_item;
        ConfigItemStringVector *vector_item;
        std::vector<std::string> IDs;

        if (!debug && (EXIT_SUCCESS != lock_down_process()))
                return EXIT_FAILURE;

	vector_item = new (std::nothrow) ConfigItemStringVector();
	if (!vector_item) {
		M_ALERT("no memory");
                return EXIT_FAILURE;
	}

        // restore signals
        sigfillset(&mask);
        pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

        if (!debug) {
                dstat = become_daemon();
                switch (dstat) {
                case PROCESS_IS_CHILD:
                        break;
                case PROCESS_IS_PARENT:
                        return EXIT_SUCCESS;
                default:
                        return EXIT_FAILURE;
                }
        }
        /*
         * We are now the controling daemon (if !debug) and must fork
         * of worker children.
         */

        if (!config->subscribe(NULL, "LOCALHOST", "CHILD_PROCESS_IDS", vector_item)) {
		delete vector_item;
                M_ERROR("could not read child process identities");
                return EXIT_FAILURE;
        }
        if (!vector_item->get(IDs)) {
                M_ERROR("could not read child process identities");
		vector_item->release();
                return EXIT_FAILURE;
        }
	vector_item->release();
        if (IDs.empty())
                generate_default_ids(IDs);

        sig_act.sa_handler = SIG_IGN;
        sigfillset(&sig_act.sa_mask);
        sig_act.sa_flags = SA_RESETHAND;
        sigaction(SIGTERM, &sig_act, NULL);

        /*
         * Fork for all available IDs and create IPC sockets for each
         * child process
         */
        count = IDs.size();
        do {
                --count;

                // create the IPC socket pair
#ifdef __linux__
                ipc_socket_type = SOCK_SEQPACKET;
#elif defined __APPLE__
                ipc_socket_type = SOCK_DGRAM;
#endif
                if (socketpair(PF_LOCAL, ipc_socket_type, 0, ipc_sockets)) {
                        M_ERROR("could not initialize IPC: %s\n", strerror(errno));
                        goto master_err;
                }

                dstat = become_daemon();
                switch (dstat) {
                case PROCESS_IS_CHILD:
			init_logging(debug, IDs[count].c_str());
                        M_DEBUG("slave created");
                        close(ipc_sockets[MASTER_SOCKET]);
                        ipc_sockets[MASTER_SOCKET] = -1;
                        goto slave;
                case PROCESS_IS_PARENT:
                        close(ipc_sockets[SLAVE_SOCKET]);
                        ipc_sockets[SLAVE_SOCKET] = -1;
                        break;
                default:
                        M_ERROR("ERROR\n");
                        goto master_err;
                }

                // create master IPC thread
                socket = ipc_sockets[MASTER_SOCKET];
                thread_arg = create_thread_arg(IDs[count].c_str(), config->config_source, socket);
                if (!thread_arg) {
                        M_ALERT("no memory");
                        goto master_err;
                }
		if (!set_min_recv_sice(thread_arg->socket, IPC_HEADER_SIZE)) {
			M_WARNING("could not set minimum receive size");
                        goto master_err;
                }			
                if (!create_ipc_thread(thread_arg, master_ipc_thread)) {
                        M_ERROR("could not create IPC thread");
                        goto master_err;
                }
        } while (0 < count);
        IDs.clear();

        /*
         * We are still the controling daemon
         */
	M_DEBUG("master done creating slaves");

	simple_item = new (std::nothrow) ConfigItemSimple();
	if (!simple_item) {
		M_ALERT("no memory");
                goto master_err;
	}
        if (!config->subscribe(NULL, "LOCALHOST", "PID_FILE_PATH", simple_item)) {
		delete simple_item;
                M_ERROR("could not read PID file name");
                goto master_err;
        }
        char *pid_file_path;
        simple_item->get(&pid_file_path);
	simple_item->release();
        if (!pid_file_path)
                pid_file_path = strdup(MERCURY_DEFAULT_PID_FILE);
        if (!pid_file_path) {
                M_ALERT("no memory");
                goto master_err;
        }

        {
                // take the control lock and write the pid
                PidFile pid_lock(pid_file_path);
                free(pid_file_path);
                if (pid_lock.error) {
                        M_ALERT("could not write pid file (%s) - dual startup?", pid_file_path);
                        goto master_err;
                }

                // create master worker thread
                socket = 0;
                thread_arg = create_thread_arg(config->default_identity, config->config_source, socket);
                if (!thread_arg)
                        goto master_err;
                if (!create_worker_thread(thread_arg, &thread_id, master_worker_thread)) {
                        M_ERROR("could not create worker thread");
                        goto master_err;
                }

                // waiting for worker thread to join
                jval = pthread_join(thread_id, NULL);
                if (jval) {
                        const char *msg = "UNKNOWN ERROR";
                        switch (jval) {
                        case EDEADLK:
                                msg = "EDEADLK";
                                break;
                        case EINVAL:
                                msg = "EINVAL";
                                break;
                        case ESRCH:
                                msg = "ESRCH";
                                break;
                        default:
                                break;
                        }
                        M_ERROR("error joining master worker thread: %s", msg);
                }
        }
        /*
         * Cleanup. All resources allocated should be deallocated
         * cleanly before exit.
         */

        retv = EXIT_SUCCESS;
master_err:
        // kill all children
	kill_slave_processes(0);

        return retv;

slave:
        /*
         * We are a child process
         */
	char *master_identity = strdup(config->default_identity);

	// set new slave identity
	free(config->default_identity);
	config->default_identity = strdup(IDs[count].c_str());
        IDs.clear();

        /*
         * handle signals
         */
        set_signal_handlers();

        //
        // Drop priveledges and switch to a lesser user and group if
        // so configured.
        //
        bool switched;
	simple_item = new (std::nothrow) ConfigItemSimple();
	if (!simple_item) {
		M_ALERT("no memory");
                goto slave_err;
	}
        if (!config->subscribe(NULL, "LOCALHOST", "USER", simple_item)) {
		delete simple_item;
                M_ERROR("could not read new user");
                goto slave_err;
        }
        char *user_name;
        simple_item->get(&user_name);
	simple_item->release();

	simple_item = new (std::nothrow) ConfigItemSimple();
	if (!simple_item) {
		M_ALERT("no memory");
                goto slave_err;
	}
        if (!config->subscribe(NULL, "LOCALHOST", "GROUP", simple_item)) {
		delete simple_item;
                M_ERROR("could not read new group");
                goto slave_err;
        }
        char *group_name;
        simple_item->get(&group_name);
	simple_item->release();
        switched = switch_user(user_name, group_name);
        free(user_name);
        free(group_name);
        if (!switched) {
                M_ALERT("could not drop priviledges");
                goto slave_err;
        }

        // create slave IPC thread
        socket = ipc_sockets[SLAVE_SOCKET];
        thread_arg = create_thread_arg(master_identity, config->config_source, socket);
	free(master_identity);
	master_identity = NULL;
        if (!thread_arg) {
		M_ALERT("no memory");
                return EXIT_FAILURE;
	}
	if (!set_min_recv_sice(thread_arg->socket, IPC_HEADER_SIZE)) {
		M_WARNING("could not set minimum receive size");
		goto slave_err;
	}
        if (!create_ipc_thread(thread_arg, slave_ipc_thread)) {
                M_ERROR("could not create IPC thread");
                goto slave_err;
        }

        // create slave worker thread
        socket = 0;
        thread_arg = create_thread_arg(config->default_identity, config->config_source, socket);
        if (!thread_arg)
                return EXIT_FAILURE;
        if (!create_worker_thread(thread_arg, &thread_id, slave_worker_thread)) {
                M_ERROR("could not create worker thread");
                goto slave_err;
        }

        // waiting for worker thread to join
        jval = pthread_join(thread_id, NULL);
        if (jval) {
                const char *msg = "UNKNOWN ERROR";
                switch (jval) {
                case EDEADLK:
                        msg = "EDEADLK";
                        break;
                case EINVAL:
                        msg = "EINVAL";
                        break;
                case ESRCH:
                        msg = "ESRCH";
                        break;
                default:
                        break;
                }
                M_ERROR("error joining slave worker thread: %s", msg);
        }
	M_INFO("slave worker thread joined - exiting");

        /*
         * Cleanup. All resources allocated should be deallocated
         * cleanly before exit.
         */

        retv = EXIT_SUCCESS;

slave_err:
	free(master_identity);
        return retv;
}

int
generic_main(int argc, char *argv[])
{
        Config *config = NULL;
        char *identity = NULL;
        int debug = 0;

        // default configuration file path
        char *conf_file_path = strdup("" /*MERCURY_DEFAULT_CONFIG_FILE*/);

        // current argv index
        int index = 0;

        int c;
        char *parameter;
        struct option_t options[] = {
                {"origin", "-origin Identifies the source used to build this server", NO_PARAM, NULL, 'o'},
                {"version", "-version Prints the version of the server", NO_PARAM, NULL, 'v'},
                {"identity", "-identity <IDENTITY> Identifies this server", NEED_PARAM, NULL, 'i'},
                {"debug", "-debug start the controller unforked and not locked down", NO_PARAM, &debug, 1},
                {"configuration_source", "-configuration_source <PATH> full path to the configuration source", NEED_PARAM, NULL, 'c'},
                {0, 0, (enum need_param_t)0, 0, 0}
        };

        // get options
        while (1) {
                c = argopt(argc,
                           argv,
                           options,
                           &index,
                           &parameter);

                switch (c) {
                case ARGOPT_OPTION_FOUND :
                        break;
                case ARGOPT_AMBIGIOUS_OPTION :
                        argopt_completions(stderr,
                                           "Ambigious option found. Possible completions:",
                                           ++argv[index],
                                           options);
                        break;
                case ARGOPT_UNKNOWN_OPTION :
                        fprintf(stderr, "%d - Unknown option found:\t%s\n", __LINE__, argv[index]);
                        argopt_help(stdout,
                                    "Unknown option found",
                                    argv[0],
                                    options);
                        break;
                case ARGOPT_NOT_OPTION :
                        fprintf(stderr, "%d - Bad or malformed option found:\t%s\n", __LINE__, argv[index]);
                        argopt_help(stdout,
                                    "Bad or malformed option found",
                                    argv[0],
                                    options);
                        break;
                case ARGOPT_MISSING_PARAM :
                        fprintf(stderr, "%d - Option missing parameter:\t%s\n", __LINE__, argv[index]);
                        argopt_help(stdout,
                                    "Option missing parameter",
                                    argv[0],
                                    options);
                        break;
                case ARGOPT_DONE :
                        goto opt_done;
		case 'o' :
			print_scm_origin();
			break;
		case 'v':
			print_version();
			break;
                case 'i' :
                        identity = strdup(parameter ? parameter : "");
                        break;
                case 'c' :
                        free(conf_file_path);
                        conf_file_path = strdup(parameter ? parameter : "");
                        break;
                default:
                        fprintf(stderr, "?? get_option() returned character code 0%o ??\n", c);
                }
                if (parameter)
                        free(parameter);
                parameter = NULL;
        }

opt_done:
        if ((index) && (index < argc)) {
                fprintf(stderr, "non-option ARGV-elements: ");
                while (index < argc)
                        fprintf(stderr, "%s ", argv[index++]);
                fprintf(stderr, "\n");

                return EXIT_FAILURE;
        }

        // initiate logging
        if (!init_logging((debug ? true : false), identity)) {
                fprintf(stderr, "could not initiate logging\n");
                return EXIT_FAILURE;
        }

        config = new(std::nothrow) Config(identity);
        if (conf_file_path) {
                bool ok = config->init(conf_file_path);
                free(conf_file_path);
		if (!ok) {
			M_ERROR("error initializing configuration reader");
			return EXIT_FAILURE;
		}
        }
	if (!config) {
		M_ALERT("no memory");
                return EXIT_FAILURE;
	}
        free(identity);

        M_INFO("starting server");
        int retv = start_server((debug ? true : false), config);
        M_INFO("server launched");

        return retv;
}
