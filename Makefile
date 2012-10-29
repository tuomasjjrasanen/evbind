package = evbind
objects = main.o err.o
programs = evbind

srcdir = .
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
sysconfdir = $(prefix)/etc
includedir = $(prefix)/include
docdir = $(datarootdir)/doc/$(package)
infodir = $(datarootdir)/info
mandir = $(datarootdir)/man
man8dir = $(mandir)/man8
man8ext = .8

CFLAGS = -g -Wall -Wextra -Werror
CFLAGS_ALL = $(CFLAGS) -std=c99

LDFLAGS =
LDFLAGS_ALL = $(LDFLAGS) -ludev

CPPFLAGS =
CPPFLAGS_ALL = $(CPPFLAGS)

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

.PHONY : all clean install installdirs uninstall manpages

all : $(programs) manpages

manpages :
	$(MAKE) -C man

%.o : %.c
	$(CC) $(CPPFLAGS_ALL) $(CFLAGS_ALL) -c $<

evbind : main.o err.o
	$(CC) $(LDFLAGS_ALL) $(CFLAGS_ALL) -o $@ $^

installdirs : mkinstalldirs
	$(srcdir)/mkinstalldirs \
		$(DESTDIR)$(sbindir)

install : $(programs) installdirs
	$(INSTALL_PROGRAM) $< $(DESTDIR)$(sbindir)/$<

uninstall : $(DESTDIR)$(sbindir)/evbind
	rm -rf $<

clean :
	$(MAKE) -C man clean
	rm -rf $(objects)
	rm -rf $(programs)
