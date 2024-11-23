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
PROGS     =$(BUILDDIR)/mount.gemini$(EXE) $(BUILDDIR)/geminifs_uri$(EXE) $(BUILDDIR)/geminifs_cnx$(EXE)
LIBS      =-lfuse3
HEADERS   =$(shell find . -name '*.h')

MNT_SOURCES = fuse3_main.c cnx_dir.c cnx.c gmi.c uri.c pool.c
URI_SOURCES = uri_main.c uri.c
CNX_SOURCES = cnx_main.c cnx_dir.c cnx.c gmi.c uri.c pool.c

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
$(BUILDDIR)/geminifs_cnx$(EXE): $(patsubst %.c, $(BUILDDIR)/obj/%.o, $(CNX_SOURCES))
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
##
$(BUILDDIR)/obj/%.o: %.c $(HEADERS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
## sudo apt-get -y install libfuse3-dev
## -- BLOCK:c --
clean: clean-c
clean-c:
	rm -f *.o
## -- BLOCK:c --
