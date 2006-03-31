

all:	examples/plugins.so host/simplehost test

examples/plugins.so:
	$(MAKE) -C examples all

host/simplehost:
	$(MAKE) -C host all

test:
	$(MAKE) -C host test

clean:
	$(MAKE) -C examples clean

distclean:
	$(MAKE) -C examples distclean
	$(MAKE) -C host distclean
	rm -f *~ *.bak $(TARGET)

