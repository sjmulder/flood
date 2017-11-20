flood
=====

Rapidly invoke (*flood*) a command.

[![asciicast](https://asciinema.org/a/sriM9Wrp44rkzPsC7IRYsSBrA.png)](https://asciinema.org/a/sriM9Wrp44rkzPsC7IRYsSBrA)

 * Website: https://github.com/sjmulder/flood
 * Video: https://asciinema.org/a/sriM9Wrp44rkzPsC7IRYsSBrA
 * Homebrew tap (macOS): https://github.com/nickolasburr/homebrew-pfa

Usage
-----

**flood** [**-d** *delay*] [**-j** *maxjobs*] [**-n** *count*] *command* [*argument* ...]

**flood** repeatedly invokes a command with a short delay between invocations.
It does not wait for previous invocations to finish. Output is discarded,
but results are summarily reported using single characters:

| Character | Meaning                                |
|-----------|----------------------------------------|
| `.`       | Command invoked                        |
| `*`       | Command completed successfully         |
| `!`       | Command completed with an error status |
| `@`       | Error invoking the command             |

A tally of the number of invocations, successes and failures is printed
when the program terminates, by *SIGINT* (Ctrl+C) or otherwise, or when
sent *SIGINFO* (Ctrl+T) on supported systems like BSD and macOS.

The name and inspiration come from the *ping(1)* **-f** option.

The following options are supported:

**-d** *delay*

Minimum delay between command executions, in miliseconds.
Defaults to 100 (10 per second).

**-j** *maxjobs*

Limits the number of simultaneously running commands.  Once
reached, flood waits for previously invoked commands to complete
before starting a new one. Defaults to 0, which means no limit.

**-n** *count*

Limits the total number of command invocations.  Once reached,
flood waits for all previously launched commands to complete,
prints a tally, and exits. Defaults to 0, which means no limit.

Examlpes
--------

Flood *example.com* with 100 requests using *curl(1)* then print a tally:

    $ flood -n100 curl example.com

Repeatedly run the *sleep(1)* utility as quickly as possible, but never
have more than 10 running at the same time:

    $ flood -d0 -j10 sleep 1

Building
--------

Should build without changes on Unix-like systems. If not, please file an
issue on GitHub. To build:

    make

There are *install* and *uninstall* targets, too. *PREFIX* is set to
*/usr/local* by default.

Author
------

By Sijmen J. Mulder (<ik@sjmulder.nl>)
