OBJECTS = trim.o map.o logger.o ini.o tcpcli.o diygeiger.o gqgeiger.o gclog.o
CFLAGS += -Wall -Wextra -Os
#LDFLAGS += 
PREFIX ?= /usr

all: gclog

gclog: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)
#	which upx >/dev/null 2>&1 && upx -9 -qq $@ || true

%.o: %.c
	$(CC) $(CFLAGS) -c $<

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -d $(DESTDIR)/etc
	install -d $(DESTDIR)/etc/init.d
	install -d $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 755 gclog $(DESTDIR)$(PREFIX)/bin/
	install -m 600 gclog.conf $(DESTDIR)/etc/
	install -m 755 gclogd $(DESTDIR)/etc/init.d/
	install -m 644 gclog.1 $(DESTDIR)$(PREFIX)/share/man/man1/

clean:
	rm -f $(OBJECTS)
