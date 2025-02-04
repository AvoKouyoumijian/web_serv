# Compiler
CC = gcc
CFLAGS = -Wall -Werror -g -O2 
CLINK = -lssl -lcrypto

# Output files
SERVER = server
SERVER2 = server2


# Compile	
all: $(SERVER2) $(SERVER)

$(SERVER): server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER) $(CLINK)

$(SERVER2): server2.c
	$(CC) $(CFLAGS) server2.c -o $(SERVER2) $(CLINK)

# Clean up build artifacts
clean:
	rm -f $(SERVER2) $(SERVER)
