CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lssl -lcrypto

# Targets
all: client server test

client: client.o
	$(CC) $(CFLAGS) -o client client.o $(LDFLAGS)

server: server.o utils.o
	$(CC) $(CFLAGS) -o server server.o utils.o $(LDFLAGS)

test: test.o utils.o
	$(CC) $(CFLAGS) -o test test.o utils.o $(LDFLAGS)

# Object files
client.o: client.c models.h
	$(CC) $(CFLAGS) -c client.c

server.o: server.c models.h utils.h
	$(CC) $(CFLAGS) -c server.c

test.o: test.c models.h utils.h
	$(CC) $(CFLAGS) -c test.c

utils.o: utils.c models.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f client server test *.o
