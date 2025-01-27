#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdbool.h>

#include "helper.h"

// server setting defn's
#define PORT "3490"
#define BACKLOG 5
#define MAX_DATA_SIZE 5000

#define SLASH_OFFSET 1

// err defn's
#define ERR_ADDR_INF 3
#define ERR_BIND 2
#define ERR 1

// open file options
#define HTML 0
#define CSS 1
#define JS 2
#define OTHER 3

// function definitions
void process_requests(int sockfd);
int search_server_connection(struct addrinfo *servinfo);
void get_serv_info(struct addrinfo **servinfo);
int read_file(char *file_name, char **file_contents, char *file_format);

int main(void)
{
    // gather config info for server
    int sockfd;
    struct addrinfo *servinfo;
    get_serv_info(&servinfo);

    // search for a valid sever connection method
    sockfd = search_server_connection(servinfo);

    printf("server: waiting for connections...\n");

    // process the incoming requests on the running server
    process_requests(sockfd);

    return 0;
}

/**
 * parameter: servinfo
 * searches for a valid server setup configuration
 * returns: sockfd
 */
int search_server_connection(struct addrinfo *servinfo)
{
    struct addrinfo *p;
    int yes = 1, sockfd;

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(ERR);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);
    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(ERR_BIND);
    }

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(ERR);
    }

    return sockfd;
}

/**
 * parameters: sockfd
 *  processes all requests to the server
 * returns: --
 */
void process_requests(int sockfd)
{
    char s[INET6_ADDRSTRLEN];
    int new_fd;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;

    while (1)
    {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);

        // printf("server: got connection from %s\n", s);

        // print what u get from the client
        int numbytes;
        char buf[MAX_DATA_SIZE];
        char **req = malloc(sizeof(char *));
        if ((numbytes = recv(new_fd, buf, MAX_DATA_SIZE - 1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }

        // group the request object
        buf[numbytes] = '\0';
        req[0] = strtok(buf, "\r\n");
        char *tok;
        printf("%s\r\n", req[0]);
        for (int i = 1; (tok = strtok(NULL, "\r\n")) != NULL; i++)
        {
            req = realloc(req, sizeof(char *) * (i + 1));
            req[i] = tok;
            printf("%s\r\n", req[i]);
        }
        printf("\n");

        // get main details of the request
        char *req_type = strtok(req[0], " ");
        char *route = strtok(NULL, " ");
        char *version = strtok(NULL, " ");

        char *accept_format;
        for (int i = 1; req[i] != NULL; i++)
        {
            if ((accept_format = strtok(strstr(req[i], "Accept:"), ",")))
            {
                accept_format += sizeof("Accept:");
                break;
            }
            if (req[i + 1] == NULL)
            {
                printf("[server]err: req format field missing\n");
                continue;
            }
        }

        if (!fork())
        {
            char *res = NULL;
            int res_len;

            // .ico not implemented
            if (strstr(route, ".ico") != NULL)
            {
                close(new_fd);
                free(res);
                exit(EXIT_SUCCESS);
            }
            else if (strcmp("/", route) == 0)
            {
                // printf("route: %s\n", route);
                res_len = read_file("index.html", &res, "text/html");
            }
            else
            {
                if (!strcmp("text/html", accept_format) || !strcmp("text/css", accept_format) ||
                    !strcmp("text/js", accept_format) || !strcmp("*/*", accept_format))
                {
                    res_len = read_file(route + SLASH_OFFSET, &res, accept_format);
                }
                // not implented routes
                else
                {
                    close(sockfd);
                    exit(EXIT_SUCCESS);
                }
            }

            close(sockfd);

            if (send(new_fd, res, res_len, 0) == -1)
            {
                perror("send");
            }
            close(new_fd);
            free(res);
            exit(EXIT_SUCCESS);
        }
        close(new_fd);
    }
    return;
}

/**
 * parameters: servinfo
 * gets config options for the server
 *  returns: via *servinfo
 */
void get_serv_info(struct addrinfo **servinfo)
{
    struct addrinfo hints;
    int rv;
    int sockfd;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        exit(ERR_ADDR_INF);
    }

    return;
};

/**
 * params: file_name, **file_contents
 * reads a given file and returns its contents via a pointer as well as its lenght
 */
int read_file(char *file_name, char **file_contents, char *file_format)
{
    // init a html response header
    char *header = malloc(1024);
    int len;
    // open the file descriptor
    FILE *fp = fopen(file_name, "r");

    if (!fp)
    {
        // open the 404 file descriptor
        fp = fopen(".notfound.html", "r");

        // get the lenght of the file
        fseek(fp, 0L, SEEK_END);
        len = ftell(fp);
        // move the curser back
        fseek(fp, 0L, SEEK_SET);

        // Prepare HTTP response header
        snprintf(header, 1024, "HTTP/1.1 404 Not Found\r\n"
                               "Content-Type: text/html; charset=UTF-8\r\n"
                               "Content-Length: %d\r\n"
                               "\r\n",
                 len);
    }
    else
    {
        // detirmine valid file format
        if (!strcmp(file_format, "*/*"))
        {
            char *ext = strrchr(file_name, '.');
            if (ext == NULL)
            {
                file_format = strdup("text/plain");
            }
            else
            {
                file_format = malloc(strlen("text/") + strlen(ext) + 1);
                snprintf(file_format, len, "text/%s", ext + 1);
            }
        }

        // get the lenght of the file
        fseek(fp, 0L, SEEK_END);
        len = ftell(fp);
        // move the curser back
        fseek(fp, 0L, SEEK_SET);

        // Prepare HTTP response header
        snprintf(header, 1024, "HTTP/1.1 /%s 200 OK\r\n"
                               "Content-Type: %s; charset=UTF-8\r\n"
                               "Content-Length: %d\r\n"
                               "\r\n",
                 file_name, file_format, len);
    }

    // concat the header with the HTML response
    size_t head_len = strlen(header);
    header = realloc(header, strlen(header) + 1);
    *file_contents = malloc(len + 1 + head_len);
    strcpy(*file_contents, header);
    fread(*file_contents + strlen(header), len, 1, fp);

    // set last byte to null and close
    (*file_contents)[len + strlen(header)] = 0;
    fclose(fp);

    printf("%s", header);
    // free header
    free(header);

    return len + head_len;
}
