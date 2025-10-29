CC=gcc
CFLAGS=-Wall -Wextra -O2

all: server client

server: server.c common.h
	$(CC) $(CFLAGS) server.c -o server -lm

client: client.c common.h
	$(CC) $(CFLAGS) client.c -o client -lm

clean:
	rm -f server client
