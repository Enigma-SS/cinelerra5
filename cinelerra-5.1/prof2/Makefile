TARGETS = libprofile.so prof smap
BITS=64
#CFLAGS += -O
CFLAGS += -g -Wall
BIN_PATH ?= /usr/bin
LIB_PATH ?= $(dir $(lastword $(wildcard /lib*/ld-*.so /lib*/*-linux-gnu/ld-*.so)))
$(if $(LIB_PATH),,$(error cant find ld-.so))
CFLAGS += -DLIB='"$(LIB_PATH)"'
ASM_PATH ?= $(dir $(lastword $(wildcard /usr/include/asm/signal.h /usr/include/x86_64-linux-gnu/asm/signal.h)))

.PHONY: all
all:	$(TARGETS)

.PHONY: install
install:
	cp -a libprofile.so $(DESTDIR)$(LIB_PATH)/.
	cp -a prof smap $(DESTDIR)$(BIN_PATH)/.

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(LIB_PATH)/libprofile.so
	rm -f $(DESTDIR)$(BIN_PATH)/prof
	rm -f $(DESTDIR)$(BIN_PATH)/smap

libprofile.so:	profile.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

prof:	prof.c sys.h
	$(CC) $(CFLAGS) -o $@ $<

smap:	smap.C
	$(CXX) $(CFLAGS) -o $@ $< \
	 -Wall -I/usr/include/libiberty -llzma -liberty

tst:	tst.c

sys.h:	$(ASM_PATH/signal.h $(ASM_PATH)/unistd.h
	echo "char *sysreq_name[] = {" > sys.h
	./mksyscallent < $(ASM_PATH)/unistd_$(BITS).h >> sys.h
	echo "};" >> sys.h
	echo "char *signal_name[] = {" >> sys.h
	./mksignalent < $(ASM_PATH)/signal.h >> sys.h
	echo "};" >> sys.h

.PHONY: clean
clean:
	rm -f $(TARGETS) tst sys.h
