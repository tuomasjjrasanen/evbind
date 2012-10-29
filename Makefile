package = evbind
objects = main.o err.o
programs = evbind

srcdir = .
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
datarootdir = $(prefix)/share
datadir = $(datarootdir)
sysconfdir = $(prefix)/etc
includedir = $(prefix)/include
docdir = $(datarootdir)/doc/$(package)
infodir = $(datarootdir)/info

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
		$(DESTDIR)$(bindir)

install : $(programs) installdirs
	$(INSTALL_PROGRAM) $< $(DESTDIR)$(bindir)/$<

uninstall : $(DESTDIR)$(bindir)/evbind
	rm -rf $<

clean :
	$(MAKE) -C man clean
	rm -rf $(objects)
	rm -rf $(programs)
