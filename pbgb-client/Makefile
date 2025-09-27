OPT := -O2 -Wall -Wextra
CFLAGS := $(OPT) $(shell pkg-config --cflags sdl3)
LDLIBS := $(shell pkg-config --libs sdl3)

SOURCES := client.c
OBJECTS := client.o


all: client
client: $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

