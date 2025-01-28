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
#include <sys/stat.h>
#include <malloc.h>

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
void process_requests(int sockfd, char *dir);
int search_server_connection(struct addrinfo *servinfo);
void get_serv_info(struct addrinfo **servinfo);
int read_file(char *file_name, char **file_contents, char *file_format, char *dir, char *file_type);
char *handshake(char *route, char **req, int req_len, char *file_type, char *ext);
char *route_str_ext(char *route);
char *route_str__rel_path(char *route);

int main(int argc, char *argv[])
{
    // get a directory to serv web pages from
    char *dir;
    if (argc < 2)
    {
        dir = ".";
    }
    else
    {
        if (argv[1][strlen(argv[1] - 1)] == '/')
        {
            argv[1][strlen(argv[1]) - 1] = '\0';
        }
        dir = argv[1];
    }
    // gather config info for server
    int sockfd;
    struct addrinfo *servinfo;
    get_serv_info(&servinfo);

    // search for a valid sever connection method
    sockfd = search_server_connection(servinfo);

    printf("server: waiting for connections...\n");

    // process the incoming requests on the running server
    process_requests(sockfd, dir);

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
void process_requests(int sockfd, char *dir)
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
        req[0] = strtok(buf, "\r\n");
        char *tok;
        printf("%s\r\n", req[0]);
        int req_len = 1;
        while ((tok = strtok(NULL, "\r\n")) != NULL)
        {
            req = realloc(req, sizeof(char *) * (req_len + 1));
            req[req_len] = tok;
            printf("%s\r\n", req[req_len]);
            req_len++;
        }
        printf("\n");

        // get main details of the request
        char *req_type = strtok(req[0], " ");
        char *route = strtok(NULL, " ");
        if (!strcmp(route, "/"))
        {
            route = "/index.html";
        }
        char *version = strtok(NULL, " ");
        // filler
        req_type++;
        version++;
        // filler
        char *ext = route_str_ext(route);

        char *file_type;
        for (int i = 1; req[i] != NULL; i++)
        {
            file_type = strstr(req[i], "Sec-Fetch-Dest: ");
            if (file_type != NULL)
            {
                file_type += strlen("Sec-Fetch-Dest: ");
                break;
            }
        }
        char *accept_format;
        if (ext != NULL)
        {
            if ((accept_format = handshake(route + 1, req, req_len, file_type, ext)) == NULL)
            {
                free(req);
                close(new_fd);
                continue;
            }
        }
        else
        {
            accept_format = strdup("text/plain");
        }

        if (!fork())
        {
            char *res = NULL;
            int res_len;

            res_len = read_file(route + 1, &res, accept_format, dir, file_type);

            close(sockfd);

            if (send(new_fd, res, res_len, 0) == -1)
            {
                perror("send");
            }
            close(new_fd);
            free(accept_format);
            free(res);
            free(req);
            exit(EXIT_SUCCESS);
        }
        free(req);
        free(accept_format);
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
int read_file(char *file_name, char **file_contents, char *file_format, char *dir, char *file_type)
{
    // init a html response header
    char *header = malloc(1024);

    // init data
    char *type_addon = "";
    int len;
    // open the file descriptor

    char *path = malloc(strlen(dir) + strlen("/") + strlen(file_name) + 1);
    snprintf(path, strlen(dir) + strlen("/") + strlen(file_name) + 1, "%s/%s", dir, file_name);
    FILE *fp = fopen(path, "rb");

    if (!fp)
    {
        // open the 404 file descriptor
        path = realloc(path, strlen(dir) + strlen("/.notfound.html") + 1);
        snprintf(path, strlen(dir) + strlen("/.notfound.html") + 1, "%s/.notfound.html", dir);
        fp = fopen(path, "rb");

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
        if (!strcmp("text/html", file_format) || !strcmp("text/css", file_format) ||
            !strcmp("text/js", file_format))
        {
            type_addon = "; charset=UTF-8";
        }

        // get the lenght of the file
        fseek(fp, 0L, SEEK_END);
        len = ftell(fp);
        // move the curser back
        fseek(fp, 0L, SEEK_SET);

        // Prepare HTTP response header
        snprintf(header, 1024, "HTTP/1.1 /%s 200 OK\r\n"
                               "Content-Type: %s%s\r\n"
                               "Content-Length: %d\r\n"
                               "\r\n",
                 file_name, file_format, type_addon, len);
    }

    // concat the header with the HTML response
    size_t head_len = strlen(header);
    header = realloc(header, head_len + 1);
    *file_contents = malloc(len + head_len + 1);
    strcpy(*file_contents, header);
    fread(*file_contents + head_len, len, 1, fp);

    // set last byte to null and close
    (*file_contents)[len + head_len] = 0;
    fclose(fp);

    printf("%s", header);
    // free use memory(if malloced)
    free(header);
    free(path);

    return len + head_len;
}

char *handshake(char *route, char **req, int req_len, char *file_type, char *ext)
{
    // pics a format from the list of the available formats based on the extension of ur
    // file
    char *accept_format;
    for (int i = 1; i < req_len; i++)
    {
        accept_format = strstr(req[i], "Accept: ");
        if ((accept_format = strtok(accept_format, ",")))
        {
            accept_format += strlen("Accept: ");
            // loop through till u find a suitable file format
            for (char *a = accept_format; strchr(a, '*') == NULL;
                 a = strtok(NULL, ","))
            {
                if (!strcmp(strrchr(route + 1, '.') + 1, strrchr(a, '/') + 1))
                {
                    accept_format = strdup(a);
                    return accept_format;
                }
            }
            // deal with web txt
            if (!strcmp("document", file_type) || !strcmp("script", file_type) ||
                !strcmp("style", file_type))
            {
                accept_format = malloc(strlen("text/") + strlen(ext) + 1);
                snprintf(accept_format, strlen("text/") + strlen(ext) + 1, "text/%s", ext + 1);
            }
            // deal with image
            else if (!strcmp("image", file_type))
            {
                accept_format = malloc(strlen("image/") + strlen(ext) + 1);
                snprintf(accept_format, strlen("image/") + strlen(ext) + 1, "image/%s", ext + 1);
            }
            // deal with font
            else if (!strcmp("font", file_type))
            {
                accept_format = malloc(strlen("font/") + strlen(ext) + 1);
                snprintf(accept_format, strlen("font/") + strlen(ext) + 1, "font/%s", ext + 1);
            }
            // any other form;
            else
            {
                accept_format = strdup("text/plain");
            }
            return accept_format;
        }
    }
    fprintf(stderr, "[error(server)]: req format field missing\n");
    free(accept_format);
    return NULL;
}

/**
 * prams: char *str
 * yieids a string with only alhabetical characters till it encounters
 * a non alphabetical character
 * returns: char *alpstr
 */
char *stralp(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        // checks weather the string is an alphabetical character
        if (!((str[i] >= 'A' && str[i] <= 'Z') ||
              (str[i] >= 'a' && str[i] <= 'z')) &&
            i != 0 && str[i] == '.')
        {
            // cut the string if true
            str[i] = '\0';
            return str;
        }
    }
    // returns the whole string back if it is alphabetical
    return str;
}

/**
 * prams: char *str
 * yieids the ext of a given HTTP route
 * returns: char *ext
 */
char *route_str_ext(char *route)
{
    char *path = route_str__rel_path(route);
    return strrchr(path, '.');
}

/**
 * prams: char *str
 * yieids the path of a given HTTP route
 * returns: char *rel_path
 */
char *route_str__rel_path(char *route)
{
    for (int i = 0; i < strlen(route); i++)
    {
        // check if there is an incoming argument
        if (route[i] == '?')
        {
            // cut the string if true
            route[i] = '\0';
            return route;
        }
    }
    // returns the whole string if no args
    return route;
}