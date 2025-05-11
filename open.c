#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#include <malloc.h>

char *find_dir(char *path);
char *strredup(char *src, char *dup);
void tofree(void *mem);

int main()
{

    char *path = strdup(".");
    char *out = find_dir(strdup(path));
    if (!out)
    {
        printf("nothing found :(");
        return 0;
    }
    // char *print = out + strlen("/home/avo-kouyoumijian/project/web_serv");
    printf("right yee full: %s\n", out);
    free(out);
    printf("path: %s\n", path);
    // free(e);
    // printf("right yee path: %s\n", print);
    free(path);
    return 0;
}

char *find_dir(char *path)
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
            path_cpy = find_dir(path_cpy);
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
}

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