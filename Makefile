DESTDIR   ?=
PREFIX    ?= /usr/local
MANPREFIX ?= $(PREFIX)/man

CFLAGS += -ansi -g
CFLAGS += -D_POSIX_C_SOURCE=199309L
CFLAGS += -Wall -Wextra -pedantic

all: flood

clean:
	rm -f flood

install: all
	install -d $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(MANPREFIX)/man1
	install -m755 flood   $(DESTDIR)$(PREFIX)/bin/
	install -m644 flood.1 $(DESTDIR)$(MANPREFIX)/man1/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/flood
	rm -f $(DESTDIR)$(MANPREFIX)/man1/flood.1

.PHONY: all clean install uninstall
