CC = gcc
CFLAGS = -Wall -Iinclude -std=gnu99
SERVER_OBJS = obj/server.o
CLIENT_OBJS = obj/client.o

all: server client

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f server client obj/*.o
	rm -f log.txt