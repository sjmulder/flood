#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

static void
onsigchld(int sig)
{
	int status;

	while (waitpid(0, &status, WNOHANG) > 0) {
		putchar(status ? '!' : '*');
		fflush(stdout);
	}
}

int
main(int argc, char **argv)
{
	int outfd;
	long delay = 100;
	struct timespec ts;

	if (argc < 2) {
		fputs("usage: flood command [argument ...]\n", stderr);
		return 1;
	}

	signal(SIGCHLD, onsigchld);

	while (1) {
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
			execvp(argv[1], argv+1);
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
