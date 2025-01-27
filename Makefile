# Compiler
CC = gcc

# Output files
CLIENT = client
SERVER = server

# Compile	
all: $(CLIENT) $(SERVER)

$(CLIENT): client.c
	$(CC) client.c -o $(CLIENT)

$(SERVER): server.c
	$(CC) server.c -o $(SERVER)

# Clean up build artifacts
clean:
	rm -f $(CLIENT) $(SERVER)
