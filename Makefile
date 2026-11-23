# GAIN BOY — GBDK-2020 4.5.0
#
#   export GBDK_HOME=/opt/gbdk
#   make                 # → rom/gainboy.gb
#   make assets          # regenerate tiles / wavetables after editing tools/gen_assets.py

GBDK_HOME ?= /opt/gbdk
LCC       := $(GBDK_HOME)/bin/lcc

SRCDIR    := src
INCDIR    := include
ROMDIR    := rom
OBJDIR    := build
TARGET    := $(ROMDIR)/gainboy.gb

OBJS      := \
	$(OBJDIR)/main.o \
	$(OBJDIR)/songs.o \
	$(OBJDIR)/sound.o \
	$(OBJDIR)/player.o \
	$(OBJDIR)/seq.o \
	$(OBJDIR)/gig.o \
	$(OBJDIR)/band.o \
	$(OBJDIR)/helpers.o \
	$(OBJDIR)/save.o \
	$(OBJDIR)/assets_data.o

# Compile flags (per .c → .o). main.c is large and takes a few minutes alone.
CFLAGS    := -I$(INCDIR) -Wm-yn"GAINBOY"

# Link: MBC1 + RAM + battery, 2 ROM banks, 1 RAM bank; emit .map / .noi
LDFLAGS   := -Wm-yn"GAINBOY" -Wl-yt0x03 -Wl-yo2 -Wl-ya1 -Wl-m -Wl-j

.PHONY: all clean assets pages-serve

all: $(TARGET)

$(ROMDIR) $(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/assets.h $(INCDIR)/game.h | $(OBJDIR)
	$(LCC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS) | $(ROMDIR)
	$(LCC) $(LDFLAGS) -o $@ $(OBJS)
	mkdir -p docs/rom
	cp $@ docs/rom/gainboy.gb

assets:
	python3 tools/gen_assets.py

# Local preview of the GitHub Pages play page (needs network for EmulatorJS CDN)
pages-serve:
	python3 -m http.server 8080 --directory docs

clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET) $(ROMDIR)/gainboy.map $(ROMDIR)/gainboy.noi *.o *.lst *.map *.ihx *.sym *.noi
