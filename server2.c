#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/err.h>

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in addr = {0}; // Initialize to zero
    addr.sin_family = AF_INET;
    addr.sin_port = htons(3490);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    { // Cast to (struct sockaddr *)
        perror("Failed to bind socket");
        return 1;
    }

    if (listen(sockfd, 10) < 0)
    {
        perror("Failed to listen on socket");
        return 1;
    }

    int clientfd = accept(sockfd, NULL, NULL);
    if (clientfd < 0)
    {
        perror("Failed to accept client connection");
        return 1;
    }

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == NULL)
    {
        perror("Failed to create SSL context");
        return 1;
    }

    SSL *ssl = SSL_new(ctx);
    if (ssl == NULL)
    {
        perror("Failed to create SSL object");
        return 1;
    }

    SSL_set_fd(ssl, clientfd);
    if (SSL_use_certificate_chain_file(ssl, "fullchain.pem") <= 0)
    {
        perror("Failed to use certificate");
        return 1;
    }

    if (SSL_use_PrivateKey_file(ssl, "server-key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        perror("Failed to use private key");
        return 1;
    }

    if (SSL_accept(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        perror("SSL accept failed");
        return 1;
    }

    char buffer[1024] = {0};
    if (SSL_read(ssl, buffer, sizeof(buffer) - 1) <= 0)
    {
        perror("Failed to read from SSL");
        return 1;
    }

    // GET /file ....

    char *file_request = buffer + 5;
    char response[1024] = {0};
    const char *metadata = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";

    memcpy(response, metadata, strlen(metadata));

    if (strncmp(file_request, "index.html ", 11) == 0)
    {
        FILE *f = fopen("index.html", "r");
        if (f == NULL)
        {
            perror("Failed to open file");
            return 1;
        }

        size_t bytes_read = fread(response + strlen(metadata), 1, sizeof(response) - strlen(metadata) - 1, f);
        if (bytes_read == 0)
        {
            perror("Failed to read from file");
            return 1;
        }
        fclose(f);
    }
    else
    {
        const char *error = "No page found";
        memcpy(response + strlen(metadata), error, strlen(error));
    }

    SSL_write(ssl, response, sizeof(response));
    SSL_shutdown(ssl);

    SSL_free(ssl);
    SSL_CTX_free(ctx);

    // Use close() instead of pclose()
    close(clientfd);
    close(sockfd);

    return 0; // Return 0 to indicate success
}
