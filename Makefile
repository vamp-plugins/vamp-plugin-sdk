

all:
	$(MAKE) -C examples all

clean:
	$(MAKE) -C examples clean

distclean:
	$(MAKE) -C examples distclean
	rm -f *~ *.bak $(TARGET)

