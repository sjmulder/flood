PREFIX ?= /usr/local
CFLAGS += -ansi -Wall

all: flood

clean:
	rm -f flood

install: flood
	install -d $(PREFIX)/bin \
		   $(PREFIX)/share/doc/flood
	install flood $(PREFIX)/bin/
	install LICENSE.md $(PREFIX)/share/doc/flood/

uninstall:
	rm -f $(PREFIX)/bin/flood \
	      $(PREFIX)/share/doc/flood/LICENSE.md
	-rmdir $(PREFIX)/share/doc/flood/

.PHONY: all clean install uninstall
