CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lssl -lcrypto

# Paths
SHARED_DIR = shared
LIB_DIR = lib
EXTERNAL_DIR = external
# Targets
all: client server init checkDB

client: client.o
	$(CC) $(CFLAGS) -o client client.o $(LDFLAGS)

server: server.o $(SHARED_DIR)/utils.o $(LIB_DIR)/customer.o $(LIB_DIR)/employee.o $(LIB_DIR)/manager.o $(LIB_DIR)/admin.o
	$(CC) $(CFLAGS) -o server server.o $(SHARED_DIR)/utils.o $(LIB_DIR)/customer.o $(LIB_DIR)/employee.o $(LIB_DIR)/manager.o $(LIB_DIR)/admin.o $(LDFLAGS)

init: init.o $(SHARED_DIR)/utils.o
	$(CC) $(CFLAGS) -o init init.o $(SHARED_DIR)/utils.o $(LDFLAGS)

checkDB: checkDB.o $(SHARED_DIR)/utils.o
	$(CC) $(CFLAGS) -o checkDB checkDB.o $(SHARED_DIR)/utils.o $(LDFLAGS)

# Object files
client.o: client.c
	$(CC) $(CFLAGS) -c client.c

server.o: server.c $(SHARED_DIR)/models.h $(SHARED_DIR)/utils.h $(LIB_DIR)/customer.h $(LIB_DIR)/employee.h $(LIB_DIR)/manager.h $(LIB_DIR)/admin.h
	$(CC) $(CFLAGS) -c server.c

init.o: init.c $(SHARED_DIR)/models.h $(SHARED_DIR)/utils.h
	$(CC) $(CFLAGS) -c init.c

checkDB.o: checkDB.c $(SHARED_DIR)/models.h $(SHARED_DIR)/utils.h
	$(CC) $(CFLAGS) -c checkDB.c

$(SHARED_DIR)/utils.o: $(SHARED_DIR)/utils.c $(SHARED_DIR)/models.h
	$(CC) $(CFLAGS) -c $(SHARED_DIR)/utils.c -o $(SHARED_DIR)/utils.o

$(LIB_DIR)/customer.o: $(LIB_DIR)/customer.c $(LIB_DIR)/customer.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/customer.c -o $(LIB_DIR)/customer.o

$(LIB_DIR)/employee.o: $(LIB_DIR)/employee.c $(LIB_DIR)/employee.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/employee.c -o $(LIB_DIR)/employee.o

$(LIB_DIR)/manager.o: $(LIB_DIR)/manager.c $(LIB_DIR)/manager.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/manager.c -o $(LIB_DIR)/manager.o

$(LIB_DIR)/admin.o: $(LIB_DIR)/admin.c $(LIB_DIR)/admin.h
	$(CC) $(CFLAGS) -c $(LIB_DIR)/admin.c -o $(LIB_DIR)/admin.o

# Add SHA-256 object files
external_sha256.o: $(EXTERNAL_DIR)/sha256.c $(EXTERNAL_DIR)/sha256.h
	$(CC) $(CFLAGS) -c $(EXTERNAL_DIR)/sha256.c -o external_sha256.o

# Link SHA-256 object in targets that need it
client: external_sha256.o
server: external_sha256.o
init: external_sha256.o
checkDB: external_sha256.o

clean:
	rm -f client server init checkDB *.o $(SHARED_DIR)/utils.o $(LIB_DIR)/*.o
