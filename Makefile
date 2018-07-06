# BSD conventions by default, override to taste
prefix  ?= /usr/local
bindir  ?= $(prefix)/bin
man1dir ?= $(prefix)/man/man1

CFLAGS += -ansi -g
CFLAGS += -Wall -Wextra -pedantic

all: flood

clean:
	rm -f flood

install: all
	install -d $(bindir) $(man1dir)
	install flood $(bindir)/
	install flood.1 (man1dir)/

uninstall:
	rm -f $(bindir)/flood $(man1dir)/flood.1

.PHONY: all clean install uninstall
