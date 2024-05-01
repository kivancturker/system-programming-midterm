CC = gcc
CFLAGS = -Wall -Iinclude -std=gnu99
SERVER_OBJS = obj/server.o obj/myutil.o obj/ipc.o obj/command_handler.o obj/queue.o obj/child.o obj/fileops.o
CLIENT_OBJS = obj/client.o obj/myutil.o obj/ipc.o obj/command_handler.o obj/queue.o obj/fileops.o

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