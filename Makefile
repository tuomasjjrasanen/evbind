prefix = /usr/local
exec_prefix = $(prefix)
sbindir = $(exec_prefix)/sbin
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man8dir = $(mandir)/man8

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

subdirs = man src
clean-subdirs = $(subdirs:%=clean-%)

.PHONY : all clean install installdirs uninstall $(subdirs) $(clean-subdirs)

all : $(subdirs)

$(subdirs) :
	$(MAKE) -C $@

installdirs : tools/mkinstalldirs
	tools/mkinstalldirs $(DESTDIR)$(sbindir)

install : $(subdirs) installdirs
	$(INSTALL_PROGRAM) src/evbind $(DESTDIR)$(sbindir)/

$(clean-subdirs) :
	$(MAKE) -C $(@:clean-%=%) clean

clean : $(clean-subdirs)
