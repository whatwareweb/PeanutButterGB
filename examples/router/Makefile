# PeanutButter: a DMG emulator for wifi routers


CFLAGS := -DENABLE_SOUND -DENABLE_SOUND_MINIGB -DMINIGB_APU_AUDIO_FORMAT_S16SYS

OPT := -O2 -Wall -Wextra

SOURCES := routergb.c minigb_apu/minigb_apu.c fileutils/fileutils.c
OBJECTS := routergb.o minigb_apu/minigb_apu.o fileutils/fileutils.o

LDLIBS := -lm -latomic

all: peanutbutter
peanutbutter: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

routergb.o: routergb.c ../../peanut_gb.h