objects = main.o err.o
programs = evbind
manpages = man/evbind.8

prefix = /usr/local
exec_prefix = $(prefix)
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man8dir = $(mandir)/man8

CFLAGS = -g -Wall -Wextra -Werror
CFLAGS_ALL = $(CFLAGS) -std=c99

LDFLAGS =
LDFLAGS_ALL = $(LDFLAGS) -ludev

CPPFLAGS =
CPPFLAGS_ALL = $(CPPFLAGS)

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

.PHONY : all clean install installdirs uninstall

all : $(programs) $(manpages)

$(manpages) : % : %.rst
	rst2man $< $@

%.o : %.c
	$(CC) $(CPPFLAGS_ALL) $(CFLAGS_ALL) -c $<

$(programs) : $(objects)
	$(CC) $(LDFLAGS_ALL) $(CFLAGS_ALL) -o $@ $^

installdirs : tools/mkinstalldirs
	tools/mkinstalldirs $(DESTDIR)$(sbindir)

install : $(programs) installdirs
	$(INSTALL_PROGRAM) $(programs) $(DESTDIR)$(sbindir)/

clean :
	rm -rf $(objects)
	rm -rf $(programs)
	rm -rf $(manpages)
