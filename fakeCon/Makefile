C = clang

CFLAGS = -std=c99 -Wall -o -pedantic 

DEBUG = -g

HEADER = fake_connection.h
FAKE = fake_connection.c

all: $(MUSH) $(HEADER)
	$(CC) $(DEBUG) $(CFLAGS) -o fake $(FAKE)

%.o: %.c
	$(CC) $(DEBUG) $(CFLAGS) -c -o $@ $^

clean:
	$(RM) fake *.o