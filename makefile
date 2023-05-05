CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server -pthread

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client
