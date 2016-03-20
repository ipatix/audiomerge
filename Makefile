CC = gcc
CFLAGS = -Wall -Wextra -Wconversion -std=c99 -O2
LIBS = -lsndfile
BINARY = audiomerge

.PHONY: all
all: $(BINARY)


$(BINARY): audiomerge.c
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
