/*****************************************************************************

 @(#) File: src/test/test-etsi_t.c

 -----------------------------------------------------------------------------

 Copyright (c) 2008-2015  Monavacon Limited <http://www.monavacon.com/>
 Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>
 Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>

 All Rights Reserved.

 Unauthorized distribution or duplication is prohibited.

 This software and related documentation is protected by copyright and
 distributed under licenses restricting its use, copying, distribution and
 decompilation.  No part of this software or related documentation may be
 reproduced in any form by any means without the prior written authorization
 of the copyright holder, and licensors, if any.

 The recipient of this document, by its retention and use, warrants that the
 recipient will protect this information and keep it confidential, and will
 not disclose the information contained in this document without the written
 permission of its owner.

 The author reserves the right to revise this software and documentation for
 any reason, including but not limited to, conformity with standards
 promulgated by various agencies, utilization of advances in the state of the
 technical arts, or the reflection of changes in the design of any techniques,
 or procedures embodied, described, or referred to herein.  The author is
 under no obligation to provide any feature listed herein.

 -----------------------------------------------------------------------------

 As an exception to the above, this software may be distributed under the GNU
 Affero General Public License (AGPL) Version 3, so long as the software is
 distributed with, and only used for the testing of, OpenSS7 modules, drivers,
 and libraries.

 -----------------------------------------------------------------------------

 U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on
 behalf of the U.S. Government ("Government"), the following provisions apply
 to you.  If the Software is supplied by the Department of Defense ("DoD"), it
 is classified as "Commercial Computer Software" under paragraph 252.227-7014
 of the DoD Supplement to the Federal Acquisition Regulations ("DFARS") (or any
 successor regulations) and the Government is acquiring only the license rights
 granted herein (the license rights customarily provided to non-Government
 users).  If the Software is supplied to any unit or agency of the Government
 other than DoD, it is classified as "Restricted Computer Software" and the
 Government's rights in the Software are defined in paragraph 52.227-19 of the
 Federal Acquisition Regulations ("FAR") (or any successor regulations) or, in
 the cases of NASA, in paragraph 18.52.227-86 of the NASA Supplement to the FAR
 (or any successor regulations).

 -----------------------------------------------------------------------------

 Commercial licensing and support of this software is available from OpenSS7
 Corporation at a fee.  See http://www.openss7.com/

 *****************************************************************************/

static char const ident[] = "src/test/test-etsi_t.c (" PACKAGE_ENVR ") " PACKAGE_DATE;

/*
 *  This file is for testing the sctp_t module.  It is provided for the
 *  purpose of testing the OpenSS7 sctp_t module only.
 *
 *  This file contains test cases for the ETSI TS 102 369 test purposes.  It
 *  skips the ATS and goes directly to implemented test cases.
 *
 *  This test suite uses Ferry Clip testing, it opens a pipe and pushes SCTP
 *  as the Implementation Under Test as a module over one end of the pipe and
 *  uses the other end of the pipe as a Protocol Tester.  Another approach is
 *  to send IP packets to the IUT as a driver, but that is another program.
 *
 *  This test program exchenages NPI primitives with one side of the open pipe
 *  and excehage TPI primities on the other side of the pipe.  The side of the
 *  pipe with the "sctp_t" STREAMS module pushed represents the Upper Layer
 *  Program user of the SCTP module.  The other side of the pipe is the test
 *  harness, as shown below:
 *
 *                               USER SPACE
 *                              TEST PROGRAM
 *  _________________________________________________________________________
 *               \   /  |                            \   /  |
 *                \ /   |                             \ /   |
 *                 |   / \                             |    |
 *     ____________|__/___\__________                  |    |
 *    |                              |                 |    |
 *    |                              |                 |    |
 *    |             SCTP             |                 |    |
 *    |                              |                 |    |
 *    |        STREAMS MODULE        |                 |    |
 *    |                              |                 |    |
 *    |                              |                 |    |
 *    |______________________________|                 |    |
 *               \   /  |                              |    |
 *                \ /   |                              |    |
 *                 |    |                              |    |
 *                 |   / \                             |    |
 *     ____________|__/___\____________________________|____|_____________
 *    |                                                                   |
 *    |                                                                   |
 *    |                               PIPE                                |
 *    |                                                                   |
 *    |___________________________________________________________________|
 *
 *
 *  This test arrangement results in a ferry-clip around the SCTP module, where
 *  NPI primitives are injected and removed beneath the module as well as TPI
 *  primitives being performed above the module.
 *
 *  To preserve the environment and ensure that the tests are repeatable in any
 *  order, the entire test harness (pipe) is assembled and disassembled for each
 *  test.  A test preamble is used to place the module in the correct state for
 *  a test case to begin and then a postamble is used to ensure that the module
 *  can be removed correctly.
 */

#include <sys/types.h>
#include <stropts.h>
#include <stdlib.h>

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#else
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/uio.h>
#include <time.h>

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef _GNU_SOURCE
#include <getopt.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define NEED_T_USCALAR_T

#include <xti.h>
#include <tihdr.h>
#include <timod.h>
#include <xti_inet.h>
#include <sys/xti_sctp.h>

#if 1
#define SCTP_VERSION_2 1

#ifndef SCTP_VERSION_2
#   ifndef sctp_addr_t
typedef struct sctp_addr {
	uint16_t port __attribute__ ((packed));
	uint32_t addr[0] __attribute__ ((packed));
} sctp_addr_t;

#	define sctp_addr_t sctp_addr_t
#   endif			/* sctp_addr_t */
#endif				/* SCTP_VERSION_2 */
#endif

/*
 *  -------------------------------------------------------------------------
 *
 *  Configuration
 *
 *  -------------------------------------------------------------------------
 */

static const char *lpkgname = "OpenSS7 SCTP Driver";

/* static const char *spkgname = "SCTP"; */
static const char *lstdname = "RFC 2960, SCTP-IG, ETSI TS 102 144";
static const char *sstdname = "RFC2960/TS102144";
static const char *shortname = "SCTP";

static char devname[256] = "/dev/streams/clone/sctp_t";
static char modname[256] = "sctp_t";

static const int test_level = T_INET_SCTP;

static int repeat_verbose = 0;
static int repeat_on_success = 0;
static int repeat_on_failure = 0;
static int exit_on_failure = 0;

static int client_port_specified = 0;
static int server_port_specified = 0;
static int client_host_specified = 0;
static int server_host_specified = 0;

static int verbose = 1;

static int client_exec = 0;		/* execute client side */
static int server_exec = 0;		/* execute server side */

static int show_msg = 0;
static int show_acks = 0;
static int show_timeout = 0;

//static int show_data = 1;

static int last_prim = 0;
static int last_event = 0;
static int last_errno = 0;
static int last_retval = 0;
static int last_t_errno = 0;
static int last_qlen = 2;
static int last_sequence = 1;
static int last_servtype = T_COTS_ORD;
static int last_provflag = T_SENDZERO | T_ORDRELDATA | T_XPG4_1;
static int last_tstate = TS_UNBND;
struct T_info_ack last_info = { 0, };
static int last_prio = 0;

static int MORE_flag = 0;
static int DATA_flag = T_ODF_EX | T_ODF_MORE;

#define NUM_STREAMS 3
int test_fd[NUM_STREAMS] = { 0, 0, 0 };

#define BUFSIZE 32*4096

#define SHORT_WAIT	  20	// 100 // 10
#define NORMAL_WAIT	 100	// 500 // 100
#define LONG_WAIT	 500	// 5000 // 500
#define LONGER_WAIT	1000	// 10000 // 5000
#define LONGEST_WAIT	5000	// 20000 // 10000
#define TEST_DURATION	20000
#define INFINITE_WAIT	-1

static ulong test_duration = TEST_DURATION;	/* wait on other side */

ulong seq[10] = { 0, };
ulong tok[10] = { 0, };
ulong tsn[10] = { 0, };
ulong sid[10] = { 0, };
ulong ssn[10] = { 0, };
ulong ppi[10] = { 0, };
ulong exc[10] = { 0, };

char cbuf[BUFSIZE];
char dbuf[BUFSIZE];

struct strbuf ctrl = { BUFSIZE, -1, cbuf };
struct strbuf data = { BUFSIZE, -1, dbuf };

static int test_pflags = MSG_BAND;	/* MSG_BAND | MSG_HIPRI */
static int test_pband = 0;
static int test_gflags = 0;		/* MSG_BAND | MSG_HIPRI */
static int test_gband = 0;
static int test_timout = 200;

static int test_bufsize = 256;
static int test_tidu = 256;
static int test_mgmtflags = T_NEGOTIATE;
static struct sockaddr_in *test_addr = NULL;
static socklen_t test_alen = sizeof(*test_addr);
static const char *test_data = NULL;
static int test_dlen = 0;
static int test_resfd = -1;
static void *test_opts = NULL;
static int test_olen = 0;
static int test_prio = 1;

struct strfdinsert fdi = {
	{BUFSIZE, 0, cbuf},
	{BUFSIZE, 0, dbuf},
	0,
	0,
	0
};
int flags = 0;

int dummy = 0;

#if 1
#ifndef SCTP_VERSION_2
typedef struct addr {
	uint16_t port __attribute__ ((packed));
	struct in_addr addr[3] __attribute__ ((packed));
} addr_t;
#endif				/* SCTP_VERSION_2 */
#endif

struct timeval when;

/*
 *  -------------------------------------------------------------------------
 *
 *  Events and Actions
 *
 *  -------------------------------------------------------------------------
 */
enum {
	__EVENT_EOF = -7, __EVENT_NO_MSG = -6, __EVENT_TIMEOUT = -5, __EVENT_UNKNOWN = -4,
	__RESULT_DECODE_ERROR = -3, __RESULT_SCRIPT_ERROR = -2,
	__RESULT_INCONCLUSIVE = -1, __RESULT_SUCCESS = 0, __RESULT_FAILURE = 1,
	__RESULT_NOTAPPL = 3, __RESULT_SKIPPED = 77,
};

/*
 *  -------------------------------------------------------------------------
 */

int show = 1;

enum {
	__TEST_CONN_REQ = 100, __TEST_CONN_RES, __TEST_DISCON_REQ,
	__TEST_DATA_REQ, __TEST_EXDATA_REQ, __TEST_INFO_REQ, __TEST_BIND_REQ,
	__TEST_UNBIND_REQ, __TEST_UNITDATA_REQ, __TEST_OPTMGMT_REQ,
	__TEST_ORDREL_REQ, __TEST_OPTDATA_REQ, __TEST_ADDR_REQ,
	__TEST_CAPABILITY_REQ, __TEST_CONN_IND, __TEST_CONN_CON,
	__TEST_DISCON_IND, __TEST_DATA_IND, __TEST_EXDATA_IND,
	__TEST_INFO_ACK, __TEST_BIND_ACK, __TEST_ERROR_ACK, __TEST_OK_ACK,
	__TEST_UNITDATA_IND, __TEST_UDERROR_IND, __TEST_OPTMGMT_ACK,
	__TEST_ORDREL_IND, __TEST_NRM_OPTDATA_IND, __TEST_EXP_OPTDATA_IND,
	__TEST_ADDR_ACK, __TEST_CAPABILITY_ACK, __TEST_WRITE, __TEST_WRITEV,
	__TEST_PUTMSG_DATA, __TEST_PUTPMSG_DATA, __TEST_PUSH, __TEST_POP,
	__TEST_READ, __TEST_READV, __TEST_GETMSG, __TEST_GETPMSG,
	__TEST_DATA,
	__TEST_DATACK_REQ, __TEST_DATACK_IND, __TEST_RESET_REQ,
	__TEST_RESET_IND, __TEST_RESET_RES, __TEST_RESET_CON,
	__TEST_O_TI_GETINFO, __TEST_O_TI_OPTMGMT, __TEST_O_TI_BIND,
	__TEST_O_TI_UNBIND,
	__TEST__O_TI_GETINFO, __TEST__O_TI_OPTMGMT, __TEST__O_TI_BIND,
	__TEST__O_TI_UNBIND, __TEST__O_TI_GETMYNAME, __TEST__O_TI_GETPEERNAME,
	__TEST__O_TI_XTI_HELLO, __TEST__O_TI_XTI_GET_STATE,
	__TEST__O_TI_XTI_CLEAR_EVENT, __TEST__O_TI_XTI_MODE,
	__TEST__O_TI_TLI_MODE,
	__TEST_TI_GETINFO, __TEST_TI_OPTMGMT, __TEST_TI_BIND,
	__TEST_TI_UNBIND, __TEST_TI_GETMYNAME, __TEST_TI_GETPEERNAME,
	__TEST_TI_SETMYNAME, __TEST_TI_SETPEERNAME, __TEST_TI_SYNC,
	__TEST_TI_GETADDRS, __TEST_TI_CAPABILITY,
	__TEST_TI_SETMYNAME_DATA, __TEST_TI_SETPEERNAME_DATA,
	__TEST_TI_SETMYNAME_DISC, __TEST_TI_SETPEERNAME_DISC,
	__TEST_TI_SETMYNAME_DISC_DATA, __TEST_TI_SETPEERNAME_DISC_DATA,
	__TEST_PRIM_TOO_SHORT, __TEST_PRIM_WAY_TOO_SHORT,
	__TEST_O_NONBLOCK, __TEST_O_BLOCK,
	__TEST_T_ACCEPT, __TEST_T_BIND, __TEST_T_CLOSE, __TEST_T_CONNECT,
	__TEST_T_GETINFO, __TEST_T_GETPROTADDR, __TEST_T_GETSTATE,
	__TEST_T_LISTEN, __TEST_T_LOOK, __TEST_T_OPTMGMT, __TEST_T_RCV,
	__TEST_T_RCVCONNECT, __TEST_T_RCVDIS, __TEST_T_RCVREL,
	__TEST_T_RCVRELDATA, __TEST_T_RCVUDATA, __TEST_T_RCVUDERR,
	__TEST_T_RCVV, __TEST_T_RCVVUDATA, __TEST_T_SND, __TEST_T_SNDDIS,
	__TEST_T_SNDREL, __TEST_T_SNDRELDATA, __TEST_T_SNDUDATA,
	__TEST_T_SNDV, __TEST_T_SNDVUDATA, __TEST_T_SYNC, __TEST_T_UNBIND,
	__TEST_BUNDLE,
	__TEST_ABORT,
	__TEST_INIT,
	__TEST_INIT_ACK,
	__TEST_COOKIE_ECHO,
	__TEST_COOKIE_ACK,
	__TEST_DATA_CHUNK,
	__TEST_SACK,
	__TEST_ERROR,
	__TEST_SHUTDOWN,
	__TEST_SHUTDOWN_ACK,
	__TEST_SHUTDOWN_COMPLETE,
};

/*
 *  -------------------------------------------------------------------------
 *
 *  Timer Functions
 *
 *  -------------------------------------------------------------------------
 */

/*
 *  Timer values for tests: each timer has a low range (minus error margin)
 *  and a high range (plus error margin).
 */

static long timer_scale = 1;

#define TEST_TIMEOUT 5000

typedef struct timer_range {
	long lo;
	long hi;
} timer_range_t;

enum {
	t1 = 0, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15,
	t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29,
	t30, t31, t32, t33, t34, t35, t36, t37, t38, tmax
};

long test_start = 0;

static int state = 0;
static const char *failure_string = NULL;

#define __stringify_1(x) #x
#define __stringify(x) __stringify_1(x)
#define FAILURE_STRING(string) "[" __stringify(__LINE__) "] " string

#if 1
#undef lockf
#define lockf(x,y,z) 0
#endif

#if 0
/*
 *  Return the current time in milliseconds.
 */
static long
dual_milliseconds(int child, int t1, int t2)
{
	long ret;
	struct timeval now;
	static const char *msgs[] = {
		"             %1$-6.6s !      %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld     :                    [%7$d:%8$03d]\n",
		"                    :      %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld     ! %1$-6.6s             [%7$d:%8$03d]\n",
		"                    :      %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld  !  : %1$-6.6s             [%7$d:%8$03d]\n",
		"                    !  %1$-6.6s %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld  !                    [%7$d:%8$03d]\n",
	};
	static const char *blank[] = {
		"                    !                                   :                    \n",
		"                    :                                   !                    \n",
		"                    :                                !  :                    \n",
		"                    !                                   !                    \n",
	};
	static const char *plus[] = {
		"               +    !                                   :                    \n",
		"                    :                                   !    +               \n",
		"                    :                                !  :    +               \n",
		"                    !      +                            !                    \n",
	};

	gettimeofday(&now, NULL);
	if (!test_start)	/* avoid blowing over precision */
		test_start = now.tv_sec;
	ret = (now.tv_sec - test_start) * 1000;
	ret += (now.tv_usec + 500) / 1000;

	if (show && verbose > 0) {
		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, blank[child]);
		fprintf(stdout, msgs[child], timer[t1].name, timer[t1].lo / 1000, timer[t1].lo - ((timer[t1].lo / 1000) * 1000), timer[t1].name, timer[t1].hi / 1000, timer[t1].hi - ((timer[t1].hi / 1000) * 1000), child, state);
		fprintf(stdout, plus[child]);
		fprintf(stdout, msgs[child], timer[t2].name, timer[t2].lo / 1000, timer[t2].lo - ((timer[t2].lo / 1000) * 1000), timer[t2].name, timer[t2].hi / 1000, timer[t2].hi - ((timer[t2].hi / 1000) * 1000), child, state);
		fprintf(stdout, blank[child]);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}

	return ret;
}

/*
 *  Return the current time in milliseconds.
 */
static long
milliseconds(int child, int t)
{
	long ret;
	struct timeval now;
	static const char *msgs[] = {
		"             %1$-6.6s !      %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld     :                    [%7$d:%8$03d]\n",
		"                    :      %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld     ! %1$-6.6s             [%7$d:%8$03d]\n",
		"                    :      %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld  !  : %1$-6.6s             [%7$d:%8$03d]\n",
		"                    !  %1$-6.6s %2$3ld.%3$03ld <= %4$-2.2s <= %5$3ld.%6$03ld  !                    [%7$d:%8$03d]\n",
	};
	static const char *blank[] = {
		"                    !                                   :                    \n",
		"                    :                                   !                    \n",
		"                    :                                !  :                    \n",
		"                    !                                   !                    \n",
	};

	gettimeofday(&now, NULL);
	if (!test_start)	/* avoid blowing over precision */
		test_start = now.tv_sec;
	ret = (now.tv_sec - test_start) * 1000;
	ret += (now.tv_usec + 500) / 1000;

	if (show && verbose > 0) {
		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, blank[child]);
		fprintf(stdout, msgs[child], timer[t].name, timer[t].lo / 1000, timer[t].lo - ((timer[t].lo / 1000) * 1000), timer[t].name, timer[t].hi / 1000, timer[t].hi - ((timer[t].hi / 1000) * 1000), child, state);
		fprintf(stdout, blank[child]);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}

	return ret;
}

/*
 *  Check the current time against the beginning time provided as an argnument
 *  and see if the time inverval falls between the low and high values for the
 *  timer as specified by arguments.  Return SUCCESS if the interval is within
 *  the allowable range and FAILURE otherwise.
 */
static int
check_time(int child, const char *t, long beg, long lo, long hi)
{
	long i;
	struct timeval now;
	static const char *msgs[] = {
		"       check %1$-6.6s ? [%2$3ld.%3$03ld <= %4$3ld.%5$03ld <= %6$3ld.%7$03ld]   |                    [%8$d:%9$03d]\n",
		"                    | [%2$3ld.%3$03ld <= %4$3ld.%5$03ld <= %6$3ld.%7$03ld]   ? %1$-6.6s check       [%8$d:%9$03d]\n",
		"                    | [%2$3ld.%3$03ld <= %4$3ld.%5$03ld <= %6$3ld.%7$03ld]?  | %1$-6.6s check       [%8$d:%9$03d]\n",
		"       check %1$-6.6s ? [%2$3ld.%3$03ld <= %4$3ld.%5$03ld <= %6$3ld.%7$03ld]   ?                    [%8$d:%9$03d]\n",
	};

	if (gettimeofday(&now, NULL)) {
		printf("****ERROR: gettimeofday\n");
		printf("           %s: %s\n", __FUNCTION__, strerror(errno));
		fflush(stdout);
		return __RESULT_FAILURE;
	}

	i = (now.tv_sec - test_start) * 1000;
	i += (now.tv_usec + 500) / 1000;
	i -= beg;

	if (show && verbose > 0) {
		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, msgs[child], t, (lo - 100) / 1000, (lo - 100) - (((lo - 100) / 1000) * 1000), i / 1000, i - ((i / 1000) * 1000), (hi + 100) / 1000, (hi + 100) - (((hi + 100) / 1000) * 1000), child, state);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}

	if (lo - 100 <= i && i <= hi + 100)
		return __RESULT_SUCCESS;
	else
		return __RESULT_FAILURE;
}
#endif

static int
time_event(int child, int event)
{
	static const char *msgs[] = {
		"                    ! %11.6g                       |                    <%d:%03d>\n",
		"                    |                       %11.6g !                    <%d:%03d>\n",
		"                    |                    %11.6g !  |                    <%d:%03d>\n",
		"                    !            %11.6g            !                    <%d:%03d>\n",
	};

	if ((verbose > 4 && show) || (verbose > 5 && show_msg)) {
		float t, m;
		struct timeval now;

		gettimeofday(&now, NULL);
		if (!test_start)
			test_start = now.tv_sec;
		t = (now.tv_sec - test_start);
		m = now.tv_usec;
		m = m / 1000000;
		t += m;

		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, msgs[child], t, child, state);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}
	return (event);
}

static int timer_timeout = 0;
static int last_signum = 0;

static void
signal_handler(int signum)
{
	last_signum = signum;
	if (signum == SIGALRM)
		timer_timeout = 1;
	return;
}

static int
start_signals(void)
{
	sigset_t mask;
	struct sigaction act;

	act.sa_handler = signal_handler;
//      act.sa_flags = SA_RESTART | SA_ONESHOT;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGALRM, &act, NULL))
		return __RESULT_FAILURE;
	if (sigaction(SIGPOLL, &act, NULL))
		return __RESULT_FAILURE;
	if (sigaction(SIGURG, &act, NULL))
		return __RESULT_FAILURE;
	if (sigaction(SIGPIPE, &act, NULL))
		return __RESULT_FAILURE;
	if (sigaction(SIGHUP, &act, NULL))
		return __RESULT_FAILURE;
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigaddset(&mask, SIGPOLL);
	sigaddset(&mask, SIGURG);
	sigaddset(&mask, SIGPIPE);
	sigaddset(&mask, SIGHUP);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	siginterrupt(SIGALRM, 1);
	siginterrupt(SIGPOLL, 1);
	siginterrupt(SIGURG, 1);
	siginterrupt(SIGPIPE, 1);
	siginterrupt(SIGHUP, 1);
	return __RESULT_SUCCESS;
}

/*
 *  Start an interval timer as the overall test timer.
 */
static int
start_tt(long duration)
{
	struct itimerval setting = {
		{0, 0},
		{duration / 1000, (duration % 1000) * 1000}
	};

	if (duration == (long) INFINITE_WAIT)
		return __RESULT_SUCCESS;
	if (start_signals())
		return __RESULT_FAILURE;
	if (setitimer(ITIMER_REAL, &setting, NULL))
		return __RESULT_FAILURE;
	timer_timeout = 0;
	return __RESULT_SUCCESS;
}

#if 0
static int
start_st(long duration)
{
	long sdur = (duration + timer_scale - 1) / timer_scale;

	return start_tt(sdur);
}
#endif

static int
stop_signals(void)
{
	int result = __RESULT_SUCCESS;
	sigset_t mask;
	struct sigaction act;

	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGALRM, &act, NULL))
		result = __RESULT_FAILURE;
	if (sigaction(SIGPOLL, &act, NULL))
		result = __RESULT_FAILURE;
	if (sigaction(SIGURG, &act, NULL))
		result = __RESULT_FAILURE;
	if (sigaction(SIGPIPE, &act, NULL))
		result = __RESULT_FAILURE;
	if (sigaction(SIGHUP, &act, NULL))
		result = __RESULT_FAILURE;
	sigemptyset(&mask);
	sigaddset(&mask, SIGALRM);
	sigaddset(&mask, SIGPOLL);
	sigaddset(&mask, SIGURG);
	sigaddset(&mask, SIGPIPE);
	sigaddset(&mask, SIGHUP);
	sigprocmask(SIG_BLOCK, &mask, NULL);
	return (result);
}

static int
stop_tt(void)
{
	struct itimerval setting = { {0, 0}, {0, 0} };
	int result = __RESULT_SUCCESS;

	if (setitimer(ITIMER_REAL, &setting, NULL))
		return __RESULT_FAILURE;
	if (stop_signals() != __RESULT_SUCCESS)
		result = __RESULT_FAILURE;
	timer_timeout = 0;
	return (result);
}

/*
 *  Addresses
 */
#if 1
#ifndef SCTP_VERSION_2
addr_t addrs[4];
#else				/* SCTP_VERSION_2 */
struct sockaddr_in addrs[4][3];
#endif				/* SCTP_VERSION_2 */
#else
struct sockaddr_in addrs[4];
#endif
int anums[4] = { 3, 3, 3, 3 };

#define TEST_PORT_NUMBER 18000
unsigned short ports[4] = { TEST_PORT_NUMBER + 0, TEST_PORT_NUMBER + 1, TEST_PORT_NUMBER + 2, TEST_PORT_NUMBER + 3 };
const char *addr_strings[4] = { "127.0.0.1", "127.0.0.2", "127.0.0.3", "127.0.0.4" };

/*
 *  Options
 */

/*
 * data options
 */
struct {
#if 1
	struct t_opthdr tos_hdr;
	union {
		unsigned char opt_val;
		t_scalar_t opt_fil;
	} tos_val;
	struct t_opthdr ttl_hdr;
	union {
		unsigned char opt_val;
		t_scalar_t opt_fil;
	} ttl_val;
#endif
	struct t_opthdr drt_hdr;
	t_scalar_t drt_val;
#if 0
	struct t_opthdr csm_hdr;
	t_scalar_t csm_val;
#endif
#if 1
	struct t_opthdr ppi_hdr;
	t_uscalar_t ppi_val;
	struct t_opthdr sid_hdr;
	t_scalar_t sid_val;
#endif
} opt_data = {
#if 1
	{
	sizeof(struct t_opthdr) + sizeof(unsigned char), T_INET_IP, T_IP_TOS, T_SUCCESS}, {
	.opt_val = 0x0}
	, {
	sizeof(struct t_opthdr) + sizeof(unsigned char), T_INET_IP, T_IP_TTL, T_SUCCESS}, {
	.opt_val = 64},
#endif
	{
	sizeof(struct t_opthdr) + sizeof(unsigned int), T_INET_IP, T_IP_DONTROUTE, T_SUCCESS}, T_NO
#if 0
	    , {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), T_INET_UDP, T_UDP_CHECKSUM, T_SUCCESS}
	, T_NO
#endif
#if 1
	    , {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), T_INET_SCTP, T_SCTP_PPI, T_SUCCESS}
	, 50, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_SID, T_SUCCESS}
	, 0
#endif
};

/*
 * connect options
 */
struct {
	struct t_opthdr dbg_hdr;
	t_uscalar_t dbg_val;
	struct t_opthdr lin_hdr;
	struct t_linger lin_val;
	struct t_opthdr rbf_hdr;
	t_uscalar_t rbf_val;
	struct t_opthdr rlw_hdr;
	t_uscalar_t rlw_val;
	struct t_opthdr sbf_hdr;
	t_uscalar_t sbf_val;
	struct t_opthdr slw_hdr;
	t_uscalar_t slw_val;
	struct t_opthdr tos_hdr;
	union {
		unsigned char opt_val;
		t_scalar_t opt_fil;
	} tos_val;
	struct t_opthdr ttl_hdr;
	union {
		unsigned char opt_val;
		t_scalar_t opt_fil;
	} ttl_val;
	struct t_opthdr drt_hdr;
	t_scalar_t drt_val;
	struct t_opthdr bca_hdr;
	t_scalar_t bca_val;
	struct t_opthdr reu_hdr;
	t_scalar_t reu_val;
#if 1
	struct t_opthdr ist_hdr;
	t_scalar_t ist_val;
	struct t_opthdr ost_hdr;
	t_scalar_t ost_val;
#endif
} opt_conn = {
	{
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_DEBUG, T_SUCCESS}
	, 0x0, {
	sizeof(struct t_opthdr) + sizeof(struct t_linger), XTI_GENERIC, XTI_LINGER, T_SUCCESS}, {
	T_NO, T_UNSPEC}
	, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_RCVBUF, T_SUCCESS}
	, 32767, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_RCVLOWAT, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_SNDBUF, T_SUCCESS}
	, 32767, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_SNDLOWAT, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(unsigned char), T_INET_IP, T_IP_TOS, T_SUCCESS}, {
	.opt_val = 0x0}
	, {
	sizeof(struct t_opthdr) + sizeof(unsigned char), T_INET_IP, T_IP_TTL, T_SUCCESS}, {
	.opt_val = 64}
	, {
	sizeof(struct t_opthdr) + sizeof(unsigned int), T_INET_IP, T_IP_DONTROUTE, T_SUCCESS}, T_NO, {
	sizeof(struct t_opthdr) + sizeof(unsigned int), T_INET_IP, T_IP_BROADCAST, T_SUCCESS}, T_NO, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), T_INET_IP, T_IP_REUSEADDR, T_SUCCESS}
	, T_NO
#if 1
	    , {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_ISTREAMS, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_OSTREAMS, T_SUCCESS}
	, 1
#endif
};

/*
 * management options
 */
struct {
#if 0
#if 1
	struct t_opthdr xdb_hdr;
	t_uscalar_t xdb_val;
#else
	struct t_opthdr dbg_hdr;
	t_uscalar_t dbg_val;
#endif
#endif
	struct t_opthdr lin_hdr;
	struct t_linger lin_val;
	struct t_opthdr rbf_hdr;
	t_uscalar_t rbf_val;
	struct t_opthdr rlw_hdr;
	t_uscalar_t rlw_val;
	struct t_opthdr sbf_hdr;
	t_uscalar_t sbf_val;
	struct t_opthdr slw_hdr;
	t_uscalar_t slw_val;
	struct t_opthdr tos_hdr;
	union {
		unsigned char opt_val;
		t_scalar_t opt_fil;
	} tos_val;
	struct t_opthdr ttl_hdr;
	union {
		unsigned char opt_val;
		t_scalar_t opt_fil;
	} ttl_val;
	struct t_opthdr drt_hdr;
	t_scalar_t drt_val;
	struct t_opthdr bca_hdr;
	t_scalar_t bca_val;
	struct t_opthdr reu_hdr;
	t_scalar_t reu_val;
#if 0
	struct t_opthdr csm_hdr;
	t_scalar_t csm_val;
#endif
#if 0
	struct t_opthdr ndl_hdr;
	t_scalar_t ndl_val;
	struct t_opthdr mxs_hdr;
	t_scalar_t mxs_val;
#endif
#if 0
	struct t_opthdr kpa_hdr;
	struct t_kpalive kpa_val;
#endif
#if 1
	struct t_opthdr nod_hdr;
	t_scalar_t nod_val;
	struct t_opthdr crk_hdr;
	t_scalar_t crk_val;
	struct t_opthdr ppi_hdr;
	t_scalar_t ppi_val;
	struct t_opthdr sid_hdr;
	t_scalar_t sid_val;
	struct t_opthdr rcv_hdr;
	t_scalar_t rcv_val;
	struct t_opthdr ckl_hdr;
	t_scalar_t ckl_val;
	struct t_opthdr skd_hdr;
	t_scalar_t skd_val;
	struct t_opthdr prt_hdr;
	t_scalar_t prt_val;
	struct t_opthdr art_hdr;
	t_scalar_t art_val;
	struct t_opthdr irt_hdr;
	t_scalar_t irt_val;
	struct t_opthdr hbi_hdr;
	t_scalar_t hbi_val;
	struct t_opthdr rin_hdr;
	t_scalar_t rin_val;
	struct t_opthdr rmn_hdr;
	t_scalar_t rmn_val;
	struct t_opthdr rmx_hdr;
	t_scalar_t rmx_val;
	struct t_opthdr ist_hdr;
	t_scalar_t ist_val;
	struct t_opthdr ost_hdr;
	t_scalar_t ost_val;
	struct t_opthdr cin_hdr;
	t_scalar_t cin_val;
	struct t_opthdr tin_hdr;
	t_scalar_t tin_val;
	struct t_opthdr mac_hdr;
	t_scalar_t mac_val;
	struct t_opthdr dbg_hdr;
	t_scalar_t dbg_val;
#endif
} opt_optm = {
	{
#if 0
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_DEBUG, T_SUCCESS}
	, 0x0, {
#endif
	sizeof(struct t_opthdr) + sizeof(struct t_linger), XTI_GENERIC, XTI_LINGER, T_SUCCESS}, {
	T_NO, T_UNSPEC}
	, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_RCVBUF, T_SUCCESS}
	, 32767, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_RCVLOWAT, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_SNDBUF, T_SUCCESS}
	, 32767, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), XTI_GENERIC, XTI_SNDLOWAT, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(unsigned char), T_INET_IP, T_IP_TOS, T_SUCCESS}, {
	.opt_val = 0x0}
	, {
	sizeof(struct t_opthdr) + sizeof(unsigned char), T_INET_IP, T_IP_TTL, T_SUCCESS}, {
	.opt_val = 64}
	, {
	sizeof(struct t_opthdr) + sizeof(unsigned int), T_INET_IP, T_IP_DONTROUTE, T_SUCCESS}, T_NO, {
	sizeof(struct t_opthdr) + sizeof(unsigned int), T_INET_IP, T_IP_BROADCAST, T_SUCCESS}, T_NO, {
	sizeof(struct t_opthdr) + sizeof(unsigned int), T_INET_IP, T_IP_REUSEADDR, T_SUCCESS}, T_NO
#if 0
	    , {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), T_INET_UDP, T_UDP_CHECKSUM, T_SUCCESS}
	, T_NO
#endif
#if 0
	    , {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_TCP, T_TCP_NODELAY, T_SUCCESS}
	, T_NO, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_TCP, T_TCP_MAXSEG, T_SUCCESS}
	, 576
#endif
#if 0
	    , {
	sizeof(struct t_opthdr) + sizeof(struct t_kpalive), T_INET_TCP, T_TCP_KEEPALIVE, T_SUCCESS}, {
	T_NO, 0}
#endif
#if 1
	, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_NODELAY, T_SUCCESS}
	, T_YES, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_CORK, T_SUCCESS}
	, T_YES, {
	sizeof(struct t_opthdr) + sizeof(t_uscalar_t), T_INET_SCTP, T_SCTP_PPI, T_SUCCESS}
	, 10, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_SID, T_SUCCESS}
	, 0, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_RECVOPT, T_SUCCESS}
	, T_NO, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_COOKIE_LIFE, T_SUCCESS}
	, 60000, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_SACK_DELAY, T_SUCCESS}
	, 200, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_PATH_MAX_RETRANS, T_SUCCESS}
	, 5, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_ASSOC_MAX_RETRANS, T_SUCCESS}
	, 12, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_MAX_INIT_RETRIES, T_SUCCESS}
	, 12, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_HEARTBEAT_ITVL, T_SUCCESS}
	, 1000, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_RTO_INITIAL, T_SUCCESS}
	, 200, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_RTO_MIN, T_SUCCESS}
	, 10, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_RTO_MAX, T_SUCCESS}
	, 2000, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_OSTREAMS, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_ISTREAMS, T_SUCCESS}
	, 1, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_COOKIE_INC, T_SUCCESS}
	, 1000, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_THROTTLE_ITVL, T_SUCCESS}
	, 50, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_MAC_TYPE, T_SUCCESS}
	, T_SCTP_HMAC_NONE, {
	sizeof(struct t_opthdr) + sizeof(t_scalar_t), T_INET_SCTP, T_SCTP_DEBUG, T_SUCCESS}
	, 0
#endif
};

struct t_opthdr *
find_option(int level, int name, const char *cmd_buf, size_t opt_ofs, size_t opt_len)
{
	const char *opt_ptr = cmd_buf + opt_ofs;
	struct t_opthdr *oh = NULL;

	for (oh = _T_OPT_FIRSTHDR_OFS(opt_ptr, opt_len, 0); oh; oh = _T_OPT_NEXTHDR_OFS(opt_ptr, opt_len, oh, 0)) {
		int len = oh->len - sizeof(*oh);

		if (len < 0) {
			oh = NULL;
			break;
		}
		if (oh->level != level)
			continue;
		if (oh->name != name)
			continue;
		break;
	}
	return (oh);
}

/*
 *  -------------------------------------------------------------------------
 *
 *  Printing things
 *
 *  -------------------------------------------------------------------------
 */

char *
errno_string(long err)
{
	switch (err) {
	case 0:
		return ("ok");
	case EPERM:
		return ("[EPERM]");
	case ENOENT:
		return ("[ENOENT]");
	case ESRCH:
		return ("[ESRCH]");
	case EINTR:
		return ("[EINTR]");
	case EIO:
		return ("[EIO]");
	case ENXIO:
		return ("[ENXIO]");
	case E2BIG:
		return ("[E2BIG]");
	case ENOEXEC:
		return ("[ENOEXEC]");
	case EBADF:
		return ("[EBADF]");
	case ECHILD:
		return ("[ECHILD]");
	case EAGAIN:
		return ("[EAGAIN]");
	case ENOMEM:
		return ("[ENOMEM]");
	case EACCES:
		return ("[EACCES]");
	case EFAULT:
		return ("[EFAULT]");
	case ENOTBLK:
		return ("[ENOTBLK]");
	case EBUSY:
		return ("[EBUSY]");
	case EEXIST:
		return ("[EEXIST]");
	case EXDEV:
		return ("[EXDEV]");
	case ENODEV:
		return ("[ENODEV]");
	case ENOTDIR:
		return ("[ENOTDIR]");
	case EISDIR:
		return ("[EISDIR]");
	case EINVAL:
		return ("[EINVAL]");
	case ENFILE:
		return ("[ENFILE]");
	case EMFILE:
		return ("[EMFILE]");
	case ENOTTY:
		return ("[ENOTTY]");
	case ETXTBSY:
		return ("[ETXTBSY]");
	case EFBIG:
		return ("[EFBIG]");
	case ENOSPC:
		return ("[ENOSPC]");
	case ESPIPE:
		return ("[ESPIPE]");
	case EROFS:
		return ("[EROFS]");
	case EMLINK:
		return ("[EMLINK]");
	case EPIPE:
		return ("[EPIPE]");
	case EDOM:
		return ("[EDOM]");
	case ERANGE:
		return ("[ERANGE]");
	case EDEADLK:
		return ("[EDEADLK]");
	case ENAMETOOLONG:
		return ("[ENAMETOOLONG]");
	case ENOLCK:
		return ("[ENOLCK]");
	case ENOSYS:
		return ("[ENOSYS]");
	case ENOTEMPTY:
		return ("[ENOTEMPTY]");
	case ELOOP:
		return ("[ELOOP]");
	case ENOMSG:
		return ("[ENOMSG]");
	case EIDRM:
		return ("[EIDRM]");
	case ECHRNG:
		return ("[ECHRNG]");
	case EL2NSYNC:
		return ("[EL2NSYNC]");
	case EL3HLT:
		return ("[EL3HLT]");
	case EL3RST:
		return ("[EL3RST]");
	case ELNRNG:
		return ("[ELNRNG]");
	case EUNATCH:
		return ("[EUNATCH]");
	case ENOCSI:
		return ("[ENOCSI]");
	case EL2HLT:
		return ("[EL2HLT]");
	case EBADE:
		return ("[EBADE]");
	case EBADR:
		return ("[EBADR]");
	case EXFULL:
		return ("[EXFULL]");
	case ENOANO:
		return ("[ENOANO]");
	case EBADRQC:
		return ("[EBADRQC]");
	case EBADSLT:
		return ("[EBADSLT]");
	case EBFONT:
		return ("[EBFONT]");
	case ENOSTR:
		return ("[ENOSTR]");
	case ENODATA:
		return ("[ENODATA]");
	case ETIME:
		return ("[ETIME]");
	case ENOSR:
		return ("[ENOSR]");
	case ENONET:
		return ("[ENONET]");
	case ENOPKG:
		return ("[ENOPKG]");
	case EREMOTE:
		return ("[EREMOTE]");
	case ENOLINK:
		return ("[ENOLINK]");
	case EADV:
		return ("[EADV]");
	case ESRMNT:
		return ("[ESRMNT]");
	case ECOMM:
		return ("[ECOMM]");
	case EPROTO:
		return ("[EPROTO]");
	case EMULTIHOP:
		return ("[EMULTIHOP]");
	case EDOTDOT:
		return ("[EDOTDOT]");
	case EBADMSG:
		return ("[EBADMSG]");
	case EOVERFLOW:
		return ("[EOVERFLOW]");
	case ENOTUNIQ:
		return ("[ENOTUNIQ]");
	case EBADFD:
		return ("[EBADFD]");
	case EREMCHG:
		return ("[EREMCHG]");
	case ELIBACC:
		return ("[ELIBACC]");
	case ELIBBAD:
		return ("[ELIBBAD]");
	case ELIBSCN:
		return ("[ELIBSCN]");
	case ELIBMAX:
		return ("[ELIBMAX]");
	case ELIBEXEC:
		return ("[ELIBEXEC]");
	case EILSEQ:
		return ("[EILSEQ]");
	case ERESTART:
		return ("[ERESTART]");
	case ESTRPIPE:
		return ("[ESTRPIPE]");
	case EUSERS:
		return ("[EUSERS]");
	case ENOTSOCK:
		return ("[ENOTSOCK]");
	case EDESTADDRREQ:
		return ("[EDESTADDRREQ]");
	case EMSGSIZE:
		return ("[EMSGSIZE]");
	case EPROTOTYPE:
		return ("[EPROTOTYPE]");
	case ENOPROTOOPT:
		return ("[ENOPROTOOPT]");
	case EPROTONOSUPPORT:
		return ("[EPROTONOSUPPORT]");
	case ESOCKTNOSUPPORT:
		return ("[ESOCKTNOSUPPORT]");
	case EOPNOTSUPP:
		return ("[EOPNOTSUPP]");
	case EPFNOSUPPORT:
		return ("[EPFNOSUPPORT]");
	case EAFNOSUPPORT:
		return ("[EAFNOSUPPORT]");
	case EADDRINUSE:
		return ("[EADDRINUSE]");
	case EADDRNOTAVAIL:
		return ("[EADDRNOTAVAIL]");
	case ENETDOWN:
		return ("[ENETDOWN]");
	case ENETUNREACH:
		return ("[ENETUNREACH]");
	case ENETRESET:
		return ("[ENETRESET]");
	case ECONNABORTED:
		return ("[ECONNABORTED]");
	case ECONNRESET:
		return ("[ECONNRESET]");
	case ENOBUFS:
		return ("[ENOBUFS]");
	case EISCONN:
		return ("[EISCONN]");
	case ENOTCONN:
		return ("[ENOTCONN]");
	case ESHUTDOWN:
		return ("[ESHUTDOWN]");
	case ETOOMANYREFS:
		return ("[ETOOMANYREFS]");
	case ETIMEDOUT:
		return ("[ETIMEDOUT]");
	case ECONNREFUSED:
		return ("[ECONNREFUSED]");
	case EHOSTDOWN:
		return ("[EHOSTDOWN]");
	case EHOSTUNREACH:
		return ("[EHOSTUNREACH]");
	case EALREADY:
		return ("[EALREADY]");
	case EINPROGRESS:
		return ("[EINPROGRESS]");
	case ESTALE:
		return ("[ESTALE]");
	case EUCLEAN:
		return ("[EUCLEAN]");
	case ENOTNAM:
		return ("[ENOTNAM]");
	case ENAVAIL:
		return ("[ENAVAIL]");
	case EISNAM:
		return ("[EISNAM]");
	case EREMOTEIO:
		return ("[EREMOTEIO]");
	case EDQUOT:
		return ("[EDQUOT]");
	case ENOMEDIUM:
		return ("[ENOMEDIUM]");
	case EMEDIUMTYPE:
		return ("[EMEDIUMTYPE]");
	default:
	{
		static char buf[32];

		snprintf(buf, sizeof(buf), "[%ld]", (long) err);
		return buf;
	}
	}
}

char *
terrno_string(ulong terr, long uerr)
{
	switch (terr) {
	case TBADADDR:
		return ("[TBADADDR]");
	case TBADOPT:
		return ("[TBADOPT]");
	case TACCES:
		return ("[TACCES]");
	case TBADF:
		return ("[TBADF]");
	case TNOADDR:
		return ("[TNOADDR]");
	case TOUTSTATE:
		return ("[TOUTSTATE]");
	case TBADSEQ:
		return ("[TBADSEQ]");
	case TSYSERR:
		return errno_string(uerr);
	case TLOOK:
		return ("[TLOOK]");
	case TBADDATA:
		return ("[TBADDATA]");
	case TBUFOVFLW:
		return ("[TBUFOVFLW]");
	case TFLOW:
		return ("[TFLOW]");
	case TNODATA:
		return ("[TNODATA]");
	case TNODIS:
		return ("[TNODIS]");
	case TNOUDERR:
		return ("[TNOUDERR]");
	case TBADFLAG:
		return ("[TBADFLAG]");
	case TNOREL:
		return ("[TNOREL]");
	case TNOTSUPPORT:
		return ("[TNOTSUPPORT]");
	case TSTATECHNG:
		return ("[TSTATECHNG]");
	case TNOSTRUCTYPE:
		return ("[TNOSTRUCTYPE]");
	case TBADNAME:
		return ("[TBADNAME]");
	case TBADQLEN:
		return ("[TBADQLEN]");
	case TADDRBUSY:
		return ("[TADDRBUSY]");
	case TINDOUT:
		return ("[TINDOUT]");
	case TPROVMISMATCH:
		return ("[TPROVMISMATCH]");
	case TRESQLEN:
		return ("[TRESQLEN]");
	case TRESADDR:
		return ("[TRESADDR]");
	case TQFULL:
		return ("[TQFULL]");
	case TPROTO:
		return ("[TPROTO]");
	default:
	{
		static char buf[32];

		snprintf(buf, sizeof(buf), "[%lu]", (ulong) terr);
		return buf;
	}
	}
}

#define ICMP_ECHOREPLY		0	/* Echo Reply */
#define ICMP_DEST_UNREACH	3	/* Destination Unreachable */
#define ICMP_SOURCE_QUENCH	4	/* Source Quench */
#define ICMP_REDIRECT		5	/* Redirect (change route) */
#define ICMP_ECHO		8	/* Echo Request */
#define ICMP_TIME_EXCEEDED	11	/* Time Exceeded */
#define ICMP_PARAMETERPROB	12	/* Parameter Problem */
#define ICMP_TIMESTAMP		13	/* Timestamp Request */
#define ICMP_TIMESTAMPREPLY	14	/* Timestamp Reply */
#define ICMP_INFO_REQUEST	15	/* Information Request */
#define ICMP_INFO_REPLY		16	/* Information Reply */
#define ICMP_ADDRESS		17	/* Address Mask Request */
#define ICMP_ADDRESSREPLY	18	/* Address Mask Reply */
#define NR_ICMP_TYPES		18

/* Codes for UNREACH. */
#define ICMP_NET_UNREACH	0	/* Network Unreachable */
#define ICMP_HOST_UNREACH	1	/* Host Unreachable */
#define ICMP_PROT_UNREACH	2	/* Protocol Unreachable */
#define ICMP_PORT_UNREACH	3	/* Port Unreachable */
#define ICMP_FRAG_NEEDED	4	/* Fragmentation Needed/DF set */
#define ICMP_SR_FAILED		5	/* Source Route failed */
#define ICMP_NET_UNKNOWN	6
#define ICMP_HOST_UNKNOWN	7
#define ICMP_HOST_ISOLATED	8
#define ICMP_NET_ANO		9
#define ICMP_HOST_ANO		10
#define ICMP_NET_UNR_TOS	11
#define ICMP_HOST_UNR_TOS	12
#define ICMP_PKT_FILTERED	13	/* Packet filtered */
#define ICMP_PREC_VIOLATION	14	/* Precedence violation */
#define ICMP_PREC_CUTOFF	15	/* Precedence cut off */
#define NR_ICMP_UNREACH		15	/* instead of hardcoding immediate value */

/* Codes for REDIRECT. */
#define ICMP_REDIR_NET		0	/* Redirect Net */
#define ICMP_REDIR_HOST		1	/* Redirect Host */
#define ICMP_REDIR_NETTOS	2	/* Redirect Net for TOS */
#define ICMP_REDIR_HOSTTOS	3	/* Redirect Host for TOS */

/* Codes for TIME_EXCEEDED. */
#define ICMP_EXC_TTL		0	/* TTL count exceeded */
#define ICMP_EXC_FRAGTIME	1	/* Fragment Reass time exceeded */

char *
etype_string(t_uscalar_t etype)
{
	switch (etype) {
	case TBADADDR:
		return ("[TBADADDR]");
	case TBADOPT:
		return ("[TBADOPT]");
	case TACCES:
		return ("[TACCES]");
	case TBADF:
		return ("[TBADF]");
	case TNOADDR:
		return ("[TNOADDR]");
	case TOUTSTATE:
		return ("[TOUTSTATE]");
	case TBADSEQ:
		return ("[TBADSEQ]");
	case TSYSERR:
		return ("[TSYSERR]");
	case TLOOK:
		return ("[TLOOK]");
	case TBADDATA:
		return ("[TBADDATA]");
	case TBUFOVFLW:
		return ("[TBUFOVFLW]");
	case TFLOW:
		return ("[TFLOW]");
	case TNODATA:
		return ("[TNODATA]");
	case TNODIS:
		return ("[TNODIS]");
	case TNOUDERR:
		return ("[TNOUDERR]");
	case TBADFLAG:
		return ("[TBADFLAG]");
	case TNOREL:
		return ("[TNOREL]");
	case TNOTSUPPORT:
		return ("[TNOTSUPPORT]");
	case TSTATECHNG:
		return ("[TSTATECHNG]");
	case TNOSTRUCTYPE:
		return ("[TNOSTRUCTYPE]");
	case TBADNAME:
		return ("[TBADNAME]");
	case TBADQLEN:
		return ("[TBADQLEN]");
	case TADDRBUSY:
		return ("[TADDRBUSY]");
	case TINDOUT:
		return ("[TINDOUT]");
	case TPROVMISMATCH:
		return ("[TPROVMISMATCH]");
	case TRESQLEN:
		return ("[TRESQLEN]");
	case TRESADDR:
		return ("[TRESADDR]");
	case TQFULL:
		return ("[TQFULL]");
	case TPROTO:
		return ("[TPROTO]");
	default:
	{
		unsigned char code = ((etype >> 0) & 0x00ff);
		unsigned char type = ((etype >> 8) & 0x00ff);

		switch (type) {
		case ICMP_DEST_UNREACH:
			switch (code) {
			case ICMP_NET_UNREACH:
				return ("<ICMP_NET_UNREACH>");
			case ICMP_HOST_UNREACH:
				return ("<ICMP_HOST_UNREACH>");
			case ICMP_PROT_UNREACH:
				return ("<ICMP_PROT_UNREACH>");
			case ICMP_PORT_UNREACH:
				return ("<ICMP_PORT_UNREACH>");
			case ICMP_FRAG_NEEDED:
				return ("<ICMP_FRAG_NEEDED>");
			case ICMP_NET_UNKNOWN:
				return ("<ICMP_NET_UNKNOWN>");
			case ICMP_HOST_UNKNOWN:
				return ("<ICMP_HOST_UNKNOWN>");
			case ICMP_HOST_ISOLATED:
				return ("<ICMP_HOST_ISOLATED>");
			case ICMP_NET_ANO:
				return ("<ICMP_NET_ANO>");
			case ICMP_HOST_ANO:
				return ("<ICMP_HOST_ANO>");
			case ICMP_PKT_FILTERED:
				return ("<ICMP_PKT_FILTERED>");
			case ICMP_PREC_VIOLATION:
				return ("<ICMP_PREC_VIOLATION>");
			case ICMP_PREC_CUTOFF:
				return ("<ICMP_PREC_CUTOFF>");
			case ICMP_SR_FAILED:
				return ("<ICMP_SR_FAILED>");
			case ICMP_NET_UNR_TOS:
				return ("<ICMP_NET_UNR_TOS>");
			case ICMP_HOST_UNR_TOS:
				return ("<ICMP_HOST_UNR_TOS>");
			default:
				return ("<ICMP_DEST_UNREACH?>");
			}
		case ICMP_SOURCE_QUENCH:
			return ("<ICMP_SOURCE_QUENCH>");
		case ICMP_TIME_EXCEEDED:
			switch (code) {
			case ICMP_EXC_TTL:
				return ("<ICMP_EXC_TTL>");
			case ICMP_EXC_FRAGTIME:
				return ("<ICMP_EXC_FRAGTIME>");
			default:
				return ("<ICMP_TIME_EXCEEDED?>");
			}
		case ICMP_PARAMETERPROB:
			return ("<ICMP_PARAMETERPROB>");
		default:
			return ("<ICMP_???? >");
		}
	}
	}
}

const char *
event_string(int child, int event)
{
	switch (event) {
	case __EVENT_EOF:
		return ("END OF FILE");
	case __EVENT_NO_MSG:
		return ("NO MESSAGE");
	case __EVENT_TIMEOUT:
		return ("TIMEOUT");
	case __EVENT_UNKNOWN:
		return ("UNKNOWN");
	case __RESULT_DECODE_ERROR:
		return ("DECODE ERROR");
	case __RESULT_SCRIPT_ERROR:
		return ("SCRIPT ERROR");
	case __RESULT_INCONCLUSIVE:
		return ("INCONCLUSIVE");
	case __RESULT_SUCCESS:
		return ("SUCCESS");
	case __RESULT_FAILURE:
		return ("FAILURE");
	case __TEST_CONN_REQ:
		return ("T_CONN_REQ");
	case __TEST_CONN_RES:
		return ("T_CONN_RES");
	case __TEST_DISCON_REQ:
		return ("T_DISCON_REQ");
	case __TEST_DATA_REQ:
		return ("T_DATA_REQ");
	case __TEST_EXDATA_REQ:
		return ("T_EXDATA_REQ");
	case __TEST_OPTDATA_REQ:
		return ("T_OPTDATA_REQ");
	case __TEST_INFO_REQ:
		return ("T_INFO_REQ");
	case __TEST_BIND_REQ:
		return ("T_BIND_REQ");
	case __TEST_UNBIND_REQ:
		return ("T_UNBIND_REQ");
	case __TEST_UNITDATA_REQ:
		return ("T_UNITDATA_REQ");
	case __TEST_OPTMGMT_REQ:
		return ("T_OPTMGMT_REQ");
	case __TEST_ORDREL_REQ:
		return ("T_ORDREL_REQ");
	case __TEST_CONN_IND:
		return ("T_CONN_IND");
	case __TEST_CONN_CON:
		return ("T_CONN_CON");
	case __TEST_DISCON_IND:
		return ("T_DISCON_IND");
	case __TEST_DATA_IND:
		return ("T_DATA_IND");
	case __TEST_EXDATA_IND:
		return ("T_EXDATA_IND");
	case __TEST_NRM_OPTDATA_IND:
		return ("T_OPTDATA_IND(nrm)");
	case __TEST_EXP_OPTDATA_IND:
		return ("T_OPTDATA_IND(exp)");
	case __TEST_INFO_ACK:
		return ("T_INFO_ACK");
	case __TEST_BIND_ACK:
		return ("T_BIND_ACK");
	case __TEST_ERROR_ACK:
		return ("T_ERROR_ACK");
	case __TEST_OK_ACK:
		return ("T_OK_ACK");
	case __TEST_UNITDATA_IND:
		return ("T_UNITDATA_IND");
	case __TEST_UDERROR_IND:
		return ("T_UDERROR_IND");
	case __TEST_OPTMGMT_ACK:
		return ("T_OPTMGMT_ACK");
	case __TEST_ORDREL_IND:
		return ("T_ORDREL_IND");
	case __TEST_ADDR_REQ:
		return ("T_ADDR_REQ");
	case __TEST_ADDR_ACK:
		return ("T_ADDR_ACK");
	case __TEST_CAPABILITY_REQ:
		return ("T_CAPABILITY_REQ");
	case __TEST_CAPABILITY_ACK:
		return ("T_CAPABILITY_ACK");
	case __TEST_ABORT:
		return ("ABORT");
	case __TEST_INIT:
		return ("INIT");
	case __TEST_INIT_ACK:
		return ("INIT-ACK");
	case __TEST_COOKIE_ECHO:
		return ("COOKIE-ECHO");
	case __TEST_COOKIE_ACK:
		return ("COOKIE-ACK");
	case __TEST_DATA_CHUNK:
		return ("DATA");
	case __TEST_SACK:
		return ("SACK");
	case __TEST_ERROR:
		return ("ERROR");
	case __TEST_SHUTDOWN:
		return ("SHUTDOWN");
	case __TEST_SHUTDOWN_ACK:
		return ("SHUTDOWN-ACK");
	case __TEST_SHUTDOWN_COMPLETE:
		return ("SHUTDOWN-COMPLETE");
	}
	return ("(unexpected)");
}

const char *
ioctl_string(int cmd, intptr_t arg)
{
	switch (cmd) {
	case I_NREAD:
		return ("I_NREAD");
	case I_PUSH:
		return ("I_PUSH");
	case I_POP:
		return ("I_POP");
	case I_LOOK:
		return ("I_LOOK");
	case I_FLUSH:
		return ("I_FLUSH");
	case I_SRDOPT:
		return ("I_SRDOPT");
	case I_GRDOPT:
		return ("I_GRDOPT");
	case I_STR:
		if (arg) {
			struct strioctl *icp = (struct strioctl *) arg;

			switch (icp->ic_cmd) {
#if 0
			case _O_TI_BIND:
				return ("_O_TI_BIND");
			case O_TI_BIND:
				return ("O_TI_BIND");
			case _O_TI_GETINFO:
				return ("_O_TI_GETINFO");
			case O_TI_GETINFO:
				return ("O_TI_GETINFO");
			case _O_TI_GETMYNAME:
				return ("_O_TI_GETMYNAME");
			case _O_TI_GETPEERNAME:
				return ("_O_TI_GETPEERNAME");
			case _O_TI_OPTMGMT:
				return ("_O_TI_OPTMGMT");
			case O_TI_OPTMGMT:
				return ("O_TI_OPTMGMT");
			case _O_TI_TLI_MODE:
				return ("_O_TI_TLI_MODE");
			case _O_TI_UNBIND:
				return ("_O_TI_UNBIND");
			case O_TI_UNBIND:
				return ("O_TI_UNBIND");
			case _O_TI_XTI_CLEAR_EVENT:
				return ("_O_TI_XTI_CLEAR_EVENT");
			case _O_TI_XTI_GET_STATE:
				return ("_O_TI_XTI_GET_STATE");
			case _O_TI_XTI_HELLO:
				return ("_O_TI_XTI_HELLO");
			case _O_TI_XTI_MODE:
				return ("_O_TI_XTI_MODE");
			case TI_BIND:
				return ("TI_BIND");
			case TI_CAPABILITY:
				return ("TI_CAPABILITY");
			case TI_GETADDRS:
				return ("TI_GETADDRS");
			case TI_GETINFO:
				return ("TI_GETINFO");
			case TI_GETMYNAME:
				return ("TI_GETMYNAME");
			case TI_GETPEERNAME:
				return ("TI_GETPEERNAME");
			case TI_OPTMGMT:
				return ("TI_OPTMGMT");
			case TI_SETMYNAME:
				return ("TI_SETMYNAME");
			case TI_SETPEERNAME:
				return ("TI_SETPEERNAME");
			case TI_SYNC:
				return ("TI_SYNC");
			case TI_UNBIND:
				return ("TI_UNBIND");
#endif
			}
		}
		return ("I_STR");
	case I_SETSIG:
		return ("I_SETSIG");
	case I_GETSIG:
		return ("I_GETSIG");
	case I_FIND:
		return ("I_FIND");
	case I_LINK:
		return ("I_LINK");
	case I_UNLINK:
		return ("I_UNLINK");
	case I_RECVFD:
		return ("I_RECVFD");
	case I_PEEK:
		return ("I_PEEK");
	case I_FDINSERT:
		return ("I_FDINSERT");
	case I_SENDFD:
		return ("I_SENDFD");
#if 0
	case I_E_RECVFD:
		return ("I_E_RECVFD");
#endif
	case I_SWROPT:
		return ("I_SWROPT");
	case I_GWROPT:
		return ("I_GWROPT");
	case I_LIST:
		return ("I_LIST");
	case I_PLINK:
		return ("I_PLINK");
	case I_PUNLINK:
		return ("I_PUNLINK");
	case I_FLUSHBAND:
		return ("I_FLUSHBAND");
	case I_CKBAND:
		return ("I_CKBAND");
	case I_GETBAND:
		return ("I_GETBAND");
	case I_ATMARK:
		return ("I_ATMARK");
	case I_SETCLTIME:
		return ("I_SETCLTIME");
	case I_GETCLTIME:
		return ("I_GETCLTIME");
	case I_CANPUT:
		return ("I_CANPUT");
#if 0
	case I_SERROPT:
		return ("I_SERROPT");
	case I_GERROPT:
		return ("I_GERROPT");
	case I_ANCHOR:
		return ("I_ANCHOR");
#endif
#if 0
	case I_S_RECVFD:
		return ("I_S_RECVFD");
	case I_STATS:
		return ("I_STATS");
	case I_BIGPIPE:
		return ("I_BIGPIPE");
#endif
#if 0
	case I_GETTP:
		return ("I_GETTP");
	case I_AUTOPUSH:
		return ("I_AUTOPUSH");
	case I_HEAP_REPORT:
		return ("I_HEAP_REPORT");
	case I_FIFO:
		return ("I_FIFO");
	case I_PUTPMSG:
		return ("I_PUTPMSG");
	case I_GETPMSG:
		return ("I_GETPMSG");
	case I_FATTACH:
		return ("I_FATTACH");
	case I_FDETACH:
		return ("I_FDETACH");
	case I_PIPE:
		return ("I_PIPE");
#endif
	default:
		return ("(unexpected)");
	}
}

const char *
signal_string(int signum)
{
	switch (signum) {
	case SIGHUP:
		return ("SIGHUP");
	case SIGINT:
		return ("SIGINT");
	case SIGQUIT:
		return ("SIGQUIT");
	case SIGILL:
		return ("SIGILL");
	case SIGABRT:
		return ("SIGABRT");
	case SIGFPE:
		return ("SIGFPE");
	case SIGKILL:
		return ("SIGKILL");
	case SIGSEGV:
		return ("SIGSEGV");
	case SIGPIPE:
		return ("SIGPIPE");
	case SIGALRM:
		return ("SIGALRM");
	case SIGTERM:
		return ("SIGTERM");
	case SIGUSR1:
		return ("SIGUSR1");
	case SIGUSR2:
		return ("SIGUSR2");
	case SIGCHLD:
		return ("SIGCHLD");
	case SIGCONT:
		return ("SIGCONT");
	case SIGSTOP:
		return ("SIGSTOP");
	case SIGTSTP:
		return ("SIGTSTP");
	case SIGTTIN:
		return ("SIGTTIN");
	case SIGTTOU:
		return ("SIGTTOU");
	case SIGBUS:
		return ("SIGBUS");
	case SIGPOLL:
		return ("SIGPOLL");
	case SIGPROF:
		return ("SIGPROF");
	case SIGSYS:
		return ("SIGSYS");
	case SIGTRAP:
		return ("SIGTRAP");
	case SIGURG:
		return ("SIGURG");
	case SIGVTALRM:
		return ("SIGVTALRM");
	case SIGXCPU:
		return ("SIGXCPU");
	case SIGXFSZ:
		return ("SIGXFSZ");
	default:
		return ("unknown");
	}
}

const char *
poll_string(short events)
{
	if (events & POLLIN)
		return ("POLLIN");
	if (events & POLLPRI)
		return ("POLLPRI");
	if (events & POLLOUT)
		return ("POLLOUT");
	if (events & POLLRDNORM)
		return ("POLLRDNORM");
	if (events & POLLRDBAND)
		return ("POLLRDBAND");
	if (events & POLLWRNORM)
		return ("POLLWRNORM");
	if (events & POLLWRBAND)
		return ("POLLWRBAND");
	if (events & POLLERR)
		return ("POLLERR");
	if (events & POLLHUP)
		return ("POLLHUP");
	if (events & POLLNVAL)
		return ("POLLNVAL");
	if (events & POLLMSG)
		return ("POLLMSG");
	return ("none");
}

const char *
poll_events_string(short events)
{
	static char string[256] = "";
	int offset = 0, size = 256, len = 0;

	if (events & POLLIN) {
		len = snprintf(string + offset, size, "POLLIN|");
		offset += len;
		size -= len;
	}
	if (events & POLLPRI) {
		len = snprintf(string + offset, size, "POLLPRI|");
		offset += len;
		size -= len;
	}
	if (events & POLLOUT) {
		len = snprintf(string + offset, size, "POLLOUT|");
		offset += len;
		size -= len;
	}
	if (events & POLLRDNORM) {
		len = snprintf(string + offset, size, "POLLRDNORM|");
		offset += len;
		size -= len;
	}
	if (events & POLLRDBAND) {
		len = snprintf(string + offset, size, "POLLRDBAND|");
		offset += len;
		size -= len;
	}
	if (events & POLLWRNORM) {
		len = snprintf(string + offset, size, "POLLWRNORM|");
		offset += len;
		size -= len;
	}
	if (events & POLLWRBAND) {
		len = snprintf(string + offset, size, "POLLWRBAND|");
		offset += len;
		size -= len;
	}
	if (events & POLLERR) {
		len = snprintf(string + offset, size, "POLLERR|");
		offset += len;
		size -= len;
	}
	if (events & POLLHUP) {
		len = snprintf(string + offset, size, "POLLHUP|");
		offset += len;
		size -= len;
	}
	if (events & POLLNVAL) {
		len = snprintf(string + offset, size, "POLLNVAL|");
		offset += len;
		size -= len;
	}
	if (events & POLLMSG) {
		len = snprintf(string + offset, size, "POLLMSG|");
		offset += len;
		size -= len;
	}
	return (string);
}

const char *
service_type(t_uscalar_t type)
{
	switch (type) {
	case T_CLTS:
		return ("T_CLTS");
	case T_COTS:
		return ("T_COTS");
	case T_COTS_ORD:
		return ("T_COTS_ORD");
	default:
		return ("(unknown)");
	}
}

const char *
state_string(t_uscalar_t state)
{
	switch (state) {
	case TS_UNBND:
		return ("TS_UNBND");
	case TS_WACK_BREQ:
		return ("TS_WACK_BREQ");
	case TS_WACK_UREQ:
		return ("TS_WACK_UREQ");
	case TS_IDLE:
		return ("TS_IDLE");
	case TS_WACK_OPTREQ:
		return ("TS_WACK_OPTREQ");
	case TS_WACK_CREQ:
		return ("TS_WACK_CREQ");
	case TS_WCON_CREQ:
		return ("TS_WCON_CREQ");
	case TS_WRES_CIND:
		return ("TS_WRES_CIND");
	case TS_WACK_CRES:
		return ("TS_WACK_CRES");
	case TS_DATA_XFER:
		return ("TS_DATA_XFER");
	case TS_WIND_ORDREL:
		return ("TS_WIND_ORDREL");
	case TS_WREQ_ORDREL:
		return ("TS_WRES_ORDREL");
	case TS_WACK_DREQ6:
		return ("TS_WACK_DREQ6");
	case TS_WACK_DREQ7:
		return ("TS_WACK_DREQ7");
	case TS_WACK_DREQ9:
		return ("TS_WACK_DREQ9");
	case TS_WACK_DREQ10:
		return ("TS_WACK_DREQ10");
	case TS_WACK_DREQ11:
		return ("TS_WACK_DREQ11");
	default:
		return ("(unknown)");
	}
}

#ifndef SCTP_VERSION_2
void
print_addr(char *add_ptr, size_t add_len)
{
	sctp_addr_t *a = (sctp_addr_t *) add_ptr;
	size_t anum = add_len >= sizeof(a->port) ? (add_len - sizeof(a->port)) / sizeof(a->addr[0]) : 0;

	dummy = lockf(fileno(stdout), F_LOCK, 0);
	if (add_len) {
		int i;

		if (add_len != sizeof(a->port) + anum * sizeof(a->addr[0]))
			fprintf(stdout, "Aaarrg! add_len = %d, anum = %d, ", add_len, anum);
		fprintf(stdout, "[%d]", ntohs(a->port));
		for (i = 0; i < anum; i++) {
			fprintf(stdout, "%s%d.%d.%d.%d", i ? "," : "", (a->addr[i] >> 0) & 0xff, (a->addr[i] >> 8) & 0xff, (a->addr[i] >> 16) & 0xff, (a->addr[i] >> 24) & 0xff);
		}
	} else
		fprintf(stdout, "(no address)");
	fprintf(stdout, "\n");
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

char *
addr_string(char *add_ptr, size_t add_len)
{
	static char buf[128];
	size_t len = 0;
	sctp_addr_t *a = (sctp_addr_t *) add_ptr;
	size_t anum = add_len >= sizeof(a->port) ? (add_len - sizeof(a->port)) / sizeof(a->addr[0]) : 0;

	if (add_len) {
		int i;

		if (add_len != sizeof(a->port) + anum * sizeof(a->addr[0]))
			len += snprintf(buf + len, sizeof(buf) - len, "Aaarrg! add_len = %d, anum = %d, ", add_len, anum);
		len += snprintf(buf + len, sizeof(buf) - len, "[%d]", ntohs(a->port));
		for (i = 0; i < anum; i++) {
			len += snprintf(buf + len, sizeof(buf) - len, "%s%d.%d.%d.%d", i ? "," : "", (a->addr[i] >> 0) & 0xff, (a->addr[i] >> 8) & 0xff, (a->addr[i] >> 16) & 0xff, (a->addr[i] >> 24) & 0xff);
		}
	} else
		len += snprintf(buf + len, sizeof(buf) - len, "(no address)");
	/* len += snprintf(buf + len, sizeof(buf) - len, "\0"); */
	return buf;
}

void
print_addrs(int fd, char *add_ptr, size_t add_len)
{
	fprintf(stdout, "Stupid!\n");
}
#else				/* SCTP_VERSION_2 */
void
print_addr(char *add_ptr, size_t add_len)
{
	struct sockaddr_in *a = (struct sockaddr_in *) add_ptr;
	size_t anum = add_len / sizeof(*a);

	dummy = lockf(fileno(stdout), F_LOCK, 0);
	if (add_len > 0) {
		int i;

		if (add_len != anum * sizeof(*a))
			fprintf(stdout, "Aaarrg! add_len = %lu, anum = %lu, ", (ulong) add_len, (ulong) anum);
		fprintf(stdout, "[%d]", ntohs(a[0].sin_port));
		for (i = 0; i < anum; i++) {
			uint32_t addr = a[i].sin_addr.s_addr;

			fprintf(stdout, "%s%d.%d.%d.%d", i ? "," : "", (addr >> 0) & 0xff, (addr >> 8) & 0xff, (addr >> 16) & 0xff, (addr >> 24) & 0xff);
		}
	} else
		fprintf(stdout, "(no address)");
	fprintf(stdout, "\n");
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

char *
addr_string(char *add_ptr, size_t add_len)
{
	static char buf[128];
	size_t len = 0;
	struct sockaddr_in *a = (struct sockaddr_in *) add_ptr;
	size_t anum = add_len / sizeof(*a);

	if (add_len > 0) {
		int i;

		if (add_len != anum * sizeof(*a))
			len += snprintf(buf + len, sizeof(buf) - len, "Aaarrg! add_len = %lu, anum = %lu, ", (ulong) add_len, (ulong) anum);
		len += snprintf(buf + len, sizeof(buf) - len, "[%d]", ntohs(a[0].sin_port));
		for (i = 0; i < anum; i++) {
			uint32_t addr = a[i].sin_addr.s_addr;

			len += snprintf(buf + len, sizeof(buf) - len, "%s%d.%d.%d.%d", i ? "," : "", (addr >> 0) & 0xff, (addr >> 8) & 0xff, (addr >> 16) & 0xff, (addr >> 24) & 0xff);
		}
	} else
		len += snprintf(buf + len, sizeof(buf) - len, "(no address)");
	/* len += snprintf(buf + len, sizeof(buf) - len, "\0"); */
	return buf;
}
void print_string(int child, const char *string);
void
print_addrs(int child, char *add_ptr, size_t add_len)
{
	struct sockaddr_in *sin;

	if (verbose < 3 || !show)
		return;
	if (add_len == 0)
		print_string(child, "(no address)");
	for (sin = (typeof(sin)) add_ptr; add_len >= sizeof(*sin); sin++, add_len -= sizeof(*sin)) {
		char buf[128];

		snprintf(buf, sizeof(buf), "%d.%d.%d.%d:%d", (sin->sin_addr.s_addr >> 0) & 0xff, (sin->sin_addr.s_addr >> 8) & 0xff, (sin->sin_addr.s_addr >> 16) & 0xff, (sin->sin_addr.s_addr >> 24) & 0xff, ntohs(sin->sin_port));
		print_string(child, buf);
	}
}
#endif				/* SCTP_VERSION_2 */

#if 1
char *
status_string(struct t_opthdr *oh)
{
	switch (oh->status) {
	case 0:
		return (NULL);
	case T_SUCCESS:
		return ("T_SUCCESS");
	case T_FAILURE:
		return ("T_FAILURE");
	case T_PARTSUCCESS:
		return ("T_PARTSUCCESS");
	case T_READONLY:
		return ("T_READONLY");
	case T_NOTSUPPORT:
		return ("T_NOTSUPPORT");
	default:
	{
		static char buf[32];

		snprintf(buf, sizeof(buf), "(unknown status %ld)", (long) oh->status);
		return buf;
	}
	}
}

#ifndef T_ALLLEVELS
#define T_ALLLEVELS -1
#endif

char *
level_string(struct t_opthdr *oh)
{
	switch (oh->level) {
	case T_ALLLEVELS:
		return ("T_ALLLEVELS");
	case XTI_GENERIC:
		return ("XTI_GENERIC");
	case T_INET_IP:
		return ("T_INET_IP");
	case T_INET_UDP:
		return ("T_INET_UDP");
	case T_INET_TCP:
		return ("T_INET_TCP");
	case T_INET_SCTP:
		return ("T_INET_SCTP");
	default:
	{
		static char buf[32];

		snprintf(buf, sizeof(buf), "(unknown level %ld)", (long) oh->level);
		return buf;
	}
	}
}

char *
name_string(struct t_opthdr *oh)
{
	if (oh->name == T_ALLOPT)
		return ("T_ALLOPT");
	switch (oh->level) {
	case XTI_GENERIC:
		switch (oh->name) {
		case XTI_DEBUG:
			return ("XTI_DEBUG");
		case XTI_LINGER:
			return ("XTI_LINGER");
		case XTI_RCVBUF:
			return ("XTI_RCVBUF");
		case XTI_RCVLOWAT:
			return ("XTI_RCVLOWAT");
		case XTI_SNDBUF:
			return ("XTI_SNDBUF");
		case XTI_SNDLOWAT:
			return ("XTI_SNDLOWAT");
		}
		break;
	case T_INET_IP:
		switch (oh->name) {
		case T_IP_OPTIONS:
			return ("T_IP_OPTIONS");
		case T_IP_TOS:
			return ("T_IP_TOS");
		case T_IP_TTL:
			return ("T_IP_TTL");
		case T_IP_REUSEADDR:
			return ("T_IP_REUSEADDR");
		case T_IP_DONTROUTE:
			return ("T_IP_DONTROUTE");
		case T_IP_BROADCAST:
			return ("T_IP_BROADCAST");
		case T_IP_ADDR:
			return ("T_IP_ADDR");
		}
		break;
	case T_INET_UDP:
		switch (oh->name) {
		case T_UDP_CHECKSUM:
			return ("T_UDP_CHECKSUM");
		}
		break;
	case T_INET_TCP:
		switch (oh->name) {
		case T_TCP_NODELAY:
			return ("T_TCP_NODELAY");
		case T_TCP_MAXSEG:
			return ("T_TCP_MAXSEG");
		case T_TCP_KEEPALIVE:
			return ("T_TCP_KEEPALIVE");
		case T_TCP_CORK:
			return ("T_TCP_CORK");
		case T_TCP_KEEPIDLE:
			return ("T_TCP_KEEPIDLE");
		case T_TCP_KEEPINTVL:
			return ("T_TCP_KEEPINTVL");
		case T_TCP_KEEPCNT:
			return ("T_TCP_KEEPCNT");
		case T_TCP_SYNCNT:
			return ("T_TCP_SYNCNT");
		case T_TCP_LINGER2:
			return ("T_TCP_LINGER2");
		case T_TCP_DEFER_ACCEPT:
			return ("T_TCP_DEFER_ACCEPT");
		case T_TCP_WINDOW_CLAMP:
			return ("T_TCP_WINDOW_CLAMP");
		case T_TCP_INFO:
			return ("T_TCP_INFO");
		case T_TCP_QUICKACK:
			return ("T_TCP_QUICKACK");
		}
		break;
	case T_INET_SCTP:
		switch (oh->name) {
		case T_SCTP_NODELAY:
			return ("T_SCTP_NODELAY");
		case T_SCTP_CORK:
			return ("T_SCTP_CORK");
		case T_SCTP_PPI:
			return ("T_SCTP_PPI");
		case T_SCTP_SID:
			return ("T_SCTP_SID");
		case T_SCTP_SSN:
			return ("T_SCTP_SSN");
		case T_SCTP_TSN:
			return ("T_SCTP_TSN");
		case T_SCTP_RECVOPT:
			return ("T_SCTP_RECVOPT");
		case T_SCTP_COOKIE_LIFE:
			return ("T_SCTP_COOKIE_LIFE");
		case T_SCTP_SACK_DELAY:
			return ("T_SCTP_SACK_DELAY");
		case T_SCTP_PATH_MAX_RETRANS:
			return ("T_SCTP_PATH_MAX_RETRANS");
		case T_SCTP_ASSOC_MAX_RETRANS:
			return ("T_SCTP_ASSOC_MAX_RETRANS");
		case T_SCTP_MAX_INIT_RETRIES:
			return ("T_SCTP_MAX_INIT_RETRIES");
		case T_SCTP_HEARTBEAT_ITVL:
			return ("T_SCTP_HEARTBEAT_ITVL");
		case T_SCTP_RTO_INITIAL:
			return ("T_SCTP_RTO_INITIAL");
		case T_SCTP_RTO_MIN:
			return ("T_SCTP_RTO_MIN");
		case T_SCTP_RTO_MAX:
			return ("T_SCTP_RTO_MAX");
		case T_SCTP_OSTREAMS:
			return ("T_SCTP_OSTREAMS");
		case T_SCTP_ISTREAMS:
			return ("T_SCTP_ISTREAMS");
		case T_SCTP_COOKIE_INC:
			return ("T_SCTP_COOKIE_INC");
		case T_SCTP_THROTTLE_ITVL:
			return ("T_SCTP_THROTTLE_ITVL");
		case T_SCTP_MAC_TYPE:
			return ("T_SCTP_MAC_TYPE");
		case T_SCTP_CKSUM_TYPE:
			return ("T_SCTP_CKSUM_TYPE");
		case T_SCTP_ECN:
			return ("T_SCTP_ECN");
		case T_SCTP_ALI:
			return ("T_SCTP_ALI");
		case T_SCTP_ADD:
			return ("T_SCTP_ADD");
		case T_SCTP_SET:
			return ("T_SCTP_SET");
		case T_SCTP_ADD_IP:
			return ("T_SCTP_ADD_IP");
		case T_SCTP_DEL_IP:
			return ("T_SCTP_DEL_IP");
		case T_SCTP_SET_IP:
			return ("T_SCTP_SET_IP");
		case T_SCTP_PR:
			return ("T_SCTP_PR");
		case T_SCTP_LIFETIME:
			return ("T_SCTP_LIFETIME");
		case T_SCTP_DISPOSITION:
			return ("T_SCTP_DISPOSITION");
		case T_SCTP_MAX_BURST:
			return ("T_SCTP_MAX_BURST");
		case T_SCTP_HB:
			return ("T_SCTP_HB");
		case T_SCTP_RTO:
			return ("T_SCTP_RTO");
		case T_SCTP_MAXSEG:
			return ("T_SCTP_MAXSEG");
		case T_SCTP_STATUS:
			return ("T_SCTP_STATUS");
		case T_SCTP_DEBUG:
			return ("T_SCTP_DEBUG");
		}
		break;
	}
	{
		static char buf[32];

		snprintf(buf, sizeof(buf), "(unknown name %ld)", (long) oh->name);
		return buf;
	}
}

char *
yesno_string(struct t_opthdr *oh)
{
	switch (*((t_uscalar_t *) T_OPT_DATA(oh))) {
	case T_YES:
		return ("T_YES");
	case T_NO:
		return ("T_NO");
	default:
		return ("(invalid)");
	}
}

char *
number_string(struct t_opthdr *oh)
{
	static char buf[32];

	snprintf(buf, 32, "%d", *((t_scalar_t *) T_OPT_DATA(oh)));
	return (buf);
}

char *
value_string(int child, struct t_opthdr *oh)
{
	static char buf[64] = "(invalid)";

	if (oh->len == sizeof(*oh))
		return (NULL);
	switch (oh->level) {
	case XTI_GENERIC:
		switch (oh->name) {
		case XTI_DEBUG:
			break;
		case XTI_LINGER:
			break;
		case XTI_RCVBUF:
			break;
		case XTI_RCVLOWAT:
			break;
		case XTI_SNDBUF:
			break;
		case XTI_SNDLOWAT:
			break;
		}
		break;
	case T_INET_IP:
		switch (oh->name) {
		case T_IP_OPTIONS:
			break;
		case T_IP_TOS:
			if (oh->len == sizeof(*oh) + sizeof(unsigned char))
				snprintf(buf, sizeof(buf), "0x%02x", *((unsigned char *) T_OPT_DATA(oh)));
			return buf;
		case T_IP_TTL:
			if (oh->len == sizeof(*oh) + sizeof(unsigned char))
				snprintf(buf, sizeof(buf), "0x%02x", *((unsigned char *) T_OPT_DATA(oh)));
			return buf;
		case T_IP_REUSEADDR:
			return yesno_string(oh);
		case T_IP_DONTROUTE:
			return yesno_string(oh);
		case T_IP_BROADCAST:
			return yesno_string(oh);
		case T_IP_ADDR:
			if (oh->len == sizeof(*oh) + sizeof(uint32_t)) {
				uint32_t addr = *((uint32_t *) T_OPT_DATA(oh));

				snprintf(buf, sizeof(buf), "%d.%d.%d.%d", (addr >> 0) & 0x00ff, (addr >> 8) & 0x00ff, (addr >> 16) & 0x00ff, (addr >> 24) & 0x00ff);
			}
			return buf;
		}
		break;
	case T_INET_UDP:
		switch (oh->name) {
		case T_UDP_CHECKSUM:
			return yesno_string(oh);
		}
		break;
	case T_INET_TCP:
		switch (oh->name) {
		case T_TCP_NODELAY:
			return yesno_string(oh);
		case T_TCP_MAXSEG:
			if (oh->len == sizeof(*oh) + sizeof(t_uscalar_t))
				snprintf(buf, sizeof(buf), "%lu", (ulong) *((t_uscalar_t *) T_OPT_DATA(oh)));
			return buf;
		case T_TCP_KEEPALIVE:
			break;
		case T_TCP_CORK:
			return yesno_string(oh);
		case T_TCP_KEEPIDLE:
			break;
		case T_TCP_KEEPINTVL:
			break;
		case T_TCP_KEEPCNT:
			break;
		case T_TCP_SYNCNT:
			break;
		case T_TCP_LINGER2:
			break;
		case T_TCP_DEFER_ACCEPT:
			break;
		case T_TCP_WINDOW_CLAMP:
			break;
		case T_TCP_INFO:
			break;
		case T_TCP_QUICKACK:
			break;
		}
		break;
	case T_INET_SCTP:
		switch (oh->name) {
		case T_SCTP_NODELAY:
			return yesno_string(oh);
		case T_SCTP_CORK:
			return yesno_string(oh);
		case T_SCTP_PPI:
			return number_string(oh);;
		case T_SCTP_SID:
#if 1
			sid[child] = *((t_uscalar_t *) T_OPT_DATA(oh));
#endif
			return number_string(oh);;
		case T_SCTP_SSN:
		case T_SCTP_TSN:
			return number_string(oh);;
		case T_SCTP_RECVOPT:
			return yesno_string(oh);
		case T_SCTP_COOKIE_LIFE:
		case T_SCTP_SACK_DELAY:
		case T_SCTP_PATH_MAX_RETRANS:
		case T_SCTP_ASSOC_MAX_RETRANS:
		case T_SCTP_MAX_INIT_RETRIES:
		case T_SCTP_HEARTBEAT_ITVL:
		case T_SCTP_RTO_INITIAL:
		case T_SCTP_RTO_MIN:
		case T_SCTP_RTO_MAX:
		case T_SCTP_OSTREAMS:
		case T_SCTP_ISTREAMS:
		case T_SCTP_COOKIE_INC:
		case T_SCTP_THROTTLE_ITVL:
			return number_string(oh);;
		case T_SCTP_MAC_TYPE:
			break;
		case T_SCTP_CKSUM_TYPE:
			break;
		case T_SCTP_ECN:
			break;
		case T_SCTP_ALI:
			break;
		case T_SCTP_ADD:
			break;
		case T_SCTP_SET:
			break;
		case T_SCTP_ADD_IP:
			break;
		case T_SCTP_DEL_IP:
			break;
		case T_SCTP_SET_IP:
			break;
		case T_SCTP_PR:
			break;
		case T_SCTP_LIFETIME:
		case T_SCTP_DISPOSITION:
		case T_SCTP_MAX_BURST:
		case T_SCTP_HB:
		case T_SCTP_RTO:
		case T_SCTP_MAXSEG:
			return number_string(oh);
		case T_SCTP_STATUS:
			break;
		case T_SCTP_DEBUG:
			break;
		}
		break;
	}
	return ("(unknown value)");
}
#endif

#if 1
void
parse_options(int fd, char *opt_ptr, size_t opt_len)
{
	struct t_opthdr *oh;

	for (oh = _T_OPT_FIRSTHDR_OFS(opt_ptr, opt_len, 0); oh; oh = _T_OPT_NEXTHDR_OFS(opt_ptr, opt_len, oh, 0)) {
		if (oh->len == sizeof(*oh))
			continue;
		switch (oh->level) {
		case T_INET_SCTP:
			switch (oh->name) {
			case T_SCTP_PPI:
				ppi[fd] = *((t_uscalar_t *) T_OPT_DATA(oh));
				continue;
			case T_SCTP_SID:
				sid[fd] = *((t_uscalar_t *) T_OPT_DATA(oh));
				continue;
			case T_SCTP_SSN:
				ssn[fd] = *((t_uscalar_t *) T_OPT_DATA(oh));
				continue;
			case T_SCTP_TSN:
				tsn[fd] = *((t_uscalar_t *) T_OPT_DATA(oh));
				continue;
			}
		}
	}
}
#endif

char *
mgmtflag_string(t_uscalar_t flag)
{
	switch (flag) {
	case T_NEGOTIATE:
		return ("T_NEGOTIATE");
	case T_CHECK:
		return ("T_CHECK");
	case T_DEFAULT:
		return ("T_DEFAULT");
	case T_SUCCESS:
		return ("T_SUCCESS");
	case T_FAILURE:
		return ("T_FAILURE");
	case T_CURRENT:
		return ("T_CURRENT");
	case T_PARTSUCCESS:
		return ("T_PARTSUCCESS");
	case T_READONLY:
		return ("T_READONLY");
	case T_NOTSUPPORT:
		return ("T_NOTSUPPORT");
	}
	return "(unknown flag)";
}

#if 1
char *
size_string(t_uscalar_t size)
{
	static char buf[128];

	switch (size) {
	case T_INFINITE:
		return ("T_INFINITE");
	case T_INVALID:
		return ("T_INVALID");
	case T_UNSPEC:
		return ("T_UNSPEC");
	}
	snprintf(buf, sizeof(buf), "%lu", (ulong) size);
	return buf;
}
#endif

const char *
prim_string(t_uscalar_t prim)
{
	switch (prim) {
	case T_CONN_REQ:
		return ("T_CONN_REQ------");
	case T_CONN_RES:
		return ("T_CONN_RES------");
	case T_DISCON_REQ:
		return ("T_DISCON_REQ----");
	case T_DATA_REQ:
		return ("T_DATA_REQ------");
	case T_EXDATA_REQ:
		return ("T_EXDATA_REQ----");
	case T_INFO_REQ:
		return ("T_INFO_REQ------");
	case T_BIND_REQ:
		return ("T_BIND_REQ------");
	case T_UNBIND_REQ:
		return ("T_UNBIND_REQ----");
	case T_UNITDATA_REQ:
		return ("T_UNITDATA_REQ--");
	case T_OPTMGMT_REQ:
		return ("T_OPTMGMT_REQ---");
	case T_ORDREL_REQ:
		return ("T_ORDREL_REQ----");
	case T_OPTDATA_REQ:
		return ("T_OPTDATA_REQ---");
	case T_ADDR_REQ:
		return ("T_ADDR_REQ------");
	case T_CAPABILITY_REQ:
		return ("T_CAPABILITY_REQ");
	case T_CONN_IND:
		return ("T_CONN_IND------");
	case T_CONN_CON:
		return ("T_CONN_CON------");
	case T_DISCON_IND:
		return ("T_DISCON_IND----");
	case T_DATA_IND:
		return ("T_DATA_IND------");
	case T_EXDATA_IND:
		return ("T_EXDATA_IND----");
	case T_INFO_ACK:
		return ("T_INFO_ACK------");
	case T_BIND_ACK:
		return ("T_BIND_ACK------");
	case T_ERROR_ACK:
		return ("T_ERROR_ACK-----");
	case T_OK_ACK:
		return ("T_OK_ACK--------");
	case T_UNITDATA_IND:
		return ("T_UNITDATA_IND--");
	case T_UDERROR_IND:
		return ("T_UDERROR_IND---");
	case T_OPTMGMT_ACK:
		return ("T_OPTMGMT_ACK---");
	case T_ORDREL_IND:
		return ("T_ORDREL_IND----");
	case T_OPTDATA_IND:
		return ("T_OPTDATA_IND---");
	case T_ADDR_ACK:
		return ("T_ADDR_ACK------");
	case T_CAPABILITY_ACK:
		return ("T_CAPABILITY_ACK");
	default:
		return ("T_????_??? -----");
	}
}

char *
t_errno_string(t_scalar_t err, t_scalar_t syserr)
{
	switch (err) {
	case 0:
		return ("ok");
	case TBADADDR:
		return ("[TBADADDR]");
	case TBADOPT:
		return ("[TBADOPT]");
	case TACCES:
		return ("[TACCES]");
	case TBADF:
		return ("[TBADF]");
	case TNOADDR:
		return ("[TNOADDR]");
	case TOUTSTATE:
		return ("[TOUTSTATE]");
	case TBADSEQ:
		return ("[TBADSEQ]");
	case TSYSERR:
		return errno_string(syserr);
	case TLOOK:
		return ("[TLOOK]");
	case TBADDATA:
		return ("[TBADDATA]");
	case TBUFOVFLW:
		return ("[TBUFOVFLW]");
	case TFLOW:
		return ("[TFLOW]");
	case TNODATA:
		return ("[TNODATA]");
	case TNODIS:
		return ("[TNODIS]");
	case TNOUDERR:
		return ("[TNOUDERR]");
	case TBADFLAG:
		return ("[TBADFLAG]");
	case TNOREL:
		return ("[TNOREL]");
	case TNOTSUPPORT:
		return ("[TNOTSUPPORT]");
	case TSTATECHNG:
		return ("[TSTATECHNG]");
	case TNOSTRUCTYPE:
		return ("[TNOSTRUCTYPE]");
	case TBADNAME:
		return ("[TBADNAME]");
	case TBADQLEN:
		return ("[TBADQLEN]");
	case TADDRBUSY:
		return ("[TADDRBUSY]");
	case TINDOUT:
		return ("[TINDOUT]");
	case TPROVMISMATCH:
		return ("[TPROVMISMATCH]");
	case TRESQLEN:
		return ("[TRESQLEN]");
	case TRESADDR:
		return ("[TRESADDR]");
	case TQFULL:
		return ("[TQFULL]");
	case TPROTO:
		return ("[TPROTO]");
	default:
	{
		static char buf[32];

		snprintf(buf, sizeof(buf), "[%ld]", (long) err);
		return buf;
	}
	}
}

void
print_simple(int child, const char *msgs[])
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, "%s", msgs[child]);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_simple_int(int child, const char *msgs[], int val)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], val);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_double_int(int child, const char *msgs[], int val, int val2)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], val, val2);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_triple_int(int child, const char *msgs[], int val, int val2, int val3)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], val, val2, val3);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_simple_string(int child, const char *msgs[], const char *string)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], string);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_string_state(int child, const char *msgs[], const char *string)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], string, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_triple_string(int child, const char *msgs[], const char *string)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], "", child, state);
	fprintf(stdout, msgs[child], string, child, state);
	fprintf(stdout, msgs[child], "", child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_more(int child)
{
	show = 1;
}

void
print_less(int child)
{
	static const char *msgs[] = {
		"         . %1$6.6s . | <------         .         ------> :                    [%2$d:%3$03d]\n",
		"                    : <------         .         ------> | . %1$-6.6s .         [%2$d:%3$03d]\n",
		"                    : <------         .      ------> |  : . %1$-6.6s .         [%2$d:%3$03d]\n",
		"         . %1$6.6s . : <------         .      ------> :> : . %1$-6.6s .         [%2$d:%3$03d]\n",
	};

	if (show && verbose > 0)
		print_triple_string(child, msgs, "(more)");
	show = 0;
}

void
print_pipe(int child)
{
	static const char *msgs[] = {
		"  pipe()      ----->v  v<------------------------------>v                   \n",
		"                    v  v<------------------------------>v<-----     pipe()  \n",
		"                    .  .                                .                   \n",
	};

	if (show && verbose > 3)
		print_simple(child, msgs);
}

void
print_open(int child, const char *name)
{
	static const char *msgs[] = {
		"  open()      ----->v  . %-30.30s .                   \n",
		"                    |  v %-30.30s v<-----     open()  \n",
		"                    |  v<%-30.30s-.------     open()  \n",
		"                    .  . %-30.30s .                   \n",
	};

	if (show && verbose > 3)
		print_simple(child, msgs);
}

void
print_close(int child)
{
	static const char *msgs[] = {
		"  close()     ----->X  |                                |                   \n",
		"                    .  X                                X<-----    close()  \n",
		"                    .  X<-------------------------------.------    close()  \n",
		"                    .  .                                .                   \n",
	};

	if (show && verbose > 3)
		print_simple(child, msgs);
}

void
print_preamble(int child)
{
	static const char *msgs[] = {
		"--------------------+  +----------Preamble--------------+                   \n",
		"                    +  +----------Preamble--------------+-------------------\n",
		"                    +--+----------Preamble--------------+                   \n",
		"--------------------+--+----------Preamble--------------+-------------------\n",
	};

	if (verbose > 0)
		print_simple(child, msgs);
}

void
print_failure(int child, const char *string)
{
	static const char *msgs[] = {
		"....................|..|%-32.32s|                    [%d:%03d]\n",
		"                    |  |%-32.32s|................... [%d:%03d]\n",
		"                    |..|%-32.32s.................... [%d:%03d]\n",
		"....................|..|%-32.32s|................... [%d:%03d]\n",
	};

	if (string && strnlen(string, 32) > 0 && verbose > 0)
		print_string_state(child, msgs, string);
}

void
print_notapplicable(int child)
{
	static const char *msgs[] = {
		"X-X-X-X-X-X-X-X-X-X-|  |-X-X-X NOT APPLICABLE -X-X-X-X-X|                    [%d:%03d]\n",
		"                    |  |-X-X-X NOT APPLICABLE -X-X-X-X-X|X-X-X-X-X-X-X-X-X-X [%d:%03d]\n",
		"                    |X-|-X-X-X NOT APPLICABLE -X-X-X-X-X|                    [%d:%03d]\n",
		"X-X-X-X-X-X-X-X-X-X-|X-|-X-X-X NOT APPLICABLE -X-X-X-X-X|X-X-X-X-X-X-X-X-X-X [%d:%03d]\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, state);
	print_failure(child, failure_string);
}

void
print_skipped(int child)
{
	static const char *msgs[] = {
		"::::::::::::::::::::|  |::::::::: SKIPPED ::::::::::::::|                    [%d:%03d]\n",
		"                    |  |::::::::: SKIPPED ::::::::::::::|::::::::::::::::::: [%d:%03d]\n",
		"                    |::|::::::::: SKIPPED ::::::::::::::|                    [%d:%03d]\n",
		"::::::::::::::::::::|::|::::::::: SKIPPED ::::::::::::::|::::::::::::::::::: [%d:%03d]\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, state);
	print_failure(child, failure_string);
}

void
print_inconclusive(int child)
{
	static const char *msgs[] = {
		"????????????????????|  |??????? INCONCLUSIVE ???????????|                    [%d:%03d]\n",
		"                    |  |??????? INCONCLUSIVE ???????????|??????????????????? [%d:%03d]\n",
		"                    |??|??????? INCONCLUSIVE ???????????|                    [%d:%03d]\n",
		"????????????????????|??|??????? INCONCLUSIVE ???????????|??????????????????? [%d:%03d]\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, state);
	print_failure(child, failure_string);
}

void
print_test(int child)
{
	static const char *msgs[] = {
		"--------------------+  +------------Test----------------+                   \n",
		"                    +  +------------Test----------------+-------------------\n",
		"                    +--+------------Test----------------+                   \n",
		"--------------------+--+------------Test----------------+-------------------\n",
	};

	if (verbose > 0)
		print_simple(child, msgs);
}

void
print_failed(int child)
{
	static const char *msgs[] = {
		"XXXXXXXXXXXXXXXXXXXX|  |XXXXXXXXXX FAILED XXXXXXXXXXXXXX|                    [%d:%03d]\n",
		"                    |  |XXXXXXXXXX FAILED XXXXXXXXXXXXXX|XXXXXXXXXXXXXXXXXXX [%d:%03d]\n",
		"                    |XX|XXXXXXXXXX FAILED XXXXXXXXXXXXXX|                    [%d:%03d]\n",
		"XXXXXXXXXXXXXXXXXXXX|XX|XXXXXXXXXX FAILED XXXXXXXXXXXXXX|XXXXXXXXXXXXXXXXXXX [%d:%03d]\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, state);
	print_failure(child, failure_string);
}

void
print_script_error(int child)
{
	static const char *msgs[] = {
		"####################|  |######## SCRIPT ERROR ##########|                    [%d:%03d]\n",
		"                    |  |######## SCRIPT ERROR ##########|################### [%d:%03d]\n",
		"                    |##|######## SCRIPT ERROR ##########|                    [%d:%03d]\n",
		"####################|##|######## SCRIPT ERROR ##########|################### [%d:%03d]\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, state);
	print_failure(child, failure_string);
}

void
print_passed(int child)
{
	static const char *msgs[] = {
		"********************|  |********** PASSED **************|                    [%d:%03d]\n",
		"                    |  |********** PASSED **************|******************* [%d:%03d]\n",
		"                    |**|********** PASSED **************|                    [%d:%03d]\n",
		"********************|**|********** PASSED **************|******************* [%d:%03d]\n",
	};

	if (verbose > 2)
		print_double_int(child, msgs, child, state);
	print_failure(child, failure_string);
}

void
print_postamble(int child)
{
	static const char *msgs[] = {
		"--------------------+  +----------Postamble-------------+                   \n",
		"                    +  +----------Postamble-------------+-------------------\n",
		"                    +--+----------Postamble-------------+                   \n",
		"--------------------+--+----------Postamble-------------+-------------------\n",
	};

	if (verbose > 0)
		print_simple(child, msgs);
}

void
print_test_end(int child)
{
	static const char *msgs[] = {
		"--------------------+  +--------------------------------+                   \n",
		"                    +  +--------------------------------+-------------------\n",
		"                    +--+--------------------------------+                   \n",
		"--------------------+--+--------------------------------+-------------------\n",
	};

	if (verbose > 0)
		print_simple(child, msgs);
}

void
print_terminated(int child, int signal)
{
	static const char *msgs[] = {
		"@@@@@@@@@@@@@@@@@@@@|  |@@@@@@@@ TERMINATED @@@@@@@@@@@@|                    {%d:%03d}\n",
		"                    |  |@@@@@@@@ TERMINATED @@@@@@@@@@@@|@@@@@@@@@@@@@@@@@@@ {%d:%03d}\n",
		"                    |@@|@@@@@@@@ TERMINATED @@@@@@@@@@@@|                    {%d:%03d}\n",
		"@@@@@@@@@@@@@@@@@@@@|@@|@@@@@@@@ TERMINATED @@@@@@@@@@@@|@@@@@@@@@@@@@@@@@@@ {%d:%03d}\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, signal);
}

void
print_stopped(int child, int signal)
{
	static const char *msgs[] = {
		"&&&&&&&&&&&&&&&&&&&&|  |&&&&&&&&& STOPPED &&&&&&&&&&&&&&|                    {%d:%03d}\n",
		"                    |  |&&&&&&&&& STOPPED &&&&&&&&&&&&&&|&&&&&&&&&&&&&&&&&&& {%d:%03d}\n",
		"                    |&&|&&&&&&&&& STOPPED &&&&&&&&&&&&&&|                    {%d:%03d}\n",
		"&&&&&&&&&&&&&&&&&&&&|&&|&&&&&&&&& STOPPED &&&&&&&&&&&&&&|&&&&&&&&&&&&&&&&&&& {%d:%03d}\n",
	};

	if (verbose > 0)
		print_double_int(child, msgs, child, signal);
}

void
print_timeout(int child)
{
	static const char *msgs[] = {
		"++++++++++++++++++++|  |+++++++++ TIMEOUT! +++++++++++++|                    [%d:%03d]\n",
		"                    |  |+++++++++ TIMEOUT! +++++++++++++|+++++++++++++++++++ [%d:%03d]\n",
		"                    |++|+++++++++ TIMEOUT! +++++++++++++|                    [%d:%03d]\n",
		"++++++++++++++++++++|++|+++++++++ TIMEOUT! +++++++++++++|+++++++++++++++++++ [%d:%03d]\n",
	};

	if (show_timeout || verbose > 0) {
		print_double_int(child, msgs, child, state);
		show_timeout--;
	}
}

void
print_nothing(int child)
{
	static const char *msgs[] = {
		"- - - - - - - - - - |  |- - - - - nothing! - - - - - - -|                    [%d:%03d]\n",
		"                    |  |- - - - - nothing! - - - - - - -|- - - - - - - - - - [%d:%03d]\n",
		"                    |- |- - - - - nothing! - - - - - - -|                    [%d:%03d]\n",
		"- - - - - - - - - - |- |- - - - - nothing! - - - - - - -|- - - - - - - - - - [%d:%03d]\n",
	};

	if (show && verbose > 1)
		print_double_int(child, msgs, child, state);
}

void
print_syscall(int child, const char *command)
{
	static const char *msgs[] = {
		"  %-14s--->|  |                                |                    [%d:%03d]\n",
		"                    |  |                                |<---%14s  [%d:%03d]\n",
		"                    |  |                                |<---%14s  [%d:%03d]\n",
		"                    |  |                                |                    [%d:%03d]\n",
	};

	if ((verbose && show) || (verbose > 5 && show_msg))
		print_string_state(child, msgs, command);
}

void
print_tx_prim(int child, const char *command)
{
	static const char *msgs[] = {
		"--%16s->|  |                                |                    [%d:%03d]\n",
		"                    |  |<- - - - - - - - - - - - - - - -|<-%16s- [%d:%03d]\n",
		"                    |  |<- - - - - - - - - - - - - - - -|<-%16s- [%d:%03d]\n",
		"                    |  |                                |                    [%d:%03d]\n",
	};

	if (show && verbose > 0)
		print_string_state(child, msgs, command);
}

void
print_rx_prim(int child, const char *command)
{
	static const char *msgs[] = {
		"<-%16s--|  |                                |                    [%d:%03d]\n",
		"                    |  |- - - - - - - - - - - - - - - ->|-%16s-> [%d:%03d]\n",
		"                    |  |- - - - - - - - - - - - - - - ->|-%16s-> [%d:%03d]\n",
		"                    |  |      <%16s>        |                    [%d:%03d]\n",
	};

	if (show && verbose > 0)
		print_string_state(child, msgs, command);
}

void
print_ack_prim(int child, const char *command)
{
	static const char *msgs[] = {
		"<-%16s-/|  |                                |                    [%d:%03d]\n",
		"                    |  |- - - - - - - - - - - - - - - ->|\\%16s-> [%d:%03d]\n",
		"                    |  |- - - - - - - - - - - - - - - ->|\\%16s-> [%d:%03d]\n",
		"                    |  |      <%16s>        |                    [%d:%03d]\n",
	};

	if (show && verbose > 0)
		print_string_state(child, msgs, command);
}

void
print_long_state(int child, const char *msgs[], long value)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], value, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_no_prim(int child, long prim)
{
	static const char *msgs[] = {
		"????%4ld????  ?----?|  | ?- - - - - - - - - - - - - -? |                     [%d:%03d]\n",
		"                    |  | ?- - - - - - - - - - - - - -? |?--? ????%4ld????    [%d:%03d]\n",
		"                    |  | ?- - - - - - - - - - - - - -? |?--? ????%4ld????    [%d:%03d]\n",
		"                    |  | ?- - - - - %4ld  - - - - - -? |                     [%d:%03d]\n",
	};

	if (verbose > 0)
		print_long_state(child, msgs, prim);
}

void
print_signal(int child, int signum)
{
	static const char *msgs[] = {
		">>>>>>>>>>>>>>>>>>>>|>>|>>>>>>>>> %-8.8s <<<<<<<<<<<<<|                    [%d:%03d]\n",
		"                    |>>|>>>>>>>>> %-8.8s <<<<<<<<<<<<<|<<<<<<<<<<<<<<<<<<< [%d:%03d]\n",
		"                    |>>|>>>>>>>>> %-8.8s <<<<<<<<<<<<<|<<<<<<<<<<<<<<<<<<< [%d:%03d]\n",
		">>>>>>>>>>>>>>>>>>>>|>>|>>>>>>>>> %-8.8s <<<<<<<<<<<<<|<<<<<<<<<<<<<<<<<<< [%d:%03d]\n",
	};

	if (verbose > 0)
		print_string_state(child, msgs, signal_string(signum));
}

void
print_double_string_state(int child, const char *msgs[], const char *string1, const char *string2)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], string1, string2, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_command_info(int child, const char *command, const char *info)
{
	static const char *msgs[] = {
		"%1$-14s----->|  |      %2$-16.16s          |                    [%3$d:%4$03d]\n",
		"                    |  |      %2$-16.16s          |<---%1$-14s  [%3$d:%4$03d]\n",
		"                    |  |<---- %2$-16.16s ---------+----%1$-14s  [%3$d:%4$03d]\n",
		"                    |  |%1$-14s %2$-16.16s |                    [%3$d:%4$03d]\n",
	};

	if (show && verbose > 3)
		print_double_string_state(child, msgs, command, info);
}

void
print_string_int_state(int child, const char *msgs[], const char *string, int val)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], string, val, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_tx_data(int child, const char *command, size_t bytes)
{
	static const char *msgs[] = {
		"--%1$16s->|- -%2$5d bytes- - - - - - - - ->|- |                    [%3$d:%4$03d]\n",
		"                    |< -%2$5d bytes- - - - - - - - - |  |<-%1$16s- [%3$d:%4$03d]\n",
		"                    |< -%2$5d bytes- - - - - - - - - |- |<-%1$16s- [%3$d:%4$03d]\n",
		"                    |- -%2$5d bytes %1$16s |  |                    [%3$d:%4$03d]\n",
	};

	if ((verbose && show) || verbose > 4)
		print_string_int_state(child, msgs, command, bytes);
}

void
print_rx_data(int child, const char *command, size_t bytes)
{
	static const char *msgs[] = {
		"<-%1$16s--|  |  %2$4d bytes                    |                    [%3$d:%4$03d]\n",
		"                    |  |- %2$4d bytes - - - - - - - - - >|--%1$16s> [%3$d:%4$03d]\n",
		"                    |  |- %2$4d bytes - - - - - - - - - >|--%1$16s> [%3$d:%4$03d]\n",
		"                    |  |  %2$4d bytes  %1$16s  |                    [%3$d:%4$03d]\n",
	};

	if ((verbose && show) || (verbose > 5 && show_msg))
		print_string_int_state(child, msgs, command, bytes);
}

void
print_errno(int child, long error)
{
	static const char *msgs[] = {
		"  %-14s<--/|  |                                |                    [%d:%03d]\n",
		"                    |  |                                |\\-->%14s  [%d:%03d]\n",
		"                    |  |                                |\\-->%14s  [%d:%03d]\n",
		"                    |  |       [%14s]         |                    [%d:%03d]\n",
	};

	if ((verbose > 4 && show) || (verbose > 5 && show_msg))
		print_string_state(child, msgs, errno_string(error));
}

void
print_success(int child)
{
	static const char *msgs[] = {
		"  ok          <----/|  |                                |                    [%d:%03d]\n",
		"                    |  |                                |\\---->         ok   [%d:%03d]\n",
		"                    |  |                                |\\---->         ok   [%d:%03d]\n",
		"                    |  |              ok                |                    [%d:%03d]\n",
	};

	if ((verbose > 4 && show) || (verbose > 5 && show_msg))
		print_double_int(child, msgs, child, state);
}

void
print_success_value(int child, int value)
{
	static const char *msgs[] = {
		"  %10d  <----/|  |                                |                    [%d:%03d]\n",
		"                    |  |                                |\\---->  %10d  [%d:%03d]\n",
		"                    |  |                                |\\---->  %10d  [%d:%03d]\n",
		"                    |  |         [%10d]           |                    [%d:%03d]\n",
	};

	if ((verbose > 4 && show) || (verbose > 5 && show_msg))
		print_triple_int(child, msgs, value, child, state);
}

void
print_int_string_state(int child, const char *msgs[], const int value, const char *string)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], value, string, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_poll_value(int child, int value, short revents)
{
	static const char *msgs[] = {
		"  %1$10d  <----/| %2$-30.30s |  |                    [%3$d:%4$03d]\n",
		"                    | %2$-30.30s |  |\\---->  %1$10d  [%3$d:%4$03d]\n",
		"                    | %2$-30.30s |\\-+----->  %1$10d  [%3$d:%4$03d]\n",
		"                    | %2$-17.17s [%1$10d] |  |                    [%3$d:%4$03d]\n",
	};

	if (show && verbose > 3)
		print_int_string_state(child, msgs, value, poll_events_string(revents));
}

void
print_ti_ioctl(int child, int cmd, intptr_t arg)
{
	static const char *msgs[] = {
		"--ioctl(2)--------->|  |    %16s            |                    [%d:%03d]\n",
		"                    |  |    %16s            |<---ioctl(2)------  [%d:%03d]\n",
		"                    |  |    %16s            |<---ioctl(2)------  [%d:%03d]\n",
		"                    |  |    %16s ioctl(2)   |                    [%d:%03d]\n",
	};

	if (show && verbose > 1)
		print_string_state(child, msgs, ioctl_string(cmd, arg));
}

void
print_ioctl(int child, int cmd, intptr_t arg)
{
	print_command_info(child, "ioctl(2)------", ioctl_string(cmd, arg));
}

void
print_poll(int child, short events)
{
	print_command_info(child, "poll(2)-------", poll_string(events));
}

void
print_datcall(int child, const char *command, size_t bytes)
{
	static const char *msgs[] = {
		"  %1$16s->|- | %2$4d bytes- - - - - - - - - - >|                    [%3$d:%4$03d]\n",
		"                    |< + %2$4d bytes- - - - - - - - - - -|<-%1$16s  [%3$d:%4$03d]\n",
		"                    |< + %2$4d bytes- - - - - - - - - - -|<-%1$16s  [%3$d:%4$03d]\n",
		"                    |< + %2$4d bytes %1$16s |  |                    [%3$d:%4$03d]\n",
	};

	if ((verbose > 4 && show) || (verbose > 5 && show_msg))
		print_string_int_state(child, msgs, command, bytes);
}

void
print_libcall(int child, const char *command)
{
	static const char *msgs[] = {
		"  %-16s->|  |                                |                    [%d:%03d]\n",
		"                    |  |                                |<-%16s  [%d:%03d]\n",
		"                    |  |                                |<-%16s  [%d:%03d]\n",
		"                    |  |     [%16s]         |                    [%d:%03d]\n",
	};

	if (show && verbose > 1)
		print_string_state(child, msgs, command);
}

#if 1
void
print_terror(int child, long error, long terror)
{
	static const char *msgs[] = {
		"  %-14s<--/|  |                                |                    [%d:%03d]\n",
		"                    |  |                                |\\-->%14s  [%d:%03d]\n",
		"                    |  |                                |\\-->%14s  [%d:%03d]\n",
		"                    |  |       [%14s]         |                    [%d:%03d]\n",
	};

	if (show && verbose > 1)
		print_string_state(child, msgs, t_errno_string(terror, error));
}
#endif

#if 0
void
print_tlook(int child, int tlook)
{
	static const char *msgs[] = {
		"  %-14s<--/|                                |  |                    [%d:%03d]\n",
		"                    |                                |  |\\-->%14s  [%d:%03d]\n",
		"                    |                                |\\-|--->%14s  [%d:%03d]\n",
		"                    |          [%14s]      |  |                    [%d:%03d]\n",
	};

	if (show && verbose > 1)
		print_string_state(child, msgs, t_look_string(tlook));
}
#endif

void
print_expect(int child, int want)
{
	static const char *msgs[] = {
		" (%-16s) |  | - - - -[Expected]- - - - - - - |                     [%d:%03d]\n",
		"                    |  | - - - -[Expected]- - - - - - - | (%-16s)  [%d:%03d]\n",
		"                    |  | - - - -[Expected]- - - - - - - | (%-16s)  [%d:%03d]\n",
		"                    |- |- [Expected %-16s ] -|                     [%d:%03d]\n",
	};

	if (verbose > 0 && show)
		print_string_state(child, msgs, event_string(child, want));
}

void
print_string(int child, const char *string)
{
	static const char *msgs[] = {
		"%-20s|  |                                |                    \n",
		"                    |  |                                |%-20s\n",
		"                    |  |                                |%-20s\n",
		"                    |  |    %-20s        |                    \n",
	};

	if (show && verbose > 1)
		print_simple_string(child, msgs, string);
}

void
print_string_val(int child, const char *string, ulong val)
{
	static const char *msgs[] = {
		"%1$20.20s|          %2$15u          |                    \n",
		"                    |          %2$15u          |%1$-20.20s\n",
		"                    |          %2$15u       |   %1$-20.20s\n",
		"                    |%1$-20.20s%2$15u|                    \n",
	};

	if (show && verbose > 0) {
		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, msgs[child], string, val);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}
}

void
print_command_state(int child, const char *string)
{
	static const char *msgs[] = {
		"%20s|                                |  |                    [%d:%03d]\n",
		"                    |                                |  |%-20s[%d:%03d]\n",
		"                    |                                |  .%-20s[%d:%03d]\n",
		"                    |       %-20s     |  |                    [%d:%03d]\n",
	};

	if (show && verbose > 1)
		print_string_state(child, msgs, string);
}

void
print_time_state(int child, const char *msgs[], ulong time)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], time, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_waiting(int child, ulong time)
{
	static const char *msgs[] = {
		"/ / / / / / / / / / | /|/ / Waiting %03lu seconds / / / / |                    [%d:%03d]\n",
		"                    |  |/ / Waiting %03lu seconds / / / / | / / / / / / / / /  [%d:%03d]\n",
		"                    | /|/ / Waiting %03lu seconds / / / / |                    [%d:%03d]\n",
		"/ / / / / / / / / / | /|/ / Waiting %03lu seconds / / / / | / / / / / / / / /  [%d:%03d]\n",
	};

	if (verbose > 0 && show)
		print_time_state(child, msgs, time);
}

void
print_float_state(int child, const char *msgs[], float time)
{
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, msgs[child], time, child, state);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

void
print_mwaiting(int child, struct timespec *time)
{
	static const char *msgs[] = {
		"/ / / / / / / / / / |  |/ Waiting %8.4f seconds / / /|                    [%d:%03d]\n",
		"                    |  |/ Waiting %8.4f seconds / / /| / / / / / / / / /  [%d:%03d]\n",
		"                    | /|/ Waiting %8.4f seconds / / /|                    [%d:%03d]\n",
		"/ / / / / / / / / / | /|/ Waiting %8.4f seconds / / /| / / / / / / / / /  [%d:%03d]\n",
	};

	if (verbose > 0 && show) {
		float delay;

		delay = time->tv_nsec;
		delay = delay / 1000000000;
		delay = delay + time->tv_sec;
		print_float_state(child, msgs, delay);
	}
}

void
print_mgmtflag(int child, ulong flag)
{
	print_string(child, mgmtflag_string(flag));
}

#if 1
void
print_opt_level(int child, struct t_opthdr *oh)
{
	char *level = level_string(oh);

	if (level)
		print_string(child, level);
}

void
print_opt_name(int child, struct t_opthdr *oh)
{
	char *name = name_string(oh);

	if (name)
		print_string(child, name);
}

void
print_opt_status(int child, struct t_opthdr *oh)
{
	char *status = status_string(oh);

	if (status)
		print_string(child, status);
}

void
print_opt_length(int child, struct t_opthdr *oh)
{
	int len = oh->len - sizeof(*oh);

	if (len) {
		char buf[32];

		snprintf(buf, sizeof(buf), "(len=%d)", len);
		print_string(child, buf);
	}
}
void
print_opt_value(int child, struct t_opthdr *oh)
{
	char *value = value_string(child, oh);

	if (value)
		print_string(child, value);
}
#endif

void
print_options(int child, const char *cmd_buf, size_t opt_ofs, size_t opt_len)
{
	struct t_opthdr *oh;
	const char *opt_ptr = cmd_buf + opt_ofs;
	char buf[64];

	if (verbose < 3 || !show)
		return;
	if (opt_len == 0) {
		snprintf(buf, sizeof(buf), "(no options)");
		print_string(child, buf);
		return;
	}
	snprintf(buf, sizeof(buf), "opt len = %lu", (ulong) opt_len);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "opt ofs = %lu", (ulong) opt_ofs);
	print_string(child, buf);
	oh = _T_OPT_FIRSTHDR_OFS(opt_ptr, opt_len, 0);
	if (oh) {
		for (; oh; oh = _T_OPT_NEXTHDR_OFS(opt_ptr, opt_len, oh, 0)) {
			int len = oh->len - sizeof(*oh);

			print_opt_level(child, oh);
			print_opt_name(child, oh);
			print_opt_status(child, oh);
			print_opt_length(child, oh);
			if (len < 0)
				break;
			print_opt_value(child, oh);
		}
	} else {
		oh = (typeof(oh)) opt_ptr;
		print_opt_level(child, oh);
		print_opt_name(child, oh);
		print_opt_status(child, oh);
		print_opt_length(child, oh);
	}
}

void
print_info(int child, struct T_info_ack *info)
{
	char buf[64];

	if (verbose < 4 || !show)
		return;
	snprintf(buf, sizeof(buf), "TSDU  = %ld", (long) info->TSDU_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "ETSDU = %ld", (long) info->ETSDU_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "CDATA = %ld", (long) info->CDATA_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "DDATA = %ld", (long) info->DDATA_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "ADDR  = %ld", (long) info->ADDR_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "OPT   = %ld", (long) info->OPT_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "TIDU  = %ld", (long) info->TIDU_size);
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "<%s>", service_type(info->SERV_type));
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "<%s>", state_string(info->CURRENT_state));
	print_string(child, buf);
	snprintf(buf, sizeof(buf), "PROV  = %ld", (long) info->PROVIDER_flag);
	print_string(child, buf);
}

#if 0
int
ip_n_open(const char *name, int *fdp)
{
	int fd;

	for (;;) {
		print_open(fdp, name);
		if ((fd = open(name, O_NONBLOCK | O_RDWR)) >= 0) {
			*fdp = fd;
			print_success(fd);
			return (__RESULT_SUCCESS);
		}
		print_errno(fd, (last_errno = errno));
		if (last_errno == EINTR || last_errno == ERESTART)
			continue;
		return (__RESULT_FAILURE);
	}
}

int
ip_close(int *fdp)
{
	int fd = *fdp;

	*fdp = 0;
	for (;;) {
		print_close(fdp);
		if (close(fd) >= 0) {
			print_success(fd);
			return __RESULT_SUCCESS;
		}
		print_errno(fd, (last_errno = errno));
		if (last_errno == EINTR || last_errno == ERESTART)
			continue;
		return __RESULT_FAILURE;
	}
}

int
ip_datack_req(int fd)
{
	int ret;

	data.len = -1;
	ctrl.len = sizeof(cmd.npi.datack_req) + sizeof(qos_data);
	cmd.prim = N_DATACK_REQ;
	bcopy(&qos_data, cbuf + sizeof(cmd.npi.datack_req), sizeof(qos_data));
	ret = put_msg(fd, 0, MSG_BAND, 0);
	return (ret);
}
#endif

/*
 *  -------------------------------------------------------------------------
 *
 *  Driver actions.
 *
 *  -------------------------------------------------------------------------
 */

int
test_waitsig(int child)
{
	int signum;
	sigset_t set;

	sigemptyset(&set);
	while ((signum = last_signum) == 0)
		sigsuspend(&set);
	print_signal(child, signum);
	return (__RESULT_SUCCESS);

}

int
test_ioctl(int child, int cmd, intptr_t arg)
{
	print_ioctl(child, cmd, arg);
	for (;;) {
		if ((last_retval = ioctl(test_fd[child], cmd, arg)) == -1) {
			print_errno(child, (last_errno = errno));
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			return (__RESULT_FAILURE);
		}
		if (show && verbose > 3)
			print_success_value(child, last_retval);
		return (__RESULT_SUCCESS);
	}
}

int
test_insertfd(int child, int resfd, int offset, struct strbuf *ctrl, struct strbuf *data, int flags)
{
	struct strfdinsert fdi;

	if (ctrl) {
		fdi.ctlbuf.maxlen = ctrl->maxlen;
		fdi.ctlbuf.len = ctrl->len;
		fdi.ctlbuf.buf = ctrl->buf;
	} else {
		fdi.ctlbuf.maxlen = -1;
		fdi.ctlbuf.len = 0;
		fdi.ctlbuf.buf = NULL;
	}
	if (data) {
		fdi.databuf.maxlen = data->maxlen;
		fdi.databuf.len = data->len;
		fdi.databuf.buf = data->buf;
	} else {
		fdi.databuf.maxlen = -1;
		fdi.databuf.len = 0;
		fdi.databuf.buf = NULL;
	}
	fdi.flags = flags;
	fdi.fildes = resfd;
	fdi.offset = offset;
	if (show && verbose > 4) {
		int i;

		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, "fdinsert to %d: [%d,%d]\n", child, ctrl ? ctrl->len : -1, data ? data->len : -1);
		fprintf(stdout, "[");
		for (i = 0; i < (ctrl ? ctrl->len : 0); i++)
			fprintf(stdout, "%02X", (uint8_t) ctrl->buf[i]);
		fprintf(stdout, "]\n");
		fprintf(stdout, "[");
		for (i = 0; i < (data ? data->len : 0); i++)
			fprintf(stdout, "%02X", (uint8_t) data->buf[i]);
		fprintf(stdout, "]\n");
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}
	if (test_ioctl(child, I_FDINSERT, (intptr_t) &fdi) != __RESULT_SUCCESS)
		return __RESULT_FAILURE;
	return __RESULT_SUCCESS;
}

int
test_putmsg(int child, struct strbuf *ctrl, struct strbuf *data, int flags)
{
	print_datcall(child, "putmsg(2)-----", data ? data->len : -1);
	if ((last_retval = putmsg(test_fd[child], ctrl, data, flags)) == -1) {
		print_errno(child, (last_errno = errno));
		return (__RESULT_FAILURE);
	}
	print_success_value(child, last_retval);
	return (__RESULT_SUCCESS);
}

int
test_putpmsg(int child, struct strbuf *ctrl, struct strbuf *data, int band, int flags)
{
	if (flags & MSG_BAND || band) {
		if ((verbose > 3 && show) || (verbose > 5 && show_msg)) {
			int i;

			dummy = lockf(fileno(stdout), F_LOCK, 0);
			fprintf(stdout, "putpmsg to %d: [%d,%d]\n", child, ctrl ? ctrl->len : -1, data ? data->len : -1);
			fprintf(stdout, "[");
			for (i = 0; i < (ctrl ? ctrl->len : 0); i++)
				fprintf(stdout, "%02X", (uint8_t) ctrl->buf[i]);
			fprintf(stdout, "]\n");
			fprintf(stdout, "[");
			for (i = 0; i < (data ? data->len : 0); i++)
				fprintf(stdout, "%02X", (uint8_t) data->buf[i]);
			fprintf(stdout, "]\n");
			fflush(stdout);
			dummy = lockf(fileno(stdout), F_ULOCK, 0);
		}
		if (ctrl == NULL || data != NULL)
			print_datcall(child, "M_DATA----------", data ? data->len : 0);
		for (;;) {
			if ((last_retval = putpmsg(test_fd[child], ctrl, data, band, flags)) == -1) {
				if (last_errno == EINTR || last_errno == ERESTART)
					continue;
				print_errno(child, (last_errno = errno));
				return (__RESULT_FAILURE);
			}
			if ((verbose > 3 && show) || (verbose > 5 && show_msg))
				print_success_value(child, last_retval);
			return (__RESULT_SUCCESS);
		}
	} else {
		if ((verbose > 3 && show) || (verbose > 5 && show_msg)) {
			dummy = lockf(fileno(stdout), F_LOCK, 0);
			fprintf(stdout, "putmsg to %d: [%d,%d]\n", child, ctrl ? ctrl->len : -1, data ? data->len : -1);
			dummy = lockf(fileno(stdout), F_ULOCK, 0);
			fflush(stdout);
		}
		if (ctrl == NULL || data != NULL)
			print_datcall(child, "M_DATA----------", data ? data->len : 0);
		for (;;) {
			if ((last_retval = putmsg(test_fd[child], ctrl, data, flags)) == -1) {
				if (last_errno == EINTR || last_errno == ERESTART)
					continue;
				print_errno(child, (last_errno = errno));
				return (__RESULT_FAILURE);
			}
			if ((verbose > 3 && show) || (verbose > 5 && show_msg))
				print_success_value(child, last_retval);
			return (__RESULT_SUCCESS);
		}
	}
}

int
test_write(int child, const void *buf, size_t len)
{
	print_syscall(child, "write(2)------");
	for (;;) {
		if ((last_retval = write(test_fd[child], buf, len)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_writev(int child, const struct iovec *iov, int num)
{
	print_syscall(child, "writev(2)-----");
	for (;;) {
		if ((last_retval = writev(test_fd[child], iov, num)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_getmsg(int child, struct strbuf *ctrl, struct strbuf *data, int *flagp)
{
	print_syscall(child, "getmsg(2)-----");
	for (;;) {
		if ((last_retval = getmsg(test_fd[child], ctrl, data, flagp)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_getpmsg(int child, struct strbuf *ctrl, struct strbuf *data, int *bandp, int *flagp)
{
	print_syscall(child, "getpmsg(2)----");
	for (;;) {
		if ((last_retval = getpmsg(test_fd[child], ctrl, data, bandp, flagp)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_read(int child, void *buf, size_t count)
{
	print_syscall(child, "read(2)-------");
	for (;;) {
		if ((last_retval = read(test_fd[child], buf, count)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_readv(int child, const struct iovec *iov, int count)
{
	print_syscall(child, "readv(2)------");
	for (;;) {
		if ((last_retval = readv(test_fd[child], iov, count)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_ti_ioctl(int child, int cmd, intptr_t arg)
{
	int tpi_error;

	if (cmd == I_STR && verbose > 3) {
		struct strioctl *icp = (struct strioctl *) arg;

		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, "ioctl from %d: cmd=%d, timout=%d, len=%d, dp=%p\n", child, icp->ic_cmd, icp->ic_timout, icp->ic_len, icp->ic_dp);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}
	print_ti_ioctl(child, cmd, arg);
	for (;;) {
		if ((last_retval = ioctl(test_fd[child], cmd, arg)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		if (show && verbose > 3)
			print_success_value(child, last_retval);
		break;
	}
	if (cmd == I_STR && show && verbose > 3) {
		struct strioctl *icp = (struct strioctl *) arg;

		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, "got ioctl from %d: cmd=%d, timout=%d, len=%d, dp=%p\n", child, icp->ic_cmd, icp->ic_timout, icp->ic_len, icp->ic_dp);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}
	if (last_retval == 0)
		return __RESULT_SUCCESS;
	tpi_error = last_retval & 0x00ff;
	if (tpi_error == TSYSERR)
		last_errno = (last_retval >> 8) & 0x00ff;
	else
		last_errno = 0;
	if (verbose) {
		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, "***************ERROR: ioctl failed\n");
		if (show && verbose > 3)
			fprintf(stdout, "                    : %s; result = %d\n", __FUNCTION__, last_retval);
		fprintf(stdout, "                    : %s; TPI error = %d\n", __FUNCTION__, tpi_error);
		if (tpi_error == TSYSERR)
			fprintf(stdout, "                    : %s; %s\n", __FUNCTION__, strerror(last_errno));
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
		fflush(stdout);
	}
	return (__RESULT_FAILURE);
}

int
test_nonblock(int child)
{
	long flags;

	print_syscall(child, "fcntl(2)------");
	for (;;) {
		if ((flags = last_retval = fcntl(test_fd[child], F_GETFL)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	print_syscall(child, "fcntl(2)------");
	for (;;) {
		if ((last_retval = fcntl(test_fd[child], F_SETFL, flags | O_NONBLOCK)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_block(int child)
{
	long flags;

	print_syscall(child, "fcntl(2)------");
	for (;;) {
		if ((flags = last_retval = fcntl(test_fd[child], F_GETFL)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	print_syscall(child, "fcntl(2)------");
	for (;;) {
		if ((last_retval = fcntl(test_fd[child], F_SETFL, flags & ~O_NONBLOCK)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

int
test_isastream(int child)
{
	int result;

	print_syscall(child, "isastream(2)--");
	for (;;) {
		if ((result = last_retval = isastream(test_fd[child])) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_success_value(child, last_retval);
		break;
	}
	return (__RESULT_SUCCESS);
}

#if 0
int
test_poll(int child, const short events, short *revents, long ms)
{
	struct pollfd pfd = {.fd = test_fd[child],.events = events,.revents = 0 };
	int result;

	print_poll(child, events);
	for (;;) {
		if ((result = last_retval = poll(&pfd, 1, ms)) == -1) {
			if (last_errno == EINTR || last_errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		}
		print_poll_value(child, last_retval, pfd.revents);
		if (last_retval == 1 && revents)
			*revents = pfd.revents;
		break;
	}
	return (__RESULT_SUCCESS);
}
#endif

int
test_pipe(int child)
{
	int fds[2];

	for (;;) {
		print_pipe(child);
		if (pipe(fds) >= 0) {
			test_fd[child + 0] = fds[0];
			test_fd[child + 1] = fds[1];
			print_success(child);
			return (__RESULT_SUCCESS);
		}
		if (last_errno == EINTR || last_errno == ERESTART)
			continue;
		print_errno(child, (last_errno = errno));
		return (__RESULT_FAILURE);
	}
}

int
test_fopen(int child, const char *name, int flags)
{
	int fd;

	print_open(child, name);
	if ((fd = open(name, flags)) >= 0) {
		print_success(child);
		return (fd);
	}
	print_errno(child, (last_errno = errno));
	return (fd);
}

int
test_fclose(int child, int fd)
{
	print_close(child);
	if (close(fd) >= 0) {
		print_success(child);
		return __RESULT_SUCCESS;
	}
	print_errno(child, (last_errno = errno));
	return __RESULT_FAILURE;
}

int
test_open(int child, const char *name, int flags)
{
	int fd;

	for (;;) {
		print_open(child, name);
		if ((fd = open(name, flags)) >= 0) {
			test_fd[child] = fd;
			print_success(child);
			return (__RESULT_SUCCESS);
		}
		if (last_errno == EINTR || last_errno == ERESTART)
			continue;
		print_errno(child, (last_errno = errno));
		return (__RESULT_FAILURE);
	}
}

int
test_close(int child)
{
	int fd = test_fd[child];

	test_fd[child] = 0;
	for (;;) {
		print_close(child);
		if (close(fd) >= 0) {
			print_success(child);
			return __RESULT_SUCCESS;
		}
		if (last_errno == EINTR || last_errno == ERESTART)
			continue;
		print_errno(child, (last_errno = errno));
		return __RESULT_FAILURE;
	}
}

int
test_push(int child, const char *name)
{
	if (show && verbose > 1)
		print_command_state(child, ":push");
	if (test_ioctl(child, I_PUSH, (intptr_t) name))
		return __RESULT_FAILURE;
	return __RESULT_SUCCESS;
}

int
test_pop(int child)
{
	if (show && verbose > 1)
		print_command_state(child, ":pop");
	if (test_ioctl(child, I_POP, (intptr_t) 0))
		return __RESULT_FAILURE;
	return __RESULT_SUCCESS;
}

/*
 *  -------------------------------------------------------------------------
 *
 *  STREAM Initialization
 *
 *  -------------------------------------------------------------------------
 */

static int
stream_start(int child, int index)
{
	int offset = 3 * index;
	int i;

	for (i = 0; i < anums[3]; i++) {
#ifndef SCTP_VERSION_2
		addrs[3].port = htons(ports[3] + offset);
		inet_aton(addr_strings[i], &addrs[child].addr[i]);
#else				/* SCTP_VERSION_2 */
		addrs[3][i].sin_family = AF_INET;
		addrs[3][i].sin_port = htons(ports[3] + offset);
		inet_aton(addr_strings[i], &addrs[3][i].sin_addr);
#endif				/* SCTP_VERSION_2 */
	}
	switch (child) {
	case 1:
#if 1
		/* set up the test harness */
		if (test_pipe(0) != __RESULT_SUCCESS)
			goto failure;
		if (test_ioctl(0, I_SRDOPT, (intptr_t) RMSGD) != __RESULT_SUCCESS)
			goto failure;
		if (test_ioctl(1, I_SRDOPT, (intptr_t) RMSGD) != __RESULT_SUCCESS)
			goto failure;
		if (test_ioctl(0, I_SWROPT, (intptr_t) SNDZERO) != __RESULT_SUCCESS)
			goto failure;
		if (test_ioctl(1, I_SWROPT, (intptr_t) SNDZERO) != __RESULT_SUCCESS)
			goto failure;
		if (test_ioctl(1, I_PUSH, (intptr_t) "pipemod") != __RESULT_SUCCESS)
			goto failure;
		return __RESULT_SUCCESS;
#endif
	case 2:
#if 1
		return __RESULT_SUCCESS;
#endif
	case 0:
#if 0
		for (i = 0; i < 3; i++) {
#ifndef SCTP_VERSION_2
			addrs[child].port = htons(ports[child] + offset);
			inet_aton(addr_strings[i], &addrs[child].addr[i]);
#else				/* SCTP_VERSION_2 */
			addrs[child][i].sin_family = AF_INET;
			addrs[child][i].sin_port = htons(ports[child] + offset);
			inet_aton(addr_strings[i], &addrs[child][i].sin_addr);
#endif				/* SCTP_VERSION_2 */
		}
		if (test_open(child, devname) != __RESULT_SUCCESS)
			goto failure;
		if (test_ioctl(child, I_SRDOPT, (intptr_t) RMSGD) != __RESULT_SUCCESS)
			goto failure;
#endif
		return __RESULT_SUCCESS;
	default:
	      failure:
		return __RESULT_FAILURE;
	}
}

static int
stream_stop(int child)
{
	switch (child) {
	case 1:
#if 1
		if (test_ioctl(1, I_POP, (intptr_t) NULL) != __RESULT_SUCCESS)
			return __RESULT_FAILURE;
		if (test_close(0) != __RESULT_SUCCESS)
			return __RESULT_FAILURE;
		if (test_close(1) != __RESULT_SUCCESS)
			return __RESULT_FAILURE;
		return __RESULT_SUCCESS;
#endif
	case 2:
#if 1
		return __RESULT_SUCCESS;
#endif
	case 0:
#if 0
		if (test_close(child) != __RESULT_SUCCESS)
			return __RESULT_FAILURE;
#endif
		return __RESULT_SUCCESS;
	default:
		return __RESULT_FAILURE;
	}
}

void
test_sleep(int child, unsigned long t)
{
	print_waiting(child, t);
	sleep(t);
}

void
test_msleep(int child, unsigned long m)
{
	struct timespec time;

	time.tv_sec = m / 1000;
	time.tv_nsec = (m % 1000) * 1000000;
	print_mwaiting(child, &time);
	nanosleep(&time, NULL);
}

/*
 *  -------------------------------------------------------------------------
 *
 *  Test harness initialization and termination.
 *
 *  -------------------------------------------------------------------------
 */

static int
begin_tests(int index)
{
	state = 0;
	if (stream_start(0, index) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (stream_start(1, index) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (stream_start(2, index) != __RESULT_SUCCESS)
		goto failure;
	state++;
	show_acks = 1;
	return __RESULT_SUCCESS;
      failure:
	return __RESULT_FAILURE;
}

static int
end_tests(int index)
{
	show_acks = 0;
	if (stream_stop(2) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (stream_stop(1) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (stream_stop(0) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return __RESULT_SUCCESS;
      failure:
	return __RESULT_FAILURE;
}

int
begin_tests_p(int index)
{
	if (begin_tests(index) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (test_push(0, "tpiperf") != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (test_push(1, "tpiperf") != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (test_push(2, "tpiperf") != __RESULT_SUCCESS)
		goto failure;
	state++;
	return __RESULT_SUCCESS;
      failure:
	return __RESULT_FAILURE;
}

int
end_tests_p(int index)
{
	if (test_pop(2) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (test_pop(1) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (test_pop(0) != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (end_tests(index) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return __RESULT_SUCCESS;
      failure:
	return __RESULT_FAILURE;
}

/*
 *  -------------------------------------------------------------------------
 *
 *  Injected event encoding and display functions.
 *
 *  -------------------------------------------------------------------------
 */

static int
do_signal(int child, int action)
{
	struct strbuf ctrl_buf, data_buf, *ctrl = &ctrl_buf, *data = &data_buf;
	char cbuf[BUFSIZE], dbuf[BUFSIZE];
	union T_primitives *p = (typeof(p)) cbuf;
	struct strioctl ic;

	ic.ic_cmd = 0;
	ic.ic_timout = test_timout;
	ic.ic_len = sizeof(cbuf);
	ic.ic_dp = cbuf;
	ctrl->maxlen = 0;
	ctrl->buf = cbuf;
	data->maxlen = 0;
	data->buf = dbuf;
	test_pflags = MSG_BAND;
	test_pband = 0;
	switch (action) {
	case __TEST_WRITE:
		data->len = snprintf(dbuf, BUFSIZE, "%s", "Write test data.");
		return test_write(child, dbuf, data->len);
	case __TEST_WRITEV:
	{
		struct iovec vector[4];

		vector[0].iov_base = dbuf;
		vector[0].iov_len = sprintf(vector[0].iov_base, "Writev test datum for vector 0.");
		vector[1].iov_base = dbuf + vector[0].iov_len;
		vector[1].iov_len = sprintf(vector[1].iov_base, "Writev test datum for vector 1.");
		vector[2].iov_base = dbuf + vector[1].iov_len;
		vector[2].iov_len = sprintf(vector[2].iov_base, "Writev test datum for vector 2.");
		vector[3].iov_base = dbuf + vector[2].iov_len;
		vector[3].iov_len = sprintf(vector[3].iov_base, "Writev test datum for vector 3.");
		return test_writev(child, vector, 4);
	}
	}
	switch (action) {
	case __TEST_PUSH:
		return test_push(child, "tirdwr");
	case __TEST_POP:
		return test_pop(child);
	case __TEST_PUTMSG_DATA:
		ctrl = NULL;
		data->len = snprintf(dbuf, BUFSIZE, "%s", "Putmsg test data.");
		test_pflags = MSG_BAND;
		test_pband = 0;
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_PUTPMSG_DATA:
		ctrl = NULL;
		data->len = snprintf(dbuf, BUFSIZE, "%s", "Putpmsg band test data.");
		test_pflags = MSG_BAND;
		test_pband = 1;
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_CONN_REQ:
		ctrl->len = sizeof(p->conn_req)
		    + (test_addr ? test_alen : 0)
		    + (test_opts ? test_olen : 0);
		p->conn_req.PRIM_type = T_CONN_REQ;
		p->conn_req.DEST_length = test_addr ? test_alen : 0;
		p->conn_req.DEST_offset = test_addr ? sizeof(p->conn_req) : 0;
		p->conn_req.OPT_length = test_opts ? test_olen : 0;
		p->conn_req.OPT_offset = test_opts ? sizeof(p->conn_req) + p->conn_req.DEST_length : 0;
		if (test_addr)
			bcopy(test_addr, ctrl->buf + p->conn_req.DEST_offset, p->conn_req.DEST_length);
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->conn_req.OPT_offset, p->conn_req.OPT_length);
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
		print_string(child, addr_string(cbuf + p->conn_req.DEST_offset, p->conn_req.DEST_length));
#else
		print_addrs(child, cbuf + p->conn_req.DEST_offset, p->conn_req.DEST_length);
#endif
		print_options(child, cbuf, p->conn_req.OPT_offset, p->conn_req.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_CONN_IND:
		ctrl->len = sizeof(p->conn_ind);
		p->conn_ind.PRIM_type = T_CONN_IND;
		p->conn_ind.SRC_length = 0;
		p->conn_ind.SRC_offset = 0;
		p->conn_ind.OPT_length = 0;
		p->conn_ind.OPT_offset = 0;
		p->conn_ind.SEQ_number = last_sequence;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_options(child, cbuf, p->conn_ind.OPT_offset, p->conn_ind.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_CONN_RES:
		ctrl->len = sizeof(p->conn_res)
		    + (test_opts ? test_olen : 0);
		p->conn_res.PRIM_type = T_CONN_RES;
		p->conn_res.ACCEPTOR_id = 0;
		p->conn_res.OPT_length = test_opts ? test_olen : 0;
		p->conn_res.OPT_offset = test_opts ? sizeof(p->conn_res) : 0;
		p->conn_res.SEQ_number = last_sequence;
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->conn_res.OPT_offset, p->conn_res.OPT_length);
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_options(child, cbuf, p->conn_res.OPT_offset, p->conn_res.OPT_length);
		if (test_resfd == -1)
			return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
		else
			return test_insertfd(child, test_resfd, 4, ctrl, data, 0);
	case __TEST_CONN_CON:
		ctrl->len = sizeof(p->conn_con);
		p->conn_con.PRIM_type = T_CONN_CON;
		p->conn_con.RES_length = 0;
		p->conn_con.RES_offset = 0;
		p->conn_con.OPT_length = 0;
		p->conn_con.OPT_offset = 0;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_options(child, cbuf, p->conn_con.OPT_offset, p->conn_con.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_DISCON_REQ:
		ctrl->len = sizeof(p->discon_req);
		p->discon_req.PRIM_type = T_DISCON_REQ;
		p->discon_req.SEQ_number = last_sequence;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_DISCON_IND:
		ctrl->len = sizeof(p->discon_ind);
		p->discon_ind.PRIM_type = T_DISCON_IND;
		p->discon_ind.DISCON_reason = 0;
		p->discon_ind.SEQ_number = last_sequence;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_DATA_REQ:
		ctrl->len = sizeof(p->data_req);
		p->data_req.PRIM_type = T_DATA_REQ;
		p->data_req.MORE_flag = MORE_flag;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		if ((verbose && show) || verbose > 4) {
			print_tx_prim(child, prim_string(p->type));
			if (data)
				print_tx_data(child, "M_DATA----------", data->len);
		}
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_DATA_IND:
		ctrl->len = sizeof(p->data_ind);
		p->data_ind.PRIM_type = T_DATA_IND;
		p->data_ind.MORE_flag = MORE_flag;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		if ((verbose && show) || verbose > 4)
			print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_EXDATA_REQ:
		ctrl->len = sizeof(p->exdata_req);
		p->exdata_req.PRIM_type = T_EXDATA_REQ;
		p->exdata_req.MORE_flag = MORE_flag;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 1;
		if ((verbose && show) || verbose > 4) {
			print_tx_prim(child, prim_string(p->type));
			if (data)
				print_tx_data(child, "M_DATA----------", data->len);
		}
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_EXDATA_IND:
		ctrl->len = sizeof(p->exdata_ind);
		p->data_ind.PRIM_type = T_EXDATA_IND;
		p->data_ind.MORE_flag = MORE_flag;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 1;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_INFO_REQ:
		ctrl->len = sizeof(p->info_req);
		p->info_req.PRIM_type = T_INFO_REQ;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_INFO_ACK:
		ctrl->len = sizeof(p->info_ack);
		p->info_ack.PRIM_type = T_INFO_ACK;
		p->info_ack.TSDU_size = test_bufsize;
		p->info_ack.ETSDU_size = test_bufsize;
		p->info_ack.CDATA_size = test_bufsize;
		p->info_ack.DDATA_size = test_bufsize;
		p->info_ack.ADDR_size = test_bufsize;
		p->info_ack.OPT_size = test_bufsize;
		p->info_ack.TIDU_size = test_tidu;
		p->info_ack.SERV_type = last_servtype;
		p->info_ack.CURRENT_state = last_tstate;
		p->info_ack.PROVIDER_flag = last_provflag;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_info(child, &p->info_ack);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_BIND_REQ:
		ctrl->len = sizeof(p->bind_req) + (test_addr ? test_alen : 0);
		p->bind_req.PRIM_type = T_BIND_REQ;
		p->bind_req.ADDR_length = test_addr ? test_alen : 0;
		p->bind_req.ADDR_offset = test_addr ? sizeof(p->bind_req) : 0;
		p->bind_req.CONIND_number = last_qlen;
		if (test_addr)
			bcopy(test_addr, ctrl->buf + p->bind_req.ADDR_offset, p->bind_req.ADDR_length);
		data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
		print_string(child, addr_string(cbuf + p->bind_ack.ADDR_offset, p->bind_ack.ADDR_length));
#else
		print_addrs(child, cbuf + p->bind_ack.ADDR_offset, p->bind_ack.ADDR_length);
#endif
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_BIND_ACK:
		ctrl->len = sizeof(p->bind_ack);
		p->bind_ack.PRIM_type = T_BIND_ACK;
		p->bind_ack.ADDR_length = 0;
		p->bind_ack.ADDR_offset = 0;
		p->bind_ack.CONIND_number = last_qlen;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
		print_string(child, addr_string(cbuf + p->bind_ack.ADDR_offset, p->bind_ack.ADDR_length));
#else
		print_addrs(child, cbuf + p->bind_ack.ADDR_offset, p->bind_ack.ADDR_length);
#endif
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_UNBIND_REQ:
		ctrl->len = sizeof(p->unbind_req);
		p->unbind_req.PRIM_type = T_UNBIND_REQ;
		data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_ERROR_ACK:
		ctrl->len = sizeof(p->error_ack);
		p->error_ack.PRIM_type = T_ERROR_ACK;
		p->error_ack.ERROR_prim = last_prim;
		p->error_ack.TLI_error = last_t_errno;
		p->error_ack.UNIX_error = last_errno;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_string(child, terrno_string(p->error_ack.TLI_error, p->error_ack.UNIX_error));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_OK_ACK:
		ctrl->len = sizeof(p->ok_ack);
		p->ok_ack.PRIM_type = T_OK_ACK;
		p->ok_ack.CORRECT_prim = 0;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_UNITDATA_REQ:
		ctrl->len = sizeof(p->unitdata_req)
		    + (test_addr ? test_alen : 0)
		    + (test_opts ? test_olen : 0);
		p->unitdata_req.PRIM_type = T_UNITDATA_REQ;
		p->unitdata_req.DEST_length = test_addr ? test_alen : 0;
		p->unitdata_req.DEST_offset = test_addr ? sizeof(p->unitdata_req) : 0;
		p->unitdata_req.OPT_length = test_opts ? test_olen : 0;
		p->unitdata_req.OPT_offset = test_opts ? sizeof(p->unitdata_req) + p->unitdata_req.DEST_length : 0;
		if (test_addr)
			bcopy(test_addr, ctrl->buf + p->unitdata_req.DEST_offset, p->unitdata_req.DEST_length);
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->unitdata_req.OPT_offset, p->unitdata_req.OPT_length);
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
		print_string(child, addr_string(cbuf + p->unitdata_req.DEST_offset, p->unitdata_req.DEST_length));
#else
		print_addrs(child, cbuf + p->unitdata_req.DEST_offset, p->unitdata_req.DEST_length);
#endif
		print_options(child, cbuf, p->unitdata_req.OPT_offset, p->unitdata_req.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_UNITDATA_IND:
		ctrl->len = sizeof(p->unitdata_ind);
		p->unitdata_ind.PRIM_type = T_UNITDATA_IND;
		p->unitdata_ind.SRC_length = 0;
		p->unitdata_ind.SRC_offset = 0;
		p->unitdata_ind.OPT_length = 0;
		p->unitdata_ind.OPT_offset = 0;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
		print_string(child, addr_string(cbuf + p->unitdata_ind.SRC_offset, p->unitdata_ind.SRC_length));
#else
		print_addrs(child, cbuf + p->unitdata_ind.SRC_offset, p->unitdata_ind.SRC_length);
#endif
		print_options(child, cbuf, p->unitdata_ind.OPT_offset, p->unitdata_ind.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_UDERROR_IND:
		ctrl->len = sizeof(p->uderror_ind);
		p->uderror_ind.PRIM_type = T_UDERROR_IND;
		p->uderror_ind.DEST_length = 0;
		p->uderror_ind.DEST_offset = 0;
		p->uderror_ind.OPT_length = 0;
		p->uderror_ind.OPT_offset = 0;
		p->uderror_ind.ERROR_type = 0;
		data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_OPTMGMT_REQ:
		ctrl->len = sizeof(p->optmgmt_req)
		    + (test_opts ? test_olen : 0);
		p->optmgmt_req.PRIM_type = T_OPTMGMT_REQ;
		p->optmgmt_req.OPT_length = test_opts ? test_olen : 0;
		p->optmgmt_req.OPT_offset = test_opts ? sizeof(p->optmgmt_req) : 0;
		p->optmgmt_req.MGMT_flags = test_mgmtflags;
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->optmgmt_req.OPT_offset, p->optmgmt_req.OPT_length);
		data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_mgmtflag(child, p->optmgmt_req.MGMT_flags);
		print_options(child, cbuf, p->optmgmt_req.OPT_offset, p->optmgmt_req.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_OPTMGMT_ACK:
		ctrl->len = sizeof(p->optmgmt_ack);
		p->optmgmt_ack.PRIM_type = T_OPTMGMT_ACK;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		print_mgmtflag(child, p->optmgmt_ack.MGMT_flags);
		print_options(child, cbuf, p->optmgmt_ack.OPT_offset, p->optmgmt_ack.OPT_length);
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_ORDREL_REQ:
		ctrl->len = sizeof(p->ordrel_req);
		p->ordrel_req.PRIM_type = T_ORDREL_REQ;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_ORDREL_IND:
		ctrl->len = sizeof(p->ordrel_ind);
		p->ordrel_ind.PRIM_type = T_ORDREL_IND;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_OPTDATA_REQ:
		ctrl->len = sizeof(p->optdata_req)
		    + (test_opts ? test_olen : 0);
		p->optdata_req.PRIM_type = T_OPTDATA_REQ;
		p->optdata_req.DATA_flag = DATA_flag;
		p->optdata_req.OPT_length = test_opts ? test_olen : 0;
		p->optdata_req.OPT_offset = test_opts ? sizeof(p->optdata_req) : 0;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->optdata_req.OPT_offset, p->optdata_req.OPT_length);
		test_pflags = MSG_BAND;
		test_pband = (DATA_flag & T_ODF_EX) ? 1 : 0;
		if (p->optdata_req.DATA_flag & T_ODF_EX) {
			if ((verbose && show) || verbose > 4) {
				if (p->optdata_req.DATA_flag & T_ODF_MORE)
					print_tx_prim(child, "T_OPTDATA_REQ!+ ");
				else
					print_tx_prim(child, "T_OPTDATA_REQ!  ");
			}
		} else {
			if ((verbose && show) || verbose > 4) {
				if (p->optdata_req.DATA_flag & T_ODF_MORE)
					print_tx_prim(child, "T_OPTDATA_REQ+  ");
				else
					print_tx_prim(child, "T_OPTDATA_REQ   ");
			}
		}
		if ((verbose && show) || verbose > 4) {
			print_options(child, cbuf, p->optdata_req.OPT_offset, p->optdata_req.OPT_length);
			if (data)
				print_tx_data(child, "M_DATA----------", data->len);
		}
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_NRM_OPTDATA_IND:
		ctrl->len = sizeof(p->optdata_ind);
		p->optdata_ind.PRIM_type = T_OPTDATA_IND;
		p->optdata_ind.DATA_flag = 0;
		p->optdata_ind.OPT_length = 0;
		p->optdata_ind.OPT_offset = 0;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		if ((verbose && show) || verbose > 4) {
			if (p->optdata_ind.DATA_flag & T_ODF_MORE)
				print_tx_prim(child, "T_OPTDATA_IND+  ");
			else
				print_tx_prim(child, "T_OPTDATA_IND   ");
			print_options(child, cbuf, p->optdata_ind.OPT_offset, p->optdata_ind.OPT_length);
			if (data)
				print_tx_data(child, "M_DATA----------", data->len);
		}
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_EXP_OPTDATA_IND:
		ctrl->len = sizeof(p->optdata_ind);
		p->optdata_ind.PRIM_type = T_OPTDATA_IND;
		p->optdata_ind.DATA_flag = T_ODF_EX;
		p->optdata_ind.OPT_length = 0;
		p->optdata_ind.OPT_offset = 0;
		if (test_data)
			data->len = snprintf(dbuf, BUFSIZE, "%s", test_data);
		else
			data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 1;
		if ((verbose && show) || verbose > 4) {
			if (p->optdata_ind.DATA_flag & T_ODF_MORE)
				print_tx_prim(child, "T_OPTDATA_IND!+ ");
			else
				print_tx_prim(child, "T_OPTDATA_IND!  ");
			print_options(child, cbuf, p->optdata_ind.OPT_offset, p->optdata_ind.OPT_length);
			if (data)
				print_tx_data(child, "M_DATA----------", data->len);
		}
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_ADDR_REQ:
		ctrl->len = sizeof(p->addr_req);
		p->addr_req.PRIM_type = T_ADDR_REQ;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_ADDR_ACK:
		ctrl->len = sizeof(p->addr_ack);
		p->addr_ack.PRIM_type = T_ADDR_ACK;
		p->addr_ack.LOCADDR_length = 0;
		p->addr_ack.LOCADDR_offset = 0;
		p->addr_ack.REMADDR_length = 0;
		p->addr_ack.REMADDR_offset = 0;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
		print_string(child, addr_string(cbuf + p->addr_ack.LOCADDR_offset, p->addr_ack.LOCADDR_length));
		print_string(child, addr_string(cbuf + p->addr_ack.REMADDR_offset, p->addr_ack.REMADDR_length));
#else
		print_addrs(child, cbuf + p->addr_ack.LOCADDR_offset, p->addr_ack.LOCADDR_length);
		print_addrs(child, cbuf + p->addr_ack.REMADDR_offset, p->addr_ack.REMADDR_length);
#endif
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_CAPABILITY_REQ:
		ctrl->len = sizeof(p->capability_req);
		p->capability_req.PRIM_type = T_CAPABILITY_REQ;
		p->capability_req.CAP_bits1 = TC1_INFO | TC1_ACCEPTOR_ID;
		data = NULL;
		test_pflags = test_prio ? MSG_HIPRI : MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_CAPABILITY_ACK:
		ctrl->len = sizeof(p->capability_ack);
		p->capability_ack.PRIM_type = T_CAPABILITY_ACK;
		p->capability_ack.CAP_bits1 = TC1_INFO | TC1_ACCEPTOR_ID;
		p->capability_ack.INFO_ack.TSDU_size = test_bufsize;
		p->capability_ack.INFO_ack.ETSDU_size = test_bufsize;
		p->capability_ack.INFO_ack.CDATA_size = test_bufsize;
		p->capability_ack.INFO_ack.DDATA_size = test_bufsize;
		p->capability_ack.INFO_ack.ADDR_size = test_bufsize;
		p->capability_ack.INFO_ack.OPT_size = test_bufsize;
		p->capability_ack.INFO_ack.TIDU_size = test_tidu;
		p->capability_ack.INFO_ack.SERV_type = last_servtype;
		p->capability_ack.INFO_ack.CURRENT_state = last_tstate;
		p->capability_ack.INFO_ack.PROVIDER_flag = last_provflag;
		p->capability_ack.ACCEPTOR_id = 0;
		data = NULL;
		test_pflags = MSG_HIPRI;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_PRIM_TOO_SHORT:
		ctrl->len = sizeof(p->type);
		p->type = last_prim;
		data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_PRIM_WAY_TOO_SHORT:
		ctrl->len = sizeof(p->type) >> 1;
		p->type = last_prim;
		data = NULL;
		test_pflags = MSG_BAND;
		test_pband = 0;
		print_tx_prim(child, prim_string(p->type));
		return test_putpmsg(child, ctrl, data, test_pband, test_pflags);
	case __TEST_O_TI_GETINFO:
		ic.ic_cmd = O_TI_GETINFO;
		ic.ic_len = sizeof(p->info_ack);
		p->info_req.PRIM_type = T_INFO_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_O_TI_OPTMGMT:
		ic.ic_cmd = O_TI_OPTMGMT;
		ic.ic_len = sizeof(p->optmgmt_ack)
		    + (test_opts ? test_olen : 0);
		p->optmgmt_req.PRIM_type = T_OPTMGMT_REQ;
		p->optmgmt_req.OPT_length = test_opts ? test_olen : 0;
		p->optmgmt_req.OPT_offset = test_opts ? sizeof(p->optmgmt_req) : 0;
		p->optmgmt_req.MGMT_flags = test_mgmtflags;
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->optmgmt_req.OPT_offset, p->optmgmt_req.OPT_length);
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_O_TI_BIND:
		ic.ic_cmd = O_TI_BIND;
		ic.ic_len = sizeof(p->bind_ack);
		p->bind_req.PRIM_type = T_BIND_REQ;
		p->bind_req.ADDR_length = 0;
		p->bind_req.ADDR_offset = 0;
		p->bind_req.CONIND_number = last_qlen;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_O_TI_UNBIND:
		ic.ic_cmd = O_TI_UNBIND;
		ic.ic_len = sizeof(p->ok_ack);
		p->unbind_req.PRIM_type = T_UNBIND_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_GETINFO:
		ic.ic_cmd = _O_TI_GETINFO;
		ic.ic_len = sizeof(p->info_ack);
		p->info_req.PRIM_type = T_INFO_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_OPTMGMT:
		ic.ic_cmd = _O_TI_OPTMGMT;
		ic.ic_len = sizeof(p->optmgmt_ack)
		    + (test_opts ? test_olen : 0);
		p->optmgmt_req.PRIM_type = T_OPTMGMT_REQ;
		p->optmgmt_req.OPT_length = test_opts ? test_olen : 0;
		p->optmgmt_req.OPT_offset = test_opts ? sizeof(p->optmgmt_req) : 0;
		p->optmgmt_req.MGMT_flags = test_mgmtflags;
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->optmgmt_req.OPT_offset, p->optmgmt_req.OPT_length);
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_BIND:
		ic.ic_cmd = _O_TI_BIND;
		ic.ic_len = sizeof(p->bind_ack);
		p->bind_req.PRIM_type = T_BIND_REQ;
		p->bind_req.ADDR_length = 0;
		p->bind_req.ADDR_offset = 0;
		p->bind_req.CONIND_number = last_qlen;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_UNBIND:
		ic.ic_cmd = _O_TI_UNBIND;
		ic.ic_len = sizeof(p->ok_ack);
		p->unbind_req.PRIM_type = T_UNBIND_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_GETMYNAME:
		ic.ic_cmd = _O_TI_GETMYNAME;
		ic.ic_len = sizeof(p->addr_ack);
		p->addr_req.PRIM_type = T_ADDR_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_GETPEERNAME:
		ic.ic_cmd = _O_TI_GETPEERNAME;
		ic.ic_len = sizeof(p->addr_ack);
		p->addr_req.PRIM_type = T_ADDR_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_XTI_HELLO:
		ic.ic_cmd = _O_TI_XTI_HELLO;
		ic.ic_len = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_XTI_GET_STATE:
		ic.ic_cmd = _O_TI_XTI_GET_STATE;
		ic.ic_len = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_XTI_CLEAR_EVENT:
		ic.ic_cmd = _O_TI_XTI_CLEAR_EVENT;
		ic.ic_len = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_XTI_MODE:
		ic.ic_cmd = _O_TI_XTI_MODE;
		ic.ic_len = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST__O_TI_TLI_MODE:
		ic.ic_cmd = _O_TI_TLI_MODE;
		ic.ic_len = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_GETINFO:
		ic.ic_cmd = TI_GETINFO;
		ic.ic_len = sizeof(p->info_ack);
		p->info_req.PRIM_type = T_INFO_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_OPTMGMT:
		ic.ic_cmd = TI_OPTMGMT;
		ic.ic_len = sizeof(p->optmgmt_ack)
		    + (test_opts ? test_olen : 0);
		p->optmgmt_req.PRIM_type = T_OPTMGMT_REQ;
		p->optmgmt_req.OPT_length = test_opts ? test_olen : 0;
		p->optmgmt_req.OPT_offset = test_opts ? sizeof(p->optmgmt_req) : 0;
		p->optmgmt_req.MGMT_flags = test_mgmtflags;
		if (test_opts)
			bcopy(test_opts, ctrl->buf + p->optmgmt_req.OPT_offset, p->optmgmt_req.OPT_length);
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_BIND:
		ic.ic_cmd = TI_BIND;
		ic.ic_len = sizeof(p->bind_ack);
		p->bind_req.PRIM_type = T_BIND_REQ;
		p->bind_req.ADDR_length = 0;
		p->bind_req.ADDR_offset = 0;
		p->bind_req.CONIND_number = last_qlen;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_UNBIND:
		ic.ic_cmd = TI_UNBIND;
		ic.ic_len = sizeof(p->ok_ack);
		p->unbind_req.PRIM_type = T_UNBIND_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_GETMYNAME:
		ic.ic_cmd = TI_GETMYNAME;
		ic.ic_len = sizeof(p->addr_ack);
		p->addr_req.PRIM_type = T_ADDR_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_GETPEERNAME:
		ic.ic_cmd = TI_GETPEERNAME;
		ic.ic_len = sizeof(p->addr_ack);
		p->addr_req.PRIM_type = T_ADDR_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETMYNAME:
		ic.ic_cmd = TI_SETMYNAME;
		ic.ic_len = sizeof(p->conn_res);
		p->conn_res.PRIM_type = T_CONN_RES;
		p->conn_res.ACCEPTOR_id = 0;
		p->conn_res.OPT_length = 0;
		p->conn_res.OPT_offset = 0;
		p->conn_res.SEQ_number = last_sequence;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETPEERNAME:
		ic.ic_cmd = TI_SETPEERNAME;
		ic.ic_len = sizeof(p->conn_req);
		p->conn_req.PRIM_type = T_CONN_REQ;
		p->conn_req.DEST_length = 0;
		p->conn_req.DEST_offset = 0;
		p->conn_req.OPT_length = 0;
		p->conn_req.OPT_offset = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETMYNAME_DISC:
		ic.ic_cmd = TI_SETMYNAME;
		ic.ic_len = sizeof(p->discon_req);
		p->discon_req.PRIM_type = T_DISCON_REQ;
		p->discon_req.SEQ_number = last_sequence;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETPEERNAME_DISC:
		ic.ic_cmd = TI_SETPEERNAME;
		ic.ic_len = sizeof(p->discon_req);
		p->discon_req.PRIM_type = T_DISCON_REQ;
		p->discon_req.SEQ_number = last_sequence;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETMYNAME_DATA:
		ic.ic_cmd = TI_SETMYNAME;
		ic.ic_len = sizeof(p->conn_res) + sprintf(cbuf + sizeof(p->conn_res), "IO control test data.");
		p->conn_res.PRIM_type = T_CONN_RES;
		p->conn_res.ACCEPTOR_id = 0;
		p->conn_res.OPT_length = 0;
		p->conn_res.OPT_offset = 0;
		p->conn_res.SEQ_number = last_sequence;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETPEERNAME_DATA:
		ic.ic_cmd = TI_SETPEERNAME;
		ic.ic_len = sizeof(p->conn_req) + sprintf(cbuf + sizeof(p->conn_res), "IO control test data.");
		p->conn_req.PRIM_type = T_CONN_REQ;
		p->conn_req.DEST_length = 0;
		p->conn_req.DEST_offset = 0;
		p->conn_req.OPT_length = 0;
		p->conn_req.OPT_offset = 0;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETMYNAME_DISC_DATA:
		ic.ic_cmd = TI_SETMYNAME;
		ic.ic_len = sizeof(p->discon_req) + sprintf(cbuf + sizeof(p->conn_res), "IO control test data.");
		p->discon_req.PRIM_type = T_DISCON_REQ;
		p->discon_req.SEQ_number = last_sequence;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SETPEERNAME_DISC_DATA:
		ic.ic_cmd = TI_SETPEERNAME;
		ic.ic_len = sizeof(p->discon_req) + sprintf(cbuf + sizeof(p->conn_res), "IO control test data.");
		p->discon_req.PRIM_type = T_DISCON_REQ;
		p->discon_req.SEQ_number = last_sequence;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_SYNC:
	{
		union {
			struct ti_sync_req req;
			struct ti_sync_ack ack;
		} *s = (typeof(s)) p;

		ic.ic_cmd = TI_SYNC;
		ic.ic_len = sizeof(*s);
		s->req.tsr_flags = TSRF_INFO_REQ | TSRF_IS_EXP_IN_RCVBUF | TSRF_QLEN_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	}
	case __TEST_TI_GETADDRS:
		ic.ic_cmd = TI_GETADDRS;
		ic.ic_len = sizeof(p->addr_ack);
		p->addr_req.PRIM_type = T_ADDR_REQ;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	case __TEST_TI_CAPABILITY:
		ic.ic_cmd = TI_CAPABILITY;
		ic.ic_len = sizeof(p->capability_ack);
		p->capability_req.PRIM_type = T_CAPABILITY_REQ;
		p->capability_req.CAP_bits1 = TC1_INFO | TC1_ACCEPTOR_ID;
		return test_ti_ioctl(child, I_STR, (intptr_t) &ic);
	default:
		return __RESULT_SCRIPT_ERROR;
	}
	return __RESULT_SCRIPT_ERROR;
}

/*
 *  -------------------------------------------------------------------------
 *
 *  Received event decoding and display functions.
 *
 *  -------------------------------------------------------------------------
 */

static int
do_decode_data(int child, struct strbuf *ctrl, struct strbuf *data)
{
	int event = __RESULT_DECODE_ERROR;

	if (data->len >= 0) {
		event = __TEST_DATA;
		print_rx_data(child, "M_DATA----------", data->len);
	}
	return ((last_event = event));
}

static int
do_decode_ctrl(int child, struct strbuf *ctrl, struct strbuf *data)
{
	int event = __RESULT_DECODE_ERROR;
	union T_primitives *p = (union T_primitives *) ctrl->buf;

	if (ctrl->len >= sizeof(p->type)) {
		switch ((last_prim = p->type)) {
		case T_CONN_REQ:
			event = __TEST_CONN_REQ;
			print_rx_prim(child, prim_string(p->type));
			print_options(child, cbuf, p->conn_req.OPT_offset, p->conn_req.OPT_length);
			break;
		case T_CONN_RES:
			event = __TEST_CONN_RES;
			print_rx_prim(child, prim_string(p->type));
			print_options(child, cbuf, p->conn_res.OPT_offset, p->conn_res.OPT_length);
			break;
		case T_DISCON_REQ:
			event = __TEST_DISCON_REQ;
			last_sequence = p->discon_req.SEQ_number;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_DATA_REQ:
			event = __TEST_DATA_REQ;
			MORE_flag = p->data_req.MORE_flag;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_EXDATA_REQ:
			event = __TEST_EXDATA_REQ;
			MORE_flag = p->data_req.MORE_flag;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_INFO_REQ:
			event = __TEST_INFO_REQ;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_BIND_REQ:
			event = __TEST_BIND_REQ;
			last_qlen = p->bind_req.CONIND_number;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_UNBIND_REQ:
			event = __TEST_UNBIND_REQ;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_UNITDATA_REQ:
			event = __TEST_UNITDATA_REQ;
			print_rx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
			print_string(child, addr_string(cbuf + p->unitdata_req.DEST_offset, p->unitdata_req.DEST_length));
#else
			print_addrs(child, cbuf + p->unitdata_req.DEST_offset, p->unitdata_req.DEST_length);
#endif
			print_options(child, cbuf, p->unitdata_req.OPT_offset, p->unitdata_req.OPT_length);
			break;
		case T_OPTMGMT_REQ:
			event = __TEST_OPTMGMT_REQ;
			print_rx_prim(child, prim_string(p->type));
			print_mgmtflag(child, p->optmgmt_req.MGMT_flags);
			print_options(child, cbuf, p->optmgmt_req.OPT_offset, p->optmgmt_req.OPT_length);
			break;
		case T_ORDREL_REQ:
			event = __TEST_ORDREL_REQ;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_OPTDATA_REQ:
			event = __TEST_OPTDATA_REQ;
			if (p->optdata_req.DATA_flag & T_ODF_EX) {
				if (p->optdata_req.DATA_flag & T_ODF_MORE)
					print_rx_prim(child, "T_OPTDATA_REQ!+ ");
				else
					print_rx_prim(child, "T_OPTDATA_REQ!  ");
			} else {
				if (p->optdata_req.DATA_flag & T_ODF_MORE)
					print_rx_prim(child, "T_OPTDATA_REQ+  ");
				else
					print_rx_prim(child, "T_OPTDATA_REQ   ");
			}
			print_options(child, cbuf, p->optdata_req.OPT_offset, p->optdata_req.OPT_length);
			break;
		case T_ADDR_REQ:
			event = __TEST_ADDR_REQ;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_CAPABILITY_REQ:
			event = __TEST_CAPABILITY_REQ;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_CONN_IND:
			event = __TEST_CONN_IND;
			last_sequence = p->conn_ind.SEQ_number;
			print_rx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
			print_string(child, addr_string(cbuf + p->conn_ind.SRC_offset, p->conn_ind.SRC_length));
#else
			print_addrs(child, cbuf + p->conn_ind.SRC_offset, p->conn_ind.SRC_length);
#endif
			print_options(child, cbuf, p->conn_ind.OPT_offset, p->conn_ind.OPT_length);
			break;
		case T_CONN_CON:
			event = __TEST_CONN_CON;
			print_rx_prim(child, prim_string(p->type));
			print_options(child, cbuf, p->conn_con.OPT_offset, p->conn_con.OPT_length);
			break;
		case T_DISCON_IND:
			event = __TEST_DISCON_IND;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_DATA_IND:
			event = __TEST_DATA_IND;
			if ((verbose && show) || verbose > 4)
				print_rx_prim(child, prim_string(p->type));
			break;
		case T_EXDATA_IND:
			event = __TEST_EXDATA_IND;
			if ((verbose && show) || verbose > 4)
				print_rx_prim(child, prim_string(p->type));
			break;
		case T_INFO_ACK:
			event = __TEST_INFO_ACK;
			last_info = p->info_ack;
			print_ack_prim(child, prim_string(p->type));
			print_info(child, &p->info_ack);
			break;
		case T_BIND_ACK:
			event = __TEST_BIND_ACK;
			last_qlen = p->bind_ack.CONIND_number;
			print_ack_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
			print_string(child, addr_string(cbuf + p->bind_ack.ADDR_offset, p->bind_ack.ADDR_length));
#else
			print_addrs(child, cbuf + p->bind_ack.ADDR_offset, p->bind_ack.ADDR_length);
#endif
			break;
		case T_ERROR_ACK:
			event = __TEST_ERROR_ACK;
			last_t_errno = p->error_ack.TLI_error;
			last_errno = p->error_ack.UNIX_error;
			print_ack_prim(child, prim_string(p->type));
			print_string(child, terrno_string(p->error_ack.TLI_error, p->error_ack.UNIX_error));
			break;
		case T_OK_ACK:
			event = __TEST_OK_ACK;
			print_ack_prim(child, prim_string(p->type));
			break;
		case T_UNITDATA_IND:
			event = __TEST_UNITDATA_IND;
			print_rx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
			print_string(child, addr_string(cbuf + p->unitdata_ind.SRC_offset, p->unitdata_ind.SRC_length));
#else
			print_addrs(child, cbuf + p->unitdata_ind.SRC_offset, p->unitdata_ind.SRC_length);
#endif
			print_options(child, cbuf, p->unitdata_ind.OPT_offset, p->unitdata_ind.OPT_length);
			break;
		case T_UDERROR_IND:
			event = __TEST_UDERROR_IND;
			print_rx_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
			print_string(child, addr_string(cbuf + p->uderror_ind.DEST_offset, p->uderror_ind.DEST_length));
#else
			print_addrs(child, cbuf + p->uderror_ind.DEST_offset, p->uderror_ind.DEST_length);
#endif
			print_options(child, cbuf, p->uderror_ind.OPT_offset, p->uderror_ind.OPT_length);
			print_string(child, etype_string(p->uderror_ind.ERROR_type));
			break;
		case T_OPTMGMT_ACK:
			event = __TEST_OPTMGMT_ACK;
			test_mgmtflags = p->optmgmt_ack.MGMT_flags;
			print_ack_prim(child, prim_string(p->type));
			print_mgmtflag(child, p->optmgmt_ack.MGMT_flags);
			print_options(child, cbuf, p->optmgmt_ack.OPT_offset, p->optmgmt_ack.OPT_length);
			break;
		case T_ORDREL_IND:
			event = __TEST_ORDREL_IND;
			print_rx_prim(child, prim_string(p->type));
			break;
		case T_OPTDATA_IND:
			test_dlen = data ? data->len : 0;
			if (p->optdata_ind.DATA_flag & T_ODF_EX) {
				event = __TEST_EXP_OPTDATA_IND;
				if ((verbose && show) || verbose > 4) {
					if (p->optdata_ind.DATA_flag & T_ODF_MORE)
						print_rx_prim(child, "T_OPTDATA_IND!+ ");
					else
						print_rx_prim(child, "T_OPTDATA_IND!  ");
				}
			} else {
				event = __TEST_NRM_OPTDATA_IND;
				if ((verbose && show) || verbose > 4) {
					if (p->optdata_ind.DATA_flag & T_ODF_MORE)
						print_rx_prim(child, "T_OPTDATA_IND+  ");
					else
						print_rx_prim(child, "T_OPTDATA_IND   ");
				}
			}
			if (p->optdata_ind.OPT_length) {
				struct t_opthdr *oh;
				unsigned char *op = (unsigned char *) p + p->optdata_ind.OPT_offset;
				int olen = p->optdata_ind.OPT_length;

				for (oh = _T_OPT_FIRSTHDR_OFS(op, olen, 0); oh; oh = _T_OPT_NEXTHDR_OFS(op, olen, oh, 0)) {
					if (oh->level == T_INET_SCTP) {
						switch (oh->name) {
						case T_SCTP_SID:
							sid[child] = (*((t_scalar_t *) (oh + 1))) & 0xffff;
							opt_data.sid_val = sid[child];
							break;
						case T_SCTP_PPI:
							ppi[child] = (*((t_scalar_t *) (oh + 1))) & 0xffffffff;
							opt_data.ppi_val = ppi[child];
							break;
						}
					}
				}
			}
			if ((verbose && show) || verbose > 4)
				print_options(child, cbuf, p->optdata_ind.OPT_offset, p->optdata_ind.OPT_length);
			break;
		case T_ADDR_ACK:
			event = __TEST_ADDR_ACK;
			print_ack_prim(child, prim_string(p->type));
#ifndef SCTP_VERSION_2
			print_string(child, addr_string(cbuf + p->addr_ack.LOCADDR_offset, p->addr_ack.LOCADDR_length));
			print_string(child, addr_string(cbuf + p->addr_ack.REMADDR_offset, p->addr_ack.REMADDR_length));
#else
			print_addrs(child, cbuf + p->addr_ack.LOCADDR_offset, p->addr_ack.LOCADDR_length);
			print_addrs(child, cbuf + p->addr_ack.REMADDR_offset, p->addr_ack.REMADDR_length);
#endif
			break;
		case T_CAPABILITY_ACK:
			event = __TEST_CAPABILITY_ACK;
			last_info = p->capability_ack.INFO_ack;
			print_ack_prim(child, prim_string(p->type));
			break;
		default:
			event = __EVENT_UNKNOWN;
			print_no_prim(child, p->type);
			break;
		}
		if (data && data->len >= 0)
			if ((last_event = do_decode_data(child, ctrl, data)) != __TEST_DATA)
				event = last_event;
	}
	return ((last_event = event));
}

static int
do_decode_msg(int child, struct strbuf *ctrl, struct strbuf *data)
{
	if (ctrl->len > 0) {
		if ((last_event = do_decode_ctrl(child, ctrl, data)) != __EVENT_UNKNOWN)
			return time_event(child, last_event);
	} else if (data->len > 0) {
		if ((last_event = do_decode_data(child, ctrl, data)) != __TEST_DATA)
			return time_event(child, last_event);
	}
	return ((last_event = __EVENT_NO_MSG));
}

#if 0
#define IUT 0x00000001
#define PT  0x00000002
#define ANY 0x00000003

int
any_wait_event(int source, int wait)
{
	while (1) {
		struct pollfd pfd[] = {
			{test_fd[0], POLLIN | POLLPRI, 0},
			{test_fd[1], POLLIN | POLLPRI, 0}
		};

		if (timer_timeout) {
			timer_timeout = 0;
			print_timeout(3);
			last_event = __EVENT_TIMEOUT;
			return time_event(__EVENT_TIMEOUT);
		}
		if (verbose > 3) {
			dummy = lockf(fileno(stdout), F_LOCK, 0);
			fprintf(stdout, "polling:\n");
			fflush(stdout);
			dummy = lockf(fileno(stdout), F_ULOCK, 0);
		}
		pfd[0].fd = test_fd[0];
		pfd[0].events = (source & IUT) ? (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND | POLLMSG | POLLERR | POLLHUP) : 0;
		pfd[0].revents = 0;
		pfd[1].fd = test_fd[1];
		pfd[1].events = (source & PT) ? (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND | POLLMSG | POLLERR | POLLHUP) : 0;
		pfd[1].revents = 0;
		switch (poll(pfd, 2, wait)) {
		case -1:
			if ((errno == EAGAIN || errno == EINTR))
				break;
			print_errno(3, (last_errno = errno));
			break;
		case 0:
			print_nothing(3);
			last_event = __EVENT_NO_MSG;
			return time_event(__EVENT_NO_MSG);
		case 1:
		case 2:
			if (pfd[0].revents) {
				int flags = 0;
				char cbuf[BUFSIZE];
				char dbuf[BUFSIZE];
				struct strbuf ctrl = { BUFSIZE, 0, cbuf };
				struct strbuf data = { BUFSIZE, 0, dbuf };

				if (verbose > 3) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "getmsg from top:\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				if (getmsg(test_fd[0], &ctrl, &data, &flags) == 0) {
					if (verbose > 3) {
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "gotmsg from top [%d,%d]:\n", ctrl.len, data.len);
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
					}
					if ((last_event = do_decode_msg(0, &ctrl, &data)) != __EVENT_NO_MSG)
						return time_event(last_event);
				}
			}
			if (pfd[1].revents) {
				int flags = 0;
				char cbuf[BUFSIZE];
				char dbuf[BUFSIZE];
				struct strbuf ctrl = { BUFSIZE, 0, cbuf };
				struct strbuf data = { BUFSIZE, 0, dbuf };

				if (verbose > 3) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "getmsg from bot:\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				if (getmsg(test_fd[1], &ctrl, &data, &flags) == 0) {
					if (verbose > 3) {
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "gotmsg from bot [%d,%d,%d]:\n", ctrl.len, data.len, flags);
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
					}
					if ((last_event = do_decode_msg(1, &ctrl, &data)) != __EVENT_NO_MSG)
						return time_event(last_event);
				}
			}
		default:
			break;
		}
	}
}
#endif

int
wait_event(int child, int wait)
{
	while (1) {
		struct pollfd pfd[] = { {test_fd[child], POLLIN | POLLPRI, 0} };

		if (timer_timeout) {
			timer_timeout = 0;
			print_timeout(child);
			last_event = __EVENT_TIMEOUT;
			return time_event(child, __EVENT_TIMEOUT);
		}
		if (show && verbose > 4)
			print_syscall(child, "poll()");
		pfd[0].fd = test_fd[child];
		pfd[0].events = POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND | POLLMSG | POLLERR | POLLHUP;
		pfd[0].revents = 0;
		switch (poll(pfd, 1, wait)) {
		case -1:
			if (errno == EINTR || errno == ERESTART)
				continue;
			print_errno(child, (last_errno = errno));
			return (__RESULT_FAILURE);
		case 0:
			if (show && verbose > 4)
				print_success(child);
			print_nothing(child);
			last_event = __EVENT_NO_MSG;
			return time_event(child, __EVENT_NO_MSG);
		case 1:
			if (show && verbose > 4)
				print_success(child);
			if (pfd[0].revents) {
				int ret;

				ctrl.maxlen = BUFSIZE;
				ctrl.len = -1;
				ctrl.buf = cbuf;
				data.maxlen = BUFSIZE;
				data.len = -1;
				data.buf = dbuf;
				flags = 0;
				for (;;) {
					if ((verbose > 3 && show) || (verbose > 5 && show_msg))
						print_syscall(child, "getmsg()");
					if ((ret = getmsg(test_fd[child], &ctrl, &data, &flags)) >= 0)
						break;
					if (errno == EINTR || errno == ERESTART)
						continue;
					print_errno(child, (last_errno = errno));
					if (errno == EAGAIN)
						break;
					return __RESULT_FAILURE;
				}
				if (ret < 0)
					break;
				if (ret == 0) {
					if ((verbose > 3 && show) || (verbose > 5 && show_msg))
						print_success(child);
					if ((verbose > 3 && show) || (verbose > 5 && show_msg)) {
						int i;

						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "gotmsg from %d [%d,%d]:\n", child, ctrl.len, data.len);
						fprintf(stdout, "[");
						for (i = 0; i < ctrl.len; i++)
							fprintf(stdout, "%02X", (uint8_t) ctrl.buf[i]);
						fprintf(stdout, "]\n");
						fprintf(stdout, "[");
						for (i = 0; i < data.len; i++)
							fprintf(stdout, "%02X", (uint8_t) data.buf[i]);
						fprintf(stdout, "]\n");
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
					}
					last_prio = (flags == RS_HIPRI);
					if ((last_event = do_decode_msg(child, &ctrl, &data)) != __EVENT_NO_MSG)
						return time_event(child, last_event);
				}
			}
		default:
			break;
		}
	}
	return __EVENT_UNKNOWN;
}

int
get_event(int child)
{
	return wait_event(child, -1);
}

int
get_data(int child, int action)
{
	int ret = 0;

	switch (action) {
	case __TEST_READ:
	{
		char buf[BUFSIZE];

		test_read(child, buf, BUFSIZE);
		ret = last_retval;
		break;
	}
	case __TEST_READV:
	{
		char buf[BUFSIZE];
		static const size_t count = 4;
		struct iovec vector[] = {
			{buf, (BUFSIZE >> 2)},
			{buf + (BUFSIZE >> 2), (BUFSIZE >> 2)},
			{buf + (BUFSIZE >> 2) + (BUFSIZE >> 2), (BUFSIZE >> 2)},
			{buf + (BUFSIZE >> 2) + (BUFSIZE >> 2) + (BUFSIZE >> 2), (BUFSIZE >> 2)}
		};

		test_readv(child, vector, count);
		ret = last_retval;
		break;
	}
	case __TEST_GETMSG:
	{
		char buf[BUFSIZE];
		struct strbuf data = { BUFSIZE, 0, buf };

		if (test_getmsg(child, NULL, &data, &test_gflags) == __RESULT_FAILURE) {
			ret = last_retval;
			break;
		}
		ret = data.len;
		break;
	}
	case __TEST_GETPMSG:
	{
		char buf[BUFSIZE];
		struct strbuf data = { BUFSIZE, 0, buf };

		if (test_getpmsg(child, NULL, &data, &test_gband, &test_gflags) == __RESULT_FAILURE) {
			ret = last_retval;
			break;
		}
		ret = data.len;
		break;
	}
	}
	return (ret);
}

int
expect(int child, int wait, int want)
{
	if ((last_event = wait_event(child, wait)) == want)
		return (__RESULT_SUCCESS);
	print_expect(child, want);
	return (__RESULT_FAILURE);
}

/*
 *  -------------------------------------------------------------------------
 *
 *  Preambles and postambles
 *
 *  -------------------------------------------------------------------------
 */

static int
preamble_1_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_1_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_1_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_1_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_2_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_2_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_2_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_2_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_3_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_3_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_3_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_3_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_4_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_4_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_4_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_4_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_5_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_5_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_5_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_5_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_6_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_6_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_6_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_6_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_7_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_7_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_7_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_7_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_8_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_8_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_8_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_8_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_9_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_9_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_9_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_9_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_10_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_10_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_10_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_10_pt(int child)
{
	return (__RESULT_FAILURE);
}

static int
preamble_11_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
preamble_11_pt(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_11_iut(int child)
{
	return (__RESULT_FAILURE);
}
static int
postamble_11_pt(int child)
{
	return (__RESULT_FAILURE);
}

/* These are preambles intended to place the IUT in a specific state with
 * regard to an association. */

/* The IUT has no association with the PT and never did. */
static int
preamble_iut_unint_iut(int child)
{
	if (start_tt(TEST_DURATION) != __RESULT_SUCCESS)
		goto failure;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}
static int
preamble_iut_unint_pt(int child)
{
	if (start_tt(TEST_DURATION) != __RESULT_SUCCESS)
		goto failure;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT is bound, but not listening */
static int
preamble_iut_bound_iut(int child)
{
	if (preamble_iut_unint_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Bind IUT on address not listening */
	if (do_signal(child, __TEST_BIND_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for bind ack */
	if (expect(child, INFINITE_WAIT, __TEST_BIND_ACK) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}
static int
preamble_iut_bound_pt(int child)
{
	if (preamble_iut_unint_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for IUT NPI bind request */
	state++;
	/* Send NPI bind ack */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has launched an INIT */
static int
preamble_iut_cookie_wait_iut(int child)
{
	if (preamble_iut_bound_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send connect request */
	if (do_signal(child, __TEST_CONN_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for connect con */
	if (expect(child, INFINITE_WAIT, __TEST_CONN_CON) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has received an INIT */
static int
preamble_iut_cookie_wait_pt(int child)
{
	if (preamble_iut_bound_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for INIT (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has done INIT/INIT-ACK/COOKIE-ECHO sequence. */
static int
preamble_iut_cookie_echoed_iut(int child)
{
	if (preamble_iut_cookie_wait_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);

}

/* The PT has received a COOKIE-ECHO */
static int
preamble_iut_cookie_echoed_pt(int child)
{
	if (preamble_iut_cookie_wait_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send INIT-ACK (N_UNITDATA_IND) */
	state++;
	/* Wait for COOKIE-ECHO (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has done INIT/INIT-ACK/COOKIE-ECHO/COOKIE-ACK */
static int
preamble_iut_established_iut(int child)
{
	if (preamble_iut_cookie_echoed_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for connection confirmation. */
	if (expect(child, INFINITE_WAIT, __TEST_CONN_CON) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has sent COOKIE-ACK */
static int
preamble_iut_established_pt(int child)
{
	if (preamble_iut_cookie_echoed_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send COOKIE-ACK. (N_UNITDATA_IND) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The other way around */

/* The IUT is listening, but no association exists yet. */
static int
preamble_iut_listen_iut(int child)
{
	if (preamble_iut_unint_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Bind IUT on address listening */
	if (do_signal(child, __TEST_BIND_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for bind ack */
	if (expect(child, INFINITE_WAIT, __TEST_BIND_ACK) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}
static int
preamble_iut_listen_pt(int child)
{
	if (preamble_iut_unint_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for NPI Bind request from IUT */
	state++;
	/* Send NPI bind ack */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

int
preamble_pt_initialized_iut(int child)
{
	if (preamble_iut_listen_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Short wait for PT to launch INIT and receive INIT-ACK */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

int
preamble_pt_initialized_pt(int child)
{
	if (preamble_iut_listen_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send INIT (N_UNITDATA_IND) */
	state++;
	/* Wait for INIT-ACK (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

int
preamble_pt_established_iut(int child)
{
	if (preamble_iut_cookie_wait_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for connection indication */
	if (expect(child, INFINITE_WAIT, __TEST_CONN_IND) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Return connection response */
	if (do_signal(child, __TEST_CONN_RES) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

int
preamble_pt_established_pt(int child)
{
	if (preamble_iut_cookie_wait_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send COOKIE-ECHO (N_UNITDATA_IND) */
	state++;
	/* Wait for COOKIE-ACK (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has received a SHUTDOWN but has pending data */
int
preamble_iut_shutdown_received_iut(int child)
{
	if (preamble_iut_established_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send DATA */
	if (do_signal(child, __TEST_DATA) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has sent a SHUTDOWN but not acknowledged some data */
int
preamble_iut_shutdown_received_pt(int child)
{
	if (preamble_iut_established_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for DATA (N_UNITDATA_REQ) */
	state++;
	/* Send SHUTDOWN (N_UNITDATA_IND) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);

}

/* The IUT has sent a SHUTDOWN-ACK */
int
preamble_iut_shutdown_ack_sent_iut(int child)
{
	if (preamble_iut_established_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for orderly release indication */
	if (expect(child, INFINITE_WAIT, __TEST_ORDREL_IND) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Issue orderly release request */
	if (do_signal(child, __TEST_ORDREL_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has received a SHUTDOWN-ACK */
int
preamble_iut_shutdown_ack_sent_pt(int child)
{
	if (preamble_iut_established_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send SHUTDOWN (N_UNITDATA_IND) */
	state++;
	/* Wait for SHUTDOWN-ACK (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has performed a Terminate */
int
preamble_iut_shutdown_pending_iut(int child)
{
	if (preamble_iut_established_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send data */
	if (do_signal(child, __TEST_DATA) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Issue orderly release request */
	if (do_signal(child, __TEST_ORDREL_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has not acknowledged some data */
int
preamble_iut_shutdown_pending_pt(int child)
{
	if (preamble_iut_established_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for data (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);

}

/* The IUT has sent a SHUTDOWN */
int
preamble_iut_shutdown_sent_iut(int child)
{
	if (preamble_iut_established_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Issue orderly release request */
	if (do_signal(child, __TEST_ORDREL_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has received a SHUTDOWN */
int
preamble_iut_shutdown_sent_pt(int child)
{
	if (preamble_iut_established_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for SHUTDOWN (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has sent a SHUTDOWN-COMPLETE */
int
preamble_iut_closed_iut(int child)
{
	if (preamble_iut_established_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Issue orderly release request */
	if (do_signal(child, __TEST_ORDREL_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for orderly release indication */
	if (expect(child, INFINITE_WAIT, __TEST_ORDREL_IND) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The PT has received a SHUTDOWN-COMPLETE */
int
preamble_iut_closed_pt(int child)
{
	if (preamble_iut_established_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* wait for SHUTDOWN (N_UNITDATA_REQ) */
	state++;
	/* Send SHUTDOWN-ACK (N_UNITDATA_IND) */
	state++;
	/* Wait for SHUTDOWN-COMPLETE (N_UNITDATA_REQ) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* The IUT has received a SHUTDOWN-COMPLETE */
int
preamble_pt_closed_iut(int child)
{
	if (preamble_iut_established_iut(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Wait for orderly release indication */
	if (expect(child, INFINITE_WAIT, __TEST_ORDREL_IND) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send orderly release request */
	if (do_signal(child, __TEST_ORDREL_REQ) != __RESULT_SUCCESS)
		goto failure;
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

int
preamble_pt_closed_pt(int child)
{
	if (preamble_iut_established_pt(child) != __RESULT_SUCCESS)
		goto failure;
	state++;
	/* Send SHUTDOWN (N_UNITDATA_IND) */
	state++;
	/* Wait for SHUTDOWN ACK (N_UNITDATA_REQ) */
	state++;
	/* Send SHUTDOWN-COMPLETE (N_UNITDATA_IND) */
	state++;
	return (__RESULT_SUCCESS);
      failure:
	return (__RESULT_FAILURE);
}

/* These are postambles intended on gracefully exiting a given state at the
 * end of a test. */

int
postamble_iut_unint_iut(int child)
{
	int failed = -1;

	while (1) {
		expect(child, SHORT_WAIT, __EVENT_NO_MSG);
		switch (last_event) {
		case __EVENT_NO_MSG:
		case __EVENT_TIMEOUT:
			break;
		case __RESULT_FAILURE:
			break;
		default:
			failed = (failed == -1) ? state : failed;
			state++;
			continue;
		}
		break;
	}
	state++;
	if (stop_tt() != __RESULT_SUCCESS)
		goto failure;
	state++;
	if (failed != -1)
		goto failure;
	return (__RESULT_SUCCESS);
      failure:
	state = failed;
	return (__RESULT_FAILURE);
}

int
postamble_iut_unint_pt(int child)
{
	return postamble_iut_unint_iut(child);
}

int
postamble_iut_bound_iut(int child)
{
	int failed = -1;

	/* Unbind IUT */
	if (do_signal(child, __TEST_UNBIND_REQ) != __RESULT_SUCCESS)
		failed = (failed == -1) ? state : failed;
	state++;
	/* wait for unbind ack */
	if (expect(child, INFINITE_WAIT, __TEST_OK_ACK) != __RESULT_SUCCESS)
		failed = (failed == -1) ? state : failed;
	state++;
	if (postamble_iut_unint_iut(child) != __RESULT_SUCCESS)
		failed = (failed == -1) ? state : failed;
	state++;
	if (failed != -1)
		goto failure;
	return (__RESULT_SUCCESS);
      failure:
	state = failed;
	return (__RESULT_FAILURE);
}

int
postamble_iut_bound_pt(int child)
{
	/* Wait for NPI unbind request */
	state++;
	/* send NPI ok ack */
	state++;
	/* flush any remaining data */
	state++;
	return postamble_iut_unint_pt(child);
}

int
postamble_iut_cookie_wait_iut(int child)
{
	int failed = -1;

	/* disconnect request on IUT */
	if (do_signal(child, __TEST_DISCON_REQ) != __RESULT_SUCCESS)
		failed = (failed == -1) ? state : failed;
	state++;
	/* disconnect ack on IUT */
	if (expect(child, INFINITE_WAIT, __TEST_OK_ACK) != __RESULT_SUCCESS)
		failed = (failed == -1) ? state : failed;
	state++;
	/* flush any connection confirmations */
	state++;
	if (postamble_iut_bound_iut(child) != __RESULT_SUCCESS)
		failed = (failed == -1) ? state : failed;
	state++;
	if (failed != -1)
		goto failure;
	return (__RESULT_SUCCESS);
      failure:
	state = failed;
	return (__RESULT_FAILURE);
}

int
postamble_iut_cookie_wait_pt(int child)
{
	/* Send ABORT response to INIT (N_UNITDATA_IND) */
	state++;
	/* clear out any duplicate INITs */
	state++;
	return postamble_iut_bound_pt(child);
}

int
postamble_iut_cookie_echoed_iut(int child)
{
	return postamble_iut_cookie_wait_iut(child);
}

int
postamble_iut_cookie_echoed_pt(int child)
{

	/* Send ABORT response to COOKIE-ECHO (N_UNITDATA_IND) */
	state++;
	/* clear out any duplicate COOKIE-ECHOs */
	state++;
	return postamble_iut_bound_pt(child);
}

int
postamble_iut_established_iut(int child)
{
	return postamble_iut_cookie_wait_iut(child);
}

int
postamble_iut_established_pt(int child)
{
	return postamble_iut_cookie_wait_pt(child);
}

/* The other way around */
int
postamble_iut_listen_iut(int child)
{
	return postamble_iut_bound_iut(child);
}

int
postamble_iut_listen_pt(int child)
{
	return postamble_iut_bound_pt(child);
}

int
postamble_pt_initialized_iut(int child)
{
	return postamble_iut_listen_iut(child);
}

int
postamble_pt_initialized_pt(int child)
{
	return postamble_iut_listen_pt(child);
}

int
postamble_pt_established_iut(int child)
{
	return postamble_iut_established_iut(child);
}

int
postamble_pt_established_pt(int child)
{
	return postamble_iut_established_pt(child);
}

int
postamble_iut_shutdown_received_iut(int child)
{
	return postamble_pt_established_iut(child);
}

int
postamble_iut_shutdown_received_pt(int child)
{
	return postamble_pt_established_pt(child);
}

int
postamble_iut_shutdown_ack_sent_iut(int child)
{
	return postamble_iut_listen_iut(child);
}

int
postamble_iut_shutdown_ack_sent_pt(int child)
{
	/* send SHUTDOWN-COMPLETE (N_UNITDATA_IND) */
	state++;
	/* discard duplicate SHUTDOWN-ACKs */
	state++;
	return postamble_iut_listen_pt(child);
}

int
postamble_iut_shutdown_pending_iut(int child)
{
	return postamble_pt_established_iut(child);
}

int
postamble_iut_shutdown_pending_pt(int child)
{
	return postamble_pt_established_pt(child);
}

int
postamble_iut_shutdown_sent_iut(int child)
{
	return postamble_pt_established_iut(child);
}

int
postamble_iut_shutdown_sent_pt(int child)
{
	return postamble_pt_established_pt(child);
}

int
postamble_iut_closed_iut(int child)
{
	return postamble_iut_listen_iut(child);
}

int
postamble_iut_closed_pt(int child)
{
	return postamble_iut_listen_pt(child);
}

int
postamble_pt_closed_iut(int child)
{
	return postamble_iut_listen_iut(child);
}

int
postamble_pt_closed_pt(int child)
{
	return postamble_iut_listen_pt(child);
}

/*
 *  =========================================================================
 *
 *  The Test Cases...
 *
 *  =========================================================================
 */

struct test_stream {
	int (*preamble) (int);		/* test preamble */
	int (*testcase) (int);		/* test case */
	int (*postamble) (int);		/* test postamble */
};

/*
 *  Check test case guard timer.
 */
#define test_group_0 "Sanity checks"
#define tgrp_case_0_1 test_group_0
#define numb_case_0_1 "0.1"
#define name_case_0_1 "Check test case guard timer."
#define sref_case_0_1 "(none)"
#define desc_case_0_1 "\
Checks that the test case guard timer will fire and bring down the children."

int
test_case_0_1(int child)
{
	test_sleep(child, 40);
	return (__RESULT_SUCCESS);
}

#define preamble_0_1		preamble_iut_unint_iut
#define postamble_0_1		postamble_iut_unint_iut

struct test_stream test_0_1_conn = { &preamble_0_1, &test_case_0_1, &postamble_0_1 };
struct test_stream test_0_1_resp = { &preamble_0_1, &test_case_0_1, &postamble_0_1 };
struct test_stream test_0_1_list = { &preamble_0_1, &test_case_0_1, &postamble_0_1 };

/* ========================================================================= */

#define test_group_1 "Association Setup (AS)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_1_1 test_group_1
#define numb_case_1_1_1 "1.1.1"
#define tpid_case_1_1_1 "SCTP_AS_V_1_1_1"
#define name_case_1_1_1 "Successful Association Setup"
#define sref_case_1_1_1 "RFC 2960, section 5.1 and 5.1.6"
#define desc_case_1_1_1 "\
Checks that the IUT makes a complete association procedure."

#define preamble_1_1_1_top	preamble_1_iut
#define preamble_1_1_1_bot	preamble_1_pt

/* Association is not established between tester and SUT.  Configure the
 * IUT to send an INIT to the tester. */

int
test_case_1_1_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_1_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_1_1_top	postamble_1_iut
#define postamble_1_1_1_bot	postamble_1_pt

struct test_stream test_1_1_1_top = { &preamble_1_1_1_top, &test_case_1_1_1_top, &postamble_1_1_1_top };
struct test_stream test_1_1_1_bot = { &preamble_1_1_1_bot, &test_case_1_1_1_bot, &postamble_1_1_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_1_2 test_group_1
#define numb_case_1_1_2 "1.1.2"
#define tpid_case_1_1_2 "SCTP_AS_V_1_1_2"
#define name_case_1_1_2 "Successful Association Setup"
#define sref_case_1_1_2 "RFC 2960, sections 5.1 and 5.1.6"
#define desc_case_1_1_2 "\
Checsk that the IUT can establish a complete association after receiving\n\
an INIT from the PT."

#define preamble_1_1_2_top	preamble_1_iut
#define preamble_1_1_2_bot	preamble_1_pt

/* Association is not established between tester and SUT.  Arrange the
 * data at the tester such that INIT is sent to IUT. */

int
test_case_1_1_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_1_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_1_2_top	postamble_1_iut
#define postamble_1_1_2_bot	postamble_1_pt

struct test_stream test_1_1_2_top = { &preamble_1_1_2_top, &test_case_1_1_2_top, &postamble_1_1_2_top };
struct test_stream test_1_1_2_bot = { &preamble_1_1_2_bot, &test_case_1_1_2_bot, &postamble_1_1_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_2_1 test_group_1
#define numb_case_1_2_1 "1.2.1"
#define tpid_case_1_2_1 "SCTP_AS_I_1_2_1"
#define name_case_1_2_1 "T1-Init"
#define sref_case_1_2_1 "RFC 2960, sections 4. and 5.1.6"
#define desc_case_1_2_1 "\
Checks that the IUT, if T1-Init timer expires, transmits the INIT messsage\n\
again."

#define preamble_1_2_1_top	preamble_1_iut
#define preamble_1_2_1_bot	preamble_1_pt

/* Association is not esablished between the PT and SUT.  Configure the
 * SUT to send an INIT to the PT.  Arrange the data at the PT such that
 * INIT-ACK is not sent in response to INIT message. */

int
test_case_1_2_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_2_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_2_1_top	postamble_1_iut
#define postamble_1_2_1_bot	postamble_1_pt

struct test_stream test_1_2_1_top = { &preamble_1_2_1_top, &test_case_1_2_1_top, &postamble_1_2_1_top };
struct test_stream test_1_2_1_bot = { &preamble_1_2_1_bot, &test_case_1_2_1_bot, &postamble_1_2_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_2_2 test_group_1
#define numb_case_1_2_2 "1.2.2"
#define tpid_case_1_2_2 "SCTP_AS_I_1_2_2"
#define name_case_1_2_2 "T1-Cookie"
#define sref_case_1_2_2 "RFC 2960, sections 4 and 5.1.6"
#define desc_case_1_2_2 "\
Checks that the IUT, if T1-Cookie timer expires, transmits the COOKIE-ECHO\n\
message again."

#define preamble_1_2_2_top	preamble_1_iut
#define preamble_1_2_2_bot	preamble_1_pt

/* Association is not esablished betwen the PT and SUT.  Configure the
 * SUT to send an INIT to the PT.  Arrange the data at the tester such
 * that COOKIE-ACK is not sent in response to COOKIE-ECHO message. */

int
test_case_1_2_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_2_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_2_2_top	postamble_1_iut
#define postamble_1_2_2_bot	postamble_1_pt

struct test_stream test_1_2_2_top = { &preamble_1_2_2_top, &test_case_1_2_2_top, &postamble_1_2_2_top };
struct test_stream test_1_2_2_bot = { &preamble_1_2_2_bot, &test_case_1_2_2_bot, &postamble_1_2_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_3_1 test_group_1
#define numb_case_1_3_1 "1.3.1"
#define tpid_case_1_3_1 "SCTP_AS_I_1_3_1"
#define name_case_1_3_1 "INIT sent MAX.INIT.RETRIES"
#define sref_case_1_3_1 "RFC 2960, section 4 (note 2)"
#define desc_case_1_3_1 "\
Checks that the IUT, if INIT is retransmitted for MAX.INIT.RETRIES times,\n\
stops the initialization process."

#define preamble_1_3_1_top	preamble_1_iut
#define preamble_1_3_1_bot	preamble_1_pt

/* Association is not established between PT and SUT.  Configure the SUT
 * to send an INIT to the PT.  Arrange the data at the PT such that
 * INIT-ACK is never sent in response to INIT message. */

int
test_case_1_3_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_3_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_3_1_top	postamble_1_iut
#define postamble_1_3_1_bot	postamble_1_pt

struct test_stream test_1_3_1_top = { &preamble_1_3_1_top, &test_case_1_3_1_top, &postamble_1_3_1_top };
struct test_stream test_1_3_1_bot = { &preamble_1_3_1_bot, &test_case_1_3_1_bot, &postamble_1_3_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_3_2 test_group_1
#define numb_case_1_3_2 "1.3.2"
#define tpid_case_1_3_2 "SCTP_AS_I_1_3_2"
#define name_case_1_3_2 "COOKIE-ECHO sent MAX.INIT.RETRIES"
#define sref_case_1_3_2 "RFC 2960, section 4 (note 3) and section 5.1.6"
#define desc_case_1_3_2 "\
Checks that the IUT, if COOKIE-ECHO message is retransmitted for\n\
MAX.INIT.RETRANS times, stops the initialization process."

#define preamble_1_3_2_top	preamble_1_iut
#define preamble_1_3_2_bot	preamble_1_pt

/* Association is not established between PT and SUT.  Configure the
 * SUTto send an INIT to the PT.  Arrange the data at the PT such that
 * COOKIE-ACK is never sent in response to COOKIE-ECHO message. */

int
test_case_1_3_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_3_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_3_2_top	postamble_1_iut
#define postamble_1_3_2_bot	postamble_1_pt

struct test_stream test_1_3_2_top = { &preamble_1_3_2_top, &test_case_1_3_2_top, &postamble_1_3_2_top };
struct test_stream test_1_3_2_bot = { &preamble_1_3_2_bot, &test_case_1_3_2_bot, &postamble_1_3_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_4 test_group_1
#define numb_case_1_4 "1.4"
#define tpid_case_1_4 "SCTP_AS_I_1_4"
#define name_case_1_4 "No COOKIE-ECHO message."
#define sref_case_1_4 "RFC 2960, section 5.1 B (note)"
#define desc_case_1_4 "\
Checks that the IUT remains in closed state if COOKIE-ECHO message\n\
is not received."

#define preamble_1_4_top	preamble_1_iut
#define preamble_1_4_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange the data at
 * the PT such that COOKIE-ECHO is not sent in response to INIT-ACK
 * message.  Also let maximum number of associations that SUT can
 * establish be n and n-1 of them are already established.  Try to make
 * the nth association. */

int
test_case_1_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_4_top	postamble_1_iut
#define postamble_1_4_bot	postamble_1_pt

struct test_stream test_1_4_top = { &preamble_1_4_top, &test_case_1_4_top, &postamble_1_4_top };
struct test_stream test_1_4_bot = { &preamble_1_4_bot, &test_case_1_4_bot, &postamble_1_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_5_1 test_group_1
#define numb_case_1_5_1 "1.5.1"
#define tpid_case_1_5_1 "SCTP_AS_V_1_5_1"
#define name_case_1_5_1 "Random Initiate-Tag in INIT"
#define sref_case_1_5_1 "RFC 2960, section 5.3.1"
#define desc_case_1_5_1 "\
Checks that the IUT on re-establishing an association to a peer, uses\n\
a random Initiate-Tag value in the INIT message."

#define preamble_1_5_1_top	preamble_1_iut
#define preamble_1_5_1_bot	preamble_1_pt

/* Association is not established between PT and SUT.  Arrange the data
 * at the PT such that normal association can be established and
 * terminated between PT and SUT. */

int
test_case_1_5_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_5_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_5_1_top	postamble_1_iut
#define postamble_1_5_1_bot	postamble_1_pt

struct test_stream test_1_5_1_top = { &preamble_1_5_1_top, &test_case_1_5_1_top, &postamble_1_5_1_top };
struct test_stream test_1_5_1_bot = { &preamble_1_5_1_bot, &test_case_1_5_1_bot, &postamble_1_5_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_5_2 test_group_1
#define numb_case_1_5_2 "1.5.2"
#define tpid_case_1_5_2 "SCTP_AS_V_1_5_2"
#define name_case_1_5_2 "Random Initiate-Tag in INIT-ACK"
#define sref_case_1_5_2 "RFC 2960, section 5.3.1"
#define desc_case_1_5_2 "\
Checks that the IUT on re-establishing an association to a peer, uses\n\
a random Initiate-Tag value in the INIT-ACK message."

#define preamble_1_5_2_top	preamble_1_iut
#define preamble_1_5_2_bot	preamble_1_pt

/* Association is not established between PT and SUT.  Arrange the data
 * at the PT such that normal association can be established and
 * terminated between PT and SUT. */

int
test_case_1_5_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_5_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_5_2_top	postamble_1_iut
#define postamble_1_5_2_bot	postamble_1_pt

struct test_stream test_1_5_2_top = { &preamble_1_5_2_top, &test_case_1_5_2_top, &postamble_1_5_2_top };
struct test_stream test_1_5_2_bot = { &preamble_1_5_2_bot, &test_case_1_5_2_bot, &postamble_1_5_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_6_1 test_group_1
#define numb_case_1_6_1 "1.6.1"
#define tpid_case_1_6_1 "SCTP_AS_V_1_6_1"
#define name_case_1_6_1 "INIT with Parameters"
#define sref_case_1_6_1 "RFC 2960, section 3.3.2 and TS 102 144, section 4.6"
#define desc_case_1_6_1 "\
Checks that the IUT on receipt of an INIT message with parameters\n\
(IPv4 Address Parameter, IPv6 Address Parameter, Cookie Preservative,\n\
Supported Address Type Parameter) accepts this message and responds\n\
to it."

#define preamble_1_6_1_top	preamble_1_iut
#define preamble_1_6_1_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange the data at
 * the PT such that the listed parameters (Ipv4 Adddress Parameter, IPv6
 * Address Parameter, Cookie Preservative, Supported Address Type
 * Parameter) are sent in INIT message. */

int
test_case_1_6_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_6_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_6_1_top	postamble_1_iut
#define postamble_1_6_1_bot	postamble_1_pt

struct test_stream test_1_6_1_top = { &preamble_1_6_1_top, &test_case_1_6_1_top, &postamble_1_6_1_top };
struct test_stream test_1_6_1_bot = { &preamble_1_6_1_bot, &test_case_1_6_1_bot, &postamble_1_6_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_6_2 test_group_1
#define numb_case_1_6_2 "1.6.2"
#define tpid_case_1_6_2 "SCTP_AS_V_1_6_2"
#define name_case_1_6_2 "INIT-ACK with Parameters"
#define sref_case_1_6_2 "RFC 2960, section 3.3.3 and TS 102 144, section 4.6"
#define desc_case_1_6_2 "\
Checks that the IUT on receipt of an INIT-ACK message with parameters\n\
(IPv4 Address Parameter, IPv6 Address Parameter, Cookie Preservative)\n\
accepts this message and responds to it."

#define preamble_1_6_2_top	preamble_1_iut
#define preamble_1_6_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange the data at
 * the PT such that the listed parameters (IPv4 Address Parameter, IPv6
 * Address Parameter, Cookie Preservative) are send in INIT-ACK message.
 */

int
test_case_1_6_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_6_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_6_2_top	postamble_1_iut
#define postamble_1_6_2_bot	postamble_1_pt

struct test_stream test_1_6_2_top = { &preamble_1_6_2_top, &test_case_1_6_2_top, &postamble_1_6_2_top };
struct test_stream test_1_6_2_bot = { &preamble_1_6_2_bot, &test_case_1_6_2_bot, &postamble_1_6_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_7_1 test_group_1
#define numb_case_1_7_1 "1.7.1"
#define tpid_case_1_7_1 "SCTP_AS_I_1_7_1"
#define name_case_1_7_1 "INIT Stream Negotiation"
#define sref_case_1_7_1 "RFC 2960, section 5.1.1"
#define desc_case_1_7_1 "\
Checks that the IUT, if there is a mismatch in the Outbound Stream and\n\
Inbound Stream parameters in INIT and INIT-ACK message, either aborts\n\
the association or settles with the lesser of the two values."

#define preamble_1_7_1_top	preamble_1_iut
#define preamble_1_7_1_bot	preamble_1_pt

/* Association is not established between PT and SUT.  Also, let the
 * Outbound Streams of the SUT be Z.  Arrange data at the PT such that
 * INIT message is sent from PT with Maximum Inbound Streams Y < Z. */

int
test_case_1_7_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_7_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_7_1_top	postamble_1_iut
#define postamble_1_7_1_bot	postamble_1_pt

struct test_stream test_1_7_1_top = { &preamble_1_7_1_top, &test_case_1_7_1_top, &postamble_1_7_1_top };
struct test_stream test_1_7_1_bot = { &preamble_1_7_1_bot, &test_case_1_7_1_bot, &postamble_1_7_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_7_2 test_group_1
#define numb_case_1_7_2 "1.7.2"
#define tpid_case_1_7_2 "SCTP_AS_I_1_7_2"
#define name_case_1_7_2 "INIT with OutboundStreams zero (0)"
#define sref_case_1_7_2 "RFC 2960, section 3.3.2"
#define desc_case_1_7_2 "\
Checks that the IUT, if OutboundStreams are found zero in the received\n\
INIT message, sends an ABORT message for that INIT, or silently discards\n\
the received message."

#define preamble_1_7_2_top	preamble_1_iut
#define preamble_1_7_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that INIT message with Outbound Streams equal to 0 is sent
 * from the PT. */

int
test_case_1_7_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_7_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_7_2_top	postamble_1_iut
#define postamble_1_7_2_bot	postamble_1_pt

struct test_stream test_1_7_2_top = { &preamble_1_7_2_top, &test_case_1_7_2_top, &postamble_1_7_2_top };
struct test_stream test_1_7_2_bot = { &preamble_1_7_2_bot, &test_case_1_7_2_bot, &postamble_1_7_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_7_3 test_group_1
#define numb_case_1_7_3 "1.7.3"
#define tpid_case_1_7_3 "SCTP_AS_V_1_7_3"
#define name_case_1_7_3 "INIT-ACK Stream Negotiation"
#define sref_case_1_7_3 "RFC 2960, section 5.1.1"
#define desc_case_1_7_3 "\
Checks that the IUT, if there is a mismatch in the Outbound Streams and\n\
Inbound Streams parameters in INIT and INIT-ACK messages, either aborts\n\
the association or settles with the lesser of the two parameters."

#define preamble_1_7_3_top	preamble_1_iut
#define preamble_1_7_3_bot	preamble_1_pt

/* Association is not established between PT and SUT.  Also let the
 * Outbound Streams of the SUT be Z.  Arrange data at the PT such that
 * INIT-ACK message is sent from PT with Maximum Inbound Streams Y < z.
 */

int
test_case_1_7_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_7_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_7_3_top	postamble_1_iut
#define postamble_1_7_3_bot	postamble_1_pt

struct test_stream test_1_7_3_top = { &preamble_1_7_3_top, &test_case_1_7_3_top, &postamble_1_7_3_top };
struct test_stream test_1_7_3_bot = { &preamble_1_7_3_bot, &test_case_1_7_3_bot, &postamble_1_7_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_7_4 test_group_1
#define numb_case_1_7_4 "1.7.4"
#define tpid_case_1_7_4 "SCTP_AS_I_1_7_4"
#define name_case_1_7_4 "INIT-ACK with OutboundStreams zero (0)"
#define sref_case_1_7_4 "RFC 2960, section 3.3.3"
#define desc_case_1_7_4 "\
Checks that the IUT, if Outbound Streams is found zero in the received\n\
INIT-ACK message, destroys the TCB and may send an ABORT message for\n\
that INIT-ACK.  (Further message exchanges between the tester and IUT\n\
need to take place to verify the TCB removal from outside the SUT.)"

#define preamble_1_7_4_top	preamble_1_iut
#define preamble_1_7_4_bot	preamble_1_pt

/* Association not established between PT and SUT.  Also let the
 * Outbound Streams of the SUT be Z.  Arrange data at the PT such that
 * INIT-ACK message with Outbound Streams equal to 0 is send from PT. */

int
test_case_1_7_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_7_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_7_4_top	postamble_1_iut
#define postamble_1_7_4_bot	postamble_1_pt

struct test_stream test_1_7_4_top = { &preamble_1_7_4_top, &test_case_1_7_4_top, &postamble_1_7_4_top };
struct test_stream test_1_7_4_bot = { &preamble_1_7_4_bot, &test_case_1_7_4_bot, &postamble_1_7_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_7_5 test_group_1
#define numb_case_1_7_5 "1.7.5"
#define tpid_case_1_7_5 "SCTP_AS_V_1_7_5"
#define name_case_1_7_5 "Stream support"
#define sref_case_1_7_5 "RFC 2960, section 4.2"
#define desc_case_1_7_5 "\
Checks that the IUT supports at least 2 incoming streams and 2 outgoing\n\
streams."

#define preamble_1_7_5_top	preamble_1_iut
#define preamble_1_7_5_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that INIT message is sent from tester with Outbound Streams
 * and Maximum Inbound Streams set to 2. */

int
test_case_1_7_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_7_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_7_5_top	postamble_1_iut
#define postamble_1_7_5_bot	postamble_1_pt

struct test_stream test_1_7_5_top = { &preamble_1_7_5_top, &test_case_1_7_5_top, &postamble_1_7_5_top };
struct test_stream test_1_7_5_bot = { &preamble_1_7_5_bot, &test_case_1_7_5_bot, &postamble_1_7_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_8_1 test_group_1
#define numb_case_1_8_1 "1.8.1"
#define tpid_case_1_8_1 "SCTP_AS_I_1_8_1"
#define name_case_1_8_1 "Unrecognized Parameters in INIT"
#define sref_case_1_8_1 "RFC 2960, section 3.3.3.1"
#define desc_case_1_8_1 "\
Checks that the IUT on receipt of unrecognized TLV parameters (with MSB\n\
two bits in the parameter type set to 11) in a received INIT message,\n\
fills them in the Unrecognized Parameters of the INIT-ACK and continues\n\
processing of further parameters."

#define preamble_1_8_1_top	preamble_1_iut
#define preamble_1_8_1_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that a datagram with undefined paramter type and MSB two bits
 * in the parameter type equal to 11 is sent to tester. */

int
test_case_1_8_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_8_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_8_1_top	postamble_1_iut
#define postamble_1_8_1_bot	postamble_1_pt

struct test_stream test_1_8_1_top = { &preamble_1_8_1_top, &test_case_1_8_1_top, &postamble_1_8_1_top };
struct test_stream test_1_8_1_bot = { &preamble_1_8_1_bot, &test_case_1_8_1_bot, &postamble_1_8_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_8_2 test_group_1
#define numb_case_1_8_2 "1.8.2"
#define tpid_case_1_8_2 "SCTP_AS_I_1_8_2"
#define name_case_1_8_2 "Unrecognized Parameters - INIT - 00"
#define sref_case_1_8_2 "RFC 2960, section 3.3.3.1"
#define desc_case_1_8_2 "\
Check that the IUT on receipt of an unrecognized TLV parameters (with\n\
MSB two bits in the parameter type set to 00) in a received INIT\n\
message, does not process any further parameters and does not report\n\
it."

#define preamble_1_8_2_top	preamble_1_iut
#define preamble_1_8_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that a datagram with underfined parameter type and MSB two
 * bits in the parameter type equal to 00 is sent to the IUT. */

int
test_case_1_8_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_8_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_8_2_top	postamble_1_iut
#define postamble_1_8_2_bot	postamble_1_pt

struct test_stream test_1_8_2_top = { &preamble_1_8_2_top, &test_case_1_8_2_top, &postamble_1_8_2_top };
struct test_stream test_1_8_2_bot = { &preamble_1_8_2_bot, &test_case_1_8_2_bot, &postamble_1_8_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_8_3 test_group_1
#define numb_case_1_8_3 "1.8.3"
#define tpid_case_1_8_3 "SCTP_AS_I_1_8_3"
#define name_case_1_8_3 "Unrecognized Parameters - INIT - 01"
#define sref_case_1_8_3 "RFC 2960, section 3.3.3.1"
#define desc_case_1_8_3 "\
Checks that the IUT, on receipt of an unrecognized TLV parameter (with\n\
MSB two bits in the parameter type set to 01) in the received INIT\n\
message, does not process any further parameters and reports it using an\n\
Unrecognized Parameters parameter field in the INIT-ACK response\n\
message."

#define preamble_1_8_3_top	preamble_1_iut
#define preamble_1_8_3_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that a datagram with underfined parameter and MSB two bits in
 * the parameter type equal to 01 is send to the IUT. */

int
test_case_1_8_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_8_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_8_3_top	postamble_1_iut
#define postamble_1_8_3_bot	postamble_1_pt

struct test_stream test_1_8_3_top = { &preamble_1_8_3_top, &test_case_1_8_3_top, &postamble_1_8_3_top };
struct test_stream test_1_8_3_bot = { &preamble_1_8_3_bot, &test_case_1_8_3_bot, &postamble_1_8_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_8_4 test_group_1
#define numb_case_1_8_4 "1.8.4"
#define tpid_case_1_8_4 "SCTP_AS_I_1_8_4"
#define name_case_1_8_4 "Unrecognized Parameters - INIT - 10"
#define sref_case_1_8_4 "RFC 2960, section 3.3.3.1"
#define desc_case_1_8_4 "\
Checs that the IUT, on receipt of an unrecognized TLV parameter (with\n\
MSB two bits in the parameter type set to 10) in the received INIT\n\
message, skips this parameter and processes further parameters."

#define preamble_1_8_4_top	preamble_1_iut
#define preamble_1_8_4_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that a datagram with undefined parameter and MSB two bits in
 * the parameter type equal to 10 is sent to the IUT. */

int
test_case_1_8_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_8_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_8_4_top	postamble_1_iut
#define postamble_1_8_4_bot	postamble_1_pt

struct test_stream test_1_8_4_top = { &preamble_1_8_4_top, &test_case_1_8_4_top, &postamble_1_8_4_top };
struct test_stream test_1_8_4_bot = { &preamble_1_8_4_bot, &test_case_1_8_4_bot, &postamble_1_8_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_9_1 test_group_1
#define numb_case_1_9_1 "1.9.1"
#define tpid_case_1_9_1 "SCTP_AS_O_1_9_1"
#define name_case_1_9_1 "INIT for existing TCB"
#define sref_case_1_9_1 "RFC 2960, section 5.2.2 and SCTP-IG, section 2.6"
#define desc_case_1_9_1 "\
Checks that the IUT on receipt of an INIT message for starting\n\
asscociation with transport addresses that are already in association,\n\
responds with an INIT-ACK message."

#define preamble_1_9_1_top	preamble_1_iut
#define preamble_1_9_1_bot	preamble_1_pt

/* Association is established between PT and SUT.  Arrange the data at
 * the PT such that INIT message is sent for making an association with
 * SUT using same IP address. */

int
test_case_1_9_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_9_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_9_1_top	postamble_1_iut
#define postamble_1_9_1_bot	postamble_1_pt

struct test_stream test_1_9_1_top = { &preamble_1_9_1_top, &test_case_1_9_1_top, &postamble_1_9_1_top };
struct test_stream test_1_9_1_bot = { &preamble_1_9_1_bot, &test_case_1_9_1_bot, &postamble_1_9_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_9_2 test_group_1
#define numb_case_1_9_2 "1.9.2"
#define tpid_case_1_9_2 "SCTP_AS_O_1_9_2"
#define name_case_1_9_2 "INIT for existing TCB, new addresses"
#define sref_case_1_9_2 "RFC 2960, section 5.2.2 and SCTP-IG, section 2.6"
#define desc_case_1_9_2 "\
Checks that the IUT on receipt of an INIT message for starting an\n\
association with transport addresses that are not the same as the\n\
already established association, responds with an ABORT message with\n\
error \"Restart of an association with new addresses\""

#define preamble_1_9_2_top	preamble_1_iut
#define preamble_1_9_2_bot	preamble_1_pt

/* Association is established between PT and SUT.  Arrange the data at
 * the PT such that INIT message is sent for establishing an association
 * with SUT.  Te additional transport addresses should contain a new,
 * not yet used, IP address. */

int
test_case_1_9_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_9_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_9_2_top	postamble_1_iut
#define postamble_1_9_2_bot	postamble_1_pt

struct test_stream test_1_9_2_top = { &preamble_1_9_2_top, &test_case_1_9_2_top, &postamble_1_9_2_top };
struct test_stream test_1_9_2_bot = { &preamble_1_9_2_bot, &test_case_1_9_2_bot, &postamble_1_9_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_10_1 test_group_1
#define numb_case_1_10_1 "1.10.1"
#define tpid_case_1_10_1 "SCTP_AS_V_1_10_1"
#define name_case_1_10_1 "INIT with no address list"
#define sref_case_1_10_1 "RFC 2960, section 5.1.2 A"
#define desc_case_1_10_1 "\
Checks that the IUT, on receipt of an INIT message with no IP addresses\n\
sends an INIT-ACK message to the source IP address from where the INIT\n\
message was received."

#define preamble_1_10_1_top	preamble_1_iut
#define preamble_1_10_1_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that no IP addresses are sent in INIT. */

int
test_case_1_10_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_10_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_10_1_top	postamble_1_iut
#define postamble_1_10_1_bot	postamble_1_pt

struct test_stream test_1_10_1_top = { &preamble_1_10_1_top, &test_case_1_10_1_top, &postamble_1_10_1_top };
struct test_stream test_1_10_1_bot = { &preamble_1_10_1_bot, &test_case_1_10_1_bot, &postamble_1_10_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_10_2 test_group_1
#define numb_case_1_10_2 "1.10.2"
#define tpid_case_1_10_2 "SCTP_AS_V_1_10_2"
#define name_case_1_10_2 "INIT-ACK with no address list"
#define sref_case_1_10_2 "RFC 2960, section 5.1.2 A"
#define desc_case_1_10_2 "\
Checks that the IUT, on receipt of an INIT-ACK message with no optional\n\
IP addresses sends a COOKIE-ECHO message to the source IP address from\n\
which the INIT-ACK message was received."

#define preamble_1_10_2_top	preamble_1_iut
#define preamble_1_10_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT such that no IP addresses are sent in INIT-ACK optional IP address
 * field. */

int
test_case_1_10_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_10_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_10_2_top	postamble_1_iut
#define postamble_1_10_2_bot	postamble_1_pt

struct test_stream test_1_10_2_top = { &preamble_1_10_2_top, &test_case_1_10_2_top, &postamble_1_10_2_top };
struct test_stream test_1_10_2_bot = { &preamble_1_10_2_bot, &test_case_1_10_2_bot, &postamble_1_10_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_11_1 test_group_1
#define numb_case_1_11_1 "1.11.1"
#define tpid_case_1_11_1 "SCTP_AS_V_1_11_1"
#define name_case_1_11_1 "INIT with address list"
#define sref_case_1_11_1 "RFC 2960, section 5.1.2 C"
#define desc_case_1_11_1 "\
Checks that the IUT, on receipt of an INIT message with one or more IP\n\
addresses sends an INIT-ACK message to the source IP address from where\n\
the INIT message was received and uses all of the IP addresses plus the\n\
IP address from where the INIT message was received combined with the\n\
SCTP source port number as the destination transport address."

#define preamble_1_11_1_top	preamble_1_iut
#define preamble_1_11_1_bot	preamble_1_pt

/* Association not established between tester and SUT.  Arrange the data
 * at the tester such that one or more IP adddresses are sent in
 * INIT message. */

int
test_case_1_11_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_11_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_11_1_top	postamble_1_iut
#define postamble_1_11_1_bot	postamble_1_pt

struct test_stream test_1_11_1_top = { &preamble_1_11_1_top, &test_case_1_11_1_top, &postamble_1_11_1_top };
struct test_stream test_1_11_1_bot = { &preamble_1_11_1_bot, &test_case_1_11_1_bot, &postamble_1_11_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_11_2 test_group_1
#define numb_case_1_11_2 "1.11.2"
#define tpid_case_1_11_2 "SCTP_AS_V_1_11_2"
#define name_case_1_11_2 "INIT-ACK with address list"
#define sref_case_1_11_2 "RFC 2960, section 5.1.2 B"
#define desc_case_1_11_2 "\
Checks that the IUT, on receipt of an INIT-ACK message with one or more\n\
IP addresses uses all of these IP addresses plus the IP address from\n\
where the INIT-ACK comes combined with the SCTP source port number as\n\
the destinaiton transport address and sends a COOKIE-ECHO message to one\n\
of the transport addresses."

#define preamble_1_11_2_top	preamble_1_iut
#define preamble_1_11_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT to send one or more IP addresses in INIT-ACK message. */

int
test_case_1_11_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_11_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_11_2_top	postamble_1_iut
#define postamble_1_11_2_bot	postamble_1_pt

struct test_stream test_1_11_2_top = { &preamble_1_11_2_top, &test_case_1_11_2_top, &postamble_1_11_2_top };
struct test_stream test_1_11_2_bot = { &preamble_1_11_2_bot, &test_case_1_11_2_bot, &postamble_1_11_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_12_1 test_group_1
#define numb_case_1_12_1 "1.12.1"
#define tpid_case_1_12_1 "SCTP_AS_V_1_12_1"
#define name_case_1_12_1 "INIT with Host Name only"
#define sref_case_1_12_1 "TS 102 144, section 4.6"
#define desc_case_1_12_1 "\
Checks that the IUT on receipt of an INIT message with Host Name address\n\
and no other IP address sends an ABORT message with error \"Unresolvable\n\
Address\"."

#define preamble_1_12_1_top	preamble_1_iut
#define preamble_1_12_1_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT to send Host Name address to SUT with no other IP address in INIT
 * message. */

int
test_case_1_12_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_12_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_12_1_top	postamble_1_iut
#define postamble_1_12_1_bot	postamble_1_pt

struct test_stream test_1_12_1_top = { &preamble_1_12_1_top, &test_case_1_12_1_top, &postamble_1_12_1_top };
struct test_stream test_1_12_1_bot = { &preamble_1_12_1_bot, &test_case_1_12_1_bot, &postamble_1_12_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_12_2 test_group_1
#define numb_case_1_12_2 "1.12.2"
#define tpid_case_1_12_2 "SCTP_AS_I_1_12_2"
#define name_case_1_12_2 "INIT-ACK with Host Name only"
#define sref_case_1_12_2 "TS 102 144, section 4.5"
#define desc_case_1_12_2 "\
Checks that the IUT, on receipt of an INIT-ACK message with Host Name\n\
address and no other IP address sends an ABORT message with error\n\
\"Unresolvable Address\"."

#define preamble_1_12_2_top	preamble_1_iut
#define preamble_1_12_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT to send Host Name address with no other IP addresses in INIT-ACK
 * message. */

int
test_case_1_12_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_12_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_12_2_top	postamble_1_iut
#define postamble_1_12_2_bot	postamble_1_pt

struct test_stream test_1_12_2_top = { &preamble_1_12_2_top, &test_case_1_12_2_top, &postamble_1_12_2_top };
struct test_stream test_1_12_2_bot = { &preamble_1_12_2_bot, &test_case_1_12_2_bot, &postamble_1_12_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_13_1 test_group_1
#define numb_case_1_13_1 "1.13.1"
#define tpid_case_1_13_1 "SCTP_AS_V_1_13_1"
#define name_case_1_13_1 "INIT Supported addresses"
#define sref_case_1_13_1 "RFC 2960, section 5.1.2"
#define desc_case_1_13_1 "\
Checks that the IUT, on receipt of an INIT message with Supported\n\
address field sends an INIT-ACK message with the address of the type\n\
contained in the Supported address field in the received INIT."

#define preamble_1_13_1_top	preamble_1_iut
#define preamble_1_13_1_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at the
 * PT to send Supported Address field in INIT.  Also, the SUT is capable
 * of using the address type mentioned in the Supported Address field. */

int
test_case_1_13_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_13_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_13_1_top	postamble_1_iut
#define postamble_1_13_1_bot	postamble_1_pt

struct test_stream test_1_13_1_top = { &preamble_1_13_1_top, &test_case_1_13_1_top, &postamble_1_13_1_top };
struct test_stream test_1_13_1_bot = { &preamble_1_13_1_bot, &test_case_1_13_1_bot, &postamble_1_13_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_13_2 test_group_1
#define numb_case_1_13_2 "1.13.2"
#define tpid_case_1_13_2 "SCTP_AS_I_1_13_2"
#define name_case_1_13_2 "INIT Unsupported addresses"
#define sref_case_1_13_2 "RFC 2960, section 5.1.2"
#define desc_case_1_13_2 "\
Checks that the IUT, on receipt of an INIT message with Supported\n\
address type which the receiver is incapable of using sends an ABORT\n\
message with cause \"Unresolvable Address\"."

#define preamble_1_13_2_top	preamble_1_iut
#define preamble_1_13_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at PT
 * to send Supported address type field IPv6 to SUT in INIT message
 * (Supported address type IPv4 should not be sent).  Also, the SUT is
 * not capable of using supported addresss type, i.e. only IPv4. */

int
test_case_1_13_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_13_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_13_2_top	postamble_1_iut
#define postamble_1_13_2_bot	postamble_1_pt

struct test_stream test_1_13_2_top = { &preamble_1_13_2_top, &test_case_1_13_2_top, &postamble_1_13_2_top };
struct test_stream test_1_13_2_bot = { &preamble_1_13_2_bot, &test_case_1_13_2_bot, &postamble_1_13_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_14_1 test_group_1
#define numb_case_1_14_1 "1.14.1"
#define tpid_case_1_14_1 "SCTP_AS_I_1_14_1"
#define name_case_1_14_1 "INIT with Init-Tag zero (0)"
#define sref_case_1_14_1 "RFC 2960, section 3.3.2"
#define desc_case_1_14_1 "\
Checks that the IUT, on receipt of an INIT message with an Init-Tag of\n\
zero (0) detroys the TCB and may send an ABORT."

#define preamble_1_14_1_top	preamble_1_iut
#define preamble_1_14_1_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at PT
 * to send Initiate-tag field equal to zero to SUT in INIT message. */

int
test_case_1_14_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_14_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_14_1_top	postamble_1_iut
#define postamble_1_14_1_bot	postamble_1_pt

struct test_stream test_1_14_1_top = { &preamble_1_14_1_top, &test_case_1_14_1_top, &postamble_1_14_1_top };
struct test_stream test_1_14_1_bot = { &preamble_1_14_1_bot, &test_case_1_14_1_bot, &postamble_1_14_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_14_2 test_group_1
#define numb_case_1_14_2 "1.14.2"
#define tpid_case_1_14_2 "SCTP_AS_I_1_14_2"
#define name_case_1_14_2 "INIT-ACK with Init-Tag zero (0)"
#define sref_case_1_14_2 "RFC 2960, section 3.3.3 and SCTP-IG, section 2.22"
#define desc_case_1_14_2 "\
Checks that the IUT, on receipt of an INIT-ACK message with Init-Tag\n\
equal to zero destroys the TCB and may send an ABORT."

#define preamble_1_14_2_top	preamble_1_iut
#define preamble_1_14_2_bot	preamble_1_pt

/* Association not established between PT and SUT.  Arrange data at PT
 * to send Initiate-tag equal to zero to SUT in INIT-ACK message. */

int
test_case_1_14_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_14_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_14_2_top	postamble_1_iut
#define postamble_1_14_2_bot	postamble_1_pt

struct test_stream test_1_14_2_top = { &preamble_1_14_2_top, &test_case_1_14_2_top, &postamble_1_14_2_top };
struct test_stream test_1_14_2_bot = { &preamble_1_14_2_bot, &test_case_1_14_2_bot, &postamble_1_14_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_1_15 test_group_1
#define numb_case_1_15 "1.15"
#define tpid_case_1_15 "SCTP_AS_I_1_15"
#define name_case_1_15 "INIT to multi-homed peer"
#define sref_case_1_15 "TS 102 144, section 4.4"
#define desc_case_1_15 "\
Checks that the IUT uses all configured addresses for retransmitted INIT\n\
messages for association establishment."

#define preamble_1_15_top	preamble_1_iut
#define preamble_1_15_bot	preamble_1_pt

/* Association not established between PT and SUT.  PT is preconfigured
 * to be multi-homed with two addresses X and Y.  Arrange data at PT so
 * no INIT-ACK message is sent to SUT in response to INIT. */

int
test_case_1_15_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_1_15_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_1_15_top	postamble_1_iut
#define postamble_1_15_bot	postamble_1_pt

struct test_stream test_1_15_top = { &preamble_1_15_top, &test_case_1_15_top, &postamble_1_15_top };
struct test_stream test_1_15_bot = { &preamble_1_15_bot, &test_case_1_15_bot, &postamble_1_15_bot };

/* ========================================================================= */

#define test_group_2 "Association Termination (AT)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_2 test_group_2
#define numb_case_2_2 "2.2"
#define tpid_case_2_2 "SCTP_AT_V_2_2"
#define name_case_2_2 "ABORT processing"
#define sref_case_2_2 "RFC 2960, section 9.1"
#define desc_case_2_2 "\
Checks that the IUT, on receipt of an ABORT removes the association.\n\
Further message exchanges between the tester and IUT need to take place\n\
to verify the removal of the association from outside the SUT."

#define preamble_2_2_top	preamble_2_iut
#define preamble_2_2_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange data at the PT
 * to send an ABORT message to the SUT. */

int
test_case_2_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_2_top	postamble_2_iut
#define postamble_2_2_bot	postamble_2_pt

struct test_stream test_2_2_top = { &preamble_2_2_top, &test_case_2_2_top, &postamble_2_2_top };
struct test_stream test_2_2_bot = { &preamble_2_2_bot, &test_case_2_2_bot, &postamble_2_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_3 test_group_2
#define numb_case_2_3 "2.3"
#define tpid_case_2_3 "SCTP_AT_I_2_3"
#define name_case_2_3 "SHUTDOWN data outstanding"
#define sref_case_2_3 "RFC 2960, section 9.2"
#define desc_case_2_3 "\
Checks that the IUT, on receipt of a Terminate primitive will send a\n\
SHUTDOWN message to its peer only when all outstanding DATA hass been\n\
acknowledged by the PT."

#define preamble_2_3_top	preamble_2_iut
#define preamble_2_3_bot	preamble_2_pt

/* Association established between PT and SUT and DATA is sent from SUT
 * to PT.  Arrange the data at SUT to issue Terminate primitive to IUT.
 */

int
test_case_2_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_3_top	postamble_2_iut
#define postamble_2_3_bot	postamble_2_pt

struct test_stream test_2_3_top = { &preamble_2_3_top, &test_case_2_3_top, &postamble_2_3_top };
struct test_stream test_2_3_bot = { &preamble_2_3_bot, &test_case_2_3_bot, &postamble_2_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_4 test_group_2
#define numb_case_2_4 "2.4"
#define tpid_case_2_4 "SCTP_AT_I_2_4"
#define name_case_2_4 "T2-Shutdown"
#define sref_case_2_4 "RFC 2960, section 9.2"
#define desc_case_2_4 "\
Checks that the IUT starts the T2-Shutdown timer and after its expiry a\n\
SHUTDOWN message is sent again."

#define preamble_2_4_top	preamble_2_iut
#define preamble_2_4_bot	preamble_2_pt

/* Association establishe between PT and SUT.  Arrange PT so no
 * SHUTDOWN-ACK or DATA is sent in response to SHUTDOWN. */

int
test_case_2_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_4_top	postamble_2_iut
#define postamble_2_4_bot	postamble_2_pt

struct test_stream test_2_4_top = { &preamble_2_4_top, &test_case_2_4_top, &postamble_2_4_top };
struct test_stream test_2_4_bot = { &preamble_2_4_bot, &test_case_2_4_bot, &postamble_2_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_5 test_group_2
#define numb_case_2_5 "2.5"
#define tpid_case_2_5 "SCTP_AT_I_2_5"
#define name_case_2_5 "T5-Shutdown-Guard Timer"
#define sref_case_2_5 "RFC 2960, section 9.2, and SCTP-IG, section 2.12.2"
#define desc_case_2_5 "\
Checks that the IUT, after retransmitting SHUTDOWN message for\n\
ASSOCIATION.MAX.RETRANS or timeout occurs for T5-Shutdown-Guard timer,\n\
removes the association and optionally sends ABORT.  Further message\n\
exchanges between PT and IUT need to take place to verify the removal of\n\
the association from outside the SUT."

#define preamble_2_5_top	preamble_2_iut
#define preamble_2_5_bot	preamble_2_pt

/* Association estalished between PT and IUT.  Arrange the data at the
 * PT such that in response to SHUTDOWN no SHUTDOWN-ACK or DATA message
 * is sent. */

int
test_case_2_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_5_top	postamble_2_iut
#define postamble_2_5_bot	postamble_2_pt

struct test_stream test_2_5_top = { &preamble_2_5_top, &test_case_2_5_top, &postamble_2_5_top };
struct test_stream test_2_5_bot = { &preamble_2_5_bot, &test_case_2_5_bot, &postamble_2_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_6 test_group_2
#define numb_case_2_6 "2.6"
#define tpid_case_2_6 "SCTP_AT_V_2_6"
#define name_case_2_6 "SHUTDOWN-ACK in Shutdown Sent state"
#define sref_case_2_6 "RFC 2960, section 8.5.1 C"
#define desc_case_2_6 "\
Checks that the IUT, on receive of SHUTDOWN-ACK message in Shutdown Sent\n\
state send a SHUTDOWN COMPLETE message and terminates the association."

#define preamble_2_6_top	preamble_2_iut
#define preamble_2_6_bot	preamble_2_pt

/* Association is established between PT and SUT.  Arrange the PT to
 * send SHUTDOWN-ACK in response to SHUTDOWN message. */

int
test_case_2_6_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_6_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_6_top	postamble_2_iut
#define postamble_2_6_bot	postamble_2_pt

struct test_stream test_2_6_top = { &preamble_2_6_top, &test_case_2_6_top, &postamble_2_6_top };
struct test_stream test_2_6_bot = { &preamble_2_6_bot, &test_case_2_6_bot, &postamble_2_6_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_7_1 test_group_2
#define numb_case_2_7_1 "2.7.1"
#define tpid_case_2_7_1 "SCTP_AT_I_2_7_1"
#define name_case_2_7_1 "Data for transmission in Shutdown Sent state"
#define sref_case_2_7_1 "RFC 2960, section 9.2"
#define desc_case_2_7_1 "\
Checks that the IUT, if it is in Shutdown sent state and receives data\n\
for transmission from upper layer, does not send this new data."

#define preamble_2_7_1_top	preamble_2_iut
#define preamble_2_7_1_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange data in SUT so
 * upper layers send data when in Shutdown sent state. */

int
test_case_2_7_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_7_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_7_1_top	postamble_2_iut
#define postamble_2_7_1_bot	postamble_2_pt

struct test_stream test_2_7_1_top = { &preamble_2_7_1_top, &test_case_2_7_1_top, &postamble_2_7_1_top };
struct test_stream test_2_7_1_bot = { &preamble_2_7_1_bot, &test_case_2_7_1_bot, &postamble_2_7_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_7_2 test_group_2
#define numb_case_2_7_2 "2.7.2"
#define tpid_case_2_7_2 "SCTP_AT_I_2_7_2"
#define name_case_2_7_2 "Data for transmission in Shutdown Received state"
#define sref_case_2_7_2 "RFC 2960, section 9.2"
#define desc_case_2_7_2 "\
Checks that the IUT, if it is in Shutdown received state and receives\n\
data for transmission from upper layer, does not send this new data."

#define preamble_2_7_2_top	preamble_2_iut
#define preamble_2_7_2_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange the data in SUT
 * such that upper layer sends data to transmit when it is in Shutdown
 * received state. */

int
test_case_2_7_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_7_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_7_2_top	postamble_2_iut
#define postamble_2_7_2_bot	postamble_2_pt

struct test_stream test_2_7_2_top = { &preamble_2_7_2_top, &test_case_2_7_2_top, &postamble_2_7_2_top };
struct test_stream test_2_7_2_bot = { &preamble_2_7_2_bot, &test_case_2_7_2_bot, &postamble_2_7_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_7_3 test_group_2
#define numb_case_2_7_3 "2.7.3"
#define tpid_case_2_7_3 "SCTP_AT_I_2_7_3"
#define name_case_2_7_3 "Data for transmission in Shutdown Pending state"
#define sref_case_2_7_3 "RFC 2960, section 9.2"
#define desc_case_2_7_3 "\
Checks that the IUT, if it is in Shutdown Pending state and receives\n\
data for transmission from upper layer, does not send this new data."

#define preamble_2_7_3_top	preamble_2_iut
#define preamble_2_7_3_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange SUT upper layers
 * to send data to transmit when in Shutdown pending state. */

int
test_case_2_7_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_7_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_7_3_top	postamble_2_iut
#define postamble_2_7_3_bot	postamble_2_pt

struct test_stream test_2_7_3_top = { &preamble_2_7_3_top, &test_case_2_7_3_top, &postamble_2_7_3_top };
struct test_stream test_2_7_3_bot = { &preamble_2_7_3_bot, &test_case_2_7_3_bot, &postamble_2_7_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_7_4 test_group_2
#define numb_case_2_7_4 "2.7.4"
#define tpid_case_2_7_4 "SCTP_AT_I_2_7_4"
#define name_case_2_7_4 "Data for transmission, Shutdown Ack Sent state"
#define sref_case_2_7_4 "RFC 2960, section 9.2"
#define desc_case_2_7_4 "\
Checks that the IUT, if it is in Shutdown Ack Sent state and receives\n\
data for transmission from upper layer, does not send this new data."

#define preamble_2_7_4_top	preamble_2_iut
#define preamble_2_7_4_bot	preamble_2_pt

/* Assocation established between PT and SUT.  Arrange the SUT uuper
 * layer to send data to transmit when in Shutdown Ack Sent state. */

int
test_case_2_7_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_7_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_7_4_top	postamble_2_iut
#define postamble_2_7_4_bot	postamble_2_pt

struct test_stream test_2_7_4_top = { &preamble_2_7_4_top, &test_case_2_7_4_top, &postamble_2_7_4_top };
struct test_stream test_2_7_4_bot = { &preamble_2_7_4_bot, &test_case_2_7_4_bot, &postamble_2_7_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_8 test_group_2
#define numb_case_2_8 "2.8"
#define tpid_case_2_8 "SCTP_AT_I_2_8"
#define name_case_2_8 "Data received, Shutdown Sent state"
#define sref_case_2_8 "RFC 2960, section 9.2"
#define desc_case_2_8 "\
Checks that the IUT, if it is in Shutdown Sent state and receives data,\n\
acknowledges the message and restarts the T2-Shutdown timer."

#define preamble_2_8_top	preamble_2_iut
#define preamble_2_8_bot	preamble_2_pt

/* Assocation established between PT and SUT.  Arrange the PT so after
 * receiving SHUTDOWN message, DATA message is sent to SUT. */

int
test_case_2_8_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_8_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_8_top	postamble_2_iut
#define postamble_2_8_bot	postamble_2_pt

struct test_stream test_2_8_top = { &preamble_2_8_top, &test_case_2_8_top, &postamble_2_8_top };
struct test_stream test_2_8_bot = { &preamble_2_8_bot, &test_case_2_8_bot, &postamble_2_8_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_9 test_group_2
#define numb_case_2_9 "2.9"
#define tpid_case_2_9 "SCTP_AT_I_2_9"
#define name_case_2_9 ""
#define sref_case_2_9 "RFC 2960, section 9.2"
#define desc_case_2_9 "\
Checks that the IUT, if it is in Shutdown Received state and receives\n\
data, discards the data."

#define preamble_2_9_top	preamble_2_iut
#define preamble_2_9_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange PT to send DATA
 * to SUT after sending SHUTDOWN message.  Also, in SUT there is no
 * outstanding DATA for which SACK has not come from PT. */

int
test_case_2_9_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_9_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_9_top	postamble_2_iut
#define postamble_2_9_bot	postamble_2_pt

struct test_stream test_2_9_top = { &preamble_2_9_top, &test_case_2_9_top, &postamble_2_9_top };
struct test_stream test_2_9_bot = { &preamble_2_9_bot, &test_case_2_9_bot, &postamble_2_9_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_10 test_group_2
#define numb_case_2_10 "2.10"
#define tpid_case_2_10 "SCTP_AT_I_2_10"
#define name_case_2_10 "SHUTDOWN received, outstanding data"
#define sref_case_2_10 "RFC 2960, section 9.2"
#define desc_case_2_10 "\
Checks that the IUT, if there is still outstanding DATA, does not send a\n\
SHUTDOWN-ACK on reception of a SHUTDOWN (even recevied multiple times)."

#define preamble_2_10_top	preamble_2_iut
#define preamble_2_10_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange PT to send
 * SHUTDOWN message to IUT.  The SHUTDOWN should not acknowledge the
 * last used DATA of the SUT, i.e. that in IUT there is still
 * outstanding DATA that has not been acnowledged.  Then send a second
 * SHUTDOWN that again does not acknowledge the last used DATA of the
 * IUT. */

int
test_case_2_10_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_10_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_10_top	postamble_2_iut
#define postamble_2_10_bot	postamble_2_pt

struct test_stream test_2_10_top = { &preamble_2_10_top, &test_case_2_10_top, &postamble_2_10_top };
struct test_stream test_2_10_bot = { &preamble_2_10_bot, &test_case_2_10_bot, &postamble_2_10_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_11 test_group_2
#define numb_case_2_11 "2.11"
#define tpid_case_2_11 "SCTP_AT_I_2_11"
#define name_case_2_11 "T2-Shutdown timer"
#define sref_case_2_11 "RFC 2960, section 9.2"
#define desc_case_2_11 "\
Checks that the IUT, after expiry of T2-Shutdown timer, sends a\n\
SHUTDOWN-ACK message again."

#define preamble_2_11_top	preamble_2_iut
#define preamble_2_11_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange the PT to not
 * send SHUTDOWN-COMPLETE in response to SHUTDOWN-ACK. */

int
test_case_2_11_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_11_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_11_top	postamble_2_iut
#define postamble_2_11_bot	postamble_2_pt

struct test_stream test_2_11_top = { &preamble_2_11_top, &test_case_2_11_top, &postamble_2_11_top };
struct test_stream test_2_11_bot = { &preamble_2_11_bot, &test_case_2_11_bot, &postamble_2_11_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_12 test_group_2
#define numb_case_2_12 "2.12"
#define tpid_case_2_12 "SCTP_AT_I_2_12"
#define name_case_2_12 "SHUTDOWN-ACK, excessive retransmissions"
#define sref_case_2_12 "RFC 2960, section 9.2"
#define desc_case_2_12 "\
Checks that the IUT, after retransmitting SHUTDOWN-ACK message for\n\
ASSOCIATION.MAX.RETRANS times, removes the association and optionally\n\
sends an ABORT."

#define preamble_2_12_top	preamble_2_iut
#define preamble_2_12_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange PT to not send
 * SHUTDOWN-COMPLETE in response to SHUTDOWN-ACK. */

int
test_case_2_12_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_12_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_12_top	postamble_2_iut
#define postamble_2_12_bot	postamble_2_pt

struct test_stream test_2_12_top = { &preamble_2_12_top, &test_case_2_12_top, &postamble_2_12_top };
struct test_stream test_2_12_bot = { &preamble_2_12_bot, &test_case_2_12_bot, &postamble_2_12_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_13 test_group_2
#define numb_case_2_13 "2.13"
#define tpid_case_2_13 "SCTP_AT_I_2_13"
#define name_case_2_13 "SHUTDOWN-COMPLETE in Shutdown Ack Sent state"
#define sref_case_2_13 "RFC 2960, section 9.2"
#define desc_case_2_13 "\
Checks that the IUT, if it is in Shutdown Ack Sent state, accepts a\n\
SHUTDOWN-COMPLETE message and removes the association."

#define preamble_2_13_top	preamble_2_iut
#define preamble_2_13_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange PT to send
 * SHUTDOWN-COMPLETE in response to SHUTDOWN-ACK. */

int
test_case_2_13_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_13_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_13_top	postamble_2_iut
#define postamble_2_13_bot	postamble_2_pt

struct test_stream test_2_13_top = { &preamble_2_13_top, &test_case_2_13_top, &postamble_2_13_top };
struct test_stream test_2_13_bot = { &preamble_2_13_bot, &test_case_2_13_bot, &postamble_2_13_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_2_14 test_group_2
#define numb_case_2_14 "2.14"
#define tpid_case_2_14 "SCTP_AT_V_2_14"
#define name_case_2_14 "Acknowledgement of DATA with SHUTDOWN message"
#define sref_case_2_14 "RFC 2960, section 6.2 and SCTP-IG, section 2.12"
#define desc_case_2_14 "\
Checks that the IUT, if there is still outstanding DATA, receives a\n\
SHUTDOWN that acknowledges outstanding DATA, sends a SHUTDOWN-ACK."

#define preamble_2_14_top	preamble_2_iut
#define preamble_2_14_bot	preamble_2_pt

/* Association established between PT and SUT.  Arrange PT for
 * outstanding DATA in IUT that has not been acknowledged by SACK and to
 * send SHUTDOWN message to IUT that acknowledges this DATA. */

int
test_case_2_14_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_2_14_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_2_14_top	postamble_2_iut
#define postamble_2_14_bot	postamble_2_pt

struct test_stream test_2_14_top = { &preamble_2_14_top, &test_case_2_14_top, &postamble_2_14_top };
struct test_stream test_2_14_bot = { &preamble_2_14_bot, &test_case_2_14_bot, &postamble_2_14_bot };

/* ========================================================================= */

#define test_group_3 "Invalid Message Handling (IMH)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_1 test_group_3
#define numb_case_3_1 "3.1"
#define tpid_case_3_1 "SCTP_IMH_I_3_1"
#define name_case_3_1 "INIT message, too short"
#define sref_case_3_1 "RFC 2960, section 5.1"
#define desc_case_3_1 "\
Checks that the IUT, on receipt of an invalid INIT message with message\n\
length less than the length of all mandatory parameters, discards the\n\
message or may send an ABORT message."

#define preamble_3_1_top	preamble_3_iut
#define preamble_3_1_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * INIT message to SUT with message length less that the length of all
 * mandatory parameters. */

int
test_case_3_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_1_top	postamble_3_iut
#define postamble_3_1_bot	postamble_3_pt

struct test_stream test_3_1_top = { &preamble_3_1_top, &test_case_3_1_top, &postamble_3_1_top };
struct test_stream test_3_1_bot = { &preamble_3_1_bot, &test_case_3_1_bot, &postamble_3_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_2 test_group_3
#define numb_case_3_2 "3.2"
#define tpid_case_3_2 "SCTP_IMH_I_3_2"
#define name_case_3_2 "INIT-ACK message, too small"
#define sref_case_3_2 "RFC 2960, section 5.1"
#define desc_case_3_2 "\
Checks that the IUT, on receipt of an INIT-ACK message with message\n\
length less than the length of all mandatory parameters, discards the\n\
message or may send an ABORT message."

#define preamble_3_2_top	preamble_3_iut
#define preamble_3_2_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * INIT-ACK message to SUT with message length less than the length of
 * all mandatory parameters. */

int
test_case_3_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_2_top	postamble_3_iut
#define postamble_3_2_bot	postamble_3_pt

struct test_stream test_3_2_top = { &preamble_3_2_top, &test_case_3_2_top, &postamble_3_2_top };
struct test_stream test_3_2_bot = { &preamble_3_2_bot, &test_case_3_2_bot, &postamble_3_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_3 test_group_3
#define numb_case_3_3 "3.3"
#define tpid_case_3_3 "SCTP_IMH_I_3_3"
#define name_case_3_3 "COOKIE-ECHO, invalid verification tag"
#define sref_case_3_3 "RFC 2960, section 8.5 and SCTP-IG, section 2.35.2"
#define desc_case_3_3 "\
Checks that the IUT, on receipt of a COOKIE-ECHO message with an invalid\n\
verification tag, discards the COOKIE-ECHO message."

#define preamble_3_3_top	preamble_3_iut
#define preamble_3_3_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT so a
 * COOKIE-ECHO message with a different verification tag (different from
 * that received in INIT-ACK) is sent in repsonse to INIT-ACK. */

int
test_case_3_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_3_top	postamble_3_iut
#define postamble_3_3_bot	postamble_3_pt

struct test_stream test_3_3_top = { &preamble_3_3_top, &test_case_3_3_top, &postamble_3_3_top };
struct test_stream test_3_3_bot = { &preamble_3_3_bot, &test_case_3_3_bot, &postamble_3_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_4 test_group_3
#define numb_case_3_4 "3.4"
#define tpid_case_3_4 "SCTP_IMH_I_3_4"
#define name_case_3_4 "INIT, wrong checksum"
#define sref_case_3_4 "RFC 2960, section 6.8 (2)"
#define desc_case_3_4 "\
Checks that the IUT, upon receipt of an INIT message with wrong CRC-32c\n\
checksum, discards the INIT message."

#define preamble_3_4_top	preamble_3_iut
#define preamble_3_4_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * INIT message to SUT with wrong CRC-32c checksum. */

int
test_case_3_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_4_top	postamble_3_iut
#define postamble_3_4_bot	postamble_3_pt

struct test_stream test_3_4_top = { &preamble_3_4_top, &test_case_3_4_top, &postamble_3_4_top };
struct test_stream test_3_4_bot = { &preamble_3_4_bot, &test_case_3_4_bot, &postamble_3_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_5 test_group_3
#define numb_case_3_5 "3.5"
#define tpid_case_3_5 "SCTP_IMH_I_3_5"
#define name_case_3_5 "COOKIE-ECHO, wrong cookie"
#define sref_case_3_5 "RFC 2960, section 5.1.5 (1 and 2)"
#define desc_case_3_5 "\
Checks that the IUT, upon receipt of a COOKIE-ECHO message with other\n\
cookie than sent in INIT-ACK, discards the message."

#define preamble_3_5_top	preamble_3_iut
#define preamble_3_5_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * COOKIE-ECHO message with cookie different from received cookie in
 * INIT-ACK. */

int
test_case_3_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_5_top	postamble_3_iut
#define postamble_3_5_bot	postamble_3_pt

struct test_stream test_3_5_top = { &preamble_3_5_top, &test_case_3_5_top, &postamble_3_5_top };
struct test_stream test_3_5_bot = { &preamble_3_5_bot, &test_case_3_5_bot, &postamble_3_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_6 test_group_3
#define numb_case_3_6 "3.6"
#define tpid_case_3_6 "SCTP_IMH_I_3_6"
#define name_case_3_6 "COOKIE-ECHO, expired cookie"
#define sref_case_3_6 "RFC 2960, section 5.1.5 (3)"
#define desc_case_3_6 "\
Checks that the IUT, on receipt of a COOKIE-ECHO message after lifetime\n\
received in INIT-ACK messae has expired, should send an ERROR with cause\n\
\"Stale Cookie Error\"."

#define preamble_3_6_top	preamble_3_iut
#define preamble_3_6_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * COOKIE-ECHO mesage after life time of the received cookie in INIT-ACK
 * message has expired. */

int
test_case_3_6_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_6_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_6_top	postamble_3_iut
#define postamble_3_6_bot	postamble_3_pt

struct test_stream test_3_6_top = { &preamble_3_6_top, &test_case_3_6_top, &postamble_3_6_top };
struct test_stream test_3_6_bot = { &preamble_3_6_bot, &test_case_3_6_bot, &postamble_3_6_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_7 test_group_3
#define numb_case_3_7 "3.7"
#define tpid_case_3_7 "SCTP_IMH_I_3_7"
#define name_case_3_7 "ABORT, wrong verification tag"
#define sref_case_3_7 "RFC 2960, section 8.5.1 B"
#define desc_case_3_7 "\
Checks that the IUT, upon receipt of an ABORT message with incorrect\n\
verification tag, discards the message."

#define preamble_3_7_top	preamble_3_iut
#define preamble_3_7_bot	preamble_3_pt

/* Association established between PT and SUT.  Arrange PT to send ABORT
 * to SUT with incorrect verification tag. */

int
test_case_3_7_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_7_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_7_top	postamble_3_iut
#define postamble_3_7_bot	postamble_3_pt

struct test_stream test_3_7_top = { &preamble_3_7_top, &test_case_3_7_top, &postamble_3_7_top };
struct test_stream test_3_7_bot = { &preamble_3_7_bot, &test_case_3_7_bot, &postamble_3_7_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_8 test_group_3
#define numb_case_3_8 "3.8"
#define tpid_case_3_8 "SCTP_IMH_I_3_8"
#define name_case_3_8 "INIT, short packet"
#define sref_case_3_8 "RFC 2960, section 5.1"
#define desc_case_3_8 "\
Checks that the IUT, on receipt of an INIT message with a packet length\n\
smaller than the Chunk length defined, sends an ABORT message or\n\
discards the message."

#define preamble_3_8_top	preamble_3_iut
#define preamble_3_8_bot	preamble_3_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * INIT with chunk length greater than the length of the message. */

int
test_case_3_8_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_8_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_8_top	postamble_3_iut
#define postamble_3_8_bot	postamble_3_pt

struct test_stream test_3_8_top = { &preamble_3_8_top, &test_case_3_8_top, &postamble_3_8_top };
struct test_stream test_3_8_bot = { &preamble_3_8_bot, &test_case_3_8_bot, &postamble_3_8_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_9 test_group_3
#define numb_case_3_9 "3.9"
#define tpid_case_3_9 "SCTP_IMH_I_3_9"
#define name_case_3_9 "SHUTDOWN, invalid verification tag"
#define sref_case_3_9 "RFC 2960, section 8.5.1 C"
#define desc_case_3_9 "\
Checks that the IUT, on receipt of a SHUTDOWN-ACK message with invalid\n\
verification tag, discards the message."

#define preamble_3_9_top	preamble_3_iut
#define preamble_3_9_bot	preamble_3_pt

/* Association established between PT and SUT.  Arrange PT to send
 * SHUTDOWN-ACK message with invalid verification tag to SUT in Shutdown
 * Sent state. */

int
test_case_3_9_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_9_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_9_top	postamble_3_iut
#define postamble_3_9_bot	postamble_3_pt

struct test_stream test_3_9_top = { &preamble_3_9_top, &test_case_3_9_top, &postamble_3_9_top };
struct test_stream test_3_9_bot = { &preamble_3_9_bot, &test_case_3_9_bot, &postamble_3_9_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_3_10 test_group_3
#define numb_case_3_10 "3.10"
#define tpid_case_3_10 "SCTP_IMH_I_3_10"
#define name_case_3_10 "SHUTDOWN-COMPLETE, invalid verification tag"
#define sref_case_3_10 "RFC 2960, section 8.5.1 C"
#define desc_case_3_10 "\
Checks that the IUT, on receipt of a SHUTDOWN-COMPLETE message with\n\
invalid verification tag, discards the message."

#define preamble_3_10_top	preamble_3_iut
#define preamble_3_10_bot	preamble_3_pt

/* Association established between PT and SUT.  Arrange PT to send
 * SHUTDOWN-COMPLETE with invalid verification tag to SUT in Shutdown
 * Ack Sent state. */

int
test_case_3_10_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_3_10_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_3_10_top	postamble_3_iut
#define postamble_3_10_bot	postamble_3_pt

struct test_stream test_3_10_top = { &preamble_3_10_top, &test_case_3_10_top, &postamble_3_10_top };
struct test_stream test_3_10_bot = { &preamble_3_10_bot, &test_case_3_10_bot, &postamble_3_10_bot };

/* ========================================================================= */

#define test_group_4 "Duplicate Messages (DM)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_1 test_group_4
#define numb_case_4_1 "4.1"
#define tpid_case_4_1 "SCTP_DM_O_4_1"
#define name_case_4_1 "INIT, collision"
#define sref_case_4_1 "RFC 2960, section 5.2.1"
#define desc_case_4_1 "\
Checks that the IUT, on receipt of an INIT message after sending an INIT\n\
message on its own, sends an INIT-ACK message."

#define preamble_4_1_top	preamble_4_iut
#define preamble_4_1_bot	preamble_4_pt

/* Association not established between PT and SUT.  Arrange PT to send
 * INIT to SUT upon reception of an INIT from the SUT. */

int
test_case_4_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_1_top	postamble_4_iut
#define postamble_4_1_bot	postamble_4_pt

struct test_stream test_4_1_top = { &preamble_4_1_top, &test_case_4_1_top, &postamble_4_1_top };
struct test_stream test_4_1_bot = { &preamble_4_1_bot, &test_case_4_1_bot, &postamble_4_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_2_1 test_group_4
#define numb_case_4_2_1 "4.2.1"
#define tpid_case_4_2_1 "SCTP_DM_O_4_2_1"
#define name_case_4_2_1 "Duplicate INIT, Established state"
#define sref_case_4_2_1 "RFC 2960, section 5.2.2"
#define desc_case_4_2_1 "\
Checks that the IUT, on reciept of an INIT message when association is\n\
already established, sends an INIT-ACK message and existing association\n\
is not disturbed."

#define preamble_4_2_1_top	preamble_4_iut
#define preamble_4_2_1_bot	preamble_4_pt

/* Association established between PT and IUT.  Arrange PT to send INIT
 * message to SUT with source IP address, destination IP addres and port
 * numbers the same as in the established association. */

int
test_case_4_2_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_2_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_2_1_top	postamble_4_iut
#define postamble_4_2_1_bot	postamble_4_pt

struct test_stream test_4_2_1_top = { &preamble_4_2_1_top, &test_case_4_2_1_top, &postamble_4_2_1_top };
struct test_stream test_4_2_1_bot = { &preamble_4_2_1_bot, &test_case_4_2_1_bot, &postamble_4_2_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_2_2 test_group_4
#define numb_case_4_2_2 "4.2.2"
#define tpid_case_4_2_2 "SCTP_DM_O_4_2_2"
#define name_case_4_2_2 "Duplicate INIT received in Shutdown Ack Sent state"
#define sref_case_4_2_2 "RFC 2960, section 9.2"
#define desc_case_4_2_2 "\
Checks that the IUT, on receipt of INIT message, after sending a\n\
SHUTDOWN-ACK message, discards the INIT message and retransmits the\n\
SHUTDOWN-ACK message."

#define preamble_4_2_2_top	preamble_4_iut
#define preamble_4_2_2_bot	preamble_4_pt

/* Association established between PT and SUT>  Arrange PT to send INIT
 * message to SUT after receiving SHUTDOWN-ACK message. */

int
test_case_4_2_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_2_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_2_2_top	postamble_4_iut
#define postamble_4_2_2_bot	postamble_4_pt

struct test_stream test_4_2_2_top = { &preamble_4_2_2_top, &test_case_4_2_2_top, &postamble_4_2_2_top };
struct test_stream test_4_2_2_bot = { &preamble_4_2_2_bot, &test_case_4_2_2_bot, &postamble_4_2_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_3 test_group_4
#define numb_case_4_3 "4.3"
#define tpid_case_4_3 "SCTP_DM_O_4_3"
#define name_case_4_3 "Duplicate INIT-ACK in Cookie Echoed state"
#define sref_case_4_3 "RFC 2960, section 5.2.3"
#define desc_case_4_3 "\
Checks that the IUT, on receipt of a duplicate INIT-ACK message, after\n\
sending a COOKIE-ECHO message, discards the INIT-ACK message."

#define preamble_4_3_top	preamble_4_iut
#define preamble_4_3_bot	preamble_4_pt

/* Association not established between PT and SUT.  Arrange PT to resent
 * the INIT-ACK to the SUT after receiving COOKIE-ECHO message. */

int
test_case_4_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_3_top	postamble_4_iut
#define postamble_4_3_bot	postamble_4_pt

struct test_stream test_4_3_top = { &preamble_4_3_top, &test_case_4_3_top, &postamble_4_3_top };
struct test_stream test_4_3_bot = { &preamble_4_3_bot, &test_case_4_3_bot, &postamble_4_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_4 test_group_4
#define numb_case_4_4 "4.4"
#define tpid_case_4_4 "SCTP_DM_O_4_4"
#define name_case_4_4 "Duplicate COOKIE-ACK"
#define sref_case_4_4 "RFC 2960, 5.2.5"
#define desc_case_4_4 "\
Checks that the IUT, on receipt of a duplicate COOKIE-ACK message, after\n\
the association is established, discards the COOKIE-ACK message."

#define preamble_4_4_top	preamble_4_iut
#define preamble_4_4_bot	preamble_4_pt

/* Association not established between PT and SUT.  Arrange PT to
 * retransmit COOKIE-ACK message to SUT after association is established
 * between PT and SUT. */

int
test_case_4_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_4_top	postamble_4_iut
#define postamble_4_4_bot	postamble_4_pt

struct test_stream test_4_4_top = { &preamble_4_4_top, &test_case_4_4_top, &postamble_4_4_top };
struct test_stream test_4_4_bot = { &preamble_4_4_bot, &test_case_4_4_bot, &postamble_4_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_5 test_group_4
#define numb_case_4_5 "4.5"
#define tpid_case_4_5 "SCTP_DM_O_4_5"
#define name_case_4_5 "SHUTDOWN collision"
#define sref_case_4_5 "RFC 2960, section 9.2"
#define desc_case_4_5 "\
Checks that the IUT, on receipt of a SHUTDONW message, after sending a\n\
SHUTDOWN message on its own, sends a SHUTDOWN-ACK message."

#define preamble_4_5_top	preamble_4_iut
#define preamble_4_5_bot	preamble_4_pt

/* Association established between PT and SUT.  Arrange PT to send
 * SHUTDOWN message after receiving SHUTDOWN message from the other end.
 */

int
test_case_4_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_5_top	postamble_4_iut
#define postamble_4_5_bot	postamble_4_pt

struct test_stream test_4_5_top = { &preamble_4_5_top, &test_case_4_5_top, &postamble_4_5_top };
struct test_stream test_4_5_bot = { &preamble_4_5_bot, &test_case_4_5_bot, &postamble_4_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_6_1 test_group_4
#define numb_case_4_6_1 "4.6.1"
#define tpid_case_4_6_1 "SCTP_DM_O_4_6_1"
#define name_case_4_6_1 "SHUTDOWN after INIT"
#define sref_case_4_6_1 "RFC 2960, section 9.2"
#define desc_case_4_6_1 "\
Checks that the IUT, on receipt of SHUTDOWN message, after sending INIT\n\
message on its own, discards the SHUTDOWN message."

#define preamble_4_6_1_top	preamble_4_iut
#define preamble_4_6_1_bot	preamble_4_pt

/* No association between PT an SUT.  Arrange PT to send SHUTDOWN
 * message to SUT after receiving INIT message from it. */

int
test_case_4_6_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_6_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_6_1_top	postamble_4_iut
#define postamble_4_6_1_bot	postamble_4_pt

struct test_stream test_4_6_1_top = { &preamble_4_6_1_top, &test_case_4_6_1_top, &postamble_4_6_1_top };
struct test_stream test_4_6_1_bot = { &preamble_4_6_1_bot, &test_case_4_6_1_bot, &postamble_4_6_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_6_2 test_group_4
#define numb_case_4_6_2 "4.6.2"
#define tpid_case_4_6_2 "SCTP_DM_O_4_6_2"
#define name_case_4_6_2 "Duplicate SHUTDOWN in closed state"
#define sref_case_4_6_2 "RFC 2960, section 9.2"
#define desc_case_4_6_2 "\
Checks that the IUT, on receipt of a SHUTDOWN mesage in the closed\n\
state, sends an ABORT message"

#define preamble_4_6_2_top	preamble_4_iut
#define preamble_4_6_2_bot	preamble_4_pt

/* Arrange PT to send SHUTDOWN message to SUT for association in the
 * closed state. */

int
test_case_4_6_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_6_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_6_2_top	postamble_4_iut
#define postamble_4_6_2_bot	postamble_4_pt

struct test_stream test_4_6_2_top = { &preamble_4_6_2_top, &test_case_4_6_2_top, &postamble_4_6_2_top };
struct test_stream test_4_6_2_bot = { &preamble_4_6_2_bot, &test_case_4_6_2_bot, &postamble_4_6_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_6_3 test_group_4
#define numb_case_4_6_3 "4.6.3"
#define tpid_case_4_6_3 "SCTP_DM_O_4_6_3"
#define name_case_4_6_3 "SHUTDOWN in Shutdown Sent state"
#define sref_case_4_6_3 "RFC 2960, section 9.2"
#define desc_case_4_6_3 "\
Checks that the IUT, on receipt of a SHUTDOWN message in Shutdown Sent\n\
state, sends a SHUTDOWN-ACK message and restarts T2-Shutdown timer."

#define preamble_4_6_3_top	preamble_4_iut
#define preamble_4_6_3_bot	preamble_4_pt

/* Association between PT and SUT.  Arrange SUT to send Terminate
 * primitive to IUT to terminate the association.  Arrange PT to send
 * SHUTDOWN to SUT. */

int
test_case_4_6_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_6_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_6_3_top	postamble_4_iut
#define postamble_4_6_3_bot	postamble_4_pt

struct test_stream test_4_6_3_top = { &preamble_4_6_3_top, &test_case_4_6_3_top, &postamble_4_6_3_top };
struct test_stream test_4_6_3_bot = { &preamble_4_6_3_bot, &test_case_4_6_3_bot, &postamble_4_6_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_7_1 test_group_4
#define numb_case_4_7_1 "4.7.1"
#define tpid_case_4_7_1 "SCTP_DM_O_4_7_1"
#define name_case_4_7_1 "SHUTDOWN-ACK in Cookie Wait state"
#define sref_case_4_7_1 "RFC 2960, sections 8.4 (1) and 8.5.1 E"
#define desc_case_4_7_1 "\
Checks that the IUT, on receipt of a SHUTDOWN-ACK message in Cookie Wait\n\
state, sends a SHUTDOWN-COMPLETE message and current state is not\n\
disturbed."

#define preamble_4_7_1_top	preamble_4_iut
#define preamble_4_7_1_bot	preamble_4_pt

/* No association between PT and SUT.  Arrange PT to send SHUTDOWN-ACK
 * message to SUT after receiving INIT message. */

int
test_case_4_7_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_7_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_7_1_top	postamble_4_iut
#define postamble_4_7_1_bot	postamble_4_pt

struct test_stream test_4_7_1_top = { &preamble_4_7_1_top, &test_case_4_7_1_top, &postamble_4_7_1_top };
struct test_stream test_4_7_1_bot = { &preamble_4_7_1_bot, &test_case_4_7_1_bot, &postamble_4_7_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_7_2 test_group_4
#define numb_case_4_7_2 "4.7.2"
#define tpid_case_4_7_2 "SCTP_DM_O_4_7_2"
#define name_case_4_7_2 "SHUTDOWN-ACK in Established state"
#define sref_case_4_7_2 "RFC 2960, section 8.5.1 E"
#define desc_case_4_7_2 "\
Checks that the IUT, on receipt of a SHUTDOWN-ACK message in Established\n\
state, discards the message or sends an ABORT."

#define preamble_4_7_2_top	preamble_4_iut
#define preamble_4_7_2_bot	preamble_4_pt

/* Association between PT and SUT.  Arrange PT to send SHUTDOWN-ACK
 * message to SUT with correct verification tag. */

int
test_case_4_7_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_7_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_7_2_top	postamble_4_iut
#define postamble_4_7_2_bot	postamble_4_pt

struct test_stream test_4_7_2_top = { &preamble_4_7_2_top, &test_case_4_7_2_top, &postamble_4_7_2_top };
struct test_stream test_4_7_2_bot = { &preamble_4_7_2_bot, &test_case_4_7_2_bot, &postamble_4_7_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_7_3 test_group_4
#define numb_case_4_7_3 "4.7_."
#define tpid_case_4_7_3 "SCTP_DM_O_4_7_3"
#define name_case_4_7_3 "SHUTDOWN-ACK receive in Shutdown Ack Sent state"
#define sref_case_4_7_3 "RFC 2960, section 9.2"
#define desc_case_4_7_3 "\
Checks that the IUT, on receipt of a SHUTDOWN-ACK message in Shutdown\n\
Ack Sent state, sends a SHUTDOWN-COMPLETE message and closes the\n\
association."

#define preamble_4_7_3_top	preamble_4_iut
#define preamble_4_7_3_bot	preamble_4_pt

/* Association established between PT and SUT.  Arrange PT to send
 * SHUTDOWN message to SUT.  Also, SHUTDOWN-ACK messag is sent to SUT
 * after receiving SHUTDOWN-ACK message. */

int
test_case_4_7_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_7_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_7_3_top	postamble_4_iut
#define postamble_4_7_3_bot	postamble_4_pt

struct test_stream test_4_7_3_top = { &preamble_4_7_3_top, &test_case_4_7_3_top, &postamble_4_7_3_top };
struct test_stream test_4_7_3_bot = { &preamble_4_7_3_bot, &test_case_4_7_3_bot, &postamble_4_7_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_8 test_group_4
#define numb_case_4_8 "4.8"
#define tpid_case_4_8 "SCTP_DM_O_4_8"
#define name_case_4_8 "COOKIE-ECHO, invalid signature"
#define sref_case_4_8 "RFC 2960, section 5.2.4"
#define desc_case_4_8 "\
Checks that the IUT, on receipt of a COOKIE-ECHO message in Established\n\
state, discards the message and current state is not disturbed."

#define preamble_4_8_top	preamble_4_iut
#define preamble_4_8_bot	preamble_4_pt

/* Association between PT and SUT.  Arrange PT to send COOKIE-ECHO
 * message with invalid Message Authentication Code (MAC) to SUT. */

int
test_case_4_8_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_8_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_8_top	postamble_4_iut
#define postamble_4_8_bot	postamble_4_pt

struct test_stream test_4_8_top = { &preamble_4_8_top, &test_case_4_8_top, &postamble_4_8_top };
struct test_stream test_4_8_bot = { &preamble_4_8_bot, &test_case_4_8_bot, &postamble_4_8_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_9 test_group_4
#define numb_case_4_9 "4.9"
#define tpid_case_4_9 "SCTP_DM_O_4_9"
#define name_case_4_9 "SHUTDOWN-COMPLETE in Cookie Wait state"
#define sref_case_4_9 "RFC 2960, section 9.2"
#define desc_case_4_9 "\
Checks that the IUT, on receipt of a SHUTDOWN-COMPLETE message in Cookie\n\
Wait state, discards the message."

#define preamble_4_9_top	preamble_4_iut
#define preamble_4_9_bot	preamble_4_pt

/* No association between PT and SUT.  Arrange PT to send
 * SHUTDOWN-COMPLETE to SUT after receiving INIT from the SUT. */

int
test_case_4_9_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_9_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_9_top	postamble_4_iut
#define postamble_4_9_bot	postamble_4_pt

struct test_stream test_4_9_top = { &preamble_4_9_top, &test_case_4_9_top, &postamble_4_9_top };
struct test_stream test_4_9_bot = { &preamble_4_9_bot, &test_case_4_9_bot, &postamble_4_9_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_4_10 test_group_4
#define numb_case_4_10 "4.10"
#define tpid_case_4_10 "SCTP_DM_O_4_10"
#define name_case_4_10 "DATA in Shutdown Ack Sent state"
#define sref_case_4_10 "RFC 2960, section 6"
#define desc_case_4_10 "\
Checks that the IUT, on receipt of DATA mesage in Shutdown Ack Sent\n\
state, discards the message and may send an ABORT."

#define preamble_4_10_top	preamble_4_iut
#define preamble_4_10_bot	preamble_4_pt

/* Association between PT and SUT.  Arrange PT to send SHUTDOWN to SUT.
 * Also DATA message is sent from PT to SUT. */

int
test_case_4_10_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_4_10_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_4_10_top	postamble_4_iut
#define postamble_4_10_bot	postamble_4_pt

struct test_stream test_4_10_top = { &preamble_4_10_top, &test_case_4_10_top, &postamble_4_10_top };
struct test_stream test_4_10_bot = { &preamble_4_10_bot, &test_case_4_10_bot, &postamble_4_10_bot };

/* ========================================================================= */

#define test_group_5 "Fault Handling (FH)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_1_1 test_group_5
#define numb_case_5_1_1 "5.1.1"
#define tpid_case_5_1_1 "SCTP_FH_I_5_1_1"
#define name_case_5_1_1 "DATA, excessive retransmission"
#define sref_case_5_1_1 "RFC 2960, section 8.1 and SCTP-IG, section 2.10.2"
#define desc_case_5_1_1 "\
Checks that the IUT, when total number of consecutive retransmissions to\n\
a peer exceeds the ASSOCIATION.MAX.RETRANS, closes the association and\n\
may send an ABORT."

#define preamble_5_1_1_top	preamble_5_iut
#define preamble_5_1_1_bot	preamble_5_pt

/* Association between PT and SUT.  The PT is multi-homed.  Arrange the
 * PT so no SACK is sent in response to DATA received from SUT. */

int
test_case_5_1_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_1_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_1_1_top	postamble_5_iut
#define postamble_5_1_1_bot	postamble_5_pt

struct test_stream test_5_1_1_top = { &preamble_5_1_1_top, &test_case_5_1_1_top, &postamble_5_1_1_top };
struct test_stream test_5_1_1_bot = { &preamble_5_1_1_bot, &test_case_5_1_1_bot, &postamble_5_1_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_1_2 test_group_5
#define numb_case_5_1_2 "5.1.2"
#define tpid_case_5_1_2 "SCTP_FH_I_5_1_2"
#define name_case_5_1_2 "DATA, non-excessive retransmission"
#define sref_case_5_1_2 "RFC 2960, section 8.1"
#define desc_case_5_1_2 "\
Checks that the IUT, on receipt of a SACK from the PT, for a DATA that\n\
has been retransmitted, resets the counter which counts the total\n\
retransmission to an endpoint."

#define preamble_5_1_2_top	preamble_5_iut
#define preamble_5_1_2_bot	preamble_5_pt

/* Association between PT and SUT.  Arrange PT so SACK is sent in
 * response to DATA received from SUT only when the DATA has been
 * retransmitted for two or three times. */

int
test_case_5_1_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_1_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_1_2_top	postamble_5_iut
#define postamble_5_1_2_bot	postamble_5_pt

struct test_stream test_5_1_2_top = { &preamble_5_1_2_top, &test_case_5_1_2_top, &postamble_5_1_2_top };
struct test_stream test_5_1_2_bot = { &preamble_5_1_2_bot, &test_case_5_1_2_bot, &postamble_5_1_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_2 test_group_5
#define numb_case_5_2 "5.2"
#define tpid_case_5_2 "SCTP_FH_V_5_2"
#define name_case_5_2 "HEARTBEAT procedure"
#define sref_case_5_2 "RFC 2960, section 8.3"
#define desc_case_5_2 "\
Checks that the IUT, on receipt of HEARTBEAT message, send a\n\
HEARTBEAT-ACK message with the information carried in the HEARTBEAT\n\
message."

#define preamble_5_2_top	preamble_5_iut
#define preamble_5_2_bot	preamble_5_pt

/* Assocation between PT and SUT.  Arrange PT so HEARTBEAT message is
 * sent to the SUT. */

int
test_case_5_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_2_top	postamble_5_iut
#define postamble_5_2_bot	postamble_5_pt

struct test_stream test_5_2_top = { &preamble_5_2_top, &test_case_5_2_top, &postamble_5_2_top };
struct test_stream test_5_2_bot = { &preamble_5_2_bot, &test_case_5_2_bot, &postamble_5_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_3_1 test_group_5
#define numb_case_5_3_1 "5.3.1"
#define tpid_case_5_3_1 "SCTP_FH_O_5_3_1"
#define name_case_5_3_1 "DATA, no association"
#define sref_case_5_3_1 "RFC 2960, section 8.4 (1)"
#define desc_case_5_3_1 "\
Checks that the IUT, on receipt of DATA message from a transport address\n\
corresponding to which there is no association, sends and ABORT\n\
message."

#define preamble_5_3_1_top	preamble_5_iut
#define preamble_5_3_1_bot	preamble_5_pt

/* No association between PT and SUT.  Arrange PT so DATA is sent to
 * SUT. */

int
test_case_5_3_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_3_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_3_1_top	postamble_5_iut
#define postamble_5_3_1_bot	postamble_5_pt

struct test_stream test_5_3_1_top = { &preamble_5_3_1_top, &test_case_5_3_1_top, &postamble_5_3_1_top };
struct test_stream test_5_3_1_bot = { &preamble_5_3_1_bot, &test_case_5_3_1_bot, &postamble_5_3_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_3_2 test_group_5
#define numb_case_5_3_2 "5.3.2"
#define tpid_case_5_3_2 "SCTP_FH_O_5_3_2"
#define name_case_5_3_2 "ABORT, no association"
#define sref_case_5_3_2 "RFC 2960, section 8.4 (2)"
#define desc_case_5_3_2 "\
Checks that the IUT, on receipt of ABORT message from a transport\n\
address corresponding to which there is no association, discards the\n\
message."

#define preamble_5_3_2_top	preamble_5_iut
#define preamble_5_3_2_bot	preamble_5_pt

/* No association between PT and SUT.  Arrange PT to send ABORT to SUT.
 */

int
test_case_5_3_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_3_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_3_2_top	postamble_5_iut
#define postamble_5_3_2_bot	postamble_5_pt

struct test_stream test_5_3_2_top = { &preamble_5_3_2_top, &test_case_5_3_2_top, &postamble_5_3_2_top };
struct test_stream test_5_3_2_bot = { &preamble_5_3_2_bot, &test_case_5_3_2_bot, &postamble_5_3_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_3_3 test_group_5
#define numb_case_5_3_3 "5.3.3"
#define tpid_case_5_3_3 "SCTP_FH_O_5_3_3"
#define name_case_5_3_3 "SHUTDOWN-ACK, no association"
#define sref_case_5_3_3 "RFC 2960, section 8.4 (5)"
#define desc_case_5_3_3 "\
Checks that the IUT, on receipt of a SHUTDOWN-ACK message from a\n\
transport address corresponding to which there is no association, sends\n\
a SHUTDOWN-COMPLETE message with T-Bit set."

#define preamble_5_3_3_top	preamble_5_iut
#define preamble_5_3_3_bot	preamble_5_pt

/* No association between PT and SUT.  Arrange PT so SHUTDOWN-ACK is
 * sent to SUT. */

int
test_case_5_3_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_3_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_3_3_top	postamble_5_iut
#define postamble_5_3_3_bot	postamble_5_pt

struct test_stream test_5_3_3_top = { &preamble_5_3_3_top, &test_case_5_3_3_top, &postamble_5_3_3_top };
struct test_stream test_5_3_3_bot = { &preamble_5_3_3_bot, &test_case_5_3_3_bot, &postamble_5_3_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_3_4 test_group_5
#define numb_case_5_3_4 "5.3.4"
#define tpid_case_5_3_4 "SCTP_FH_O_5_3_4"
#define name_case_5_3_4 "SHUTDOWN-COMPLETE, no association"
#define sref_case_5_3_4 "RFC 2960, section 8.4 (1)"
#define desc_case_5_3_4 "\
Checks that the IUT, on receipt of a SHUTDOWN-COMPLETE message from a\n\
transport address corresponding to which there is no association,\n\
discards the message."

#define preamble_5_3_4_top	preamble_5_iut
#define preamble_5_3_4_bot	preamble_5_pt

/* No association between PT and SUT.  Arrange PT to send a
 * SHUTDOWN-COMPLETE to the SUT. */

int
test_case_5_3_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_3_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_3_4_top	postamble_5_iut
#define postamble_5_3_4_bot	postamble_5_pt

struct test_stream test_5_3_4_top = { &preamble_5_3_4_top, &test_case_5_3_4_top, &postamble_5_3_4_top };
struct test_stream test_5_3_4_bot = { &preamble_5_3_4_bot, &test_case_5_3_4_bot, &postamble_5_3_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_5_3_5 test_group_5
#define numb_case_5_3_5 "5.3.5"
#define tpid_case_5_3_5 "SCTP_FH_O_5_3_5"
#define name_case_5_3_5 "COOKIE-ECHO, non-unicast source"
#define sref_case_5_3_5 "RFC 2960, section 8.4 (1)"
#define desc_case_5_3_5 "\
Checks that the IUT, on receipt of a COOKIE-ECHO from a non-unicast\n\
address where there is no association between them, discards the\n\
message."

#define preamble_5_3_5_top	preamble_5_iut
#define preamble_5_3_5_bot	preamble_5_pt

/* No association between PT and SUT.  Arrange PT so COOKIE-ECHO is sent
 * to the SUT with non-unicast source address. */

int
test_case_5_3_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_5_3_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_5_3_5_top	postamble_5_iut
#define postamble_5_3_5_bot	postamble_5_pt

struct test_stream test_5_3_5_top = { &preamble_5_3_5_top, &test_case_5_3_5_top, &postamble_5_3_5_top };
struct test_stream test_5_3_5_bot = { &preamble_5_3_5_bot, &test_case_5_3_5_bot, &postamble_5_3_5_bot };

/* ========================================================================= */

#define test_group_6 "Error (E)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_6_1 test_group_6
#define numb_case_6_1 "6.1"
#define tpid_case_6_1 "SCTP_E_O_6_1"
#define name_case_6_1 "Stale Cookie ERROR response to COOKIE-ECHO"
#define sref_case_6_1 "RFC 2960, section 5.2.6"
#define desc_case_6_1 "\
Checks that the IUT, on receipt of an ERROR message with cause \"Stale\n\
Cookie\" in the Cookie Echoed state, takes one of the following actions\n\
depending on the implementation:\n\
\n\
1. Sends a new INIT message to the PT to generate a new cookie and\n\
   reattempts the setup procedure.\n\
\n\
2. Discards the TCB and changes the state to closed.\n\
\n\
3. Sends a new INIT message to the PT adding a cookie preservative\n\
   parameter requesting the extension of lifetime of cookie."

#define preamble_6_1_top	preamble_6_iut
#define preamble_6_1_bot	preamble_6_pt

/* No association between PT and SUT.  Arrange PT to send ERROR with
 * cause "Stale Cookie" in response to COOKIE-ECHO message. */

int
test_case_6_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_6_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_6_1_top	postamble_6_iut
#define postamble_6_1_bot	postamble_6_pt

struct test_stream test_6_1_top = { &preamble_6_1_top, &test_case_6_1_top, &postamble_6_1_top };
struct test_stream test_6_1_bot = { &preamble_6_1_bot, &test_case_6_1_bot, &postamble_6_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_6_2 test_group_6
#define numb_case_6_2 "6.2"
#define tpid_case_6_2 "SCTP_E_O_6_2"
#define name_case_6_2 "Stale Cookie ERROR not in Cookie Echoed state"
#define sref_case_6_2 "RFC 2960, sections 5.2.6 and 8.4 (7)"
#define desc_case_6_2 "\
Checks that the IUT, on receipt of ERROR message with cause \"Stale\n\
Cookie\" in state other than Cookie Echoed state, discards the message\n\
and association is not disturbed."

#define preamble_6_2_top	preamble_6_iut
#define preamble_6_2_bot	preamble_6_pt

/* Assocation between PT and SUT.  Arrange PT so ERROR message with
 * cause "Stale Cookie" is sent. */

int
test_case_6_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_6_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_6_2_top	postamble_6_iut
#define postamble_6_2_bot	postamble_6_pt

struct test_stream test_6_2_top = { &preamble_6_2_top, &test_case_6_2_top, &postamble_6_2_top };
struct test_stream test_6_2_bot = { &preamble_6_2_bot, &test_case_6_2_bot, &postamble_6_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_6_3 test_group_6
#define numb_case_6_3 "6.3"
#define tpid_case_6_3 "SCTP_E_I_6_3"
#define name_case_6_3 "DATA on invalid stream"
#define sref_case_6_3 "RFC 2960, section 3.3.10.1"
#define desc_case_6_3 "\
Checks that the IUT, on receipt of DATA message to a non-existing\n\
stream, sends an ERROR message with cause \"Invalid Stream Identifier\"\n\
or aborts the association."

#define preamble_6_3_top	preamble_6_iut
#define preamble_6_3_bot	preamble_6_pt

/* Association between PT and SUT.  Arrange PT so DATA is sent to SUT on
 * a stream that does not exist for the association. */

int
test_case_6_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_6_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_6_3_top	postamble_6_iut
#define postamble_6_3_bot	postamble_6_pt

struct test_stream test_6_3_top = { &preamble_6_3_top, &test_case_6_3_top, &postamble_6_3_top };
struct test_stream test_6_3_bot = { &preamble_6_3_bot, &test_case_6_3_bot, &postamble_6_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_6_4 test_group_6
#define numb_case_6_4 "6.4"
#define tpid_case_6_4 "SCTP_E_I_6_4"
#define name_case_6_4 "INIT-ACK with no Cookie"
#define sref_case_6_4 "RFC 2960, section 3.3.10.2"
#define desc_case_6_4 "\
Checks that the IUT, on receipt of an INIT-ACK message without a Cookie\n\
parameter, sends and ERROR message with cause \"Missing Mandatory\n\
Parameter\" or may send an ABORT or no message at all."

#define preamble_6_4_top	preamble_6_iut
#define preamble_6_4_bot	preamble_6_pt

/* No association between PT and SUT.  Arrange PT so INIT-ACK is sent to
 * SUT without Cookie parameter. */

int
test_case_6_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_6_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_6_4_top	postamble_6_iut
#define postamble_6_4_bot	postamble_6_pt

struct test_stream test_6_4_top = { &preamble_6_4_top, &test_case_6_4_top, &postamble_6_4_top };
struct test_stream test_6_4_bot = { &preamble_6_4_bot, &test_case_6_4_bot, &postamble_6_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_6_5 test_group_6
#define numb_case_6_5 "6.5"
#define tpid_case_6_5 "SCTP_E_I_6_5"
#define name_case_6_5 ""
#define sref_case_6_5 "RFC 2960, section 3.3.10.8"
#define desc_case_6_5 "\
Checks that the IUT, on receipt of an INIT-ACK mesage with an unknown\n\
TLV parameter, sends an ERROR message with cause \"Unrecognized\n\
Parameters\"."

#define preamble_6_5_top	preamble_6_iut
#define preamble_6_5_bot	preamble_6_pt

/* No association between PT and IUT.  Arrange PT so INIT-ACK is sent to
 * SUT with an unknown TLV parameter. */

int
test_case_6_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_6_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_6_5_top	postamble_6_iut
#define postamble_6_5_bot	postamble_6_pt

struct test_stream test_6_5_top = { &preamble_6_5_top, &test_case_6_5_top, &postamble_6_5_top };
struct test_stream test_6_5_bot = { &preamble_6_5_bot, &test_case_6_5_bot, &postamble_6_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_6_6 test_group_6
#define numb_case_6_6 "6.6"
#define tpid_case_6_6 "SCTP_E_I_6_6"
#define name_case_6_6 ""
#define sref_case_6_6 "RFC 2960, section 3.3.10.8"
#define desc_case_6_6 "\
Checks that the IUT, on receipt of a COOKIE-ECHO mesage bundled with\n\
error (cause \"Unrecognized Parameters\"), continues to establish the\n\
association."

#define preamble_6_6_top	preamble_6_iut
#define preamble_6_6_bot	preamble_6_pt

/* No association between PT and SUT.  Arrange PT so COOKIE-ECHO bundled
 * with ERROR (cause "Unrecongnized Parameters") is send to SUT. */

int
test_case_6_6_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_6_6_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_6_6_top	postamble_6_iut
#define postamble_6_6_bot	postamble_6_pt

struct test_stream test_6_6_top = { &preamble_6_6_top, &test_case_6_6_top, &postamble_6_6_top };
struct test_stream test_6_6_bot = { &preamble_6_6_bot, &test_case_6_6_bot, &postamble_6_6_bot };

/* ========================================================================= */

#define test_group_7 "Bundling of Data Chunks with Control Chunks (BDC)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_1 test_group_7
#define numb_case_7_1 "7.1"
#define tpid_case_7_1 "SCTP_BDC_I_7_1"
#define name_case_7_1 "DATA bundled with INIT"
#define sref_case_7_1 "RFC 2960, sections 5.1 and 6.10, SCTP-IG, section 2.11"
#define desc_case_7_1 "\
Checks that the IUT, on receipt of any DATA chunks bundled with INIT\n\
chunk discards this packet."

#define preamble_7_1_top	preamble_7_iut
#define preamble_7_1_bot	preamble_7_pt

/* No association between PT and SUT.  Arrange PT so DATA chunks are
 * bundled with INIT message.  */

int
test_case_7_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_1_top	postamble_7_iut
#define postamble_7_1_bot	postamble_7_pt

struct test_stream test_7_1_top = { &preamble_7_1_top, &test_case_7_1_top, &postamble_7_1_top };
struct test_stream test_7_1_bot = { &preamble_7_1_bot, &test_case_7_1_bot, &postamble_7_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_2 test_group_7
#define numb_case_7_2 "7.2"
#define tpid_case_7_2 "SCTP_BDC_I_7_2"
#define name_case_7_2 "DATA bunled with INIT-ACK"
#define sref_case_7_2 "RFC 2960, section 6.10"
#define desc_case_7_2 "\
Checks that the IUT, on reciept of any DATA chunks bundled with INIT-ACK\n\
chunk discards this packet or sends an ABORT or accepts the INIT-ACK but\n\
ignores the DATA chunks."

#define preamble_7_2_top	preamble_7_iut
#define preamble_7_2_bot	preamble_7_pt

/* No association between PT and SUT.  Arrange PT so DATA chunks are
 * bundled wtih INIT-ACK message.  */

int
test_case_7_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_2_top	postamble_7_iut
#define postamble_7_2_bot	postamble_7_pt

struct test_stream test_7_2_top = { &preamble_7_2_top, &test_case_7_2_top, &postamble_7_2_top };
struct test_stream test_7_2_bot = { &preamble_7_2_bot, &test_case_7_2_bot, &postamble_7_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_3 test_group_7
#define numb_case_7_3 "7.3"
#define tpid_case_7_3 "SCTP_BDC_I_7_3"
#define name_case_7_3 "DATA bundled with SHUTDOWN-COMPLETE"
#define sref_case_7_3 "RFC 2960, section 6.10"
#define desc_case_7_3 "\
Checks that the IUT, on receipt of any DATA chunks bundled with\n\
SHUTDOWN-COMPLETE chunk discards this packet and remains in Shutdown Ack\n\
Sent state or sends an ABORT or accepts the SHUTDOWN-COMPLETE but\n\
ignores the DATA chunks."

#define preamble_7_3_top	preamble_7_iut
#define preamble_7_3_bot	preamble_7_pt

/* Association between PT and SUT.  Arrange SUT in Shutdown Ack Sent
 * state.  Arrange PT so DATA chunks are bundled with SHUTDOWN-COMPLETE
 * chunk with SHUTDOWN-COMPLETE as first chunk.  */

int
test_case_7_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_3_top	postamble_7_iut
#define postamble_7_3_bot	postamble_7_pt

struct test_stream test_7_3_top = { &preamble_7_3_top, &test_case_7_3_top, &postamble_7_3_top };
struct test_stream test_7_3_bot = { &preamble_7_3_bot, &test_case_7_3_bot, &postamble_7_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_4 test_group_7
#define numb_case_7_4 "7.4"
#define tpid_case_7_4 "SCTP_BDC_V_7_4"
#define name_case_7_4 "DATA bundled with COOKIE-ECHO"
#define sref_case_7_4 "RFC 2960, sections 5.1.5 (6) and 6.10"
#define desc_case_7_4 "\
Checks that the IUT, on receipt of a COOKIE-ECHO chunk bundled with DATA\n\
chunks, accepts the packet and responsds with a COOKIE-ACK bundled with\n\
a SACK or a single COOKIE-ACK and then a SACK."

#define preamble_7_4_top	preamble_7_iut
#define preamble_7_4_bot	preamble_7_pt

/* No association between PT and SUT.  Arrange PT so DATA chunks are
 * bundled with COOKIE-ECHO chunk with COOKIE-ECHO as the first chunk.  */

int
test_case_7_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_4_top	postamble_7_iut
#define postamble_7_4_bot	postamble_7_pt

struct test_stream test_7_4_top = { &preamble_7_4_top, &test_case_7_4_top, &postamble_7_4_top };
struct test_stream test_7_4_bot = { &preamble_7_4_bot, &test_case_7_4_bot, &postamble_7_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_5 test_group_7
#define numb_case_7_5 "7.5"
#define tpid_case_7_5 "SCTP_BDC_V_7_5"
#define name_case_7_5 "DATA bundled with COOKIE-ACK"
#define sref_case_7_5 "RFC 2960, section 5.1.5 (5) and 6.10"
#define desc_case_7_5 "\
Checks that the IUT, on receipt of a COOKIE-ACK chunk bundled with DATA\n\
chunks, accepts the COOKIE-ACK and responds with a SACK."

#define preamble_7_5_top	preamble_7_iut
#define preamble_7_5_bot	preamble_7_pt

/* No association between PT and SUT.  Arrange PT so DATA chunks are
 * bundled with COOKIE-ACK chunk with COOKIE-ACK as the first chunk.  */

int
test_case_7_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_5_top	postamble_7_iut
#define postamble_7_5_bot	postamble_7_pt

struct test_stream test_7_5_top = { &preamble_7_5_top, &test_case_7_5_top, &postamble_7_5_top };
struct test_stream test_7_5_bot = { &preamble_7_5_bot, &test_case_7_5_bot, &postamble_7_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_6 test_group_7
#define numb_case_7_6 "7.6"
#define tpid_case_7_6 "SCTP_BDC_V_7_6"
#define name_case_7_6 "SACK bundled with SHUTDOWN"
#define sref_case_7_6 "RFC 2960, sections 5.1.5 (5) and 6.10"
#define desc_case_7_6 "\
Checks that the IUT, on receipt of SHUTDOWN chunk bundled with a SACK,\n\
accepts the packet and responds with a SHUTDOWN-ACK."

#define preamble_7_6_top	preamble_7_iut
#define preamble_7_6_bot	preamble_7_pt

/* Association between PT and SUT.  Arrange PT so SACK is bundled with
 * SHUTDOWN message and that there is no outstanding data. */

int
test_case_7_6_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_6_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_6_top	postamble_7_iut
#define postamble_7_6_bot	postamble_7_pt

struct test_stream test_7_6_top = { &preamble_7_6_top, &test_case_7_6_top, &postamble_7_6_top };
struct test_stream test_7_6_bot = { &preamble_7_6_bot, &test_case_7_6_bot, &postamble_7_6_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_7 test_group_7
#define numb_case_7_7 "7.7"
#define tpid_case_7_7 "SCTP_BDC_V_7_7"
#define name_case_7_7 "DATA bundled with SACK"
#define sref_case_7_7 "RFC 2960, section 9.2"
#define desc_case_7_7 "\
Checks that the IUT, on receipt of a SACK bundled with DATA chunks\n\
accepts that packet and responds with a SACK for the received DATA\n\
chunks."

#define preamble_7_7_top	preamble_7_iut
#define preamble_7_7_bot	preamble_7_pt

/* Association between PT and SUT.  Arrange PT so SACK bundled with
 * DATA is sent to SUT.  */

int
test_case_7_7_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_7_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_7_top	postamble_7_iut
#define postamble_7_7_bot	postamble_7_pt

struct test_stream test_7_7_top = { &preamble_7_7_top, &test_case_7_7_top, &postamble_7_7_top };
struct test_stream test_7_7_bot = { &preamble_7_7_bot, &test_case_7_7_bot, &postamble_7_7_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_7_8 test_group_7
#define numb_case_7_8 "7.8"
#define tpid_case_7_8 "SCTP_BDC_V_7_8"
#define name_case_7_8 "DATA bundled with SHUTDOWN-ACK"
#define sref_case_7_8 "RFC 2960, section 6.10"
#define desc_case_7_8 "\
Checks that the IUT, on receipt of a SHUTDOWN-ACK bundled with DATA\n\
chunks discards the packet and remains in Shutdown Sent state, or sends\n\
an ABORT, or accepts the SHUTDOWN-ACK but ignores the DATA chunks, which\n\
would mean that the IUT has to send a SHUTDOWN-COMPLETE."

#define preamble_7_8_top	preamble_7_iut
#define preamble_7_8_bot	preamble_7_pt

/* Association between PT and SUT and SUT is in Shutdown Sent state.
 * Arrange PT so DATA is bundled with SHUTDOWN-ACK message.  */

int
test_case_7_8_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_7_8_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_7_8_top	postamble_7_iut
#define postamble_7_8_bot	postamble_7_pt

struct test_stream test_7_8_top = { &preamble_7_8_top, &test_case_7_8_top, &postamble_7_8_top };
struct test_stream test_7_8_bot = { &preamble_7_8_bot, &test_case_7_8_bot, &postamble_7_8_bot };

/* ========================================================================= */

#define test_group_8 "Data (D)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_1 test_group_8
#define numb_case_8_1 "8.1"
#define tpid_case_8_1 "SCTP_D_V_8_1"
#define name_case_8_1 "Transmitting Unsegmented Data"
#define sref_case_8_1 "RFC 2960, sections 3.3 and 6.10"
#define desc_case_8_1 "\
Checks that the IUT is able to send unsegmented user message, if\n\
resolving sCTP packet is less than or equal to MTU size."

#define preamble_8_1_top	preamble_8_iut
#define preamble_8_1_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange SUT so data for
 * transmission fits into an MTU and is smaller than the maximum size of
 * user data. */

int
test_case_8_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_1_top	postamble_8_iut
#define postamble_8_1_bot	postamble_8_pt

struct test_stream test_8_1_top = { &preamble_8_1_top, &test_case_8_1_top, &postamble_8_1_top };
struct test_stream test_8_1_bot = { &preamble_8_1_bot, &test_case_8_1_bot, &postamble_8_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_2 test_group_8
#define numb_case_8_2 "8.2"
#define tpid_case_8_2 "SCTP_D_V_8_2"
#define name_case_8_2 "Transmitting Segmented Data"
#define sref_case_8_2 "RFC 2960, section 3.3 and 6.9"
#define desc_case_8_2 "\
Checks that the IUT, is able to perfom data segmentation and transmission."

#define preamble_8_2_top	preamble_8_iut
#define preamble_8_2_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange SUT so data for
 * transmission will not fit into an MTU.  */

int
test_case_8_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_2_top	postamble_8_iut
#define postamble_8_2_bot	postamble_8_pt

struct test_stream test_8_2_top = { &preamble_8_2_top, &test_case_8_2_top, &postamble_8_2_top };
struct test_stream test_8_2_bot = { &preamble_8_2_bot, &test_case_8_2_bot, &postamble_8_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_3 test_group_8
#define numb_case_8_3 "8.3"
#define tpid_case_8_3 "SCTP_D_V_8_3"
#define name_case_8_3 "Receiving Segmented Data"
#define sref_case_8_3 "RFC 2960, sections 3.3 and 6.9"
#define desc_case_8_3 "\
Checks that the IUT is able to receive segmented data."

#define preamble_8_3_top	preamble_8_iut
#define preamble_8_3_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT to send first, middle
 * and end piece of a segmented data.  */

int
test_case_8_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_3_top	postamble_8_iut
#define postamble_8_3_bot	postamble_8_pt

struct test_stream test_8_3_top = { &preamble_8_3_top, &test_case_8_3_top, &postamble_8_3_top };
struct test_stream test_8_3_bot = { &preamble_8_3_bot, &test_case_8_3_bot, &postamble_8_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_4 test_group_8
#define numb_case_8_4 "8.4"
#define tpid_case_8_4 "SCTP_D_V_8_4"
#define name_case_8_4 "Acknowledgement"
#define sref_case_8_4 "RFC 2960, sections 6.1, 6.3.2 and 6.3.3"
#define desc_case_8_4 "\
Checks that the IUT on receipt of SACK cancels the timer T3-rtx and does not increase it."

#define preamble_8_4_top	preamble_8_iut
#define preamble_8_4_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so SACK is sent in
 * reponse to DATA.  */

int
test_case_8_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_4_top	postamble_8_iut
#define postamble_8_4_bot	postamble_8_pt

struct test_stream test_8_4_top = { &preamble_8_4_top, &test_case_8_4_top, &postamble_8_4_top };
struct test_stream test_8_4_bot = { &preamble_8_4_bot, &test_case_8_4_bot, &postamble_8_4_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_5 test_group_8
#define numb_case_8_5 "8.5"
#define tpid_case_8_5 "SCTP_D_I_8_5"
#define name_case_8_5 "Retransmission"
#define sref_case_8_5 "RFC 2960, sections 6.1, 6.3.2 and 6.3.3"
#define desc_case_8_5 "\
Checks that the IUT, after expiry of the timer T3-rtx, sends the DATA message again."

#define preamble_8_5_top	preamble_8_iut
#define preamble_8_5_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so no SACK is sent in
 * response to a DATA message.  */

int
test_case_8_5_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_5_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_5_top	postamble_8_iut
#define postamble_8_5_bot	postamble_8_pt

struct test_stream test_8_5_top = { &preamble_8_5_top, &test_case_8_5_top, &postamble_8_5_top };
struct test_stream test_8_5_bot = { &preamble_8_5_bot, &test_case_8_5_bot, &postamble_8_5_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_6 test_group_8
#define numb_case_8_6 "8.6"
#define tpid_case_8_6 "SCTP_D_O_8_6"
#define name_case_8_6 "Reception of Duplicate DATA"
#define sref_case_8_6 "RFC 2960, sections 3.3.4 and 3.3.4"
#define desc_case_8_6 "\
Checks that the IUT, on receipt of duplicate DATA chunks should report\n\
this in a SACK and number of duplicate TSN count should be reset once\n\
reported in SACK."

#define preamble_8_6_top	preamble_8_iut
#define preamble_8_6_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT to send duplicate DATA
 * to SUT.  */

int
test_case_8_6_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_6_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_6_top	postamble_8_iut
#define postamble_8_6_bot	postamble_8_pt

struct test_stream test_8_6_top = { &preamble_8_6_top, &test_case_8_6_top, &postamble_8_6_top };
struct test_stream test_8_6_bot = { &preamble_8_6_bot, &test_case_8_6_bot, &postamble_8_6_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_7 test_group_8
#define numb_case_8_7 "8.7"
#define tpid_case_8_7 "SCTP_D_O_8_7"
#define name_case_8_7 "Zero Window Probe"
#define sref_case_8_7 "RFC 2960, section 6.1 and SCTP-IG, section 2.15"
#define desc_case_8_7 "\
Checks that the IUT, if its peer's rwnd indicates that the peer has no\n\
buffer space, does not transmit more than one DATA."

#define preamble_8_7_top	preamble_8_iut
#define preamble_8_7_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so rwnd=0 is sent in
 * SACK in response to DATA message.  */

int
test_case_8_7_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_7_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_7_top	postamble_8_iut
#define postamble_8_7_bot	postamble_8_pt

struct test_stream test_8_7_top = { &preamble_8_7_top, &test_case_8_7_top, &postamble_8_7_top };
struct test_stream test_8_7_bot = { &preamble_8_7_bot, &test_case_8_7_bot, &postamble_8_7_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_8 test_group_8
#define numb_case_8_8 "8.8"
#define tpid_case_8_8 "SCTP_D_O_8_8"
#define name_case_8_8 "Zero Window Probe, Segmeneted Data"
#define sref_case_8_8 "RFC 2960, section 6.1 and SCTP-IG, section 2.15"
#define desc_case_8_8 "\
Checks that the IUT, if its peer's rwnd indicates that the peer has no\n\
buffer space, does not transmit DATA, until it receives a SACK, where\n\
the rnwd indicates that the peer has buffer space again."

#define preamble_8_8_top	preamble_8_iut
#define preamble_8_8_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so rwnd=0 is sent in
 * SACK in response to DATA message.  Data should be in large size so it
 * will be transmitted segmented.  */

int
test_case_8_8_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_8_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_8_top	postamble_8_iut
#define postamble_8_8_bot	postamble_8_pt

struct test_stream test_8_8_top = { &preamble_8_8_top, &test_case_8_8_top, &postamble_8_8_top };
struct test_stream test_8_8_bot = { &preamble_8_8_bot, &test_case_8_8_bot, &postamble_8_8_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_9 test_group_8
#define numb_case_8_9 "8.9"
#define tpid_case_8_9 "SCTP_D_V_8_9"
#define name_case_8_9 "Receive Gap Reports"
#define sref_case_8_9 "RFC 2960, sections 6.1 and 6.2"
#define desc_case_8_9 "\
Checks that the IUT, before it sends new DATA chunks, first transmits\n\
any outstanding DATA chunks, which are marked for retransmission."

#define preamble_8_9_top	preamble_8_iut
#define preamble_8_9_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT to ignore a TSN and
 * sends SACK with gap in DATA.  */

int
test_case_8_9_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_9_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_9_top	postamble_8_iut
#define postamble_8_9_bot	postamble_8_pt

struct test_stream test_8_9_top = { &preamble_8_9_top, &test_case_8_9_top, &postamble_8_9_top };
struct test_stream test_8_9_bot = { &preamble_8_9_bot, &test_case_8_9_bot, &postamble_8_9_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_10 test_group_8
#define numb_case_8_10 "8.10"
#define tpid_case_8_10 "SCTP_D_V_8_10"
#define name_case_8_10 "SACK from alternate address"
#define sref_case_8_10 "RFC 2960, section 6.6"
#define desc_case_8_10 "\
Checks that the IUT, if it sends DATA to a multihomed endpoint on one\n\
address and receive a SACK from the alternate address in that host,\n\
accepts the SACK."

#define preamble_8_10_top	preamble_8_iut
#define preamble_8_10_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so on reception of a
 * packet containing a DATA chunk, the packet containing the
 * corresponding SACK chunk is sent with source address different from
 * the destination address of the receive packet containing the DATA
 * chunk. */

int
test_case_8_10_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_10_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_10_top	postamble_8_iut
#define postamble_8_10_bot	postamble_8_pt

struct test_stream test_8_10_top = { &preamble_8_10_top, &test_case_8_10_top, &postamble_8_10_top };
struct test_stream test_8_10_bot = { &preamble_8_10_bot, &test_case_8_10_bot, &postamble_8_10_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_11 test_group_8
#define numb_case_8_11 "8.11"
#define tpid_case_8_11 "SCTP_D_I_8_11"
#define name_case_8_11 "DATA chunk with no data"
#define sref_case_8_11 "RFC 2960, section 6.2"
#define desc_case_8_11 "\
Checks that the IUT, on receipt of a DATA chunk with no user data, sends\n\
an ABORT with cause \"No User Data\"."

#define preamble_8_11_top	preamble_8_iut
#define preamble_8_11_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so DATA chunk with no
 * user data is sent to SUT.  */

int
test_case_8_11_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_11_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_11_top	postamble_8_iut
#define postamble_8_11_bot	postamble_8_pt

struct test_stream test_8_11_top = { &preamble_8_11_top, &test_case_8_11_top, &postamble_8_11_top };
struct test_stream test_8_11_bot = { &preamble_8_11_bot, &test_case_8_11_bot, &postamble_8_11_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_12 test_group_8
#define numb_case_8_12 "8.12"
#define tpid_case_8_12 "SCTP_D_O_8_12"
#define name_case_8_12 "Duplicate SACK"
#define sref_case_8_12 "RFC 2960, section 6.2"
#define desc_case_8_12 "\
Checks that the IUT, on receipt of a SACK that contains Cummulative TSN\n\
field less than the current Cummulative TSN Ack point, discards the\n\
SACK."

#define preamble_8_12_top	preamble_8_iut
#define preamble_8_12_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so a SACK chunk with
 * cummulative TSN less than the cumulative TSN ack point of SUT is sent
 * to SUT.  */

int
test_case_8_12_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_12_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_12_top	postamble_8_iut
#define postamble_8_12_bot	postamble_8_pt

struct test_stream test_8_12_top = { &preamble_8_12_top, &test_case_8_12_top, &postamble_8_12_top };
struct test_stream test_8_12_bot = { &preamble_8_12_bot, &test_case_8_12_bot, &postamble_8_12_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_13 test_group_8
#define numb_case_8_13 "8.13"
#define tpid_case_8_13 "SCTP_D_V_8_13"
#define name_case_8_13 "Receiving Large DATA"
#define sref_case_8_13 "RFC 2960, section 6.2 and TS 102 144, section 4.8"
#define desc_case_8_13 "\
Checks that the IUT, can receive DATA equal to the maximum User Data\n\
size defined by the upper layer."

#define preamble_8_13_top	preamble_8_iut
#define preamble_8_13_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so DATA chunk with size
 * of data equal to the maximum user data size of the SUT is sent to the
 * SUT.  */

int
test_case_8_13_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_13_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_13_top	postamble_8_iut
#define postamble_8_13_bot	postamble_8_pt

struct test_stream test_8_13_top = { &preamble_8_13_top, &test_case_8_13_top, &postamble_8_13_top };
struct test_stream test_8_13_bot = { &preamble_8_13_bot, &test_case_8_13_bot, &postamble_8_13_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_14 test_group_8
#define numb_case_8_14 "8.14"
#define tpid_case_8_14 "SCTP_D_V_8_14"
#define name_case_8_14 "Transmitting Large Data"
#define sref_case_8_14 "RFC 2960, section 6.2 and TS 102 144, section 4.8"
#define desc_case_8_14 "\
Checks that the IUT can send DATA equal to the maximum User Data size\n\
defined by the upper layer."

#define preamble_8_14_top	preamble_8_iut
#define preamble_8_14_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange SUT so data for transmission
 * has a size equal to the maximum User Data size defined by the upper
 * layer.  */

int
test_case_8_14_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_14_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_14_top	postamble_8_iut
#define postamble_8_14_bot	postamble_8_pt

struct test_stream test_8_14_top = { &preamble_8_14_top, &test_case_8_14_top, &postamble_8_14_top };
struct test_stream test_8_14_bot = { &preamble_8_14_bot, &test_case_8_14_bot, &postamble_8_14_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_8_15 test_group_8
#define numb_case_8_15 "8.15"
#define tpid_case_8_15 "SCTP_D_V_8_15"
#define name_case_8_15 "Receiving DATA, too large"
#define sref_case_8_15 "RFC 2960, section 6.2 and TS 102 144, section 4.8"
#define desc_case_8_15 "\
Checks that the IUT, on receipt of DATA larger than own User Data size,\n\
sends an ABORT with cause \"Out of Resource\"."

#define preamble_8_15_top	preamble_8_iut
#define preamble_8_15_bot	preamble_8_pt

/* Association between PT and SUT.  Arrange PT so DATA chunk with size
 * of data larger than user data size of the SUT is sent to SUT.  */

int
test_case_8_15_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_8_15_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_8_15_top	postamble_8_iut
#define postamble_8_15_bot	postamble_8_pt

struct test_stream test_8_15_top = { &preamble_8_15_top, &test_case_8_15_top, &postamble_8_15_top };
struct test_stream test_8_15_bot = { &preamble_8_15_bot, &test_case_8_15_bot, &postamble_8_15_bot };

/* ------------------------------------------------------------------------- */

#define test_group_9 "Acknowledgement (A)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_9_1 test_group_9
#define numb_case_9_1 "9.1"
#define tpid_case_9_1 "SCTP_A_V_9_1"
#define name_case_9_1 "Acknowledging first received DATA"
#define sref_case_9_1 "RFC 2960, section 5.1"
#define desc_case_9_1 "\
Checks that the IUT sends a SACK for the first DATA chunk it receives\n\
immediately."

#define preamble_9_1_top	preamble_9_iut
#define preamble_9_1_bot	preamble_9_pt

/* Association between PT and SUT.  Arrange PT so it sends the first
 * DATA and waits for SACK.  */

int
test_case_9_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_9_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_9_1_top	postamble_9_iut
#define postamble_9_1_bot	postamble_9_pt

struct test_stream test_9_1_top = { &preamble_9_1_top, &test_case_9_1_top, &postamble_9_1_top };
struct test_stream test_9_1_bot = { &preamble_9_1_bot, &test_case_9_1_bot, &postamble_9_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_9_2 test_group_9
#define numb_case_9_2 "9.2"
#define tpid_case_9_2 "SCTP_A_V_9_2"
#define name_case_9_2 "Acknowledgment of Multiple DATA chunks"
#define sref_case_9_2 "RFC 2960, section 6.2"
#define desc_case_9_2 "\
Checks that the IUT can acknowledge the reception of multiple DATA chunks."

#define preamble_9_2_top	preamble_9_iut
#define preamble_9_2_bot	preamble_9_pt

/* Association between PT and SUT.  Arrange PT so it sends multiple DATA
 * chunks bunded in one SCTP packet. */

int
test_case_9_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_9_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_9_2_top	postamble_9_iut
#define postamble_9_2_bot	postamble_9_pt

struct test_stream test_9_2_top = { &preamble_9_2_top, &test_case_9_2_top, &postamble_9_2_top };
struct test_stream test_9_2_bot = { &preamble_9_2_bot, &test_case_9_2_bot, &postamble_9_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_9_3 test_group_9
#define numb_case_9_3 "9.3"
#define tpid_case_9_3 "SCTP_A_O_9_3"
#define name_case_9_3 "Gap Reporting"
#define sref_case_9_3 "RFC 2960, section 6.2.1"
#define desc_case_9_3 "\
Checks that the IUT, if it detects a gap in the received data chunk\n\
sequence, sends a SACK immediately where it reports the missing TSN in\n\
the GAP ACK block."

#define preamble_9_3_top	preamble_9_iut
#define preamble_9_3_bot	preamble_9_pt

/* Association between PT and SUT.  Arrange PT so it sends a gap in the
 * data.  */

int
test_case_9_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_9_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_9_3_top	postamble_9_iut
#define postamble_9_3_bot	postamble_9_pt

struct test_stream test_9_3_top = { &preamble_9_3_top, &test_case_9_3_top, &postamble_9_3_top };
struct test_stream test_9_3_bot = { &preamble_9_3_bot, &test_case_9_3_bot, &postamble_9_3_bot };

/* ========================================================================= */

#define test_group_10 "Miscellaneous Test Cases (M)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_10_1 test_group_10
#define numb_case_10_1 "10.1"
#define tpid_case_10_1 "SCTP_M_I_10_1"
#define name_case_10_1 "Unrecognized Chunk, high bits 11"
#define sref_case_10_1 "RFC 2960, section 3.2"
#define desc_case_10_1 "\
Checks that the IUT, on receipt of an unrecognized chunk type with\n\
highest order 2 bits set to 11, sends an ERROR with cause \"Unrecognized\n\
Chunk Type\" and a SACK for the DATA chunk."

#define preamble_10_1_top	preamble_10_iut
#define preamble_10_1_bot	preamble_10_pt

/* Association between PT and SUT.  Arrange PT so a datagram with
 * reserved chunk type is sent to IUT bundled with DATA chunk and higher
 * two bits are set to 11.  */

int
test_case_10_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_10_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_10_1_top	postamble_10_iut
#define postamble_10_1_bot	postamble_10_pt

struct test_stream test_10_1_top = { &preamble_10_1_top, &test_case_10_1_top, &postamble_10_1_top };
struct test_stream test_10_1_bot = { &preamble_10_1_bot, &test_case_10_1_bot, &postamble_10_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_10_2 test_group_10
#define numb_case_10_2 "10.2"
#define tpid_case_10_2 "SCTP_M_I_10_2"
#define name_case_10_2 "Unrecognized Chunk, high bits 00"
#define sref_case_10_2 "RFC 2960, section 3.2"
#define desc_case_10_2 "\
Checks that the IUT, on receipt of an unrecognized chunk type with\n\
highest order 2 bits set to 00, discards the SCTP packet and does not\n\
process the DATA chunk."

#define preamble_10_2_top	preamble_10_iut
#define preamble_10_2_bot	preamble_10_pt

/* Association between PT and SUT.  Arrange PT so a datagram with a
 * reserved chunk type is sent to ITU bundled with DATA chunk and higher
 * to bits are set to 00.  */

int
test_case_10_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_10_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_10_2_top	postamble_10_iut
#define postamble_10_2_bot	postamble_10_pt

struct test_stream test_10_2_top = { &preamble_10_2_top, &test_case_10_2_top, &postamble_10_2_top };
struct test_stream test_10_2_bot = { &preamble_10_2_bot, &test_case_10_2_bot, &postamble_10_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_10_3 test_group_10
#define numb_case_10_3 "10.3"
#define tpid_case_10_3 "SCTP_M_I_10_3"
#define name_case_10_3 ""
#define sref_case_10_3 "RFC 2960"
#define desc_case_10_3 "\
Checks that the IUT, on receipt of an unrecognized chunk type with\n\
highest order 2 bits set to 01, discards this SCTP packet and does not\n\
process the DATA chunk.  Additionally it has to send an ERROR with cause\n\
\"Unrecognized Chunk Type\"."

#define preamble_10_3_top	preamble_10_iut
#define preamble_10_3_bot	preamble_10_pt

/* Association between PT and SUT.  Arrange PT so a datagram with
 * reserved chunk type is sent to IUT bundled with DATA chunk and higher
 * to bits are set to 01.  */

int
test_case_10_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_10_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_10_3_top	postamble_10_iut
#define postamble_10_3_bot	postamble_10_pt

struct test_stream test_10_3_top = { &preamble_10_3_top, &test_case_10_3_top, &postamble_10_3_top };
struct test_stream test_10_3_bot = { &preamble_10_3_bot, &test_case_10_3_bot, &postamble_10_3_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_10_4 test_group_10
#define numb_case_10_4 "10.4"
#define tpid_case_10_4 "SCTP_M_I_10_4"
#define name_case_10_4 ""
#define sref_case_10_4 "RFC 2960"
#define desc_case_10_4 "\
Checks that the IUT, on receipt of an unrecognized chunk type with\n\
highest order 2 bits set to 10, discards this chunk and sends a SACK for\n\
the DATA chunk."

#define preamble_10_4_top	preamble_10_iut
#define preamble_10_4_bot	preamble_10_pt

/* Association between PT and SUT.  Arrange PT so a datagrm with
 * reserved chunk type is sent to IUT bundled with DATA chunk and higher
 * two bits are set to 10.  */

int
test_case_10_4_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_10_4_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_10_4_top	postamble_10_iut
#define postamble_10_4_bot	postamble_10_pt

struct test_stream test_10_4_top = { &preamble_10_4_top, &test_case_10_4_top, &postamble_10_4_top };
struct test_stream test_10_4_bot = { &preamble_10_4_bot, &test_case_10_4_bot, &postamble_10_4_bot };

/* ========================================================================= */

#define test_group_11 "Retransmisison Timer (RT)"

/* ------------------------------------------------------------------------- */

#define tgrp_case_11_1 test_group_11
#define numb_case_11_1 "11.1"
#define tpid_case_11_1 "SCTP_M_I_11_1"
#define name_case_11_1 "T3-rtx expiry, T3-rtx increase"
#define sref_case_11_1 "RFC 2960, section 6.3.3"
#define desc_case_11_1 "\
Checks that the IUT, if timer T3-rtx expires on a destination address,\n\
increases value of RTO for that address, i.e. increases the T3-rtx\n\
timer."

#define preamble_11_1_top	preamble_11_iut
#define preamble_11_1_bot	preamble_11_pt

/* Association between PT and SUT.  Arrange PT so SACK is not sent for
 * the data received from the SUT.  */

int
test_case_11_1_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_11_1_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_11_1_top	postamble_11_iut
#define postamble_11_1_bot	postamble_11_pt

struct test_stream test_11_1_top = { &preamble_11_1_top, &test_case_11_1_top, &postamble_11_1_top };
struct test_stream test_11_1_bot = { &preamble_11_1_bot, &test_case_11_1_bot, &postamble_11_1_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_11_2 test_group_11
#define numb_case_11_2 "11.2"
#define tpid_case_11_2 "SCTP_M_I_11_2"
#define name_case_11_2 "T3-rtx expiry, RTO increase"
#define sref_case_11_2 "RFC 2960, section 6.3.3"
#define desc_case_11_2 "\
Checks that the IUT, if timer T3-rtx expires on a destination adddress,\n\
increases the value of RTO for that address."

#define preamble_11_2_top	preamble_11_iut
#define preamble_11_2_bot	preamble_11_pt

/* Association between PT and SUT.  Arrange PT so SACK is not sent for
 * the data received.  */

int
test_case_11_2_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_11_2_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_11_2_top	postamble_11_iut
#define postamble_11_2_bot	postamble_11_pt

struct test_stream test_11_2_top = { &preamble_11_2_top, &test_case_11_2_top, &postamble_11_2_top };
struct test_stream test_11_2_bot = { &preamble_11_2_bot, &test_case_11_2_bot, &postamble_11_2_bot };

/* ------------------------------------------------------------------------- */

#define tgrp_case_11_3 test_group_11
#define numb_case_11_3 "11.3"
#define tpid_case_11_3 "SCTP_M_I_11_3"
#define name_case_11_3 "T3-rtx expiry, RTO retranmission value"
#define sref_case_11_3 "RFC 2960, section 6.3.3"
#define desc_case_11_3 "\
Checks that the IUT, if it retransmits DATA to an alternate address,\n\
uses the RTO value of that address and not that of the previous\n\
address."

#define preamble_11_3_top	preamble_11_iut
#define preamble_11_3_bot	preamble_11_pt

/* Association between PT and SUT.  Arrange PT so SACK is not sent for
 * the data received.  Before sending the DATA note the T3-rtx value
 * corresponding to both IP addresses.  */

int
test_case_11_3_top(int child)
{
	return (__RESULT_SKIPPED);
}

int
test_case_11_3_bot(int child)
{
	return (__RESULT_SKIPPED);
}

#define postamble_11_3_top	postamble_11_iut
#define postamble_11_3_bot	postamble_11_pt

struct test_stream test_11_3_top = { &preamble_11_3_top, &test_case_11_3_top, &postamble_11_3_top };
struct test_stream test_11_3_bot = { &preamble_11_3_bot, &test_case_11_3_bot, &postamble_11_3_bot };

/*
 *  -------------------------------------------------------------------------
 *
 *  Test case child scheduler
 *
 *  -------------------------------------------------------------------------
 */
int
run_stream(int child, struct test_stream *stream)
{
	int result = __RESULT_SCRIPT_ERROR;
	int pre_result = __RESULT_SCRIPT_ERROR;
	int post_result = __RESULT_SCRIPT_ERROR;

	print_preamble(child);
	state = 100;
	failure_string = NULL;
	if (stream->preamble && (pre_result = stream->preamble(child)) != __RESULT_SUCCESS) {
		switch (pre_result) {
		case __RESULT_NOTAPPL:
			print_notapplicable(child);
			result = __RESULT_NOTAPPL;
			break;
		case __RESULT_SKIPPED:
			print_skipped(child);
			result = __RESULT_SKIPPED;
			break;
		default:
			print_inconclusive(child);
			result = __RESULT_INCONCLUSIVE;
			break;
		}
	} else {
		print_test(child);
		state = 200;
		failure_string = NULL;
		switch (stream->testcase(child)) {
		default:
		case __RESULT_INCONCLUSIVE:
			print_inconclusive(child);
			result = __RESULT_INCONCLUSIVE;
			break;
		case __RESULT_NOTAPPL:
			print_notapplicable(child);
			result = __RESULT_NOTAPPL;
			break;
		case __RESULT_SKIPPED:
			print_skipped(child);
			result = __RESULT_SKIPPED;
			break;
		case __RESULT_FAILURE:
			print_failed(child);
			result = __RESULT_FAILURE;
			break;
		case __RESULT_SCRIPT_ERROR:
			print_script_error(child);
			result = __RESULT_SCRIPT_ERROR;
			break;
		case __RESULT_SUCCESS:
			print_passed(child);
			result = __RESULT_SUCCESS;
			break;
		}
		print_postamble(child);
		state = 300;
		failure_string = NULL;
		if (stream->postamble && (post_result = stream->postamble(child)) != __RESULT_SUCCESS) {
			switch (post_result) {
			case __RESULT_NOTAPPL:
				print_notapplicable(child);
				result = __RESULT_NOTAPPL;
				break;
			case __RESULT_SKIPPED:
				print_skipped(child);
				result = __RESULT_SKIPPED;
				break;
			default:
				print_inconclusive(child);
				if (result == __RESULT_SUCCESS)
					result = __RESULT_INCONCLUSIVE;
				break;
			}
		}
	}
	print_test_end(child);
	exit(result);
}

/*
 *  Fork multiple children to do the actual testing.
 *  The top child (child[0]) is the process running above the tested module, the
 *  bot child (child[1]) is the process running below (at other end of pipe) the
 *  tested module.
 */

int
test_run(struct test_stream *stream[], ulong duration)
{
	int children = 0, c, i;
	pid_t this_child, child[NUM_STREAMS] = { 0, };
	int this_status, status[NUM_STREAMS] = { 0, };

	if (start_tt(duration) != __RESULT_SUCCESS)
		goto inconclusive;
	for (i = NUM_STREAMS - 1; i >= 0; i--) {
		if (stream[i]) {
			switch ((child[i] = fork())) {
			case 00:	/* we are the child */
				exit(run_stream(i, stream[i]));	/* execute stream[i] state machine */
			case -1:
				child[i] = 0;
				for (c = 0; c < NUM_STREAMS; c++)
					if (child[c])
						kill(child[c], SIGKILL);
				return __RESULT_FAILURE;
			default:	/* we are the parent */
				children++;
				break;
			}
		} else
			status[i] = __RESULT_SUCCESS;
	}
	for (; children > 0; children--) {
	      waitagain:
		if ((this_child = wait(&this_status)) > 0) {
			for (i = 0; i < NUM_STREAMS; i++)
				if (this_child == child[i])
					break;
			if (i < NUM_STREAMS) {

				if (WIFEXITED(this_status)) {
					child[i] = 0;
					if ((status[i] = WEXITSTATUS(this_status)) != __RESULT_SUCCESS)
						for (c = 0; c < NUM_STREAMS; c++)
							if (c != i && child[c])
								kill(child[c], SIGKILL);
				} else if (WIFSIGNALED(this_status)) {
					int signal = WTERMSIG(this_status);

					print_terminated(i, signal);
					child[i] = 0;
					status[i] = (signal == SIGKILL) ? __RESULT_INCONCLUSIVE : __RESULT_FAILURE;
					for (c = 0; c < NUM_STREAMS; c++)
						if (c != i && child[c])
							kill(child[c], SIGKILL);
				} else if (WIFSTOPPED(this_status)) {
					int signal = WSTOPSIG(this_status);

					print_stopped(i, signal);
					child[i] = 0;
					status[i] = __RESULT_FAILURE;
					for (c = 0; c < NUM_STREAMS; c++)
						if (c != i && child[c])
							kill(child[c], SIGKILL);
				}
			}
		} else {
			if (timer_timeout) {
				timer_timeout = 0;
				print_timeout(NUM_STREAMS);
			}
			for (c = 0; c < NUM_STREAMS; c++)
				if (child[c])
					kill(child[c], SIGKILL);
			goto waitagain;
		}
	}
	if (stop_tt() != __RESULT_SUCCESS)
		goto inconclusive;
	for (i = 0; i < NUM_STREAMS; i++)
		if (status[i] != __RESULT_NOTAPPL)
			break;
	if (i == NUM_STREAMS)
		return (__RESULT_NOTAPPL);
	for (i = 0; i < NUM_STREAMS; i++)
		if (status[i] != __RESULT_SKIPPED)
			break;
	if (i == NUM_STREAMS)
		return (__RESULT_SKIPPED);
	for (i = 0; i < NUM_STREAMS; i++)
		if (status[i] != __RESULT_FAILURE)
			break;
	if (i == NUM_STREAMS)
		return (__RESULT_FAILURE);
	for (i = 0; i < NUM_STREAMS; i++)
		if (status[i] != __RESULT_SUCCESS)
			break;
	if (i == NUM_STREAMS)
		return (__RESULT_SUCCESS);
      inconclusive:
	return (__RESULT_INCONCLUSIVE);
}

/*
 *  -------------------------------------------------------------------------
 *
 *  Test case lists
 *
 *  -------------------------------------------------------------------------
 */

struct test_case {
	const char *numb;		/* test case number */
	const char *tgrp;		/* test case group */
	const char *sgrp;		/* test case subgroup */
	const char *name;		/* test case name */
	const char *xtra;		/* test case extra information */
	const char *desc;		/* test case description */
	const char *sref;		/* test case standards section reference */
	struct test_stream *stream[NUM_STREAMS];	/* test streams */
	int (*start) (int);		/* start function */
	int (*stop) (int);		/* stop function */
	ulong duration;			/* maximum duration */
	int run;			/* whether to run this test */
	int result;			/* results of test */
	int expect;			/* expected result */
} tests[] = {
	{
		numb_case_0_1, tgrp_case_0_1, NULL, name_case_0_1, NULL, desc_case_0_1, sref_case_0_1, {
	&test_0_1_conn, &test_0_1_resp, &test_0_1_list}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_1_1, tgrp_case_1_1_1, NULL, name_case_1_1_1, NULL, desc_case_1_1_1, sref_case_1_1_1, {
	&test_1_1_1_top, &test_1_1_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_1_2, tgrp_case_1_1_2, NULL, name_case_1_1_2, NULL, desc_case_1_1_2, sref_case_1_1_2, {
	&test_1_1_2_top, &test_1_1_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_2_1, tgrp_case_1_2_1, NULL, name_case_1_2_1, NULL, desc_case_1_2_1, sref_case_1_2_1, {
	&test_1_2_1_top, &test_1_2_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_2_2, tgrp_case_1_2_2, NULL, name_case_1_2_2, NULL, desc_case_1_2_2, sref_case_1_2_2, {
	&test_1_2_2_top, &test_1_2_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_3_1, tgrp_case_1_3_1, NULL, name_case_1_3_1, NULL, desc_case_1_3_1, sref_case_1_3_1, {
	&test_1_3_1_top, &test_1_3_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_3_2, tgrp_case_1_3_2, NULL, name_case_1_3_2, NULL, desc_case_1_3_2, sref_case_1_3_2, {
	&test_1_3_2_top, &test_1_3_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_4, tgrp_case_1_4, NULL, name_case_1_4, NULL, desc_case_1_4, sref_case_1_4, {
	&test_1_4_top, &test_1_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_5_1, tgrp_case_1_5_1, NULL, name_case_1_5_1, NULL, desc_case_1_5_1, sref_case_1_5_1, {
	&test_1_5_1_top, &test_1_5_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_5_2, tgrp_case_1_5_2, NULL, name_case_1_5_2, NULL, desc_case_1_5_2, sref_case_1_5_2, {
	&test_1_5_2_top, &test_1_5_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_6_1, tgrp_case_1_6_1, NULL, name_case_1_6_1, NULL, desc_case_1_6_1, sref_case_1_6_1, {
	&test_1_6_1_top, &test_1_6_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_6_2, tgrp_case_1_6_2, NULL, name_case_1_6_2, NULL, desc_case_1_6_2, sref_case_1_6_2, {
	&test_1_6_2_top, &test_1_6_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_7_1, tgrp_case_1_7_1, NULL, name_case_1_7_1, NULL, desc_case_1_7_1, sref_case_1_7_1, {
	&test_1_7_1_top, &test_1_7_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_7_2, tgrp_case_1_7_2, NULL, name_case_1_7_2, NULL, desc_case_1_7_2, sref_case_1_7_2, {
	&test_1_7_2_top, &test_1_7_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_7_3, tgrp_case_1_7_3, NULL, name_case_1_7_3, NULL, desc_case_1_7_3, sref_case_1_7_3, {
	&test_1_7_3_top, &test_1_7_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_7_4, tgrp_case_1_7_4, NULL, name_case_1_7_4, NULL, desc_case_1_7_4, sref_case_1_7_4, {
	&test_1_7_4_top, &test_1_7_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_7_5, tgrp_case_1_7_5, NULL, name_case_1_7_5, NULL, desc_case_1_7_5, sref_case_1_7_5, {
	&test_1_7_5_top, &test_1_7_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_8_1, tgrp_case_1_8_1, NULL, name_case_1_8_1, NULL, desc_case_1_8_1, sref_case_1_8_1, {
	&test_1_8_1_top, &test_1_8_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_8_2, tgrp_case_1_8_2, NULL, name_case_1_8_2, NULL, desc_case_1_8_2, sref_case_1_8_2, {
	&test_1_8_2_top, &test_1_8_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_8_3, tgrp_case_1_8_3, NULL, name_case_1_8_3, NULL, desc_case_1_8_3, sref_case_1_8_3, {
	&test_1_8_3_top, &test_1_8_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_8_4, tgrp_case_1_8_4, NULL, name_case_1_8_4, NULL, desc_case_1_8_4, sref_case_1_8_4, {
	&test_1_8_4_top, &test_1_8_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_9_1, tgrp_case_1_9_1, NULL, name_case_1_9_1, NULL, desc_case_1_9_1, sref_case_1_9_1, {
	&test_1_9_1_top, &test_1_9_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_9_2, tgrp_case_1_9_2, NULL, name_case_1_9_2, NULL, desc_case_1_9_2, sref_case_1_9_2, {
	&test_1_9_2_top, &test_1_9_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_10_1, tgrp_case_1_10_1, NULL, name_case_1_10_1, NULL, desc_case_1_10_1, sref_case_1_10_1, {
	&test_1_10_1_top, &test_1_10_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_10_2, tgrp_case_1_10_2, NULL, name_case_1_10_2, NULL, desc_case_1_10_2, sref_case_1_10_2, {
	&test_1_10_2_top, &test_1_10_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_11_1, tgrp_case_1_11_1, NULL, name_case_1_11_1, NULL, desc_case_1_11_1, sref_case_1_11_1, {
	&test_1_11_1_top, &test_1_11_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_11_2, tgrp_case_1_11_2, NULL, name_case_1_11_2, NULL, desc_case_1_11_2, sref_case_1_11_2, {
	&test_1_11_2_top, &test_1_11_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_12_1, tgrp_case_1_12_1, NULL, name_case_1_12_1, NULL, desc_case_1_12_1, sref_case_1_12_1, {
	&test_1_12_1_top, &test_1_12_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_12_2, tgrp_case_1_12_2, NULL, name_case_1_12_2, NULL, desc_case_1_12_2, sref_case_1_12_2, {
	&test_1_12_2_top, &test_1_12_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_13_1, tgrp_case_1_13_1, NULL, name_case_1_13_1, NULL, desc_case_1_13_1, sref_case_1_13_1, {
	&test_1_13_1_top, &test_1_13_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_13_2, tgrp_case_1_13_2, NULL, name_case_1_13_2, NULL, desc_case_1_13_2, sref_case_1_13_2, {
	&test_1_13_2_top, &test_1_13_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_14_1, tgrp_case_1_14_1, NULL, name_case_1_14_1, NULL, desc_case_1_14_1, sref_case_1_14_1, {
	&test_1_14_1_top, &test_1_14_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_14_2, tgrp_case_1_14_2, NULL, name_case_1_14_2, NULL, desc_case_1_14_2, sref_case_1_14_2, {
	&test_1_14_2_top, &test_1_14_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_1_15, tgrp_case_1_15, NULL, name_case_1_15, NULL, desc_case_1_15, sref_case_1_15, {
	&test_1_15_top, &test_1_15_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_2, tgrp_case_2_2, NULL, name_case_2_2, NULL, desc_case_2_2, sref_case_2_2, {
	&test_2_2_top, &test_2_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_3, tgrp_case_2_3, NULL, name_case_2_3, NULL, desc_case_2_3, sref_case_2_3, {
	&test_2_3_top, &test_2_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_4, tgrp_case_2_4, NULL, name_case_2_4, NULL, desc_case_2_4, sref_case_2_4, {
	&test_2_4_top, &test_2_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_5, tgrp_case_2_5, NULL, name_case_2_5, NULL, desc_case_2_5, sref_case_2_5, {
	&test_2_5_top, &test_2_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_6, tgrp_case_2_6, NULL, name_case_2_6, NULL, desc_case_2_6, sref_case_2_6, {
	&test_2_6_top, &test_2_6_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_7_1, tgrp_case_2_7_1, NULL, name_case_2_7_1, NULL, desc_case_2_7_1, sref_case_2_7_1, {
	&test_2_7_1_top, &test_2_7_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_7_2, tgrp_case_2_7_2, NULL, name_case_2_7_2, NULL, desc_case_2_7_2, sref_case_2_7_2, {
	&test_2_7_2_top, &test_2_7_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_7_3, tgrp_case_2_7_3, NULL, name_case_2_7_3, NULL, desc_case_2_7_3, sref_case_2_7_3, {
	&test_2_7_3_top, &test_2_7_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_7_4, tgrp_case_2_7_4, NULL, name_case_2_7_4, NULL, desc_case_2_7_4, sref_case_2_7_4, {
	&test_2_7_4_top, &test_2_7_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_8, tgrp_case_2_8, NULL, name_case_2_8, NULL, desc_case_2_8, sref_case_2_8, {
	&test_2_8_top, &test_2_8_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_9, tgrp_case_2_9, NULL, name_case_2_9, NULL, desc_case_2_9, sref_case_2_9, {
	&test_2_9_top, &test_2_9_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_10, tgrp_case_2_10, NULL, name_case_2_10, NULL, desc_case_2_10, sref_case_2_10, {
	&test_2_10_top, &test_2_10_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_11, tgrp_case_2_11, NULL, name_case_2_11, NULL, desc_case_2_11, sref_case_2_11, {
	&test_2_11_top, &test_2_11_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_12, tgrp_case_2_12, NULL, name_case_2_12, NULL, desc_case_2_12, sref_case_2_12, {
	&test_2_12_top, &test_2_12_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_13, tgrp_case_2_13, NULL, name_case_2_13, NULL, desc_case_2_13, sref_case_2_13, {
	&test_2_13_top, &test_2_13_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_2_14, tgrp_case_2_14, NULL, name_case_2_14, NULL, desc_case_2_14, sref_case_2_14, {
	&test_2_14_top, &test_2_14_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_1, tgrp_case_3_1, NULL, name_case_3_1, NULL, desc_case_3_1, sref_case_3_1, {
	&test_3_1_top, &test_3_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_2, tgrp_case_3_2, NULL, name_case_3_2, NULL, desc_case_3_2, sref_case_3_2, {
	&test_3_2_top, &test_3_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_3, tgrp_case_3_3, NULL, name_case_3_3, NULL, desc_case_3_3, sref_case_3_3, {
	&test_3_3_top, &test_3_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_4, tgrp_case_3_4, NULL, name_case_3_4, NULL, desc_case_3_4, sref_case_3_4, {
	&test_3_4_top, &test_3_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_5, tgrp_case_3_5, NULL, name_case_3_5, NULL, desc_case_3_5, sref_case_3_5, {
	&test_3_5_top, &test_3_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_6, tgrp_case_3_6, NULL, name_case_3_6, NULL, desc_case_3_6, sref_case_3_6, {
	&test_3_6_top, &test_3_6_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_7, tgrp_case_3_7, NULL, name_case_3_7, NULL, desc_case_3_7, sref_case_3_7, {
	&test_3_7_top, &test_3_7_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_8, tgrp_case_3_8, NULL, name_case_3_8, NULL, desc_case_3_8, sref_case_3_8, {
	&test_3_8_top, &test_3_8_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_9, tgrp_case_3_9, NULL, name_case_3_9, NULL, desc_case_3_9, sref_case_3_9, {
	&test_3_9_top, &test_3_9_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_3_10, tgrp_case_3_10, NULL, name_case_3_10, NULL, desc_case_3_10, sref_case_3_10, {
	&test_3_10_top, &test_3_10_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_1, tgrp_case_4_1, NULL, name_case_4_1, NULL, desc_case_4_1, sref_case_4_1, {
	&test_4_1_top, &test_4_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_2_1, tgrp_case_4_2_1, NULL, name_case_4_2_1, NULL, desc_case_4_2_1, sref_case_4_2_1, {
	&test_4_2_1_top, &test_4_2_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_2_2, tgrp_case_4_2_2, NULL, name_case_4_2_2, NULL, desc_case_4_2_2, sref_case_4_2_2, {
	&test_4_2_2_top, &test_4_2_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_3, tgrp_case_4_3, NULL, name_case_4_3, NULL, desc_case_4_3, sref_case_4_3, {
	&test_4_3_top, &test_4_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_4, tgrp_case_4_4, NULL, name_case_4_4, NULL, desc_case_4_4, sref_case_4_4, {
	&test_4_4_top, &test_4_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_5, tgrp_case_4_5, NULL, name_case_4_5, NULL, desc_case_4_5, sref_case_4_5, {
	&test_4_5_top, &test_4_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_6_1, tgrp_case_4_6_1, NULL, name_case_4_6_1, NULL, desc_case_4_6_1, sref_case_4_6_1, {
	&test_4_6_1_top, &test_4_6_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_6_2, tgrp_case_4_6_2, NULL, name_case_4_6_2, NULL, desc_case_4_6_2, sref_case_4_6_2, {
	&test_4_6_2_top, &test_4_6_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_6_3, tgrp_case_4_6_3, NULL, name_case_4_6_3, NULL, desc_case_4_6_3, sref_case_4_6_3, {
	&test_4_6_3_top, &test_4_6_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_7_1, tgrp_case_4_7_1, NULL, name_case_4_7_1, NULL, desc_case_4_7_1, sref_case_4_7_1, {
	&test_4_7_1_top, &test_4_7_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_7_2, tgrp_case_4_7_2, NULL, name_case_4_7_2, NULL, desc_case_4_7_2, sref_case_4_7_2, {
	&test_4_7_2_top, &test_4_7_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_7_3, tgrp_case_4_7_3, NULL, name_case_4_7_3, NULL, desc_case_4_7_3, sref_case_4_7_3, {
	&test_4_7_3_top, &test_4_7_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_8, tgrp_case_4_8, NULL, name_case_4_8, NULL, desc_case_4_8, sref_case_4_8, {
	&test_4_8_top, &test_4_8_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_9, tgrp_case_4_9, NULL, name_case_4_9, NULL, desc_case_4_9, sref_case_4_9, {
	&test_4_9_top, &test_4_9_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_4_10, tgrp_case_4_10, NULL, name_case_4_10, NULL, desc_case_4_10, sref_case_4_10, {
	&test_4_10_top, &test_4_10_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_1_1, tgrp_case_5_1_1, NULL, name_case_5_1_1, NULL, desc_case_5_1_1, sref_case_5_1_1, {
	&test_5_1_1_top, &test_5_1_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_1_2, tgrp_case_5_1_2, NULL, name_case_5_1_2, NULL, desc_case_5_1_2, sref_case_5_1_2, {
	&test_5_1_2_top, &test_5_1_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_2, tgrp_case_5_2, NULL, name_case_5_2, NULL, desc_case_5_2, sref_case_5_2, {
	&test_5_2_top, &test_5_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_3_1, tgrp_case_5_3_1, NULL, name_case_5_3_1, NULL, desc_case_5_3_1, sref_case_5_3_1, {
	&test_5_3_1_top, &test_5_3_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_3_2, tgrp_case_5_3_2, NULL, name_case_5_3_2, NULL, desc_case_5_3_2, sref_case_5_3_2, {
	&test_5_3_2_top, &test_5_3_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_3_3, tgrp_case_5_3_3, NULL, name_case_5_3_3, NULL, desc_case_5_3_3, sref_case_5_3_3, {
	&test_5_3_3_top, &test_5_3_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_3_4, tgrp_case_5_3_4, NULL, name_case_5_3_4, NULL, desc_case_5_3_4, sref_case_5_3_4, {
	&test_5_3_4_top, &test_5_3_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_5_3_5, tgrp_case_5_3_5, NULL, name_case_5_3_5, NULL, desc_case_5_3_5, sref_case_5_3_5, {
	&test_5_3_5_top, &test_5_3_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_6_1, tgrp_case_6_1, NULL, name_case_6_1, NULL, desc_case_6_1, sref_case_6_1, {
	&test_6_1_top, &test_6_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_6_2, tgrp_case_6_2, NULL, name_case_6_2, NULL, desc_case_6_2, sref_case_6_2, {
	&test_6_2_top, &test_6_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_6_3, tgrp_case_6_3, NULL, name_case_6_3, NULL, desc_case_6_3, sref_case_6_3, {
	&test_6_3_top, &test_6_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_6_4, tgrp_case_6_4, NULL, name_case_6_4, NULL, desc_case_6_4, sref_case_6_4, {
	&test_6_4_top, &test_6_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_6_5, tgrp_case_6_5, NULL, name_case_6_5, NULL, desc_case_6_5, sref_case_6_5, {
	&test_6_5_top, &test_6_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_6_6, tgrp_case_6_6, NULL, name_case_6_6, NULL, desc_case_6_6, sref_case_6_6, {
	&test_6_6_top, &test_6_6_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_1, tgrp_case_7_1, NULL, name_case_7_1, NULL, desc_case_7_1, sref_case_7_1, {
	&test_7_1_top, &test_7_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_2, tgrp_case_7_2, NULL, name_case_7_2, NULL, desc_case_7_2, sref_case_7_2, {
	&test_7_2_top, &test_7_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_3, tgrp_case_7_3, NULL, name_case_7_3, NULL, desc_case_7_3, sref_case_7_3, {
	&test_7_3_top, &test_7_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_4, tgrp_case_7_4, NULL, name_case_7_4, NULL, desc_case_7_4, sref_case_7_4, {
	&test_7_4_top, &test_7_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_5, tgrp_case_7_5, NULL, name_case_7_5, NULL, desc_case_7_5, sref_case_7_5, {
	&test_7_5_top, &test_7_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_6, tgrp_case_7_6, NULL, name_case_7_6, NULL, desc_case_7_6, sref_case_7_6, {
	&test_7_6_top, &test_7_6_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_7, tgrp_case_7_7, NULL, name_case_7_7, NULL, desc_case_7_7, sref_case_7_7, {
	&test_7_7_top, &test_7_7_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_7_8, tgrp_case_7_8, NULL, name_case_7_8, NULL, desc_case_7_8, sref_case_7_8, {
	&test_7_8_top, &test_7_8_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_1, tgrp_case_8_1, NULL, name_case_8_1, NULL, desc_case_8_1, sref_case_8_1, {
	&test_8_1_top, &test_8_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_2, tgrp_case_8_2, NULL, name_case_8_2, NULL, desc_case_8_2, sref_case_8_2, {
	&test_8_2_top, &test_8_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_3, tgrp_case_8_3, NULL, name_case_8_3, NULL, desc_case_8_3, sref_case_8_3, {
	&test_8_3_top, &test_8_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_4, tgrp_case_8_4, NULL, name_case_8_4, NULL, desc_case_8_4, sref_case_8_4, {
	&test_8_4_top, &test_8_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_5, tgrp_case_8_5, NULL, name_case_8_5, NULL, desc_case_8_5, sref_case_8_5, {
	&test_8_5_top, &test_8_5_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_6, tgrp_case_8_6, NULL, name_case_8_6, NULL, desc_case_8_6, sref_case_8_6, {
	&test_8_6_top, &test_8_6_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_7, tgrp_case_8_7, NULL, name_case_8_7, NULL, desc_case_8_7, sref_case_8_7, {
	&test_8_7_top, &test_8_7_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_8, tgrp_case_8_8, NULL, name_case_8_8, NULL, desc_case_8_8, sref_case_8_8, {
	&test_8_8_top, &test_8_8_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_9, tgrp_case_8_9, NULL, name_case_8_9, NULL, desc_case_8_9, sref_case_8_9, {
	&test_8_9_top, &test_8_9_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_10, tgrp_case_8_10, NULL, name_case_8_10, NULL, desc_case_8_10, sref_case_8_10, {
	&test_8_10_top, &test_8_10_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_11, tgrp_case_8_11, NULL, name_case_8_11, NULL, desc_case_8_11, sref_case_8_11, {
	&test_8_11_top, &test_8_11_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_12, tgrp_case_8_12, NULL, name_case_8_12, NULL, desc_case_8_12, sref_case_8_12, {
	&test_8_12_top, &test_8_12_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_13, tgrp_case_8_13, NULL, name_case_8_13, NULL, desc_case_8_13, sref_case_8_13, {
	&test_8_13_top, &test_8_13_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_14, tgrp_case_8_14, NULL, name_case_8_14, NULL, desc_case_8_14, sref_case_8_14, {
	&test_8_14_top, &test_8_14_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_8_15, tgrp_case_8_15, NULL, name_case_8_15, NULL, desc_case_8_15, sref_case_8_15, {
	&test_8_15_top, &test_8_15_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_9_1, tgrp_case_9_1, NULL, name_case_9_1, NULL, desc_case_9_1, sref_case_9_1, {
	&test_9_1_top, &test_9_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_9_2, tgrp_case_9_2, NULL, name_case_9_2, NULL, desc_case_9_2, sref_case_9_2, {
	&test_9_2_top, &test_9_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_9_3, tgrp_case_9_3, NULL, name_case_9_3, NULL, desc_case_9_3, sref_case_9_3, {
	&test_9_3_top, &test_9_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_10_1, tgrp_case_10_1, NULL, name_case_10_1, NULL, desc_case_10_1, sref_case_10_1, {
	&test_10_1_top, &test_10_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_10_2, tgrp_case_10_2, NULL, name_case_10_2, NULL, desc_case_10_2, sref_case_10_2, {
	&test_10_2_top, &test_10_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_10_3, tgrp_case_10_3, NULL, name_case_10_3, NULL, desc_case_10_3, sref_case_10_3, {
	&test_10_3_top, &test_10_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_10_4, tgrp_case_10_4, NULL, name_case_10_4, NULL, desc_case_10_4, sref_case_10_4, {
	&test_10_4_top, &test_10_4_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_11_1, tgrp_case_11_1, NULL, name_case_11_1, NULL, desc_case_11_1, sref_case_11_1, {
	&test_11_1_top, &test_11_1_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_11_2, tgrp_case_11_2, NULL, name_case_11_2, NULL, desc_case_11_2, sref_case_11_2, {
	&test_11_2_top, &test_11_2_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
		numb_case_11_3, tgrp_case_11_3, NULL, name_case_11_3, NULL, desc_case_11_3, sref_case_11_3, {
	&test_11_3_top, &test_11_3_bot, NULL}, &begin_tests, &end_tests, 0, 0, 0, __RESULT_SUCCESS}, {
	NULL,}
};

static int summary = 0;

void
print_header(void)
{
	if (verbose <= 0)
		return;
	dummy = lockf(fileno(stdout), F_LOCK, 0);
	fprintf(stdout, "\n%s - %s - %s - Conformance Test Suite\n", lstdname, lpkgname, shortname);
	fflush(stdout);
	dummy = lockf(fileno(stdout), F_ULOCK, 0);
}

int
do_tests(int num_tests)
{
	int i;
	int result = __RESULT_INCONCLUSIVE;
	int notapplicable = 0;
	int inconclusive = 0;
	int successes = 0;
	int failures = 0;
	int skipped = 0;
	int notselected = 0;
	int aborted = 0;
	int repeat = 0;
	int oldverbose = verbose;

	print_header();
	show = 0;
	if (verbose > 0) {
		dummy = lockf(fileno(stdout), F_LOCK, 0);
		fprintf(stdout, "\nUsing device %s\n\n", devname);
		fflush(stdout);
		dummy = lockf(fileno(stdout), F_ULOCK, 0);
	}
	if (num_tests == 1 || begin_tests(0) == __RESULT_SUCCESS) {
		if (num_tests != 1)
			end_tests(0);
		show = 1;
		for (i = 0; i < (sizeof(tests) / sizeof(struct test_case)) && tests[i].numb; i++) {
		      rerun:
			if (!tests[i].run) {
				tests[i].result = __RESULT_INCONCLUSIVE;
				notselected++;
				continue;
			}
			if (aborted) {
				tests[i].result = __RESULT_INCONCLUSIVE;
				inconclusive++;
				continue;
			}
			if (verbose > 0) {
				dummy = lockf(fileno(stdout), F_LOCK, 0);
				if (verbose > 1 && tests[i].tgrp)
					fprintf(stdout, "\nTest Group: %s", tests[i].tgrp);
				if (verbose > 1 && tests[i].sgrp)
					fprintf(stdout, "\nTest Subgroup: %s", tests[i].sgrp);
				if (tests[i].xtra)
					fprintf(stdout, "\nTest Case %s-%s/%s: %s (%s)\n", sstdname, shortname, tests[i].numb, tests[i].name, tests[i].xtra);
				else
					fprintf(stdout, "\nTest Case %s-%s/%s: %s\n", sstdname, shortname, tests[i].numb, tests[i].name);
				if (verbose > 1 && tests[i].sref)
					fprintf(stdout, "Test Reference: %s\n", tests[i].sref);
				if (verbose > 1 && tests[i].desc)
					fprintf(stdout, "%s\n", tests[i].desc);
				fprintf(stdout, "\n");
				fflush(stdout);
				dummy = lockf(fileno(stdout), F_ULOCK, 0);
			}
			if ((result = tests[i].result) == 0) {
				ulong duration = test_duration;

				if (duration > tests[i].duration) {
					if (tests[i].duration && duration > tests[i].duration)
						duration = tests[i].duration;
					if ((result = (*tests[i].start) (i)) != __RESULT_SUCCESS)
						goto inconclusive;
					result = test_run(tests[i].stream, duration);
					(*tests[i].stop) (i);
				} else
					result = __RESULT_SKIPPED;
				if (result == tests[i].expect) {
					switch (result) {
					case __RESULT_SUCCESS:
					case __RESULT_NOTAPPL:
					case __RESULT_SKIPPED:
						/* autotest can handle these */
						break;
					default:
					case __RESULT_INCONCLUSIVE:
					case __RESULT_FAILURE:
						/* these are expected failures */
						result = __RESULT_SUCCESS;
						break;
					}
				}
			} else {
				if (result == tests[i].expect) {
					switch (result) {
					case __RESULT_SUCCESS:
					case __RESULT_NOTAPPL:
					case __RESULT_SKIPPED:
						/* autotest can handle these */
						break;
					default:
					case __RESULT_INCONCLUSIVE:
					case __RESULT_FAILURE:
						/* these are expected failures */
						result = __RESULT_SUCCESS;
						break;
					}
				}
				switch (result) {
				case __RESULT_SUCCESS:
					print_passed(3);
					break;
				case __RESULT_FAILURE:
					print_failed(3);
					break;
				case __RESULT_NOTAPPL:
					print_notapplicable(3);
					break;
				case __RESULT_SKIPPED:
					print_skipped(3);
					break;
				default:
				case __RESULT_INCONCLUSIVE:
					print_inconclusive(3);
					break;
				}
			}
			switch (result) {
			case __RESULT_SUCCESS:
				successes++;
				if (verbose > 0) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "\n");
					fprintf(stdout, "*********\n");
					fprintf(stdout, "********* Test Case SUCCESSFUL\n");
					fprintf(stdout, "*********\n\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				break;
			case __RESULT_FAILURE:
				if (!repeat_verbose || repeat)
					failures++;
				if (verbose > 0) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "\n");
					fprintf(stdout, "XXXXXXXXX\n");
					fprintf(stdout, "XXXXXXXXX Test Case FAILED\n");
					fprintf(stdout, "XXXXXXXXX\n\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				break;
			case __RESULT_NOTAPPL:
				notapplicable++;
				if (verbose > 0) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "\n");
					fprintf(stdout, "XXXXXXXXX\n");
					fprintf(stdout, "XXXXXXXXX Test Case NOT APPLICABLE\n");
					fprintf(stdout, "XXXXXXXXX\n\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				break;
			case __RESULT_SKIPPED:
				skipped++;
				if (verbose > 0) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "\n");
					fprintf(stdout, "XXXXXXXXX\n");
					fprintf(stdout, "XXXXXXXXX Test Case SKIPPED\n");
					fprintf(stdout, "XXXXXXXXX\n\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				break;
			default:
			case __RESULT_INCONCLUSIVE:
			      inconclusive:
				if (!repeat_verbose || repeat)
					inconclusive++;
				if (verbose > 0) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "\n");
					fprintf(stdout, "?????????\n");
					fprintf(stdout, "????????? Test Case INCONCLUSIVE\n");
					fprintf(stdout, "?????????\n\n");
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
				}
				break;
			}
			if (repeat_on_failure && (result == __RESULT_FAILURE || result == __RESULT_INCONCLUSIVE))
				goto rerun;
			if (repeat_on_success && (result == __RESULT_SUCCESS))
				goto rerun;
			if (repeat) {
				repeat = 0;
				verbose = oldverbose;
			} else if (repeat_verbose && (result == __RESULT_FAILURE || result == __RESULT_INCONCLUSIVE)) {
				repeat = 1;
				verbose = 5;
				goto rerun;
			}
			tests[i].result = result;
			if (exit_on_failure && (result == __RESULT_FAILURE || result == __RESULT_INCONCLUSIVE))
				aborted = 1;
		}
		if (summary && verbose) {
			dummy = lockf(fileno(stdout), F_LOCK, 0);
			fprintf(stdout, "\n");
			fflush(stdout);
			dummy = lockf(fileno(stdout), F_ULOCK, 0);
			for (i = 0; i < (sizeof(tests) / sizeof(struct test_case)) && tests[i].numb; i++) {
				if (tests[i].run) {
					dummy = lockf(fileno(stdout), F_LOCK, 0);
					fprintf(stdout, "Test Case %s-%s/%-10s ", sstdname, shortname, tests[i].numb);
					fflush(stdout);
					dummy = lockf(fileno(stdout), F_ULOCK, 0);
					switch (tests[i].result) {
					case __RESULT_SUCCESS:
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "SUCCESS\n");
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
						break;
					case __RESULT_FAILURE:
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "FAILURE\n");
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
						break;
					case __RESULT_NOTAPPL:
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "NOT APPLICABLE\n");
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
						break;
					case __RESULT_SKIPPED:
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "SKIPPED\n");
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
						break;
					default:
					case __RESULT_INCONCLUSIVE:
						dummy = lockf(fileno(stdout), F_LOCK, 0);
						fprintf(stdout, "INCONCLUSIVE\n");
						fflush(stdout);
						dummy = lockf(fileno(stdout), F_ULOCK, 0);
						break;
					}
				}
			}
		}
		if (verbose > 0 && num_tests > 1) {
			dummy = lockf(fileno(stdout), F_LOCK, 0);
			fprintf(stdout, "\n");
			fprintf(stdout, "========= %3d successes     \n", successes);
			fprintf(stdout, "========= %3d failures      \n", failures);
			fprintf(stdout, "========= %3d inconclusive  \n", inconclusive);
			fprintf(stdout, "========= %3d not applicable\n", notapplicable);
			fprintf(stdout, "========= %3d skipped       \n", skipped);
			fprintf(stdout, "========= %3d not selected  \n", notselected);
			fprintf(stdout, "============================\n");
			fprintf(stdout, "========= %3d total         \n", successes + failures + inconclusive + notapplicable + skipped + notselected);
			if (!(aborted + failures))
				fprintf(stdout, "\nDone.\n\n");
			fflush(stdout);
			dummy = lockf(fileno(stdout), F_ULOCK, 0);
		}
		if (aborted) {
			dummy = lockf(fileno(stderr), F_LOCK, 0);
			if (verbose > 0)
				fprintf(stderr, "\n");
			fprintf(stderr, "Test Suite aborted due to failure.\n");
			if (verbose > 0)
				fprintf(stderr, "\n");
			fflush(stderr);
			dummy = lockf(fileno(stderr), F_ULOCK, 0);
		} else if (failures) {
			dummy = lockf(fileno(stderr), F_LOCK, 0);
			if (verbose > 0)
				fprintf(stderr, "\n");
			fprintf(stderr, "Test Suite failed.\n");
			if (verbose > 0)
				fprintf(stderr, "\n");
			fflush(stderr);
			dummy = lockf(fileno(stderr), F_ULOCK, 0);
		}
		if (num_tests == 1) {
			if (successes)
				return (0);
			if (failures)
				return (1);
			if (inconclusive)
				return (1);
			if (notapplicable)
				return (0);
			if (skipped)
				return (77);
		}
		return (aborted);
	} else {
		end_tests(0);
		show = 1;
		dummy = lockf(fileno(stderr), F_LOCK, 0);
		fprintf(stderr, "Test Suite setup failed!\n");
		fflush(stderr);
		dummy = lockf(fileno(stderr), F_ULOCK, 0);
		return (2);
	}
}

void
copying(int argc, char *argv[])
{
	if (!verbose)
		return;
	print_header();
	fprintf(stdout, "\
\n\
Copyright (c) 2008-2011  Monavacon Limited <http://www.monavacon.com/>\n\
Copyright (c) 2001-2008  OpenSS7 Corporation <http://www.openss7.com/>\n\
Copyright (c) 1997-2001  Brian F. G. Bidulock <bidulock@openss7.org>\n\
\n\
All Rights Reserved.\n\
\n\
Unauthorized distribution or duplication is prohibited.\n\
\n\
This software and related documentation is protected by copyright and distribut-\n\
ed under licenses restricting its use,  copying, distribution and decompilation.\n\
No part of this software or related documentation may  be reproduced in any form\n\
by any means without the prior  written  authorization of the  copyright holder,\n\
and licensors, if any.\n\
\n\
The recipient of this document,  by its retention and use, warrants that the re-\n\
cipient  will protect this  information and  keep it confidential,  and will not\n\
disclose the information contained  in this document without the written permis-\n\
sion of its owner.\n\
\n\
The author reserves the right to revise  this software and documentation for any\n\
reason,  including but not limited to, conformity with standards  promulgated by\n\
various agencies, utilization of advances in the state of the technical arts, or\n\
the reflection of changes  in the design of any techniques, or procedures embod-\n\
ied, described, or  referred to herein.   The author  is under no  obligation to\n\
provide any feature listed herein.\n\
\n\
As an exception to the above,  this software may be  distributed  under the  GNU\n\
Affero  General  Public  License  (AGPL)  Version  3, so long as the software is\n\
distributed with,  and only used for the testing of,  OpenSS7 modules,  drivers,\n\
and libraries.\n\
\n\
U.S. GOVERNMENT RESTRICTED RIGHTS.  If you are licensing this Software on behalf\n\
of the  U.S. Government  (\"Government\"),  the following provisions apply to you.\n\
If the Software is  supplied by the Department of Defense (\"DoD\"), it is classi-\n\
fied as  \"Commercial Computer Software\"  under paragraph 252.227-7014 of the DoD\n\
Supplement  to the  Federal Acquisition Regulations  (\"DFARS\") (or any successor\n\
regulations) and the  Government  is acquiring  only the license rights  granted\n\
herein (the license  rights customarily  provided to non-Government  users).  If\n\
the Software is supplied to any unit or agency of the Government other than DoD,\n\
it is classified as  \"Restricted Computer Software\" and the  Government's rights\n\
in the  Software are defined in  paragraph 52.227-19 of the Federal  Acquisition\n\
Regulations  (\"FAR\") (or any successor regulations) or, in the cases of NASA, in\n\
paragraph  18.52.227-86 of the  NASA Supplement  to the  FAR (or  any  successor\n\
regulations).\n\
\n\
");
}

void
version(int argc, char *argv[])
{
	if (!verbose)
		return;
	fprintf(stdout, "\
\n\
%1$s:\n\
    %2$s\n\
    Copyright (c) 2008-2011  Monavacon Limited.  All Rights Reserved.\n\
    Copyright (c) 1997-2008  OpenSS7 Corporation.  All Rights Reserved.\n\
\n\
    Distributed by OpenSS7 Corporation under AGPL Version 3,\n\
    incorporated here by reference.\n\
\n\
    See `%1$s --copying' for copying permission.\n\
\n\
", argv[0], ident);
}

void
usage(int argc, char *argv[])
{
	if (!verbose)
		return;
	fprintf(stderr, "\
Usage:\n\
    %1$s [options]\n\
    %1$s {-h, --help}\n\
    %1$s {-V, --version}\n\
    %1$s {-C, --copying}\n\
", argv[0]);
}

void
help(int argc, char *argv[])
{
	if (!verbose)
		return;
	fprintf(stdout, "\
\n\
Usage:\n\
    %1$s [options]\n\
    %1$s {-h, --help}\n\
    %1$s {-V, --version}\n\
    %1$s {-C, --copying}\n\
Arguments:\n\
    (none)\n\
Options:\n\
    -c, --client\n\
        execute client side (PTU) of test case only.\n\
    -S, --server\n\
        execute server side (IUT) of test case only.\n\
    -a, --again\n\
        repeat failed tests verbose.\n\
    -w, --wait\n\
        have server wait indefinitely.\n\
    -r, --repeat\n\
        repeat test cases on success or failure.\n\
    -R, --repeat-fail\n\
        repeat test cases on failure.\n\
    -p, --client-port [PORT]\n\
        port number from which to connect [default: %4$d+index*3]\n\
    -P, --server-port [PORT]\n\
        port number to which to connect or upon which to listen\n\
        [default: %4$d+index*3+2]\n\
    -i, --client-host [HOSTNAME[,HOSTNAME]*]\n\
        client host names(s) or IP numbers\n\
        [default: 127.0.0.1,127.0.0.2,127.0.0.3]\n\
    -I, --server-host [HOSTNAME[,HOSTNAME]*]\n\
        server host names(s) or IP numbers\n\
        [default: 127.0.0.1,127.0.0.2,127.0.0.3]\n\
    -d, --device DEVICE\n\
        device name to open [default: %2$s].\n\
    -M, --module MODULE\n\
        module name to push [default: %3$s].\n\
    -e, --exit\n\
        exit on the first failed or inconclusive test case.\n\
    -l, --list [RANGE]\n\
        list test case names within a range [default: all] and exit.\n\
    -f, --fast [SCALE]\n\
        increase speed of tests by scaling timers [default: 50]\n\
    -s, --summary\n\
        print a test case summary at end of testing [default: off]\n\
    -o, --onetest [TESTCASE]\n\
        run a single test case.\n\
    -t, --tests [RANGE]\n\
        run a range of test cases.\n\
    -m, --messages\n\
        display messages. [default: off]\n\
    -q, --quiet\n\
        suppress normal output (equivalent to --verbose=0)\n\
    -v, --verbose [LEVEL]\n\
        increase verbosity or set to LEVEL [default: 1]\n\
        this option may be repeated.\n\
    -h, --help, -?, --?\n\
        print this usage message and exit\n\
    -V, --version\n\
        print version and exit\n\
    -C, --copying\n\
        print copying permission and exit\n\
\n\
", argv[0], devname, modname, TEST_PORT_NUMBER);
}

#define HOST_BUF_LEN 128

int
main(int argc, char *argv[])
{
	size_t l, n;
	int range = 0;
	struct test_case *t;
	int tests_to_run = 0;
	char *hostc = "127.0.0.1,127.0.0.2,127.0.0.3";
	char *hosts = "127.0.0.1,127.0.0.2,127.0.0.3";
	char hostbufc[HOST_BUF_LEN];
	char hostbufs[HOST_BUF_LEN];

#if 0
	struct hostent *haddr;
#endif

	for (t = tests; t->numb; t++) {
		if (!t->result) {
			t->run = 1;
			tests_to_run++;
		}
	}
	for (;;) {
		int c, val;

#if defined _GNU_SOURCE
		int option_index = 0;
		/* *INDENT-OFF* */
		static struct option long_options[] = {
			{"client",	no_argument,		NULL, 'c'},
			{"server",	no_argument,		NULL, 'S'},
			{"again",	no_argument,		NULL, 'a'},
			{"wait",	no_argument,		NULL, 'w'},
			{"client-port",	required_argument,	NULL, 'p'},
			{"server-port",	required_argument,	NULL, 'P'},
			{"client-host",	required_argument,	NULL, 'i'},
			{"server-host",	required_argument,	NULL, 'I'},
			{"repeat",	no_argument,		NULL, 'r'},
			{"repeat-fail",	no_argument,		NULL, 'R'},
			{"device",	required_argument,	NULL, 'd'},
			{"module",	required_argument,	NULL, 'M'},
			{"exit",	no_argument,		NULL, 'e'},
			{"list",	optional_argument,	NULL, 'l'},
			{"fast",	optional_argument,	NULL, 'f'},
			{"summary",	no_argument,		NULL, 's'},
			{"onetest",	required_argument,	NULL, 'o'},
			{"tests",	required_argument,	NULL, 't'},
			{"messages",	no_argument,		NULL, 'm'},
			{"quiet",	no_argument,		NULL, 'q'},
			{"verbose",	optional_argument,	NULL, 'v'},
			{"help",	no_argument,		NULL, 'h'},
			{"version",	no_argument,		NULL, 'V'},
			{"copying",	no_argument,		NULL, 'C'},
			{"?",		no_argument,		NULL, 'h'},
			{NULL,		0,			NULL,  0 }
		};
		/* *INDENT-ON* */

		c = getopt_long(argc, argv, "cSawp:P:i:I:rRd:M:el::f::so:t:mqvhVC?", long_options, &option_index);
#else				/* defined _GNU_SOURCE */
		c = getopt(argc, argv, "cSawp:P:i:I:rRd:M:el::f::so:t:mqvhVC?");
#endif				/* defined _GNU_SOURCE */
		if (c == -1)
			break;
		switch (c) {
		case 'c':	/* --client */
			client_exec = 1;
			break;
		case 'S':	/* --server */
			server_exec = 1;
			break;
		case 'a':	/* --again */
			repeat_verbose = 1;
			break;
		case 'w':	/* --wait */
			test_duration = INFINITE_WAIT;
			break;
		case 'p':	/* --client-port */
			client_port_specified = 1;
			ports[3] = atoi(optarg);
			ports[0] = ports[3];
			break;
		case 'P':	/* --server-port */
			server_port_specified = 1;
			ports[3] = atoi(optarg);
			ports[1] = ports[3];
			ports[2] = ports[3] + 1;
			break;
		case 'i':	/* --client-host *//* client host */
			client_host_specified = 1;
			strncpy(hostbufc, optarg, HOST_BUF_LEN);
			hostc = hostbufc;
			break;
		case 'I':	/* --server-host *//* server host */
			server_host_specified = 1;
			strncpy(hostbufs, optarg, HOST_BUF_LEN);
			hosts = hostbufs;
			break;
		case 'r':	/* --repeat */
			repeat_on_success = 1;
			repeat_on_failure = 1;
			break;
		case 'R':	/* --repeat-fail */
			repeat_on_failure = 1;
			break;
		case 'd':
			if (optarg) {
				snprintf(devname, sizeof(devname), "%s", optarg);
				break;
			}
			goto bad_option;
		case 'M':
			if (optarg) {
				snprintf(modname, sizeof(modname), "%s", optarg);
				break;
			}
			goto bad_option;
		case 'e':
			exit_on_failure = 1;
			break;
		case 'l':
			if (optarg) {
				l = strnlen(optarg, 16);
				fprintf(stdout, "\n");
				for (n = 0, t = tests; t->numb; t++)
					if (!strncmp(t->numb, optarg, l)) {
						if (verbose > 2 && t->tgrp)
							fprintf(stdout, "Test Group: %s\n", t->tgrp);
						if (verbose > 2 && t->sgrp)
							fprintf(stdout, "Test Subgroup: %s\n", t->sgrp);
						if (t->xtra)
							fprintf(stdout, "Test Case %s-%s/%s: %s (%s)\n", sstdname, shortname, t->numb, t->name, t->xtra);
						else
							fprintf(stdout, "Test Case %s-%s/%s: %s\n", sstdname, shortname, t->numb, t->name);
						if (verbose > 2 && t->sref)
							fprintf(stdout, "Test Reference: %s\n", t->sref);
						if (verbose > 1 && t->desc)
							fprintf(stdout, "%s\n\n", t->desc);
						fflush(stdout);
						n++;
					}
				if (!n) {
					fprintf(stderr, "WARNING: specification `%s' matched no test\n", optarg);
					fflush(stderr);
					goto bad_option;
				}
				if (verbose <= 1)
					fprintf(stdout, "\n");
				fflush(stdout);
				exit(0);
			} else {
				fprintf(stdout, "\n");
				for (t = tests; t->numb; t++) {
					if (verbose > 2 && t->tgrp)
						fprintf(stdout, "Test Group: %s\n", t->tgrp);
					if (verbose > 2 && t->sgrp)
						fprintf(stdout, "Test Subgroup: %s\n", t->sgrp);
					if (t->xtra)
						fprintf(stdout, "Test Case %s-%s/%s: %s (%s)\n", sstdname, shortname, t->numb, t->name, t->xtra);
					else
						fprintf(stdout, "Test Case %s-%s/%s: %s\n", sstdname, shortname, t->numb, t->name);
					if (verbose > 2 && t->sref)
						fprintf(stdout, "Test Reference: %s\n", t->sref);
					if (verbose > 1 && t->desc)
						fprintf(stdout, "%s\n\n", t->desc);
					fflush(stdout);
				}
				if (verbose <= 1)
					fprintf(stdout, "\n");
				fflush(stdout);
				exit(0);
			}
			break;
		case 'f':
			if (optarg)
				timer_scale = atoi(optarg);
			else
				timer_scale = 50;
			fprintf(stderr, "WARNING: timers are scaled by a factor of %ld\n", (long) timer_scale);
			break;
		case 's':
			summary = 1;
			break;
		case 'o':
			if (optarg) {
				if (!range) {
					for (t = tests; t->numb; t++)
						t->run = 0;
					tests_to_run = 0;
				}
				range = 1;
				for (n = 0, t = tests; t->numb; t++)
					if (!strncmp(t->numb, optarg, 16)) {
						// if (!t->result) {
						t->run = 1;
						n++;
						tests_to_run++;
						// }
					}
				if (!n) {
					fprintf(stderr, "WARNING: specification `%s' matched no test\n", optarg);
					fflush(stderr);
					goto bad_option;
				}
				break;
			}
			goto bad_option;
		case 'q':
			verbose = 0;
			break;
		case 'v':
			if (optarg == NULL) {
				verbose++;
				break;
			}
			if ((val = strtol(optarg, NULL, 0)) < 0)
				goto bad_option;
			verbose = val;
			break;
		case 't':
			l = strnlen(optarg, 16);
			if (!range) {
				for (t = tests; t->numb; t++)
					t->run = 0;
				tests_to_run = 0;
			}
			range = 1;
			for (n = 0, t = tests; t->numb; t++)
				if (!strncmp(t->numb, optarg, l)) {
					// if (!t->result) {
					t->run = 1;
					n++;
					tests_to_run++;
					// }
				}
			if (!n) {
				fprintf(stderr, "WARNING: specification `%s' matched no test\n", optarg);
				fflush(stderr);
				goto bad_option;
			}
			break;
		case 'm':
			show_msg = 1;
			break;
		case 'H':	/* -H */
		case 'h':	/* -h, --help */
			help(argc, argv);
			exit(0);
		case 'V':
			version(argc, argv);
			exit(0);
		case 'C':
			copying(argc, argv);
			exit(0);
		case '?':
		default:
		      bad_option:
			optind--;
		      bad_nonopt:
			if (optind < argc && verbose) {
				fprintf(stderr, "%s: illegal syntax -- ", argv[0]);
				while (optind < argc)
					fprintf(stderr, "%s ", argv[optind++]);
				fprintf(stderr, "\n");
				fflush(stderr);
			}
			goto bad_usage;
		      bad_usage:
			usage(argc, argv);
			exit(2);
		}
	}
	/* 
	 * dont' ignore non-option arguments
	 */
	if (optind < argc)
		goto bad_nonopt;
	switch (tests_to_run) {
	case 0:
		if (verbose > 0) {
			fprintf(stderr, "%s: error: no tests to run\n", argv[0]);
			fflush(stderr);
		}
		exit(2);
	case 1:
		break;
	default:
		copying(argc, argv);
	}
	if (client_exec == 0 && server_exec == 0) {
		client_exec = 1;
		server_exec = 1;
	}
	if (client_host_specified) {
#if 0
		int count = 0;
		char *token = hostc, *next_token, *delim = NULL;

		fprintf(stdout, "Specified client address => %s\n", token);
		do {
			if ((delim = index(token, ','))) {
				delim[0] = '\0';
				next_token = delim + 1;
			} else
				next_token = NULL;
			haddr = gethostbyname(token);
			addrs[0][count].sin_family = AF_INET;
			addrs[3][count].sin_family = AF_INET;
			if (client_port_specified) {
				addrs[0][count].sin_port = htons(ports[0]);
				addrs[3][count].sin_port = htons(ports[3]);
			} else {
				addrs[0][count].sin_port = 0;
				addrs[3][count].sin_port = 0;
			}
			addrs[0][count].sin_addr.s_addr = *(uint32_t *) (haddr->h_addr);
			addrs[3][count].sin_addr.s_addr = *(uint32_t *) (haddr->h_addr);
			count++;
		} while ((token = next_token) && count < 4);
		anums[0] = count;
		anums[3] = count;
		fprintf(stdout, "%d client addresses assigned\n", count);
#endif
	}
	if (server_host_specified) {
#if 0
		int count = 0;
		char *token = hosts, *next_token, *delim = NULL;

		fprintf(stdout, "Specified server address => %s\n", token);
		do {
			if ((delim = index(token, ','))) {
				delim[0] = '\0';
				next_token = delim + 1;
			} else
				next_token = NULL;
			haddr = gethostbyname(token);
			addrs[1][count].sin_family = AF_INET;
			addrs[2][count].sin_family = AF_INET;
			addrs[3][count].sin_family = AF_INET;
			if (server_port_specified) {
				addrs[1][count].sin_port = htons(ports[1]);
				addrs[2][count].sin_port = htons(ports[2]);
				addrs[3][count].sin_port = htons(ports[3]);
			} else {
				addrs[1][count].sin_port = 0;
				addrs[2][count].sin_port = 0;
				addrs[3][count].sin_port = 0;
			}
			addrs[1][count].sin_addr.s_addr = *(uint32_t *) (haddr->h_addr);
			addrs[2][count].sin_addr.s_addr = *(uint32_t *) (haddr->h_addr);
			addrs[3][count].sin_addr.s_addr = *(uint32_t *) (haddr->h_addr);
			count++;
		} while ((token = next_token) && count < 4);
		anums[1] = count;
		anums[2] = count;
		anums[3] = count;
		fprintf(stdout, "%d server addresses assigned\n", count);
#else
		(void) hostc;
		(void) hosts;
#endif
	}
	exit(do_tests(tests_to_run));
}
