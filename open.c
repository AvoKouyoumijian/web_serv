#include <stdio.h>

int main(void)
{

    FILE *fp = fopen("./index.html", "r");
    if (!fp)
    {
        printf("not found\n");
    }
    char c;
    while ((c = fgetc(fp)) != EOF)
    {
        putchar(c);
    }
    return 0;
}