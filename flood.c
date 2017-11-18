#define _BSD_SOURCE

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

static void
parseopts(int argc, char **argv)
{
	int c;
	char *end;

	while ((c = getopt(argc, argv, "d:j:n:")) != -1) {
		switch (c) {
		case 'd':
			delay = strtol(optarg, &end, 10);
			if (delay < 0 || *end) {
				fputs("invalid delay (-d)\n", stderr);
				exit(1);
			}
			break;
		case 'j':
			maxjobs = strtol(optarg, &end, 10);
			if (maxjobs < 0 || *end) {
				fputs("invalid maxjobs (-j)\n", stderr);
				exit(1);
			}
			break;
		case 'n':
			maxtotal = strtol(optarg, &end, 10);
			if (maxtotal < 0 || *end) {
				fputs("invalid count (-n)\n", stderr);
				exit(1);
			}
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

static int
drainone(int bblock)
{
	int status;

	if (waitpid(0, &status, bblock ? 0 : WNOHANG) <= 0)
		return -1;

	njobs--;
	if (status) {
		nfailed++;
		write(STDOUT_FILENO, "!", 1);
	} else {
		ngood++;
		write(STDOUT_FILENO, "*", 1);
	}

	return 0;
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

	while (!bsigint && (!maxtotal || njobs+nfailed+ngood < maxtotal)) {
		startone();

		if (delay) {
			ts.tv_sec = delay / 1000;
			ts.tv_nsec = (delay % 1000) * 1e6;
		}

		do {
			/* checking here too prevents completion/failure
			   output to be written to the terminal by drainone()
			   after ^C */
			if (bsigint)
				break;

			/* poll for completions, blocking in case we've hit
			   the user-set job limit */
			while (drainone(maxjobs && njobs >= maxjobs) != -1)
				;
#ifdef SIGINFO
			if (bsiginfo) {
				pstatus();
				bsiginfo = 0;
			}
#endif
			/* the sleep will be interrupted by SIGCHLD or
			   SIGINFO, so handle these and then try again to
			   sleep the remaining time */
		} while (delay && nanosleep(&ts, &ts) == -1 && errno == EINTR);
	}

	if (!bsigint) {
		/* wait for all children */
		while (!bsigint && drainone(1) != -1)
			;
		if (errno != ECHILD) {
			perror(NULL);
			return 1;
		}
	}

	pstatus();
	signal(SIGINT, SIG_DFL); /* prevent race condition with next if */
	if (bsigint)
		raise(SIGINT);   /* for proper exit status */

	return 0;
}
