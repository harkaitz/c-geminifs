.POSIX:
.SUFFIXES:
.PHONY: all clean install check

PROJECT   =c-geminifs
VERSION   =1.0.0
CC        = $(shell which $(TPREFIX)cc $(TPREFIX)gcc 2>/dev/null | head -n 1)
CFLAGS    = -Wall -g3 -std=c99 -DDEBUG
PREFIX    =/usr/local
BUILDDIR ?=.build
UNAME_S  ?=$(shell uname -s)
EXE      ?=$(shell uname -s | awk '/Windows/ || /MSYS/ || /CYG/ { print ".exe" }')
PROGS     =$(BUILDDIR)/mount.gemini$(EXE) $(BUILDDIR)/geminifs_uri$(EXE)
LIBS      =-lfuse3 -ltls -lresolv -lssl -lcrypto
HEADERS   =$(shell find . -name '*.h')

MNT_SOURCES = fuse3_main.c cnx.c gmi.c uri.c ssl.c
URI_SOURCES = uri_main.c uri.c

all: $(PROGS)
clean:
	rm -f $(PROGS)
install:
check:

$(BUILDDIR)/mount.gemini$(EXE): $(patsubst %.c, $(BUILDDIR)/obj/%.o, $(MNT_SOURCES))
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
$(BUILDDIR)/geminifs_uri$(EXE): $(patsubst %.c, $(BUILDDIR)/obj/%.o, $(URI_SOURCES))
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

##
$(BUILDDIR)/obj/%.o: %.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
deps:
	build_libressl
	build_libfuse

## 
## -- BLOCK:c --
clean: clean-c
clean-c:
	rm -f *.o
## -- BLOCK:c --
