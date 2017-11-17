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

static int maxjobs, numjobs, numfailed, numgood;
static int bsiginfo;

static void onsigchld(int sig) { }
static void onsiginfo(int sig) { bsiginfo = 1; }

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
pstatus()
{
	int goodpct, badpct;

	if (maxjobs)
		printf("running:   %6d/%d\n", numjobs, maxjobs);
	else
		printf("running:   %6d\n", numjobs);

	goodpct = (numgood   * 100) / (numgood + numfailed);
	badpct  = (numfailed * 100) / (numgood + numfailed);

	printf("completed: %6d (%3d%%)\n", numgood, goodpct);
	printf("failed:    %6d (%3d%%)\n", numfailed, badpct);
}

int
main(int argc, char **argv)
{
	int c, outfd;
	char *end;
	long delay = 100;
	struct timespec ts;

	while ((c = getopt(argc, argv, "d:j:")) != -1) {
		switch (c) {
		case 'd':
			delay = strtol(optarg, &end, 10);
			if (delay < 0 || *end != '\0') {
				fputs("invalid delay (-d)\n", stderr);
				return 1;
			}
			break;
		case 'j':
			maxjobs = strtol(optarg, &end, 10);
			if (maxjobs < 0 || *end != '\0') {
				fputs("invalid maxjobs (-j)\n", stderr);
				return 1;
			}
			break;
		default:
			fputs(usage, stderr);
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		fputs(usage, stderr);
		return 1;
	}

	signal(SIGCHLD, onsigchld);
	signal(SIGINFO, onsiginfo);

	while (1) {

		numjobs++;

		putchar('.');
		fflush(stdout);

		switch (fork()) {
		case -1:
			perror(NULL);
			return 1;
		case 0:
			if ((outfd = open("/dev/null", 0)) == -1)
				goto err;
			dup2(outfd, STDOUT_FILENO);
			dup2(outfd, STDERR_FILENO);
			close(outfd);
			execvp(argv[0], argv);
		err:
			putchar('@');
			return 0;
		}

		ts.tv_sec = delay / 1000;
		ts.tv_nsec = (delay % 1000) * 1e6;

		do {
			while (drainone(maxjobs && numjobs >= maxjobs) != -1)
				;

			if (bsiginfo) {
				pstatus();
				bsiginfo = 0;
			}
		} while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
	}
}
