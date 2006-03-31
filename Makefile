

all:	examples_ host_ test

examples_:
	$(MAKE) -C examples all

host_:
	$(MAKE) -C host all

test:
	$(MAKE) -C host test

clean:
	$(MAKE) -C examples clean

distclean:
	$(MAKE) -C examples distclean
	$(MAKE) -C host distclean
	rm -f *~ *.bak $(TARGET)

