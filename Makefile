# Compiler
CC = gcc
CFLAGS = -Wall -Werror -g -O2 
CLINK = -lssl -lcrypto

# Output files
SERVER = server
SERVER2 = server2


# Compile	
all: $(SERVER)

$(SERVER): server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER) $(CLINK)

# Clean up build artifacts
clean:
	rm -f $(SERVER2) $(SERVER)
