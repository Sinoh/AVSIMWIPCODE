CC= gcc
CFLAGS= -g -Wall
LIBS = -lpthread


all: test

test: test.cc networks.o networks.h
	$(CC) $(CFLAGS) -o test test.cc networks.o  $(LIBS)

clean:
	rm -f server test *.o


