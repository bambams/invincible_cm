BINDIR = bin
INCDIR = include
OBJDIR = obj
SRCDIR = src

CC = gcc
MKDIR = mkdir -p
PKG_CONFIG = pkg-config

ALLEGRO_LIBS = allegro-5 allegro_acodec-5 allegro_audio-5 allegro_dialog-5 allegro_font-5 allegro_image-5 allegro_primitives-5
ALLEGRO_LIBDIR = $(shell pkg-config --libs-only-L allegro-5 | \
                   sed -re 's/(^|\s)-L/\1/g' -e 's/(^\s+|\s+$$)//g')

CFLAGS = -g3 -Iinclude -Wall $(shell $(PKG_CONFIG) --cflags $(ALLEGRO_LIBS))
LIBS = $(shell $(PKG_CONFIG) --libs $(ALLEGRO_LIBS))

GAME = $(BINDIR)/ic-b2
HEADERS = $(shell make/list-extension $(INCDIR) h)
LAUNCHER = ic-b2
SOURCES = $(shell make/list-extension $(SRCDIR) c)
OBJECTS = $(shell make/list-objects $(SRCDIR) c $(OBJDIR) o $(SOURCES))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(MKDIR) $(OBJDIR)
	$(CC) -c -o $@ $(CFLAGS) $<

.PHONY: all clean

all: $(LAUNCHER)

clean:
	rm -fR $(BINDIR) $(OBJDIR) $(LAUNCHER)

$(GAME): $(OBJECTS)
	$(MKDIR) $(BINDIR)
	$(CC) -o $@ $(CFLAGS) $(LIBS) $?

$(LAUNCHER): $(GAME)
	echo '#!/bin/sh' > $@
	echo 'set -e' >> $@
	echo 'cd media' >> $@
	echo 'LD_LIBRARY_PATH="$$LD_LIBRARY_PATH:$(ALLEGRO_LIBDIR)"' >> $@
	echo 'export LD_LIBRARY_PATH' >> $@
	echo '../$<' >> $@
	chmod ugo+x $@
