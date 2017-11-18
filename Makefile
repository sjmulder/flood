PREFIX ?= /usr/local
CFLAGS += -ansi -Wall

all: flood flood.1.gz

clean:
	rm -f flood flood.1.gz

install: all
	install -d $(PREFIX)/bin \
		   $(PREFIX)/share/man/man1 \
		   $(PREFIX)/share/doc/flood
	install flood $(PREFIX)/bin/
	install flood.1.gz $(PREFIX)/share/man/man1/
	install LICENSE.md $(PREFIX)/share/doc/flood/

uninstall:
	rm -f $(PREFIX)/bin/flood \
	      $(PREFIX)/share/man/man1/flood.1.gz \
	      $(PREFIX)/share/doc/flood/LICENSE.md
	-rmdir $(PREFIX)/share/doc/flood/

flood.1.gz: flood.1
	gzip -kf flood.1

.PHONY: all clean install uninstall
