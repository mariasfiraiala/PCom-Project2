CC=gcc
CFLAGS=-Wall -Werror -Wno-error=unused-variable

build: server subscriber

server: server.c common.c
	$(CC) -o $@ $^ $(CFLAGS)

subscriber: subscriber.c common.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf server subscriber
