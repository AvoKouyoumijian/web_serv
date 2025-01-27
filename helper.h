#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void *get_in_addr(struct sockaddr *their_addr)
{
    if (their_addr->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)their_addr)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)their_addr)->sin6_addr);
}