prefix = /usr/local
exec_prefix = $(prefix)
sbindir = $(exec_prefix)/sbin

INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

subdirs = man src
clean-subdirs = $(subdirs:%=clean-%)

.PHONY : all
all : $(subdirs)

.PHONY : $(subdirs)
$(subdirs) :
	$(MAKE) -C $@

.PHONY : installdirs
installdirs : tools/mkinstalldirs
	tools/mkinstalldirs $(DESTDIR)$(sbindir)

.PHONY : install
install : $(subdirs) installdirs
	$(INSTALL_PROGRAM) src/evbind $(DESTDIR)$(sbindir)/

.PHONY : $(clean-subdirs)
$(clean-subdirs) :
	$(MAKE) -C $(@:clean-%=%) clean

.PHONY : clean
clean : $(clean-subdirs)
