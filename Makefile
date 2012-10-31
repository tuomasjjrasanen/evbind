objects = main.o err.o
programs = evbind

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

subdirs = man
clean-subdirs = $(subdirs:%=clean-%)

.PHONY : all clean install installdirs uninstall $(subdirs) $(clean-subdirs)

all : $(programs) $(subdirs)

$(subdirs) :
	$(MAKE) -C $@

%.o : %.c
	$(CC) $(CPPFLAGS_ALL) $(CFLAGS_ALL) -c $<

$(programs) : $(objects)
	$(CC) $(LDFLAGS_ALL) $(CFLAGS_ALL) -o $@ $^

installdirs : tools/mkinstalldirs
	tools/mkinstalldirs $(DESTDIR)$(sbindir)

install : $(programs) installdirs
	$(INSTALL_PROGRAM) $(programs) $(DESTDIR)$(sbindir)/

$(clean-subdirs) :
	$(MAKE) -C $(@:clean-%=%) clean

clean : $(clean-subdirs)
	rm -rf $(objects)
	rm -rf $(programs)
