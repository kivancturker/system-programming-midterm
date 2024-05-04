CC = gcc
CFLAGS = -Wall -Iinclude -std=gnu99
SERVER_OBJS = obj/server.o obj/myutil.o obj/ipc.o obj/command_handler.o obj/queue.o obj/child.o obj/fileops.o obj/logger.o
CLIENT_OBJS = obj/client.o obj/myutil.o obj/ipc.o obj/command_handler.o obj/queue.o obj/fileops.o obj/logger.o

all: neHosServer neHosClient

neHosServer: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

neHosClient: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f neHosServer neHosClient obj/*.o
	rm -f log.txt