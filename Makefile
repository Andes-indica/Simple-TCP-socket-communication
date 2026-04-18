CC = gcc
CFLAGS = -Wall -Wextra -std=c11

TARGETS = server client

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

run-server: server
	./server

run-client: client
	./client

clean:
	rm -f $(TARGETS)

.PHONY: all clean run-server run-client
