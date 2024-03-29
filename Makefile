# Copyright (C) 2009, 2017 Chris Simmonds (chris@2net.co.uk)
#
# If cross-compiling, CC must point to your cross compiler, for example:
# make CC=arm-linux-gnueabihf-gcc

DESTDIR = /
BINDIR = opt/sbin
LOCAL_CFLAGS = -Wall -g
PROGRAM = mtd_check

$(PROGRAM): $(PROGRAM).c
	$(CC) $(CFLAGS) $(LOCAL_CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(PROGRAM)

install:
	install -d $(DESTDIR)/$(BINDIR)
	install -m 0755 $(PROGRAM) $(DESTDIR)/$(BINDIR)
	cp mtd_check mtd_check64
