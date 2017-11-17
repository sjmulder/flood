#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

static int
supervize(char **argv)
{
	pid_t pid;
	int outfd, status;

	putchar('.');
	fflush(stdout);

	switch ((pid = fork())) {
	case 0:
		if ((outfd = open("/dev/null", 0)) == -1) {
			putchar('@');
			return 1;
		}
		dup2(outfd, STDOUT_FILENO);
		close(outfd);
		execvp(argv[0], argv);
	case -1:
		putchar('@');
		return 1;
	default:
		waitpid(pid, &status, 0);	
		putchar(status ? '!' : '*');
		return 0;
	}
}

int main(int argc, char **argv)
{
	pid_t pid;
	int err, status;

	if (argc < 2) {
		fputs("usage: flood <command> <arguments ...>\n", stderr);
		return 1;
	}

	while (1) {
		switch ((pid = fork())) {
		case -1:
			err = errno;
			goto error;
		case 0:
			return supervize(argv+1);
		}

		usleep(1e4);
	}

error:
	while (wait(&status) != -1)
		;
	if (errno != ECHILD)
		perror(NULL);
	fprintf(stderr, "%s\n", strerror(err));
	return 1;
}
