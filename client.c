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

#include "helper.h"

#define PORT "3490"
#define BACKLOG 5
#define MAX_DATA_SIZE 5000

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    struct addrinfo hints, *clires, *p;
    char s[INET6_ADDRSTRLEN];
    char buf[MAX_DATA_SIZE];
    int rv, numbytes;
    int socket_fd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(argv[1], PORT, &hints, &clires)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = clires; p != NULL; p = p->ai_next)
    {
        if ((socket_fd = socket(clires->ai_family, clires->ai_socktype, clires->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (connect(socket_fd, clires->ai_addr, clires->ai_addrlen) == -1)
        {
            close(socket_fd);
            perror("client: connect");
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to bind\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(clires);

    if ((numbytes = recv(socket_fd, buf, MAX_DATA_SIZE - 1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    buf[numbytes] = '\0';
    printf("client: received '%s'\n", buf);
    close(socket_fd);
}