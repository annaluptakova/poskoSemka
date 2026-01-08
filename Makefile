CC = gcc
CFLAGS = -Wall -g

all: client server

client: client.c client.h shared.h simulation.c simulation.h
	$(CC) $(CFLAGS) client.c simulation.c -o client

server: server.c server.h shared.h simulation.c simulation.h
	$(CC) $(CFLAGS) server.c simulation.c -o server

clean:
	rm -f client server

run: client server
	./client