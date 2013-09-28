#  
# Copyright (C) 2013 by Jules Colding <jcolding@gmail.com>.
#
# All Rights Reserved.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
#

AC_PREREQ([2.68])
AC_INIT([MercuryFIX],[0.0.1],[jcolding@gmail.com])
AC_COPYRIGHT([Copyright (C) 2013, Jules Colding. All Rights Reserved.])

# For GCC extensions
AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_SRCDIR([support_files])
AC_CONFIG_HEADERS([ac_config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([subdir-objects])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl Standard checks
AC_LANG([C++])
AC_REQUIRE_CPP

dnl Initialize libtool
LT_INIT([disable-shared])
AC_SUBST([LIBTOOL_DEPS])

# unset -g -O3 in xxxFLAGS unless the user specified otherwise
AS_IF([test "x${ac_cv_env_CFLAGS_set}" = "x"], [CFLAGS=""])
AS_IF([test "x${ac_cv_env_CXXFLAGS_set}" = "x"], [CXXFLAGS=""])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.

# This macro is defined in check.m4 and tests if check.h and
# libcheck.a are installed in your system. It sets CHECK_CFLAGS and
# CHECK_LIBS accordingly.  
#  AM_PATH_CHECK([MINIMUM-VERSION,
#                [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
#AM_PATH_CHECK()
PKG_CHECK_MODULES([CHECK], [check >= 0.9.8])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h mach/mach.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/file.h sys/param.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([fdatasync usleep fullfsync localtime_r gmtime_r dup2 malloc ftruncate memmem posix_fallocate])
AC_FUNC_STRERROR_R
AC_FUNC_FORK

#
# Check implementation details of build host
#

# Check for (4 <= sizeof(int)) - Assumed in many places
AC_MSG_CHECKING([for (4 <= sizeof(int))])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <unistd.h>
		]],
		[[
			return (4 <= sizeof(int)) ? 0 : 1;
		]]
	)],
	[
		AC_MSG_RESULT([ok])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([sizeof(int) has to be at least 4 bytes])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([sizeof(int) has to be at least 4 bytes])
	]
)

# Check for (8 >= sizeof(time_t)) and (8 >= sizeof(suseconds_t))
# Assumed when storing struct timeval in sent messages database.
AC_MSG_CHECKING([for (8 >= sizeof(time_t)) and (8 >= sizeof(suseconds_t))])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <unistd.h>
		]],
		[[
			return ((8 >= sizeof(time_t)) && (8 >= sizeof(suseconds_t))) ? 0 : 1;
		]]
	)],
	[
		AC_MSG_RESULT([ok])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([sizeof(time_t) and sizeof(suseconds_t) must both be no more than 8 bytes])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([sizeof(time_t) and sizeof(suseconds_t) must both be no more than 8 bytes])
	]
)

# Check for sysconf(_SC_PAGESIZE) - Used by disruptorC
AC_MSG_CHECKING([for sysconf(_SC_PAGESIZE)])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <unistd.h>
		]],
		[[
			return (-1 != sysconf(_SC_PAGESIZE)) ? 0 : 1;
		]]
	)],
	[
		AC_MSG_RESULT([ok])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([platform support for sysconf(_SC_PAGESIZE) is required])
	],
	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([platform support for sysconf(_SC_PAGESIZE) is required])
	]
)

# Check for cache line size - Used by disruptorC
AC_MSG_CHECKING([for cache line size])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <stddef.h>

			#if defined __APPLE__
			        #include <sys/sysctl.h>
			#elif defined __linux__
			        #include <stdio.h>
			#else
				#error unrecognized platform
			#endif
		]],
		[[
			#if defined __APPLE__
			        size_t line_size = 0;
			        size_t sizeof_line_size = sizeof(line_size);
			        if (sysctlbyname("hw.cachelinesize", &line_size, &sizeof_line_size, 0, 0))
				        return 1;
			        return 0;
			#elif defined __linux__
				FILE *p = 0;
				p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
				unsigned int i = 0;
				if (p) {
				        if (1 != fscanf(p, "%u", &i))
					        return 1;
				        fclose(p);
				} else {
                                        return 1;
				}
				return 0;
			#else
				return 1;
			#endif
		]]
	)],
	[
		AC_MSG_RESULT([ok])
	],
	[
		AC_MSG_FAILURE([could not deduce cache line size])
	],
	[
		AC_MSG_FAILURE([could not deduce cache line size])
	]
)

# Check for __atomic* builtins - Used by disruptorC
AC_MSG_CHECKING([for __atomic builtins])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[

  		]],
  		[[
			int n = 1;
			int m = 0;
			m = __atomic_fetch_add(&n, 1, __ATOMIC_SEQ_CST);
  		]]
  	)],
  	[
		AC_MSG_RESULT([yes])
		AC_DEFINE([HAVE_ATOMIC_BUILTINS], [1], [the compiler supports __atomic builtins])
	],
  	[
		AC_MSG_RESULT([no])
		AC_MSG_FAILURE([compiler support for __atomic builtins are required])
	]
)

# Check for indianess
AC_C_BIGENDIAN(
	[
		AC_DEFINE([HAVE_BIG_ENDIAN], [1], [machine is big endian])
		endianess="Big Endian"
	],
	[
		AC_DEFINE([HAVE_LITTLE_ENDIAN], [1], [machine is little endian])
		endianess="Little Endian"
	],
	[
		AC_MSG_ERROR([unknown endianness])
	]
)

# Check for strongly typed enums
AC_MSG_CHECKING([for strongly typed enums])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[

  		]],
  		[[
			enum enumerated : unsigned char { a, b, c };
  		]]
  	)],
  	[
		AC_MSG_RESULT([yes])
		AC_DEFINE([HAVE_STRONGLY_TYPED_ENUMS], [1], [the compiler supports strongly typed enums])
	],
  	[
		AC_MSG_RESULT([no])
	]
)

# Check for msghdr member msg_control and friends
AC_MSG_CHECKING([for network feature: msg_control])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <sys/socket.h>
  		]],
  		[[
			struct msghdr m; m.msg_controllen = 0;
  		]]
  	)],
  	[
		have_msg_control=yes
	],
  	[
		have_msg_control=no
	]
)

if test "x$have_msg_control" = "xyes"; then
  AC_MSG_RESULT([yes])
  AC_DEFINE([HAVE_MSGHDR_MSG_CONTROL], [1], [Define if msghdr has a msg_control member])

  AC_MSG_CHECKING([for network feature: CMSG_DATA])
  AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <sys/types.h>
        		#include <sys/socket.h>
      		]],
      		[[
			struct cmsghdr cm; unsigned char *p = CMSG_DATA(&cm);
      		]]
    	)],
    	[
		AC_MSG_RESULT([yes])
      		AC_DEFINE([HAVE_CMSG_DATA], [1], [Define if CMSG_DATA is available])
    	],
    	[
		AC_MSG_RESULT([no])
    	]
  )

  AC_MSG_CHECKING([for network feature: CMSG_FIRSTHDR])
  AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <sys/types.h>
        		#include <sys/socket.h>
      		]],
      		[[
			struct msghdr m; struct cmsghdr *p = CMSG_FIRSTHDR(&m);
      		]]
    	)],
    	[
		AC_MSG_RESULT([yes])
      		AC_DEFINE([HAVE_CMSG_FIRSTHDR], [1], [Define if CMSG_FIRSTHDR is available])
    	],
    	[
		AC_MSG_RESULT([no])
    	]
  )

  AC_MSG_CHECKING([for network feature: CMSG_LEN])
  AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <sys/types.h>
        		#include <sys/socket.h>
      		]],
      		[[
			int i = CMSG_LEN(0);
		]]
    	)],
    	[
		AC_MSG_RESULT([yes])
      		AC_DEFINE([HAVE_CMSG_LEN], [1], [Define if CMSG_LEN is available])
    	],
    	[
		AC_MSG_RESULT([no])
    	]
  )

  AC_MSG_CHECKING([for network feature: CMSG_SPACE])
  AC_LINK_IFELSE(
	[AC_LANG_PROGRAM(
		[[
			#include <sys/types.h>
        		#include <sys/socket.h>
      		]],
      		[[
			int i = CMSG_SPACE(0);
		]]
    	)],
    	[
		AC_MSG_RESULT([yes])
      		AC_DEFINE([HAVE_CMSG_SPACE], [1], [Define if CMSG_SPACE is available])
		AC_MSG_CHECKING([implementation value: CMSG_SPACE(sizeof(int))])
		AC_RUN_IFELSE(
			[AC_LANG_PROGRAM(
				[[
					#include <sys/types.h>
					#include <sys/socket.h>
				]],
  				[[
					return CMSG_SPACE(sizeof(int));
  				]]
			)],
  			[
				cmsg_space_sizeof_int=-1
			],
  			[
				cmsg_space_sizeof_int=$?
			],
  			[
				AC_MSG_FAILURE([cross-compilation not supported])
			]
		)
		AC_DEFINE_UNQUOTED([CMSG_SPACE_SIZEOF_INT], [($cmsg_space_sizeof_int)], [Define to value of CMSG_SPACE(sizeof(int))])
		AC_MSG_RESULT($cmsg_space_sizeof_int)
    	],
    	[
		AC_MSG_RESULT([no])
		AC_DEFINE_UNQUOTED([CMSG_SPACE_SIZEOF_INT], [(-1)], [Define to value of CMSG_SPACE(sizeof(int))])
    	]
  )
else
	AC_MSG_RESULT([no])
fi

dnl
dnl MERCURY versions, names and default configuration values
dnl
MERCURY_NAME=AC_PACKAGE_NAME
AC_SUBST(MERCURY_NAME)

MERCURY_VERSION=AC_PACKAGE_VERSION
AC_SUBST(MERCURY_VERSION)

dnl Mercury top-level source directory
MERCURY_top_dir="`pwd`"
AC_SUBST(MERCURY_top_dir)

dnl Mercury configuration defaults
AC_DEFINE([MERCURY_DEFAULT_PID_FILE], ["/var/run/mercury.pid"], [Default Mecury PID file])

dnl SCM repository state file
MERCURY_SCM_STATE_FILE="scm_state_snapshot"
AC_SUBST(MERCURY_SCM_STATE_FILE)

target=`uname`
case "x$target" in
	xDarwin)
	AM_CONDITIONAL(OS_DARWIN, true)
	AM_CONDITIONAL(OS_LINUX, false)
	;;
	xLinux)
	AM_CONDITIONAL(OS_DARWIN, false)
	AM_CONDITIONAL(OS_LINUX, true)
	;;
	*)
	AC_MSG_ERROR([Mercury is not supported on this operating system])
	;;
esac

dnl
dnl Configure options
dnl

AC_ARG_ENABLE(debug,
	      [AS_HELP_STRING([--enable-debug[[[[=no/yes]]]]], [Enable Mercury debugging [default=yes]])],
	      [],
	      enable_debug=yes)
if test "x$enable_debug" = "xyes"; then
	AC_DEFINE([DEBUG], [1], [Define if you want to make a debug enabled build])
	msg_debug=yes
else
	msg_debug=no
fi
AM_CONDITIONAL(MERCURY_DEBUG, test "x$enable_debug" = "xyes")


dnl
dnl Program availability checks
dnl


dnl
dnl CPPFLAGS, CXXFLAGS, LIBS and stuff
dnl

dnl Check for pthread support
AX_PTHREAD(
	[
		AX_NOOP()
	],
	[
		AC_MSG_ERROR([pthreads are required])
	]
)

dnl
dnl LIBS
dnl
LIBS="$PTHREAD_LIBS $LIBS"
AC_SUBST(LIBS)

dnl
dnl Check if the compiler supports -iquote DIR (from ltrace)
dnl
ac_cv_have_iquote=no
AC_MSG_CHECKING(whether compiler accepts -iquote dir)
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS -iquote ."
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[ac_cv_have_iquote=yes],[])
CPPFLAGS="$save_CPPFLAGS"
AC_MSG_RESULT($ac_cv_have_iquote)
if test $ac_cv_have_iquote = yes; then
  iquote="-iquote "
else
  iquote="-I"
fi

dnl Should only be searchable in "" include statements
dnl MERCURY_INCLUDES="$iquote$MERCURY_top_dir $iquote$MERCURY_top_dir/utillib"
MERCURY_INCLUDES="$iquote$MERCURY_top_dir"

dnl Now set the preprocessor arguments
MERCURY_CPPFLAGS="$MERCURY_INCLUDES"
AC_SUBST(MERCURY_CPPFLAGS)

dnl
dnl CXXFLAGS
dnl
MERCURY_CXXFLAGS="$PTHREAD_CFLAGS"

dnl Workaround for https://bugzilla.redhat.com/show_bug.cgi?id=218377
CXX_VERSION=`$CXX -dumpversion`
if test "x$CXX_VERSION" = "x3.4.6"; then
   AX_CXXFLAGS_GCC_OPTION(-fno-var-tracking, MERCURY_CXXFLAGS)
fi

AX_CXXFLAGS_GCC_OPTION(-Wall, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Wextra, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Werror, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Wundef, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Wpointer-arith, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Wimplicit, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Wbad-function-cast, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-Wcast-align, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-pipe, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-fprefetch-loop-arrays, MERCURY_CXXFLAGS)
AX_CXXFLAGS_GCC_OPTION(-std=gnu++11, MERCURY_CXXFLAGS)

if test "x$enable_debug" = "xyes"; then
	AX_CXXFLAGS_GCC_OPTION(-ggdb , MERCURY_CXXFLAGS)
	AX_CXXFLAGS_GCC_OPTION(-Og, MERCURY_CXXFLAGS)
else
	AX_CXXFLAGS_GCC_OPTION(-g , MERCURY_CXXFLAGS)
	AX_CXXFLAGS_GCC_OPTION(-O3, MERCURY_CXXFLAGS)
fi
if test "x$target" = "xLinux"; then
	MERCURY_CXXFLAGS="-std=gnu++0x $MERCURY_CXXFLAGS -D__STDC_LIMIT_MACROS"
fi
AC_SUBST(MERCURY_CXXFLAGS)

dnl
dnl LDFLAGS
dnl
if test "x$target" = "xDarwin"; then
	LDFLAGS="-flat_namespace $LDFLAGS"
fi
LDFLAGS="-pthread $LDFLAGS"
LDFLAGS="-L$MERCURY_top_dir/ $LDFLAGS"
AC_SUBST(LDFLAGS)

dnl
dnl Check if this is a distribution
dnl
AM_CONDITIONAL(THIS_IS_NOT_A_DISTRIBUTION, [test -f ./nodist])


dnl ******************************
dnl Makefiles
dnl ******************************

AC_CONFIG_FILES([Makefile
                 stdlib/Makefile
                 stdlib/cmdline/Makefile
                 stdlib/config/Makefile
                 stdlib/disruptor/Makefile
                 stdlib/locks/Makefile
                 stdlib/log/Makefile
                 stdlib/marshal/Makefile
                 stdlib/network/Makefile
                 stdlib/process/Makefile
                 stdlib/rule_engine/Makefile
                 stdlib/scm_state/Makefile
		 stdlib/local_db/Makefile
                 utillib/Makefile
                 utillib/ipc/Makefile
                 utillib/config/Makefile
                 omslib/Makefile
    		 applib/Makefile
		 applib/fixutils/Makefile
                 applib/fixmsg/Makefile
                 applib/fixmsg_tests/Makefile
                 applib/fixio/Makefile
                 applib/fixio_tests/Makefile
 		 servers/Makefile
		 servers/generic/Makefile
		 servers/versatile/Makefile
	 ])
AC_OUTPUT

echo "\

	Target Platform:           $target
	Endianess:                 $endianess
	Making a debug build:      $msg_debug
        Mercury version:           $PACKAGE_VERSION
	MERCURY_CXXFLAGS:          $MERCURY_CXXFLAGS
	MERCURY_CPPFLAGS:          $MERCURY_CPPFLAGS
	Mercury LIBS:              $LIBS
"
