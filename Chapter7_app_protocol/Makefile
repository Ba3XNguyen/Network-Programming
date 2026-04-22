CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: server client

server: server.c game.c
	$(CC) $(CFLAGS) server.c game.c -o server

client: client.c game.c
	$(CC) $(CFLAGS) client.c game.c -o client

clean:
	rm -f server client