CC=gcc
CFLAGS=-Wall -pedantic -std=c99 -lX11 $(shell pkg-config --cflags --libs gdk-3.0)
DEPS = cjournal.h
OBJ = cjournal.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cjournal: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)