#define _BSD_SOURCE     /* for GNU libc, deprecated */
#define _DEFAULT_SOURCE /* for GNU libc */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <limits.h>

#define ELPOLL 0
#define ELWAIT 1

static const char usage[] =
"usage: flood [-d delay] [-j maxjobs] [-n count] command [argument ...]\n";

static char **cmdargs;                     /* command to run */
static int delay = 100, maxjobs, maxtotal; /* command line options */
static int njobs, nfailed, ngood;
static volatile sig_atomic_t bsigint;

static void onsigchld(int sig) { } /* just needed to interrupt nanosleep */
static void onsigint(int sig)  { bsigint = 1; }

#ifdef SIGINFO
static volatile sig_atomic_t bsiginfo;
static void onsiginfo(int sig) { bsiginfo = 1; }
#endif

static int
xstrtoi(char *val, char *name)
{
	long lval;
	char *end;

	lval = strtol(val, &end, 10);
	if (lval < 0 || lval > INT_MAX || *end) {
		fprintf(stderr, "invalid %s\n", name);
		exit(1);
	}

	return (int)lval;
}

static void
parseopts(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "d:j:n:")) != -1) {
		switch (c) {
		case 'd':
			delay = xstrtoi(optarg, "delay (-d)");
			break;
		case 'j':
			maxjobs = xstrtoi(optarg, "maxjobs (-j)");
			break;
		case 'n':
			maxtotal = xstrtoi(optarg, "count (-n)");
			break;
		default:
			fputs(usage, stderr);
			exit(1);
		}
	}

	cmdargs = argv + optind;
	if (!*cmdargs) {
		fputs(usage, stderr);
		exit(1);
	}
}

static void
startone(void)
{
	int outfd;

	njobs++;
	write(STDOUT_FILENO, ".", 1);

	switch (fork()) {
	case -1:
		perror(NULL);
		exit(1);
	case 0:
		if ((outfd = open("/dev/null", O_WRONLY)) == -1)
			goto err;
		dup2(outfd, STDOUT_FILENO);
		dup2(outfd, STDERR_FILENO);
		close(outfd);
		execvp(*cmdargs, cmdargs);
	err:
		putchar('@');
		exit(0);
	}
}

static void
pstatus(void)
{
	int goodpct, badpct;

	if (maxjobs)
		printf("\nrunning:   %6d/%d\n", njobs, maxjobs);
	else
		printf("\nrunning:   %6d\n", njobs);

	if (ngood + nfailed) {
		goodpct = (ngood   * 100) / (ngood + nfailed);
		badpct  = (nfailed * 100) / (ngood + nfailed);

		printf("completed: %6d (%3d%%)\n", ngood, goodpct);
		printf("failed:    %6d (%3d%%)\n", nfailed, badpct);
	} else {
		puts("completed:      0");
		puts("failed:         0");
	}
}

static int
evtloop(int flags)
{
	int status, waitflags;

	/* block for one child if ELWAIT is set */
	waitflags = (flags & ELWAIT) ? 0 : WNOHANG;

	while (1) {
		if (bsigint) {
			pstatus();

			/* exit with proper signal status */
			signal(SIGINT, SIG_DFL);
			raise(SIGINT);
			exit(1); /* fallback */
		}
#ifdef SIGINFO
		if (bsiginfo) {
			pstatus();
			bsiginfo = 0;
		}
#endif
		switch (waitpid(0, &status, waitflags)) {
		case -1:
			switch (errno) {
			case EINTR:
				break;
			case ECHILD:
				/* only a problem if we're blocking for the
				   first child because of ELWAIT */
				return (waitflags & WNOHANG) ? 0 : -1;
			default:
				perror(NULL);
				exit(1);
			}
			break;
		case 0:
			/* nothing pending (WNOHANG) */
			return 0;
		default:
			/* child exited */
			njobs--;
			if (status) {
				nfailed++;
				write(STDOUT_FILENO, "!", 1);
			} else {
				ngood++;
				write(STDOUT_FILENO, "*", 1);
			}

			/* ELWAIT is honored */
			waitflags = WNOHANG;
			break;
		}
	}
}

int
main(int argc, char **argv)
{
	struct timespec ts;

	parseopts(argc, argv);

	signal(SIGCHLD, onsigchld); /* to interrupt nanosleep() */
	signal(SIGINT, onsigint);
#ifdef SIGINFO
	signal(SIGINFO, onsiginfo);
#endif

	while (!maxtotal || njobs+nfailed+ngood < maxtotal) {
		while (maxjobs && njobs >= maxjobs)
			evtloop(ELWAIT);

		startone();
		evtloop(ELPOLL);

		if (delay) {
			ts.tv_sec = delay / 1000;
			ts.tv_nsec = (long)(delay % 1000) * 1000 * 1000;

			while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
				evtloop(ELPOLL);
		}
	}

	/* wait for all children */
	while (evtloop(ELWAIT) == 0)
		;

	pstatus();
	return 0;
}
