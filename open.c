#include <stdio.h>

int main(void)
{

    FILE *fp = fopen(".notfound.html", "r");
    if (!fp)
    {
        printf("not found\n");
    }
    return 0;
}