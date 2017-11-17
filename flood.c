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
"usage: flood [-d delay] [-j maxjobs] command [argument ...]\n";

static char **cmdargs;
static int delay = 100, maxjobs;
static int numjobs, numfailed, numgood;
static int bsigint;

static void onsigchld(int sig) { }
static void onsigint(int sig)  { bsigint = 1; }

#ifdef SIGINFO
static int bsiginfo;
static void onsiginfo(int sig) { bsiginfo = 1; }
#endif

static void
parseopts(int argc, char **argv)
{
	int c;
	char *end;

	while ((c = getopt(argc, argv, "d:j:")) != -1) {
		switch (c) {
		case 'd':
			delay = strtol(optarg, &end, 10);
			if (delay < 0 || *end != '\0') {
				fputs("invalid delay (-d)\n", stderr);
				exit(1);
			}
			break;
		case 'j':
			maxjobs = strtol(optarg, &end, 10);
			if (maxjobs < 0 || *end != '\0') {
				fputs("invalid maxjobs (-j)\n", stderr);
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

	numjobs++;

	putchar('.');
	fflush(stdout);

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

	numjobs--;
	if (status) {
		numfailed++;
		putchar('!');
	} else {
		numgood++;
		putchar('*');
	}

	fflush(stdout);
	return 0;
}

static void
pstatus(void)
{
	int goodpct, badpct;

	if (maxjobs)
		printf("running:   %6d/%d\n", numjobs, maxjobs);
	else
		printf("running:   %6d\n", numjobs);

	if (numgood + numfailed) {
		goodpct = (numgood   * 100) / (numgood + numfailed);
		badpct  = (numfailed * 100) / (numgood + numfailed);

		printf("completed: %6d (%3d%%)\n", numgood, goodpct);
		printf("failed:    %6d (%3d%%)\n", numfailed, badpct);
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

	signal(SIGCHLD, onsigchld);
	signal(SIGINT, onsigint);
#ifdef SIGINFO
	signal(SIGINFO, onsiginfo);
#endif

	while (!bsigint) {
		startone();

		ts.tv_sec = delay / 1000;
		ts.tv_nsec = (delay % 1000) * 1e6;

		do {
			if (bsigint)
				break;

			while (drainone(maxjobs && numjobs >= maxjobs) != -1)
				;

#ifdef SIGINFO
			if (bsiginfo) {
				pstatus();
				bsiginfo = 0;
			}
#endif
		} while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
	}

	putchar('\n');
	pstatus();
	signal(SIGINT, SIG_DFL);
	raise(SIGINT);
	return 0;
}
