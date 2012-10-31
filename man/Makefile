manpages = evbind.8

.PHONY = all clean

all : $(manpages)

$(manpages) : % : %.rst
	rst2man $< $@

clean :
	rm -rf $(manpages)
