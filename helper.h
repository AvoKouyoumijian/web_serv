#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// function declerations:
char *strredup(char *dest, char *src);
void *get_in_addr(struct sockaddr *their_addr);
char *find_index(char *path);
char *strredup(char *dest, char *src);
void tofree(void *mem);

void *get_in_addr(struct sockaddr *their_addr)
{
    if (their_addr->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)their_addr)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)their_addr)->sin6_addr);
};

char *find_index(char *path)
{
    DIR *folder = opendir(path);
    struct dirent *entry;
    char *path_cpy = NULL, *res = NULL;

    while ((entry = readdir(folder)) != NULL)
    {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") &&
            strcmp(entry->d_name, ".."))
        {
            path_cpy = strredup(path_cpy, path);
            size_t len = strlen(path_cpy) + strlen("/") + strlen(entry->d_name) + 1;
            path_cpy = realloc(path_cpy, len);
            snprintf(path_cpy, len, "%s/%s", path, entry->d_name);
            path_cpy = find_index(path_cpy);
            if (path_cpy != NULL)
            {
                path = strredup(path, path_cpy);
                free(path_cpy);
                closedir(folder);
                return path;
            }
        }
        else if (!strcmp(entry->d_name, "index.html") && entry->d_type == DT_REG)
        {
            res = strredup(res, path);
            size_t len = strlen(res) + strlen("/") + strlen(entry->d_name) + 1;
            res = realloc(res, len);
            snprintf(res, len, "%s/%s", path, entry->d_name);
            tofree(path_cpy);
            tofree(path);
            closedir(folder);
            return res;
        }
    }
    tofree(path_cpy);
    tofree(path);
    closedir(folder);
    return NULL;
}

/**
 * params: char *dest, char *src
 * first frees allocated mem(if mem allocaed) in dest and then mallocs source as it (with strdup)
 * returns: char *dest
 */
char *strredup(char *dest, char *src)
{
    tofree(dest);
    dest = strdup(src);
    return dest;
};

/**
 * params: void *mem
 * fress the allocted addres
 * returns: char *dest
 */
void tofree(void *mem)
{
    if (malloc_usable_size(mem))
    {
        free(mem);
    }
}