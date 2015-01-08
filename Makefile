BINDIR = bin
INCDIR = include
OBJDIR = obj
SRCDIR = src

CC = gcc
MKDIR = mkdir -p
PKG_CONFIG = pkg-config

# Allegro version detection.
# To override detection specify ALLEGRO_VERSION in the environment or on
# the command line.

ALLEGRO_LIBS_BASE = allegro- allegro_acodec- allegro_audio- allegro_dialog- allegro_font- allegro_image- allegro_primitives-
ALLEGRO_VERSION = $(shell make/detect-allegro $(PKG_CONFIG))

ifeq (,$(ALLEGRO_VERSION))
    $(error Failed to detect Allegro version. \
            Please install Allegro 5. \
            Make sure pkg-config is properly configured to find it.)
endif

ALLEGRO_LIBS=$(ALLEGRO_LIBS_BASE:-=-$(ALLEGRO_VERSION))
ALLEGRO_LIBDIR = $(shell \
        pkg-config --libs-only-L allegro-$(ALLEGRO_VERSION) | \
        sed -re 's/(^|\s)-L/\1/g' -e 's/(^\s+|\s+$$)//g')

CFLAGS = -g3 -Iinclude -Wall $(shell $(PKG_CONFIG) --cflags $(ALLEGRO_LIBS))
LIBS = -lm $(shell $(PKG_CONFIG) --libs $(ALLEGRO_LIBS))

GAME = $(BINDIR)/ic-b2
HEADERS = $(shell make/list-extension $(INCDIR) h)
LAUNCHER = ic-b2
SOURCES = $(shell make/list-extension $(SRCDIR) c)
OBJECTS = $(shell make/list-objects $(SRCDIR) c $(OBJDIR) o $(SOURCES))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(MKDIR) $(OBJDIR)
	$(CC) -c -o $@ $(CFLAGS) $<

.PHONY: all clean detect-allegro

all: $(LAUNCHER)

clean:
	rm -fR $(BINDIR) $(OBJDIR) $(LAUNCHER)

detect-allegro:
	$(PKG_CONFIG) --cflags --libs $(ALLEGRO_LIBS)

$(GAME): $(OBJECTS)
	$(MKDIR) $(BINDIR)
	$(CC) -o $@ $(CFLAGS) $^ $(LIBS)

$(LAUNCHER): $(GAME)
	echo '#!/bin/sh' > $@
	echo 'set -e' >> $@
	echo 'dir="$$(dirname "$$0")"' >> $@
	echo 'cd "$$dir/media"' >> $@
	echo 'LD_LIBRARY_PATH="$$LD_LIBRARY_PATH:$(ALLEGRO_LIBDIR)"' >> $@
	echo 'export LD_LIBRARY_PATH' >> $@
	echo '../$<' >> $@
	chmod ugo+x $@
