# Compiler
CC = gcc
CFLAGS = -Wall -Werror -g

# Output files
CLIENT = client
SERVER = server

# Compile	
all: $(CLIENT) $(SERVER)

$(CLIENT): client.c
	$(CC) $(CFLAGS) client.c -o $(CLIENT)

$(SERVER): server.c
	$(CC) $(CFLAGS) server.c -o $(SERVER)

# Clean up build artifacts
clean:
	rm -f $(CLIENT) $(SERVER)
