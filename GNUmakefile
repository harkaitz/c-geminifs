.POSIX:
.SUFFIXES:
.PHONY: all clean install check

PROJECT   =c-geminifs
VERSION   =1.0.0
CC        = $(shell which $(TPREFIX)cc $(TPREFIX)gcc 2>/dev/null | head -n 1)
CFLAGS    = -Wall -g3 -std=c99 #-DDEBUG
PREFIX    =/usr/local
BUILDDIR ?=.build
UNAME_S  ?=$(shell uname -s)
EXE      ?=$(shell uname -s | awk '/Windows/ || /MSYS/ || /CYG/ { print ".exe" }')
PROGS     =$(patsubst %, $(BUILDDIR)/%$(EXE), mount.gemini)
LIBS      =$(shell pkg-config --static --libs fuse3 libtls)
HEADERS   =$(shell find . -name '*.h')
SOURCES   = main.c gmi.c pool.c ssl.c gem.c uri.c


all: $(PROGS)
clean:
	rm -fr $(PROGS) $(BUILDDIR)/obj
install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(PROGS) $(DESTDIR)$(PREFIX)/bin
check:

$(BUILDDIR)/mount.gemini$(EXE): $(patsubst %.c, $(BUILDDIR)/obj/%.o, $(SOURCES))
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

##
$(BUILDDIR)/obj/%.o: %.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
deps:
	build_libressl
	build_libfuse
ifeq ($(LIBS),)
$(error Missing dependencies)
endif
## -- BLOCK:c --
clean: clean-c
clean-c:
	rm -f *.o
## -- BLOCK:c --
