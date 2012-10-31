manpages = evbind.8

.PHONY : all
all : $(manpages)

$(manpages) : % : %.rst
	rst2man $< $@

.PHONY : clean
clean :
	rm -rf $(manpages)
