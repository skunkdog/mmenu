PREFIX ?= /usr/local

CC  = clang

TARGET = mmenu
OBJECT1 = mmenu.c
OBJECT2 = gesture_recogniser.c

PKG_CONFIG ?= pkg-config
CFLAGS = -Wall -Wextra -pedantic -O2 -march=native -g

# Libraries go at the end of the link command.
LDLIBS += -lX11 -lXrender -lm

# Uncomment on openbsd
CFLAGS += -I/usr/X11R6/include -I/usr/local/include
LDFLAGS += -L/usr/X11R6/lib -L/usr/local/lib

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECT1) $(OBJECT2) $(LDLIBS)

test: $(TARGET)
	make all
	./$(TARGET)
clean:
	rm -f $(OBJECTS) $(TARGET)
