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

static int maxjobs, numjobs;

static void
onsigchld(int sig)
{
	int status;

	while (waitpid(0, &status, WNOHANG) > 0) {
		numjobs--;
		putchar(status ? '!' : '*');
		fflush(stdout);
	}
}

int
main(int argc, char **argv)
{
	int c, outfd;
	char *end;
	long delay = 100;
	struct timespec ts;
	sigset_t sigset;

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
	sigemptyset(&sigset);

	while (1) {
		while (maxjobs && numjobs >= maxjobs)
			sigsuspend(&sigset);

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
			close(outfd);
			execvp(argv[0], argv);
		err:
			putchar('@');
			return 0;
		}

		ts.tv_sec = delay / 1000;
		ts.tv_nsec = (delay % 1000) * 1e6;

		while (nanosleep(&ts, &ts) == -1 && errno == EINTR)
			;
	}
}
